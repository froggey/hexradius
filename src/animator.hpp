#ifndef OR_ANIMATOR_HPP
#define OR_ANIMATOR_HPP

#include <SDL/SDL.h>

#include "octradius.hpp"

namespace Animators {
class Generic {
public:
	virtual ~Generic();
	virtual bool render() = 0;
};

class ImageAnimation : public Generic {
public:
	int tx, ty;
	Uint32 init_ticks;
	Uint32 runtime;
	SDL_Surface *image;

	ImageAnimation(int tile_x, int tile_y, Uint32 runtime, const std::string &image);
	bool render();
};

class PawnCrush : public ImageAnimation {
public:
	PawnCrush(int tile_x, int tile_y);
};

class PawnPow : public ImageAnimation {
public:
	PawnPow(int tile_x, int tile_y);
};
}

#endif /* !OR_ANIMATOR_HPP */
