#include <SDL/SDL_rotozoom.h>
#include <boost/range.hpp>
#include "animator.hpp"
#include "loadimage.hpp"
#include "gui.hpp"

Animators::Generic::~Generic() {
}

Animators::ImageAnimation::ImageAnimation(int tile_x, int tile_y, Uint32 runtime, const std::string &image,
					  const scale_tween_point_vec &scale_tween) :
	tx(tile_x), ty(tile_y),
	init_ticks(SDL_GetTicks()), runtime(runtime),
	image(ImgStuff::GetImage(image)),
	scale_tween(scale_tween) {
}

bool Animators::ImageAnimation::render() {
	Uint32 current_time = SDL_GetTicks();
	if(current_time >= init_ticks+runtime) {
		return false;
	}
	float scale = scale_factor(current_time - init_ticks);

	if(scale == 1.0) {
		SDL_Rect rect = {tx, ty, 0, 0};
		ensure_SDL_BlitSurface(image, NULL, screen, &rect);
	} else {
		SDL_Surface *scaled_image = rotozoomSurface(image, 0.0, double(scale), SMOOTHING_ON);
		SDL_Rect rect = {tx, ty, 0, 0};
		ensure_SDL_BlitSurface(scaled_image, NULL, screen, &rect);
		SDL_FreeSurface(scaled_image);
	}

	return true;
}

float Animators::ImageAnimation::scale_factor(Uint32 time)
{
	if(scale_tween.empty()) return 1.0f;

	Uint32 last_time = 0;
	float last_scale = 1.0f;
	for(scale_tween_point_vec::iterator i = scale_tween.begin(); i != scale_tween.end(); ++i) {
		if(i->time > time) {
			Uint32 ticks = i->time - last_time; // ticks in this sequence.
			Uint32 dt = time - last_time; // ticks since last_time.
			float a = float(dt) / float(ticks); // position along time.
			return last_scale + (i->scale - last_scale) * a;
		}
		last_scale = i->scale;
		last_time = i->time;
	}
	return last_scale;
}

Animators::PawnCrush::PawnCrush(int tile_x, int tile_y) :
	ImageAnimation(tile_x, tile_y, 500, "graphics/crush.png") {
}

static const Animators::ImageAnimation::scale_tween_point pow_animation_tween[] = {
	{0, 0.0f},
	{300, 1.5f},
	{350, 1.0f}
};

Animators::PawnPow::PawnPow(int tile_x, int tile_y) :
	ImageAnimation(tile_x, tile_y, 700, "graphics/kapow.png",
		       scale_tween_point_vec(&pow_animation_tween[0],
					     &pow_animation_tween[boost::size(pow_animation_tween)])) {
}

static const Animators::ImageAnimation::scale_tween_point boom_animation_tween[] = {
	{0, 0.0f},
	{350, 1.0f},
	{400, 0.75f}
};

Animators::PawnBoom::PawnBoom(int tile_x, int tile_y) :
	ImageAnimation(tile_x, tile_y, 700, "graphics/boom.png",
		       scale_tween_point_vec(&boom_animation_tween[0],
					     &boom_animation_tween[boost::size(boom_animation_tween)])) {
}

static const Animators::ImageAnimation::scale_tween_point aiee_animation_tween[] = {
	{0, 1.0f},
	{1500, 0.0f}
};

Animators::PawnOhShitIFellDownAHole::PawnOhShitIFellDownAHole(int tile_x, int tile_y) :
	ImageAnimation(tile_x, tile_y, 1500, "graphics/aiee.png",
		       scale_tween_point_vec(&aiee_animation_tween[0],
					     &aiee_animation_tween[boost::size(aiee_animation_tween)])) {
}
