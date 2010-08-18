#ifndef OR_LOADIMAGE_HPP
#define OR_LOADIMAGE_HPP

#include <SDL/SDL.h>

namespace OctRadius {
	SDL_Surface *LoadImage(std::string filename, bool usecache = true);
	void FreeImages(void);
	Uint32 GetPixel(SDL_Surface *surface, int x, int y);
	void SetPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
}

#endif /* !OR_LOADIMAGE_HPP */
