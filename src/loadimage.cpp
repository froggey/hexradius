#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <string>
#include <map>
#include <stdexcept>
#include <assert.h>
#include <iostream>

#include "loadimage.hpp"

extern SDL_Surface *screen;

struct image_cache_key {
	std::string filename;
	ImgStuff::TintValues tint;
	
	image_cache_key(std::string f, const ImgStuff::TintValues &t) : filename(f), tint(t) {}
	
	bool operator<(const image_cache_key &k) const {
		if(filename != k.filename) {
			return filename < k.filename;
		}
		if(tint.r != k.tint.r) {
			return tint.r < k.tint.r;
		}
		if(tint.g != k.tint.g) {
			return tint.g < k.tint.g;
		}
		if(tint.b != k.tint.b) {
			return tint.b < k.tint.b;
		}
		
		return tint.a < k.tint.a;
	}
};

static std::map<image_cache_key,SDL_Surface*> image_cache;

SDL_Surface *ImgStuff::GetImage(std::string filename, const TintValues &tint) {
	image_cache_key key(filename, tint);
	
	std::map<image_cache_key,SDL_Surface*>::iterator i = image_cache.find(key);
	if(i != image_cache.end()) {
		return i->second;
	}
	
	SDL_Surface *s = GetImageNC(filename);
	
	if(tint.HazTint()) {
		TintSurface(s, tint);
	}
	
	SDL_Surface *conv = SDL_DisplayFormatAlpha(s);
	if(!conv) {
		throw std::runtime_error(std::string("Unable to convert surface to display format: ") + SDL_GetError());
	}
	
	SDL_FreeSurface(s);
	
	image_cache.insert(std::make_pair(key, conv));
	
	return conv;
}

/* Load an image without using the cache */
SDL_Surface *ImgStuff::GetImageNC(std::string filename) {
	SDL_Surface *s = IMG_Load(filename.c_str());
	if(!s) {
		throw std::runtime_error("Unable to load image '" + filename + "': " + SDL_GetError());
	}
	
	SDL_Surface *conv = SDL_DisplayFormatAlpha(s);
	if(!conv) {
		throw std::runtime_error(std::string("Unable to convert surface to display format: ") + SDL_GetError());
	}
	
	SDL_FreeSurface(s);
	
	return conv;
}

void ImgStuff::FreeImages(void) {
	std::map<image_cache_key,SDL_Surface*>::iterator i = image_cache.begin();
	
	while(i != image_cache.end()) {
		SDL_FreeSurface(i->second);
		i++;
	}
	
	image_cache.clear();
}

/* Fetch a single pixel from a surface
 * Courtesy of the SDL wiki
*/
Uint32 ImgStuff::GetPixel(SDL_Surface *surface, int x, int y) {
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

/* Set a pixel in an SDL surface
 * Courtesy of the SDL wiki
*/
void ImgStuff::SetPixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
	int bpp = surface->format->BytesPerPixel;
	Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;
	
	switch (bpp) {
		case 1:
			*p = pixel;
			break;
			
		case 2:
			*(Uint16 *)p = pixel;
			break;
			
		case 3:
			if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
				p[0] = (pixel >> 16) & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = pixel & 0xff;
			}else {
				p[0] = pixel & 0xff;
				p[1] = (pixel >> 8) & 0xff;
				p[2] = (pixel >> 16) & 0xff;
			}
			break;
			
		case 4:
			*(Uint32 *)p = pixel;
			break;
			
		default:
			break;
	}
}

#define FOOBAR(v) \
	if((int)v + tint.v > 255) { \
		v = 255; \
	}else if((int)v + tint.v < 0) { \
		v = 0; \
	}else{ \
		v += tint.v; \
	}

void ImgStuff::TintSurface(SDL_Surface *surface, const TintValues &tint) {
	assert(SDL_LockSurface(surface) == 0);
	
	for(int x = 0; x < surface->w; x++) {
		for(int y = 0; y < surface->h; y++) {
			Uint32 pixel = GetPixel(surface, x, y);
			
			Uint8 r, g, b, a;
			SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
			
			FOOBAR(r);
			FOOBAR(g);
			FOOBAR(b);
			FOOBAR(a);
			
			pixel = SDL_MapRGBA(surface->format, r, g, b, a);
			
			SetPixel(surface, x, y, pixel);
		}
	}
	
	SDL_UnlockSurface(surface);
}

Uint32 ImgStuff::MapColour(SDL_Colour &colour) {
	return SDL_MapRGB(screen->format, colour.r, colour.g, colour.b);
}
