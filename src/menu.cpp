#include <cassert>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <boost/foreach.hpp>
#include "menu.hpp"
#include "octradius.hpp"
#include "client.hpp"
#include "fontstuff.hpp"
#include "loadimage.hpp"
#include "network.hpp"
#include "gui.hpp"
#include "scenario.hpp"

static bool running, submenu;

static void main_join_cb(const GUI::ImgButton &button, const SDL_Event &event, void *arg) {
	JoinMenu jmenu;
	jmenu.run();
}

static void main_host_cb(const GUI::ImgButton &button, const SDL_Event &event, void *arg) {
	HostMenu hmenu;
	hmenu.run();
}

static void quit_cb(const GUI::ImgButton &button, const SDL_Event &event, void *arg) {
	running = false;
}

static void back_cb(const GUI::ImgButton &button, const SDL_Event &event, void *arg) {
	submenu = false;
}

static void join_cb(const GUI::ImgButton &button, const SDL_Event &event, void *arg) {
	JoinMenu *menu = (JoinMenu*)arg;
	
	if(menu->host_input->text.empty()) {
		std::cout << "No host" << std::endl;
		return;
	}
	
	if(menu->port_input->text.empty()) {
		std::cout << "No port" << std::endl;
		return;
	}
	
	std::string host = menu->host_input->text;
	int port = atoi(menu->port_input->text.c_str());
	
	if(port < 1 || port > 65535) {
		std::cout << "Invalid port number" << std::endl;
		return;
	}
	
	Client client(host, port);
	
	client.run();
	
	if(client.quit) {
		running = false;
	}
	
	screen = SDL_SetVideoMode(MENU_WIDTH, MENU_HEIGHT, 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	submenu = false;
}

static bool port_input_filter(const GUI::TextBox &tbox, const SDL_Event &event, void *arg) {
	return isdigit(event.key.keysym.sym);
}

static void join_textbox_enter(const GUI::TextBox &tbox, const SDL_Event &event, void *arg) {
	JoinMenu *menu = (JoinMenu*)arg;
	join_cb(*(menu->join_btn), event, arg);
}

static void app_quit_cb(const GUI &gui, const SDL_Event &event, void *arg) {
	running = false;
}

MainMenu::MainMenu() : gui(0, 0, MENU_WIDTH, MENU_HEIGHT) {
	gui.set_bg_image(ImgStuff::GetImage("graphics/menu/background.png"));
	gui.set_quit_callback(&app_quit_cb);
	
	join_btn = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/join_game.png"), 350, 300, 2, &main_join_cb);
	host_btn = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/host_game.png"), 350, 250, 1, &main_host_cb);
	quit_btn = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/quit.png"), 700, 570, 3, &quit_cb);
}

MainMenu::~MainMenu() {
	delete join_btn;
	delete host_btn;
	delete quit_btn;
}

void MainMenu::run() {
	running = true;
	
	while(running) {
		gui.poll(true);
		SDL_Delay(MENU_DELAY);
	}
}

JoinMenu::JoinMenu() : gui(0, 0, MENU_WIDTH, MENU_HEIGHT) {
	gui.set_bg_image(ImgStuff::GetImage("graphics/menu/background.png"));
	gui.set_quit_callback(&app_quit_cb);
	
	host_label = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/host.png"), 315, 250, 100);
	host_label->enable(false);
	
	host_input = new GUI::TextBox(gui, 375, 255, 200, 20, 1);
	host_input->enter_callback = &join_textbox_enter;
	host_input->enter_callback_arg = this;
	
	port_label = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/port.png"), 325, 300, 101);
	port_label->enable(false);
	
	port_input = new GUI::TextBox(gui, 375, 305, 200, 20, 2);
	port_input->enter_callback = &join_textbox_enter;
	port_input->enter_callback_arg = this;
	port_input->input_callback = &port_input_filter;
	port_input->set_text(to_string(DEFAULT_PORT));
	
	join_btn = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/join_game.png"), 350, 350, 3, &join_cb, this);
	back_btn = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/back.png"), 0, 570, 4, &back_cb);
	quit_btn = new GUI::ImgButton(gui, ImgStuff::GetImage("graphics/menu/quit.png"), 700, 570, 5, &quit_cb);
}

JoinMenu::~JoinMenu() {
	delete host_label;
	delete host_input;
	
	delete port_label;
	delete port_input;
	
	delete join_btn;
	delete back_btn;
	delete quit_btn;
}

void JoinMenu::run() {
	submenu = true;
	
	while(running && submenu) {
		gui.poll(true);
		SDL_Delay(MENU_DELAY);
	}
}

static void host_cb(const GUI::TextButton &button, const SDL_Event &event, void *arg) {
	HostMenu *menu = (HostMenu*)arg;
	
	int port = atoi(menu->port_input.text.c_str());
	std::string scenario = menu->scenario_input.text;
	
	if(port < 1 || port > 65535) {
		std::cerr << "Invalid port number" << std::endl;
		return;
	}
	
	Scenario scn;
	scn.load_file(scenario);
	
	Server server(port, scn);
	
	Client client("127.0.0.1", port);
	
	client.run();
	
	if(client.quit) {
		running = false;
	}
	
	screen = SDL_SetVideoMode(MENU_WIDTH, MENU_HEIGHT, 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	submenu = false;
}

HostMenu::HostMenu() :
		gui(0, 0, MENU_WIDTH, MENU_HEIGHT),
		
		port_label(gui, 245, 232, 100, 25, 0, "Port:"),
		port_input(gui, 355, 232, 200, 25, 1),
		
		scenario_label(gui, 245, 277, 100, 25, 0, "Scenario:"),
		scenario_input(gui, 355, 277, 200, 25, 2),
		
		host_btn(gui, 332, 322, 135, 35, 3, "Host Game", &host_cb, this)
{
	gui.set_bg_image(ImgStuff::GetImage("graphics/menu/background.png"));
	gui.set_quit_callback(&app_quit_cb);
	
	port_label.align(GUI::RIGHT);
	port_input.set_text(to_string(DEFAULT_PORT));
	
	scenario_label.align(GUI::RIGHT);
	scenario_input.set_text("scenario/hex_2p.txt");
}

void HostMenu::run() {
	submenu = true;
	
	while(running && submenu) {
		gui.poll(true);
		SDL_Delay(MENU_DELAY);
	}
}

#if 0
		host_menu.screen = screen;
		host_menu += new Button(325, 250, 40, 30, "graphics/menu/port.png", NULL);
		host_port_box = new TextInput(375, 255, 200, 20, "9001", 14, textbox_fg, textbox_bg);
		host_menu += host_port_box;
		host_menu += new Button(275, 300, 90, 30, "graphics/menu/scenario.png", NULL);
		host_scenario_box = new TextInput(375, 305, 200, 20, "scenario/hex_2p.txt", 14, textbox_fg, textbox_bg);
		host_menu += host_scenario_box;
		host_menu += new Button(350, 350, 100, 30, "graphics/menu/host_game.png", start_server_callback);
		host_menu += new Button(0, 570, 100, 30, "graphics/menu/back.png", return_callback);
		host_menu += new Button(700, 570, 100, 30, "graphics/menu/quit.png", quit_callback);
		host_menu.background = main_menu.background;
		
		wait_for_players_menu.screen = screen;
		wait_for_players_menu += new Button(300, 250, 40, 30, "graphics/menu/waiting_for_players.png", NULL);
		wait_for_players_menu.background = main_menu.background;
		wait_for_players_menu.do_stuff = host_do_stuff;
#endif
