#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <string>
#include <map>
#include <stdexcept>
#include <assert.h>
#include <iostream>

#include "loadimage.hpp"
#include "octradius.hpp"

SDL_Surface *screen = NULL;
int screen_w = -1, screen_h = -1;

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

#define FOOBAR1(v) std::max(0, std::min(255, (v)))
#define FOOBAR(v) FOOBAR1(v + tint.v)

void ImgStuff::TintSurface(SDL_Surface *surface, const TintValues &tint) {
	ensure_SDL_LockSurface(surface);

	for(int x = 0; x < surface->w; x++) {
		for(int y = 0; y < surface->h; y++) {
			Uint32 pixel = GetPixel(surface, x, y);

			Uint8 r, g, b, a;
			SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);
			
			r = FOOBAR(r);
			g = FOOBAR(g);
			b = FOOBAR(b);
			a = FOOBAR1((a * tint.a) / 255);

			pixel = SDL_MapRGBA(surface->format, r, g, b, a);

			SetPixel(surface, x, y, pixel);
		}
	}

	SDL_UnlockSurface(surface);
}

Uint32 ImgStuff::MapColour(SDL_Colour &colour) {
	return SDL_MapRGB(screen->format, colour.r, colour.g, colour.b);
}

void ImgStuff::draw_rect(SDL_Rect rect, const SDL_Colour &colour, uint8_t alpha) {
	SDL_Surface *s = SDL_CreateRGBSurface(SDL_SWSURFACE, rect.w, rect.h, screen->format->BitsPerPixel, 0, 0, 0, 0);
	assert(s);

	ensure_SDL_FillRect(s, NULL, SDL_MapRGB(s->format, colour.r, colour.g, colour.b));
	ensure_SDL_SetAlpha(s, SDL_SRCALPHA, alpha);

	ensure_SDL_BlitSurface(s, NULL, screen, &rect);

	SDL_FreeSurface(s);
}

void ImgStuff::set_mode(int w, int h) {
	if(w != screen_w || h != screen_h) {
		screen = SDL_SetVideoMode(w, h, 0, SDL_SWSURFACE);
		assert(screen != NULL);

		screen_w = w;
		screen_h = h;
	}
}
