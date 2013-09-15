#include "animator.hpp"
#include "loadimage.hpp"
#include "gui.hpp"

Animators::PawnCrush::PawnCrush(int tile_x, int tile_y) {
	tx = tile_x;
	ty = tile_y;

	init_ticks = SDL_GetTicks();
}

bool Animators::PawnCrush::render() {
	SDL_Surface *crush_img = ImgStuff::GetImage("graphics/crush.png");

	if(SDL_GetTicks() >= init_ticks+500) {
		delete this;
		return false;
	}else{
		SDL_Rect rect = {tx, ty, 0, 0};
		ensure_SDL_BlitSurface(crush_img, NULL, screen, &rect);

		return true;
	}
}

void Animators::PawnCrush::free() {
	delete this;
}
