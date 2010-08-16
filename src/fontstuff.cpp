#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <string>
#include <stdexcept>
#include <map>

#include "fontstuff.hpp"

typedef std::map<std::string,TTF_Font*> FontCache;

FontCache font_cache;

TTF_Font *FontStuff::LoadFont(std::string filename, int size) {
	FontCache::iterator i = font_cache.find(filename + (char)size);
	if(i != font_cache.end()) {
		return i->second;
	}
	
	TTF_Font *font = TTF_OpenFont(filename.c_str(), size);
	if(!font) {
		throw std::runtime_error("Unable to load font '" + filename + "': " + TTF_GetError());
	}
	
	font_cache.insert(std::make_pair(filename + (char)size, font));
	
	return font;
}

void FontStuff::FreeFonts(void) {
	FontCache::iterator i = font_cache.begin();
	
	for(; i != font_cache.end(); i++) {
		TTF_CloseFont(i->second);
	}
	
	font_cache.clear();
}

void FontStuff::BlitText(SDL_Surface *surface, SDL_Rect rect, TTF_Font *font, SDL_Color colour, std::string text) {
	SDL_Surface *rt = TTF_RenderText_Blended(font, text.c_str(), colour);
	if(!rt) {
		throw std::runtime_error(std::string("Unable to render text: ") + TTF_GetError());
	}
	
	if(SDL_BlitSurface(rt, NULL, surface, &rect) == -1) {
		throw std::runtime_error(std::string("Unable to blit text: ") + SDL_GetError());
	}
	
	SDL_FreeSurface(rt);
}

int FontStuff::TextWidth(TTF_Font *font, const std::string &text) {
	int width;
	if(TTF_SizeText(font, text.c_str(), &width, NULL) == -1) {
		throw std::runtime_error("TTF_SizeText() failed on \"" + text + "\"");
	}
	
	return width;
}
