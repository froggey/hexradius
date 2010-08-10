#ifndef OR_LOADIMAGE_HPP
#define OR_LOADIMAGE_HPP

#include <SDL/SDL.h>

namespace OctRadius {
	SDL_Surface *LoadImage(std::string filename);
	void FreeImages(void);
}

#endif /* !OR_LOADIMAGE_HPP */
