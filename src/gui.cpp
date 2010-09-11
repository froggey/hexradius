#include <utility>
#include <assert.h>
#include <SDL/SDL.h>
#include <stdexcept>
#include <iostream>

#include "gui.hpp"
#include "fontstuff.hpp"

SDL_Surface *screen = NULL;

GUI::GUI(int ax, int ay, int aw, int ah) : quit_callback(NULL), x(ax), y(ay), w(aw), h(ah), v_focus(false) {
	set_bg_colour(0, 0, 0);
}

void GUI::set_bg_colour(int r, int g, int b) {
	bgcolour = SDL_MapRGB(screen->format, r, g, b);
	bgimg = NULL;
}

void GUI::set_bg_image(SDL_Surface *img) {
	bgimg = img;
}

void GUI::poll(bool read_events) {
	SDL_Event event;
	
	while(read_events && SDL_PollEvent(&event)) {
		HandleEvent(event);
	}
	
	if(bgimg) {
		SDL_Rect rect = {0,0,0,0};
		SDL_Rect srect = rect;
		
		for(rect.x = x; rect.x < x+w; rect.x += bgimg->w) {
			for(rect.y = y; rect.y < y+h; rect.y += bgimg->h) {
				srect.w = (x+w - rect.x >= bgimg->w ? bgimg->w : x+w - rect.x);
				srect.h = (y+h - rect.y >= bgimg->h ? bgimg->h : y+h - rect.y);
				
				assert(SDL_BlitSurface(bgimg, &srect, screen, &rect) == 0);
			}
		}
	}else{
		SDL_Rect rect = {x,y,w,h};
		assert(SDL_FillRect(screen, &rect, bgcolour) == 0);
	}
	
	for(thing_set::iterator t = things.begin(); t != things.end(); t++) {
		if(!(v_focus && t == focus)) {
			(*t)->Draw();
		}
	}
	
	if(v_focus) {
		(*focus)->Draw();
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void GUI::HandleEvent(const SDL_Event &event) {
	switch(event.type) {
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			for(thing_set::iterator t = things.begin(); t != things.end(); t++) {
				if(event.button.x >= (*t)->x && event.button.x < (*t)->x+(*t)->w && event.button.y >= (*t)->y && event.button.y < (*t)->y+(*t)->h && (*t)->enabled) {
					(*t)->HandleEvent(event);
					break;
				}
			}
			
			break;
			
		case SDL_KEYDOWN:
			if(event.key.keysym.sym == SDLK_TAB) {
				focus_next();
				break;
			}
			
		case SDL_KEYUP:
			if(v_focus) {
				(*focus)->HandleEvent(event);
			}
			
			break;
			
		case SDL_QUIT:
			if(quit_callback) {
				quit_callback(*this, event);
			}
			
			break;
			
		default:
			break;
	}
}

void GUI::add_thing(Thing *thing) {
	assert(things.find(thing) == things.end());
	things.insert(thing);
	
	if(!v_focus && thing->enabled) {
		focus = things.find(thing);
	}
}

void GUI::del_thing(Thing *thing) {
	thing_set::iterator i = things.find(thing);
	
	if(focus == i) {
		focus_next();
	}
	
	things.erase(i);
}

void GUI::focus_next() {
	thing_set::iterator fv = things.end(), of = focus;
	
	for(thing_set::iterator i = things.begin(); i != things.end(); i++) {
		if((*i)->enabled) {
			fv = i;
			break;
		}
	}
	
	if(fv == things.end()) {
		v_focus = false;
		return;
	}
	
	if(!v_focus) {
		focus = fv;
		v_focus = true;
		return;
	}
	
	do {
		focus++;
	} while(focus != things.end() && !(*focus)->enabled);
	
	if(focus == things.end()) {
		if(fv == of) {
			v_focus = false;
		}else{
			focus = fv;
		}
	}
}

GUI::ImgButton::ImgButton(GUI &_gui, SDL_Surface *img, int ax, int ay, int to, callback_f cb, void *arg) : gui(_gui), callback(cb), callback_arg(arg), image(img) {
	x = ax;
	y = ay;
	w = image->w;
	h = image->h;
	tab_order = to;
	
	gui.add_thing(this);
}

GUI::ImgButton::~ImgButton() {
	gui.del_thing(this);
}

void GUI::ImgButton::HandleEvent(const SDL_Event &event) {
	if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
		x_down = event.button.x;
		y_down = event.button.y;
		
		gui.focus = gui.things.find(this);
	}
	
	if(event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
		if(event.button.x == x_down && event.button.y == y_down && callback) {
			callback(*this, event, callback_arg);
		}
	}
}

void GUI::ImgButton::Draw() {
	SDL_Rect srect = {0, 0, image->w/2, image->h/2};
	SDL_Rect rect = {x, y, 0, 0};
	
	if(*gui.focus == this) {
		srect.x = srect.w;
	}
	
	assert(SDL_BlitSurface(image, NULL, screen, &rect) == 0);
}

GUI::TextBox::TextBox(GUI &g, int ax, int ay, int aw, int ah, int to) : gui(g) {
	x = gui.x + ax;
	y = gui.y + ay;
	w = aw;
	h = ah;
	tab_order = to;
	
	enter_callback = NULL;
	enter_callback_arg = NULL;
	
	input_callback = NULL;
	input_callback_arg = NULL;
	
	gui.add_thing(this);
}

GUI::TextBox::~TextBox() {
	gui.del_thing(this);
}

void GUI::TextBox::HandleEvent(const SDL_Event &event) {
	if(event.type == SDL_MOUSEBUTTONDOWN) {
		gui.focus = gui.things.find(this);
	}else if(event.type == SDL_KEYDOWN) {
		if(event.key.keysym.sym == SDLK_BACKSPACE) {
			if(!text.empty()) {
				text.erase(text.size()-1);
			}
		}else if(event.key.keysym.sym == SDLK_RETURN) {
			if(enter_callback) {
				enter_callback(*this, event, enter_callback_arg);
			}
		}else if(isprint(event.key.keysym.sym)) {
			if(input_callback) {
				if(input_callback(*this, event, input_callback_arg)) {
					text.append(1, event.key.keysym.sym);
				}else{
					std::cerr << "Illegal character" << std::endl;
				}
			}else{
				text.append(1, event.key.keysym.sym);
			}
		}
	}
}

void GUI::TextBox::Draw() {
	SDL_Rect rect = {x, y, w, h};
	
	assert(SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 0, 0)) == 0);
	
	rect.x += 1;
	rect.y += 1;
	rect.w -= 1;
	rect.h -= 1;
	
	assert(SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 255, 255, 255)) == 0);
	
	rect.x += 1;
	rect.y += 1;
	
	TTF_Font *font = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 14);
	SDL_Colour fc = {0,0,0,0};
	
	FontStuff::BlitText(screen, rect, font, fc, text);
}

GUI::TextDisplay::TextDisplay(GUI &g, int ax, int ay, int to, std::string txt) : gui(g) {
	x = gui.x + ax;
	y = gui.y + ay;
	w = h = 0;
	tab_order = to;
	enabled = false;
	
	text = txt;
	font = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 12);
	colour = (SDL_Colour){255, 255, 255, 0};
	
	gui.add_thing(this);
}

GUI::TextDisplay::~TextDisplay() {
	gui.del_thing(this);
}

void GUI::TextDisplay::Draw() {
	SDL_Rect rect = {x, y, 0, 0};
	FontStuff::BlitText(screen, rect, font, colour, text);
}
