#include "animator.hpp"
#include "loadimage.hpp"
#include "gui.hpp"

Animators::Generic::~Generic() {
}

Animators::ImageAnimation::ImageAnimation(int tile_x, int tile_y, Uint32 runtime, const std::string &image) :
	tx(tile_x), ty(tile_y),
	init_ticks(SDL_GetTicks()), runtime(runtime),
	image(ImgStuff::GetImage(image)) {
}

bool Animators::ImageAnimation::render() {
	if(SDL_GetTicks() >= init_ticks+runtime) {
		return false;
	}
	SDL_Rect rect = {tx, ty, 0, 0};
	ensure_SDL_BlitSurface(image, NULL, screen, &rect);

	return true;
}

Animators::PawnCrush::PawnCrush(int tile_x, int tile_y) :
	ImageAnimation(tile_x, tile_y, 500, "graphics/crush.png") {
}

Animators::PawnPow::PawnPow(int tile_x, int tile_y) :
	ImageAnimation(tile_x, tile_y, 500, "graphics/kapow.png") {
}
