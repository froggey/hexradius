#ifndef OR_GUI_HPP
#define OR_GUI_HPP

#include "loadimage.hpp"

#include <string>
#include <SDL/SDL.h>
#include <set>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <boost/shared_ptr.hpp>

extern SDL_Surface *screen;

class GUI {
	public:
	
	enum alignment { LEFT, CENTER, RIGHT };
	
	struct Thing {
		virtual void HandleEvent(const SDL_Event &event) {};
		virtual void Draw() {};
		
		int x, y, w, h;
		int tab_order;
		bool enabled;
		
		Thing() : enabled(true) {}
		
		bool operator()(const Thing *left, const Thing *right) {
			if(left->tab_order == 0 && right->tab_order == 0) {
				return left < right;
			}
			
			return left->tab_order < right->tab_order;
		}
		
		void enable(bool enable) {
			enabled = enable;
		}
		
		virtual bool has_focus() { return false; };
		
		bool r_has_focus(GUI &gui) {
			return gui.v_focus && *(gui.focus) == this;
		}
	};
	
	struct ImgButton : Thing {
		typedef void (*callback_f)(const ImgButton &button, const SDL_Event &event, void *arg);
		
		GUI &gui;
		
		callback_f callback;
		void *callback_arg;
		
		SDL_Surface *image;
		
		int x_down, y_down;
		
		ImgButton(GUI &_gui, SDL_Surface *img, int ax, int ay, int to, callback_f cb = NULL, void *arg = NULL);
		~ImgButton();
		
		void HandleEvent(const SDL_Event &event);
		void Draw();
		
		bool has_focus() {
			return r_has_focus(gui);
		}
	};
	
	struct TextBox : Thing {
		typedef bool (*bool_callback)(const TextBox &tbox, const SDL_Event &event, void *arg);
		typedef void (*void_callback)(const TextBox &tbox, const SDL_Event &event, void *arg);
		
		GUI &gui;
		
		std::string text;
		
		void_callback enter_callback;
		void *enter_callback_arg;
		
		bool_callback input_callback;
		void *input_callback_arg;
		
		TextBox(GUI &g, int ax, int ay, int aw, int ah, int to);
		~TextBox();
		
		void HandleEvent(const SDL_Event &event);
		void Draw();
		
		bool has_focus() {
			return r_has_focus(gui);
		}
	};
	
	struct TextDisplay : Thing {
		GUI &gui;
		
		std::string text;
		TTF_Font *font;
		SDL_Colour colour;
		
		TextDisplay(GUI &g, int ax, int ay, int to, std::string txt = "");
		~TextDisplay();
		
		void Draw();
	};
	
	struct TextButton : Thing {
		typedef void (*void_callback)(const TextButton &button, const SDL_Event &event, void *arg);
		
		GUI &gui;
		
		std::string m_text;
		TTF_Font *m_font;
		alignment m_align;
		SDL_Colour m_fgc;
		SDL_Colour m_bgc;
		bool m_borders;
		
		void_callback m_callback;
		void *m_arg;
		
		int x_down, y_down;
		
		TextButton(GUI &g, int ax, int ay, int aw, int ah, int to, std::string text, void_callback callback = NULL, void *callback_arg = NULL);
		~TextButton();
		
		void align(alignment align) {
			m_align = align;
		}
		
		void set_fg_colour(int r, int g, int b) {
			m_fgc = ImgStuff::Colour(r, g, b);
		}
		
		void set_fg_colour(const SDL_Colour &colour) {
			m_fgc = colour;
		}
		
		void set_bg_colour(int r, int g, int b) {
			m_bgc = ImgStuff::Colour(r, g, b);
		}
		
		void set_bg_colour(const SDL_Colour &colour) {
			m_bgc = colour;
		}
		
		void Draw();
		void HandleEvent(const SDL_Event &event);
		
		bool has_focus() {
			return r_has_focus(gui);
		}
	};
	
	struct DropDown : Thing {
		struct Item {
			Item() {}
			Item(std::string t, SDL_Colour c) : text(t), colour(c) {}
			
			std::string text;
			SDL_Colour colour;
		};
		
		GUI &gui;
		
		TextButton button;
		
		std::vector<Item> items;
		std::vector<boost::shared_ptr<TextButton> > item_buttons;
		
		DropDown(GUI &g, int ax, int ay, int aw, int ah, int to);
		~DropDown();
		
		void Draw();
		void HandleEvent(const SDL_Event &event);
		
		bool has_focus() {
			return r_has_focus(gui);
		}
	};
	
	typedef std::set<Thing*,Thing> thing_set;
	
	public:
		typedef void (*void_callback)(const GUI &gui, const SDL_Event &event, void *arg);
		
		GUI(int ax, int ay, int aw, int ah);
		
		void set_bg_colour(int r, int g, int b);
		void set_bg_image(SDL_Surface *img);
		
		void set_quit_callback(void_callback callback, void *arg = NULL);
		
		void poll(bool read_events);
		void HandleEvent(const SDL_Event &event);
	
	private:
		int x, y, w, h;
		Uint32 bgcolour;
		SDL_Surface *bgimg;
		
		thing_set things;
		
		bool v_focus;
		thing_set::iterator focus;
		
		void_callback quit_callback;
		void *quit_callback_arg;
		
		void add_thing(Thing *thing);
		void del_thing(Thing *thing);
		void focus_next();
};

#endif /* !OR_GUI_HPP */
