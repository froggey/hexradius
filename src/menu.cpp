#include <cassert>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <boost/foreach.hpp>
#include <boost/filesystem.hpp>
#include "menu.hpp"
#include "octradius.hpp"
#include "client.hpp"
#include "fontstuff.hpp"
#include "loadimage.hpp"
#include "network.hpp"
#include "gui.hpp"
#include "scenario.hpp"

bool running, submenu;

void editor_main(const GUI::TextButton &button, const SDL_Event &event);
static void options_main(const GUI::TextButton &button, const SDL_Event &event);

static void main_join_cb(const GUI::TextButton &, const SDL_Event &) {
	JoinMenu jmenu;
	jmenu.run();
}

static void main_host_cb(const GUI::TextButton &, const SDL_Event &) {
	HostMenu hmenu;
	hmenu.run();
}

static void quit_cb(const GUI::TextButton &, const SDL_Event &) {
	running = false;
}

static void back_cb(const GUI::TextButton &, const SDL_Event &) {
	submenu = false;
}

static bool port_input_filter(const GUI::TextBox &, const SDL_Event &event) {
	return isdigit(event.key.keysym.sym);
}

static void app_quit_cb(const GUI &, const SDL_Event &) {
	running = false;
}

MainMenu::MainMenu() :
	gui(0, 0, MENU_WIDTH, MENU_HEIGHT),

	join_btn(gui, 332, 235, 135, 35, 1, "Join game", &main_join_cb),
	host_btn(gui, 332, 280, 135, 35, 2, "Host game", &main_host_cb),
	edit_btn(gui, 332, 325, 135, 35, 3, "Map editor", &editor_main),
	options_btn(gui, 332, 370, 135, 35, 4, "Options", &options_main),
	quit_btn(gui, 332, 415, 135, 35, 5, "Quit", &quit_cb)
{
	gui.set_bg_image(ImgStuff::GetImage("graphics/menu/background.png"));
	gui.set_quit_callback(&app_quit_cb);
}

void MainMenu::run() {
	running = true;

	while(running) {
		gui.poll(true);
		SDL_Delay(MENU_DELAY);
	}
}

static void join_cb(const GUI::TextButton &, const SDL_Event &, JoinMenu *menu) {
	if(menu->host_input.text.empty()) {
		std::cout << "No host" << std::endl;
		return;
	}

	if(menu->port_input.text.empty()) {
		std::cout << "No port" << std::endl;
		return;
	}

	std::string host = menu->host_input.text;
	int port = atoi(menu->port_input.text.c_str());

	if(port < 1 || port > 65535) {
		std::cout << "Invalid port number" << std::endl;
		return;
	}

	Client client(host, port);

	client.run();

	if(client.quit) {
		running = false;
	}

	ImgStuff::set_mode(MENU_WIDTH, MENU_HEIGHT);

	submenu = false;
}

static void join_textbox_enter(const GUI::TextBox &, const SDL_Event &event, JoinMenu *menu) {
	join_cb(menu->join_btn, event, menu);
}

JoinMenu::JoinMenu() :
	gui(0, 0, MENU_WIDTH, MENU_HEIGHT),

	host_label(gui, 245, 232, 100, 25, 0, "Host:"),
	host_input(gui, 355, 232, 200, 25, 1),

	port_label(gui, 245, 277, 100, 25, 0, "Port:"),
	port_input(gui, 355, 277, 200, 25, 2),

	join_btn(gui, 332, 322, 135, 35, 3, "Join Game", boost::bind(join_cb, _1, _2, this)),
	back_btn(gui, 20, 545, 135, 35, 4, "Back", &back_cb)
{
	gui.set_bg_image(ImgStuff::GetImage("graphics/menu/background.png"));
	gui.set_quit_callback(&app_quit_cb);

	host_label.align(GUI::RIGHT);
	host_input.set_enter_callback(boost::bind(join_textbox_enter, _1, _2, this));

	port_label.align(GUI::RIGHT);
	port_input.set_text(to_string(DEFAULT_PORT));
	port_input.set_enter_callback(boost::bind(join_textbox_enter, _1, _2, this));
	port_input.set_input_callback(&port_input_filter);
}

