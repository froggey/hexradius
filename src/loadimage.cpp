#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <string>
#include <map>
#include <stdexcept>

#include "loadimage.hpp"

static std::map<std::string,SDL_Surface*> image_cache;

SDL_Surface *OctRadius::LoadImage(std::string filename) {
	std::map<std::string,SDL_Surface*>::iterator i = image_cache.find(filename);
	if(i != image_cache.end()) {
		return i->second;
	}
	
	SDL_Surface *s = IMG_Load(filename.c_str());
	if(!s) {
		throw std::runtime_error("Unable to load image '" + filename + "': " + SDL_GetError());
	}
	
	image_cache.insert(std::make_pair(filename, s));
	return s;
}

void OctRadius::FreeImages(void) {
	std::map<std::string,SDL_Surface*>::iterator i = image_cache.begin();
	
	while(i != image_cache.end()) {
		SDL_FreeSurface(i->second);
		i++;
	}
	
	image_cache.clear();
}

Uint32 OctRadius::GetPixel(SDL_Surface *surface, int x, int y) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	
	switch (bpp) {
		case 1:
			return *p;
			
		case 2:
			return *(Uint16 *)p;
			
		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				return p[0] << 16 | p[1] << 8 | p[2];
			}else{
				return p[0] | p[1] << 8 | p[2] << 16;
			}
			
		case 4:
			return *(Uint32 *)p;
			
		default:
			return 0;
	}
}
