#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <assert.h>
#include <vector>
#include <fstream>
#include <string.h>
#include <map>
#include <SDL/SDL_ttf.h>
#include <math.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "powers.hpp"
#include "octradius.hpp"
#include "network.hpp"
#include "client.hpp"
#include "menu.hpp"
#include "gui.hpp"

const char *team_names[] = { "Blue", "Red", "Green", "Yellow", "Purple", "Orange", "Spectator" };
const SDL_Colour team_colours[] = { {0,0,255}, {255,0,0}, {0,255,0}, {255,255,0}, {160,32,240}, {255,165,0}, {190,190,190} };

struct options options;

static char *next_value(char *str) {
	char *r = str+strcspn(str, "\t ");
	
	if(r[0]) {
		r[0] = '\0';
		r += strspn(r+1, "\t ")+1;
	}
	
	return r;
}

void LoadScenario(std::string filename, Scenario &sc) {
	std::fstream file(filename.c_str(), std::fstream::in);
	assert(file.is_open());
	
	char buf[1024], *bp;
	
	while(file.good()) {
		file.getline(buf, sizeof(buf));
		buf[strcspn(buf, "\n")] = '\0';
		
		bp = next_value(buf);
		std::string name = buf;
		
		if(name == "GRID") {
			int cols = atoi(bp);
			int rows = atoi(next_value(bp));
			
			assert(cols > 0 && rows > 0);
			
			for(int r = 0; r < rows; r++) {
				for(int c = 0; c < cols; c++) {
					sc.tiles.push_back(new Tile(c, r, 0));
				}
			}
		}
		else if(name == "SPAWN") {
			/* SPAWN x y c */
			
			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int c = atoi((bp = next_value(bp)));
			
			Tile *tile = FindTile(sc.tiles, x, y);
			assert(tile);
			
			tile->pawn = new Pawn((PlayerColour)c, sc.tiles, tile);
		}
		else if(name == "HOLE") {
			int x = atoi(bp);
			int y = atoi(next_value(bp));
			
			Tile::List::iterator i = sc.tiles.begin();
			
			while(i != sc.tiles.end()) {
				if((*i)->col == x && (*i)->row == y) {
					delete *i;
					sc.tiles.erase(i);
					
					break;
				}
				
				i++;
			}
		}
		else if(name == "POWER") {
			int i = atoi(bp);
			int p = atoi(next_value(bp));
			assert(i < Powers::num_powers);
			Powers::powers[i].spawn_rate = p;
		}
		else if(name == "HEIGHT") {
			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int h = atoi((bp = next_value(bp)));
			
			Tile *tile = FindTile(sc.tiles, x, y);
			assert(tile);
			
			tile->height = h >= -2 && h <= 2? h : 0;
		}
	}
}

int main(int argc, char **argv) {
	srand(time(NULL));
	
	options.load("options.txt");
	options.save("options.txt");
	
	Scenario scn;
	const char* scenario_name = "scenario/default.txt";
	const char* host;
	int port;
	bool is_server = false;
	bool is_client = false;
	
	for(int i = 1; i < argc; i++) {
		if(strcmp(argv[i], "-s") == 0) {
			host = "127.0.0.1";
			port = atoi(argv[++i]);
			scenario_name = argv[++i];
			is_server = true;
		}
		else if(strcmp(argv[i], "-c") == 0) {
			host = argv[++i];
			port = atoi(argv[++i]);
			is_client = true;
		}
		else {
			std::cerr << "Unrecognized option " << argv[i] << ", learn to type kthx" << std::endl;
			return 1;
		}
	}

	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	assert(TTF_Init() == 0);
	SDL_EnableUNICODE(1);
	
	atexit(SDL_Quit);
	atexit(FontStuff::FreeFonts);
	atexit(ImgStuff::FreeImages);
	
	SDL_WM_SetCaption("OctRadius", "OctRadius");
	
	screen = SDL_SetVideoMode(MENU_WIDTH, MENU_HEIGHT, 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	if (is_server) {
		LoadScenario(scenario_name, scn);
		Server server(port, scn);
		
		Client client("127.0.0.1", port, options.username);
		
		do {
			server.DoStuff();
			SDL_Delay(5);
		} while(client.DoStuff());
	}
	else if (is_client) {
		Client client(host, port, options.username);

		while (client.DoStuff()) {
			uint8_t st = SDL_GetAppState();
			if (st == SDL_APPACTIVE || !st)
				SDL_Delay(200);
			else
				SDL_Delay(5);
		}
	}
	else {
		MainMenu menu;
		menu.run();
	}
	
	return 0;
}

options::options() {
	#ifdef _WIN32
	char userbuf[256];
	DWORD usersize = sizeof(userbuf);
	
	char *user = GetUserNameA(userbuf, &usersize) ? userbuf : NULL;
	#else
	char *user = getenv("USER");
	#endif
	
	username = user ? user : "Game losing mingebag";
	
	show_lines = true;
}

void options::load(std::string filename) {
	std::fstream file(filename.c_str(), std::fstream::in);
	
	if(!file.is_open()) {
		std::cerr << "Failed to load options" << std::endl;
		return;
	}
	
	char buf[1024];
	
	while(file.good()) {
		file.getline(buf, sizeof(buf));
		buf[strcspn(buf, "\r\n")] = '\0';
		
		char *eq = strchr(buf, '=');
		int len = eq - buf;
		
		if(!eq) {
			continue;
		}
		
		std::string name(buf, len);
		std::string val(eq+1);
		
		std::cout << "Option '" << name << "' = '" << val << "'" << std::endl;
		
		if(name == "username") {
			username = val;
		}else if(name == "show_lines") {
			show_lines = (val == "true" ? 1 : 0);
		}else{
			std::cerr << "Unknown option: " << name << std::endl;
		}
	}
}

void options::save(std::string filename) {
	std::ofstream file(filename.c_str());
	
	if(!file.is_open()) {
		std::cerr << "Failed to save options" << std::endl;
		return;
	}
	
	file << "username=" << username << std::endl;
	file << "show_lines=" << (show_lines ? "true" : "false") << std::endl;
}
