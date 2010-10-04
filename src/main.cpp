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
#include <boost/program_options.hpp>

#ifdef _WIN32
/* Including windows.h here makes it error when ASIO includes winsock.h since
 * windows is Retarded.
*/

// #include <windows.h>
#endif

#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "powers.hpp"
#include "octradius.hpp"
#include "network.hpp"
#include "client.hpp"
#include "menu.hpp"
#include "gui.hpp"
#include "scenario.hpp"

namespace po = boost::program_options;

const char *team_names[] = { "Blue", "Red", "Green", "Yellow", "Purple", "Orange", "Spectator" };
const SDL_Colour team_colours[] = { {0,0,255}, {255,0,0}, {0,255,0}, {255,255,0}, {160,32,240}, {255,165,0}, {190,190,190} };

struct options options;

int main(int argc, char **argv) {
	srand(time(NULL));
	
	options.load("options.txt");
	options.save("options.txt");
	
	uint16_t port;
	std::string hostname, scenario;
	
	po::options_description desc("Command line options");
	desc.add_options()
			("help", "Display this message")
			("connect,c", po::value<std::string>(&hostname), "Connect to server")
			("host,h", po::value<std::string>(&scenario), "Host game with supplied scenario")
			("port,p", po::value<uint16_t>(&port)->default_value(DEFAULT_PORT), std::string("Set TCP port (default is " + to_string(DEFAULT_PORT) + ")").c_str())
	;
	
	po::variables_map vm;
	
	try {
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);
	} catch(po::error e) {
		std::cerr << e.what() << std::endl;
		std::cerr << "Run " << argv[0] << " --help for usage" << std::endl;
		return 1;
	}
	
	if(vm.count("help")) {
		std::cout << desc << std::endl;
		return 0;
	}
	
	assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);
	assert(TTF_Init() == 0);
	SDL_EnableUNICODE(1);
	
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	
	SDL_WM_SetCaption("HexRadius", "HexRadius");
	
	ImgStuff::set_mode(MENU_WIDTH, MENU_HEIGHT);
	
	if(vm.count("host")) {
		Scenario scn;
		scn.load_file(scenario);
		
		Server server(port, scn);
		
		Client client("127.0.0.1", port);
		
		client.run();
	}else if(vm.count("connect")) {
		Client client(hostname, port);
		
		client.run();
	}else{
		MainMenu menu;
		menu.run();
	}
	
	ImgStuff::FreeImages();
	FontStuff::FreeFonts();
	SDL_Quit();
	
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

send_buf::send_buf(const protocol::message &message) : buf(NULL) {
	std::string pb;
	message.SerializeToString(&pb);
	
	uint32_t psize = htonl(pb.size());
	buf = buf_ptr(new char[size = (pb.size()+sizeof(psize))]);
	
	memcpy(buf.get(), &psize, sizeof(psize));
	memcpy(buf.get()+sizeof(psize), pb.data(), pb.size());
}
