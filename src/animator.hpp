#ifndef ANIMATOR_HPP
#define ANIMATOR_HPP

#include <SDL/SDL.h>

#include "hexradius.hpp"

class Tile;

namespace Animators {
class Generic {
public:
	virtual ~Generic();
	virtual bool render() = 0;
};

class ImageAnimation : public Generic {
public:
	// A vector of times and scaling factors.
	struct scale_tween_point {
		Uint32 time;
		float scale;
	};
	typedef std::vector<scale_tween_point> scale_tween_point_vec;
	Tile *tile;
	Uint32 init_ticks;
	Uint32 runtime;
	SDL_Surface *image;
	scale_tween_point_vec scale_tween;

	/* Runtime is number of milliseconds to run for. */
	ImageAnimation(Tile *tile, Uint32 runtime, const std::string &image,
		       const scale_tween_point_vec &scale_tween = scale_tween_point_vec());
	bool render();
private:
	float scale_factor(Uint32 time);
};

class PawnCrush : public ImageAnimation {
public:
	PawnCrush(Tile *tile);
};

class PawnPow : public ImageAnimation {
public:
	PawnPow(Tile *tile);
};

class PawnBoom : public ImageAnimation {
public:
	PawnBoom(Tile *tile);
};

class PawnOhShitIFellDownAHole : public ImageAnimation {
public:
	PawnOhShitIFellDownAHole(Tile *tile);
};
}

#endif /* !ANIMATOR_HPP */
