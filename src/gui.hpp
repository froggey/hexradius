#ifndef OR_GUI_HPP
#define OR_GUI_HPP

#include "loadimage.hpp"
#include "fontstuff.hpp"

#include <string>
#include <SDL/SDL.h>
#include <set>
#include <SDL/SDL_ttf.h>
#include <vector>
#include <boost/shared_ptr.hpp>

class GUI {
	public:
	
	enum alignment { LEFT, CENTER, RIGHT };
	
	struct Thing {
		virtual void HandleEvent(const SDL_Event &event) {};
		virtual void Draw() {};
		
		GUI &gui;
		
		int x, y, w, h;
		int tab_order;
		bool enabled;
		
		Thing(GUI &g) : gui(g), enabled(true) {}
		
		void enable(bool enable) {
			enabled = enable;
		}
		
		bool has_focus() {
			return gui.v_focus && *(gui.focus) == this;
		}
	};
	
	struct thing_compare {
		bool operator()(const Thing *left, const Thing *right) {
			if(left->tab_order == 0 && right->tab_order == 0) {
				return left < right;
			}
			
			return left->tab_order < right->tab_order;
		}
	};
	
	struct ImgButton : Thing {
		typedef void (*callback_f)(const ImgButton &button, const SDL_Event &event, void *arg);
		
		callback_f callback;
		void *callback_arg;
		
		SDL_Surface *image;
		
		int x_down, y_down;
		
		ImgButton(GUI &_gui, SDL_Surface *img, int ax, int ay, int to, callback_f cb = NULL, void *arg = NULL);
		~ImgButton();
		
		void HandleEvent(const SDL_Event &event);
		void Draw();
	};
	
	struct TextBox : Thing {
		typedef bool (*bool_callback)(const TextBox &tbox, const SDL_Event &event, void *arg);
		typedef void (*void_callback)(const TextBox &tbox, const SDL_Event &event, void *arg);
		
		std::string text;
		unsigned int insert_offset;
		
		void_callback enter_callback;
		void *enter_callback_arg;
		
		bool_callback input_callback;
		void *input_callback_arg;
		
		TextBox(GUI &g, int ax, int ay, int aw, int ah, int to);
		~TextBox();
		
		void set_text(const std::string &new_text);
		
		void set_enter_callback(void_callback callback, void *arg = NULL) {
			enter_callback = callback;
			enter_callback_arg = arg;
		}
		
		void set_input_callback(bool_callback callback, void *arg = NULL) {
			input_callback = callback;
			input_callback_arg = arg;
		}
		
		void HandleEvent(const SDL_Event &event);
		void Draw();
	};
	
	struct TextDisplay : Thing {
		std::string text;
		TTF_Font *font;
		SDL_Colour colour;
		
		TextDisplay(GUI &g, int ax, int ay, std::string txt = "");
		~TextDisplay();
		
		void set_font(std::string name, int size) {
			font = FontStuff::LoadFont(name, size);
		}
		
		void Draw();
	};
	
	struct TextButton : Thing {
		typedef void (*void_callback)(const TextButton &button, const SDL_Event &event, void *arg);
		
		std::string m_text;
		TTF_Font *m_font;
		alignment m_align;
		SDL_Colour m_fgc;
		SDL_Colour m_bgc;
		bool m_borders;
		
		uint8_t m_opacity;
		SDL_Surface *m_bgs;
		
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
			assert(SDL_FillRect(m_bgs, NULL, ImgStuff::MapColour(m_bgc)) == 0);
		}
		
		void set_bg_colour(const SDL_Colour &colour) {
			m_bgc = colour;
			assert(SDL_FillRect(m_bgs, NULL, ImgStuff::MapColour(m_bgc)) == 0);
		}
		
		void Draw();
		void HandleEvent(const SDL_Event &event);
	};
	
	struct DropDown : Thing {
		struct Item {
			Item() {}
			Item(std::string t, SDL_Colour c, int ia = 0, int ib = 0) : text(t), colour(c), i1(ia), i2(ib) {}
			
			std::string text;
			SDL_Colour colour;
			int i1, i2;
		};
		
		typedef bool (*bool_callback)(const GUI::DropDown &drop, const Item &item, void *arg);
		
		typedef std::vector<Item> item_list;
		
		TextButton button;
		
		item_list items;
		item_list::iterator selected;
		
		std::vector<boost::shared_ptr<TextButton> > item_buttons;
		
		bool_callback callback;
		void *callback_arg;
		
		DropDown(GUI &g, int ax, int ay, int aw, int ah, int to);
		~DropDown();
		
		void Draw();
		void HandleEvent(const SDL_Event &event);
		
		void select(item_list::iterator item);
	};
	
	struct Checkbox : Thing {
		typedef void (*void_callback)(const GUI::Checkbox &checkbox, void *arg);
		
		bool state;
		
		int x_down, y_down;
		
		void_callback toggle_callback;
		void *toggle_callback_arg;
		
		Checkbox(GUI &g, int ax, int ay, int aw, int ah, int to, bool default_state = false);
		~Checkbox();
		
		void set_callback(void_callback callback, void *arg = NULL);
		
		void Draw();
		void HandleEvent(const SDL_Event &event);
	};
	
	typedef std::set<Thing*,thing_compare> thing_set;
	
	public:
		typedef void (*void_callback)(const GUI &gui, const SDL_Event &event, void *arg);
		
		GUI(int ax, int ay, int aw, int ah);
		
		void set_bg_colour(int r, int g, int b);
		void set_bg_image(SDL_Surface *img);
		
		void set_quit_callback(void_callback callback, void *arg = NULL);
		
		void poll(bool read_events);
		void handle_event(const SDL_Event &event);
	
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
