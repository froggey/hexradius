#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <string>
#include <map>

#include "loadimage.hpp"

static std::map<std::string,SDL_Surface*> image_cache;

SDL_Surface *OctRadius::LoadImage(std::string filename) {
	std::map<std::string,SDL_Surface*>::iterator i = image_cache.find(filename);
	if(i != image_cache.end()) {
		return i->second;
	}
	
	SDL_Surface *s = IMG_Load(filename.c_str());
	if(!s) {
		return NULL;
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
