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

namespace Menu {
	Menu main_menu, host_menu, join_menu, wait_for_players_menu, connect_menu;
	TextInput* host_port_box;
	TextInput* host_scenario_box;
	TextInput* join_hostname_box;
	TextInput* join_port_box;
	
	bool point_in_rect(int x, int y, SDL_Rect* r) {
		return r->x < x && r->x + r->w > x && r->y < y && r->y + r->h > y;
	}
	
	UIElement::UIElement(int _x, int _y, int _w, int _h): surface(NULL), on_click(NULL), on_key(NULL), menu(NULL) {
		rect.x = _x;
		rect.y = _y;
		rect.w = _w;
		rect.h = _h;
	}
	
	void UIElement::draw(SDL_Surface* screen) {
		if (surface) {
			assert(SDL_BlitSurface(surface, NULL, screen, &rect) == 0);
		}
	}
	
	Button::Button(int x, int y, int w, int h, std::string image, ClickCallback _on_click):
		UIElement(x, y, w, h) {
		surface = ImgStuff::GetImage(image);
		on_click = _on_click;
	}
	
	TextInput::TextInput(int x, int y, int w, int h, std::string text, int size, SDL_Color _fg, SDL_Color _bg):
		UIElement(x, y, w, h), fg(_fg), bg(_bg), contents(text), font_size(size) {
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, rect.h, 32, 0, 0, 0, 0);
		update_surface();
		on_key = key_callback;
	}
	
	void TextInput::update_surface() {
		SDL_FillRect(surface, NULL, SDL_MapRGBA(surface->format, fg.r, fg.g, fg.b, 255));
		SDL_Rect r = {1, 1, surface->w-2, surface->h-2};
		SDL_FillRect(surface, &r, SDL_MapRGBA(surface->format, bg.r, bg.g, bg.b, 255));
		
		if (contents.size() > 0) {
			TTF_Font* font = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", font_size);
			FontStuff::BlitText(surface, r, font, fg, contents);
		}
		
		if (menu)
			menu->redraw(); // FIXME don't redraw the whole screen
	}
	
	Action TextInput::key_callback(UIElement* not_this, SDL_keysym keysym) {
		TextInput* also_not_this = static_cast<TextInput*>(not_this);
		if (keysym.sym == SDLK_BACKSPACE) {
			if (also_not_this->contents.size() > 0)
				also_not_this->contents.erase(also_not_this->contents.size() - 1);
		}
		else {
			also_not_this->contents += (char)keysym.unicode;
		}
		also_not_this->update_surface();
		Action a = { Action::NOTHING, NULL };
		return a;
	}
	
	Menu::Menu() : background(NULL), focused(NULL) {}
	
	void Menu::redraw() {
		if (background)
			assert(SDL_BlitSurface(background, NULL, screen, NULL) == 0);
		else
			SDL_FillRect(screen, NULL, SDL_MapRGBA(screen->format, 0, 0, 0, 255));
		
		BOOST_FOREACH(UIElement* e, elements) {
			e->draw(screen);
		}
		SDL_UpdateRect(screen, 0, 0, 0, 0);
	}
	
	void Menu::show() {
		redraw();
		
		if (do_stuff) {
			do_stuff();
		}
		else {
			while(true) {
				SDL_Event event;
		
				if(SDL_PollEvent(&event)) {
					if(event.type == SDL_QUIT) {
						exit(0);
					}
					else if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
						BOOST_FOREACH(UIElement* e, elements) {
							if(point_in_rect(event.button.x, event.button.y, &e->rect)) {
								if(e->on_click) {
									Action rv = e->on_click(e, event.button.x - e->rect.x, event.button.y - e->rect.y);
									if (rv.type == Action::PUSH_MENU) {
										rv.new_menu->show();
										redraw();
									}
									else if (rv.type == Action::POP_MENU) {
										return;
									}
								}
								focused = e;
							}
						}
					}
					else if(event.type == SDL_KEYDOWN) {
						if(event.key.keysym.sym == SDLK_ESCAPE) {
							//return; ?
							exit(0);
						}
						else if (focused && focused->on_key) {
							focused->on_key(focused, event.key.keysym);
						}
					}
				}
			}
		}
	}
	
	Action quit_callback(UIElement*, int, int) { exit(0); }
	Action return_callback(UIElement*, int, int) { Action a = { Action::POP_MENU, NULL }; return a; }
	Action goto_join_callback(UIElement*, int, int) { Action a = { Action::PUSH_MENU, &join_menu }; return a; }
	Action goto_host_callback(UIElement*, int, int) { Action a = { Action::PUSH_MENU, &host_menu }; return a; }
	Action start_server_callback(UIElement*, int, int) { Action a = { Action::PUSH_MENU, &wait_for_players_menu }; return a; }
	Action join_server_callback(UIElement*, int, int) { Action a = { Action::PUSH_MENU, &connect_menu }; return a; }
	void host_do_stuff() {
		Scenario scn;
		LoadScenario(host_scenario_box->contents, scn);
		uint16_t port = atoi(host_port_box->contents.c_str());
		Server server(port, scn, 2);
		
		Client client("127.0.0.1", port, "test");
		
		do {
			server.DoStuff();
			SDL_Delay(5);
		} while(client.DoStuff());
		
		reinit_screen();
	}
	
	void join_do_stuff() {
		std::string host = join_hostname_box->contents;
		int port = atoi(join_port_box->contents.c_str());
		Client client(host, port, "test");

		while (client.DoStuff()) {
			uint8_t st = SDL_GetAppState();
			if (st == SDL_APPACTIVE || !st)
				SDL_Delay(200);
			else
				SDL_Delay(5);
		}
	}
	
	void init() {
		SDL_Surface* screen = SDL_SetVideoMode(800, 600, 0, SDL_SWSURFACE);
		
		SDL_Color textbox_fg = {0, 0, 0};
		SDL_Color textbox_bg = {255, 255, 255};
		
		main_menu.screen = screen;
		main_menu += new Button(350, 250, 100, 30, "graphics/menus/host_game.png", goto_host_callback);
		main_menu += new Button(350, 300, 100, 30, "graphics/menus/join_game.png", goto_join_callback);
		main_menu += new Button(700, 570, 100, 30, "graphics/menus/quit.png", quit_callback);
		main_menu.background = ImgStuff::GetImage("graphics/menu_background.png");
		
		join_menu.screen = screen;
		join_menu += new Button(315, 250, 50, 30, "graphics/menus/host.png", NULL);
		join_hostname_box = new TextInput(375, 255, 200, 20, "", 14, textbox_fg, textbox_bg);
		join_menu += join_hostname_box;
		join_menu += new Button(325, 300, 40, 30, "graphics/menus/port.png", NULL);
		join_port_box = new TextInput(375, 305, 200, 20, "9001", 14, textbox_fg, textbox_bg);
		join_menu += join_port_box;
		join_menu += new Button(350, 350, 100, 30, "graphics/menus/join_game.png", join_server_callback);
		join_menu += new Button(0, 570, 100, 30, "graphics/menus/back.png", return_callback);
		join_menu += new Button(700, 570, 100, 30, "graphics/menus/quit.png", quit_callback);
		join_menu.background = main_menu.background;
		
		host_menu.screen = screen;
		host_menu += new Button(325, 250, 40, 30, "graphics/menus/port.png", NULL);
		host_port_box = new TextInput(375, 255, 200, 20, "9001", 14, textbox_fg, textbox_bg);
		host_menu += host_port_box;
		host_menu += new Button(275, 300, 90, 30, "graphics/menus/scenario.png", NULL);
		host_scenario_box = new TextInput(375, 305, 200, 20, "scenario/hex_2p.txt", 14, textbox_fg, textbox_bg);
		host_menu += host_scenario_box;
		host_menu += new Button(350, 350, 100, 30, "graphics/menus/host_game.png", start_server_callback);
		host_menu += new Button(0, 570, 100, 30, "graphics/menus/back.png", return_callback);
		host_menu += new Button(700, 570, 100, 30, "graphics/menus/quit.png", quit_callback);
		host_menu.background = main_menu.background;
		
		wait_for_players_menu.screen = screen;
		wait_for_players_menu += new Button(300, 250, 40, 30, "graphics/menus/waiting_for_players.png", NULL);
		wait_for_players_menu.background = main_menu.background;
		wait_for_players_menu.do_stuff = host_do_stuff;
		
		connect_menu.screen = screen;
		connect_menu.do_stuff = join_do_stuff;
	}
	
	void reinit_screen() {
		SDL_Surface* screen = SDL_SetVideoMode(800, 600, 0, SDL_SWSURFACE);
		main_menu.screen = join_menu.screen = host_menu.screen = wait_for_players_menu.screen = connect_menu.screen = screen;
	}
}