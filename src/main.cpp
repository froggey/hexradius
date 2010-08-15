#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <fstream>
#include <string.h>
#include <map>
#include <SDL/SDL_ttf.h>
#include <math.h>

#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "powers.hpp"
#include "octradius.hpp"
#include "network.hpp"
#include "client.hpp"

struct pmenu_entry {
	SDL_Rect rect;
	int power;
};

struct uistate {
	Pawn *dpawn;
	Pawn *mpawn;
	
	std::vector<struct pmenu_entry> pmenu;
	SDL_Rect pmenu_area;
	
	uistate() : dpawn(NULL), mpawn(NULL), pmenu_area((SDL_Rect){0,0,0,0}) {}
};

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
			sc.cols = atoi(bp);
			sc.rows = atoi(next_value(bp));
			
			assert(sc.cols > 0 && sc.rows > 0);
			
			for(int c = 0; c < sc.cols; c++) {
				for(int r = 0; r < sc.rows; r++) {
					sc.tiles.push_back(new Tile(c, r, 0));
				}
			}
		}
		if(name == "SPAWN") {
			assert(sc.cols > 0 && sc.rows > 0);
			
			/* SPAWN x y c */
			
			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int c = atoi((bp = next_value(bp)));
			
			Tile *tile = FindTile(sc.tiles, x, y);
			assert(tile);
			
			tile->pawn = new Pawn((PlayerColour)c, sc.tiles, tile);
		}
		if(name == "HOLE") {
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
	}
}

int main(int argc, char **argv) {
	if(argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [scenario]" << std::endl;
		return 1;
	}
	
	srand(time(NULL));
	
	Scenario scn;
	
	if(argc == 1) {
		LoadScenario("scenario/default.txt", scn);
	}else{
		LoadScenario(argv[1], scn);
	}
	
	Server server(9001, scn, 1);
	Client client("127.0.0.1", 9001, "test");
	
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	assert(TTF_Init() == 0);
	
	do {
		server.DoStuff();
	} while(client.DoStuff());
	
	OctRadius::FreeImages();
	FontStuff::FreeFonts();
	
	SDL_Quit();
	
	return 0;
}
