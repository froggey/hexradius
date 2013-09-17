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
	// A vector of times and scaling factors.
	struct scale_tween_point {
		Uint32 time;
		float scale;
	};
	typedef std::vector<scale_tween_point> scale_tween_point_vec;
	int tx, ty;
	Uint32 init_ticks;
	Uint32 runtime;
	SDL_Surface *image;
	scale_tween_point_vec scale_tween;

	/* Runtime is number of milliseconds to run for. */
	ImageAnimation(int tile_x, int tile_y, Uint32 runtime, const std::string &image,
		       const scale_tween_point_vec &scale_tween = scale_tween_point_vec());
	bool render();
private:
	float scale_factor(Uint32 time);
};

class PawnCrush : public ImageAnimation {
public:
	PawnCrush(int tile_x, int tile_y);
};

class PawnPow : public ImageAnimation {
public:
	PawnPow(int tile_x, int tile_y);
};

class PawnBoom : public ImageAnimation {
public:
	PawnBoom(int tile_x, int tile_y);
};

class PawnOhShitIFellDownAHole : public ImageAnimation {
public:
	PawnOhShitIFellDownAHole(int tile_x, int tile_y);
};
}

#endif /* !OR_ANIMATOR_HPP */
