#ifndef OR_MENU_HPP
#define OR_MENU_HPP

#include <SDL/SDL.h>
#include <string>
#include <vector>

#include "gui.hpp"

const uint MENU_WIDTH = 800;
const uint MENU_HEIGHT = 600;
const uint MENU_DELAY = 100;

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

#endif
