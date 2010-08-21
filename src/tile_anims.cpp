#include <cmath>
#include <iostream>
#include <boost/foreach.hpp>
#include "octradius.hpp"
#include "tile_anims.hpp"

int sign(int n) { return (n == 0)? n : (abs(n) / n); }
// Are these defined somewhere already?
const double PI = 3.14159265358979323846;
const double E = 2.71828182845904523536;

namespace TileAnimators {
	Animator* current_animator = NULL;
	
	Animator::Animator(Tile::List _tiles): tiles(_tiles) {}
	
	ElevationAnimator::ElevationAnimator(Tile::List _tiles, Tile* center, float delay_factor, uint target_elevation): Animator(_tiles) {
		BOOST_FOREACH(Tile* t, tiles) {
			if (!t->animating) {
				int rx = (t->screen_x + t->height * TILE_HEIGHT_FACTOR) - (center->screen_x + center->height * TILE_HEIGHT_FACTOR);
				int ry = (t->screen_y + t->height * TILE_HEIGHT_FACTOR) - (center->screen_y + center->height * TILE_HEIGHT_FACTOR);
				t->anim_height = t->height * TILE_HEIGHT_FACTOR;
				t->animating = true;
				t->anim_delay = sqrt(pow(rx, 2) + pow(ry, 2)) * delay_factor;
				t->initial_elevation = t->height;
				t->final_elevation = target_elevation;
			}
		}
		start_time = SDL_GetTicks();
	}
	
	void ElevationAnimator::do_stuff() {
		uint ticks = SDL_GetTicks();
		uint t = ticks - start_time;
		//uint dt = ticks - last_time;
		//last_time = ticks;
		
		bool did_stuff = false;
		
		BOOST_FOREACH(Tile* tile, tiles) {
			if (tile->animating) {
				if (tile->final_elevation == tile->initial_elevation) {
					tile->animating = false;
					continue;
				}
				
				int this_t = t - tile->anim_delay;
				if (this_t > 2000) {
					tile->animating = false;
				}
				else if (this_t >= 0) {
					tile->anim_height = (tile->initial_elevation) + 0.75 * (tile->final_elevation - tile->initial_elevation) * atan(2 * this_t / 1000.0);
				}
				
				did_stuff = true;
			}
		}
		
		if (!did_stuff) {
			assert(current_animator == this);
			current_animator = NULL;
			delete this;
		}
	}
}