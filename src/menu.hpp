#ifndef OR_MENU_HPP
#define OR_MENU_HPP

#include <SDL.h>
#include <string>
#include <vector>

namespace Menu {
	struct Action;
	struct UIElement;
	struct Menu;
	typedef Action (*ClickCallback)(UIElement* e, int x, int y);
	typedef Action (*KeyCallback)(UIElement* e, SDL_keysym key);
	
	struct UIElement {
		Menu* menu;
		SDL_Surface* surface;
		SDL_Rect rect;
		ClickCallback on_click;
		KeyCallback on_key;
		
		UIElement(int _x, int _y, int _w, int _h);
		void draw(SDL_Surface* screen);
	};
	
	struct Button: public UIElement {
		Button(int x, int y, int w, int h, std::string image, ClickCallback _on_click);
	};
	
	struct TextInput: public UIElement {
		std::string contents;
		SDL_Color fg, bg;
		int font_size;
		
		TextInput(int x, int y, int w, int h, std::string text, int size, SDL_Color fg, SDL_Color bg);
		void update_surface();
		// static because C++ hates me and won't let me put a member function pointer in a function pointer variable.
		static Action key_callback(UIElement* not_this, SDL_keysym keysym);
	};
	
	struct Menu {
		SDL_Surface* screen;
		std::vector<UIElement*> elements;
		UIElement* focused;
		SDL_Surface* background;
		void (*do_stuff)();
		
		Menu();
		void redraw();
		void show();
		
		inline Menu& operator+=(UIElement* e) { elements.push_back(e); e->menu = this; return *this; }
	};
	
	struct Action {
		enum {
			NOTHING,
			PUSH_MENU,
			POP_MENU
		} type;
		Menu* new_menu;
	};
	
	extern Menu main_menu, host_menu, join_menu;
	
	void init();
	void reinit_screen();
}

#endif