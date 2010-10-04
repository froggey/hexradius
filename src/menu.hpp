#ifndef OR_MENU_HPP
#define OR_MENU_HPP

#include <SDL/SDL.h>
#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "gui.hpp"

const int MENU_WIDTH = 800;
const int MENU_HEIGHT = 600;
const int MENU_DELAY = 50;

struct MainMenu {
	GUI gui;
	GUI::ImgButton *join_btn;
	GUI::ImgButton *host_btn;
	GUI::ImgButton *quit_btn;
	
	MainMenu();
	~MainMenu();
	
	void run();
};

struct JoinMenu {
	GUI gui;
	
	GUI::ImgButton *host_label;
	GUI::TextBox *host_input;
	
	GUI::ImgButton *port_label;
	GUI::TextBox *port_input;
	
	GUI::ImgButton *join_btn;
	GUI::ImgButton *back_btn;
	GUI::ImgButton *quit_btn;
	
	JoinMenu();
	~JoinMenu();
	
	void run();
};

struct HostMenu {
	GUI gui;
	
	GUI::TextButton port_label;
	GUI::TextBox port_input;
	
	GUI::TextButton scenario_label;
	GUI::TextBox scenario_input;
	
	GUI::TextButton host_btn;
	GUI::TextButton back_btn;
	
	HostMenu();
	
	void run();
};

#endif
