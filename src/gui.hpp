#ifndef OR_GUI_HPP
#define OR_GUI_HPP

#include <string>
#include <SDL/SDL.h>
#include <set>

extern SDL_Surface *screen;

class GUI {
	public:
	
	struct Thing {
		virtual void HandleEvent(const SDL_Event &event) {};
		virtual void Draw() {};
		
		int x, y, w, h;
		int tab_order;
		bool enabled;
		
		Thing() : enabled(true) {}
		
		bool operator()(const Thing *left, const Thing *right) {
			return left->tab_order < right->tab_order;
		}
		
		void enable(bool enable) {
			enabled = enable;
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
	};
	
	typedef std::set<Thing*,Thing> thing_set;
	
	public:
		GUI(int ax, int ay, int aw, int ah);
		
		void set_bg_colour(int r, int g, int b);
		void set_bg_image(SDL_Surface *img);
		
		void poll(bool read_events);
		void HandleEvent(const SDL_Event &event);
	
	private:
		int x, y, w, h;
		Uint32 bgcolour;
		SDL_Surface *bgimg;
		
		thing_set things;
		thing_set::iterator focus;
		
		void add_thing(Thing *thing);
		void del_thing(Thing *thing);
		void focus_next();
};

#endif /* !OR_GUI_HPP */