void JoinMenu::run() {
	submenu = true;

	while(running && submenu) {
		gui.poll(true);
		SDL_Delay(MENU_DELAY);
	}
}

static void host_cb(const GUI::TextButton &, const SDL_Event &, HostMenu *menu) {
	int port = atoi(menu->port_input.text.c_str());

	if(port < 1 || port > 65535) {
		std::cerr << "Invalid port number" << std::endl;
		return;
	}

	Server server(port, *(menu->scenario_input.selected()));

	Client client("127.0.0.1", port);

	client.run();

	if(client.quit) {
		running = false;
	}

	ImgStuff::set_mode(MENU_WIDTH, MENU_HEIGHT);

	submenu = false;
}

static void host_textbox_enter(const GUI::TextBox &, const SDL_Event &event, HostMenu *menu) {
	host_cb(menu->host_btn, event, menu);
}

HostMenu::HostMenu() :
	gui(0, 0, MENU_WIDTH, MENU_HEIGHT),

	port_label(gui, 245, 232, 100, 25, 0, "Port:"),
	port_input(gui, 355, 232, 200, 25, 1),

	scenario_label(gui, 245, 277, 100, 25, 0, "Scenario:"),
	scenario_input(gui, 355, 277, 200, 25, 2),

	host_btn(gui, 332, 322, 135, 35, 3, "Host Game", boost::bind(host_cb, _1, _2, this)),
	back_btn(gui, 20, 545, 135, 35, 4, "Back", &back_cb)
{
	gui.set_bg_image(ImgStuff::GetImage("graphics/menu/background.png"));
	gui.set_quit_callback(&app_quit_cb);

	port_label.align(GUI::RIGHT);
	port_input.set_text(to_string(DEFAULT_PORT));
	port_input.set_enter_callback(boost::bind(host_textbox_enter, _1, _2, this));
	port_input.set_input_callback(&port_input_filter);

	scenario_label.align(GUI::RIGHT);

	using namespace boost::filesystem;

	for(directory_iterator node("scenario"); node != directory_iterator(); ++node)
	{
		std::string name = node->path().string().substr(9);
		scenario_input.add_item(name, name);
	}
	
	scenario_input.select("hex_2p");
}

void HostMenu::run() {
	submenu = true;

	while(running && submenu) {
		gui.poll(true);
		SDL_Delay(MENU_DELAY);
	}
}

struct options_inputs {
	GUI::TextBox username;
	GUI::Checkbox show_lines;

	options_inputs(GUI &gui) :
		username(gui, 385, 200, 200, 25, 1),
		show_lines(gui, 385, 235, 25, 25, 2, options.show_lines) {}
};

static void save_options(const GUI::TextButton &, const SDL_Event &, struct options_inputs *inputs) {
	if(inputs->username.text.empty()) {
		std::cerr << "Username field is empty" << std::endl;
		return;
	}

	options.username = inputs->username.text;
	options.show_lines = inputs->show_lines.state;
	options.save("options.txt");

	submenu = false;
}

static void options_main(const GUI::TextButton &, const SDL_Event &) {
	GUI gui(0, 0, MENU_WIDTH, MENU_HEIGHT);

	gui.set_bg_image(ImgStuff::GetImage("graphics/menu/background.png"));
	gui.set_quit_callback(&app_quit_cb);

	options_inputs inputs(gui);

	GUI::TextButton username_label(gui, 215, 200, 160, 25, 0, "Username:");
	username_label.align(GUI::RIGHT);
	inputs.username.set_text(options.username);

	GUI::TextButton lines_label(gui, 215, 235, 160, 25, 0, "Show grid paths:");
	lines_label.align(GUI::RIGHT);

	GUI::TextButton back_btn(gui, 20, 545, 135, 35, 21, "Back", &back_cb);
	GUI::TextButton save_btn(gui, 645, 545, 135, 35, 20, "Save", boost::bind(save_options, _1, _2, &inputs));

	for(submenu = true; submenu && running;) {
		gui.poll(true);
		SDL_Delay(MENU_DELAY);
	}
}
