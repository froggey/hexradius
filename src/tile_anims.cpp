#include <cmath>
#include <iostream>
#include <boost/foreach.hpp>
#include "octradius.hpp"
#include "tile_anims.hpp"

int sign(int n) { return (n == 0)? n : (abs(n) / n); }
const double PI = 3.14159265358979323846;

namespace TileAnimators {
	Animator* current_animator = NULL;
	
	Animator::Animator(Tile::List _tiles): tiles(_tiles) {}
	
	ElevationAnimator::ElevationAnimator(Tile::List _tiles, Tile* center, uint delay): Animator(_tiles) {
		uint t = 0;
		// create bigger and bigger radials to make closer tiles animate first
		for (int r = 1; t < tiles.size(); r++) {
			std::cout << "r=" << r << std::endl;
			
			BOOST_FOREACH(Tile* t, tiles) {
				if (!t->use_anim_height) {
					t->anim_height = t->height * TILE_HEIGHT_FACTOR;
					t->use_anim_height = true;
					t->anim_delay = r * delay;
					t->initial_elevation = t->height;
					t->final_elevation = 2;
				}
			}
			
			//t += ts.size();
			t = tiles.size();
		}
		start_time = SDL_GetTicks();
	}
	
	void ElevationAnimator::do_stuff() {
		uint ticks = SDL_GetTicks();
		uint t = ticks - start_time;
		//uint dt = ticks - last_time;
		//last_time = ticks;
		
		BOOST_FOREACH(Tile* tile, tiles) {
			if (tile->use_anim_height) {
				if (tile->final_elevation == tile->initial_elevation) {
					std::cout << "hf=hi, doing nothing." << std::endl;
					tile->use_anim_height = false;
					continue;
				}
				
				int this_t = t - tile->anim_delay;
				if (this_t > 3000) {
					std::cout << "hit t=3000, stopping" << std::endl;
					tile->use_anim_height = false;
				}
				else if (this_t >= 0) {
					// This pile of fun written by Tristan. Sue him for the brain damage, not me.
					tile->anim_height = sign(tile->final_elevation - tile->initial_elevation) *
						(tile->final_elevation - pow(0.6, 2 * this_t / 1000) * cos(2 * PI * this_t / 1000) /
						pow(tile->final_elevation - tile->initial_elevation, 2 * this_t / 1000 - 1));
					std::cout << "f(" << this_t << ")=" << tile->anim_height << std::endl;
				}
			}
		}
	}
}