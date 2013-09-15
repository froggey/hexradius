#include <cmath>
#include <iostream>
#include <boost/foreach.hpp>
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "client.hpp"

int sign(int n) { return (n == 0)? n : (abs(n) / n); }
// Are these defined somewhere already?
const double PI = 3.14159265358979323846;
const double E = 2.71828182845904523536;

namespace TileAnimators {
	Animator* current_animator = NULL;

	Animator::Animator(Client* _client, Tile::List _tiles): client(_client), tiles(_tiles) {}

	ElevationAnimator::ElevationAnimator(Client* _client, Tile::List _tiles, Tile* center, float delay_factor, ElevationMode mode, int target_elevation):
		Animator(_client, _tiles) {
		BOOST_FOREACH(Tile* t, tiles) {
			if (!t->animating) {
				int rx = (t->screen_x + t->height * TILE_HEIGHT_FACTOR) - (center->screen_x + center->height * TILE_HEIGHT_FACTOR);
				int ry = (t->screen_y + t->height * TILE_HEIGHT_FACTOR) - (center->screen_y + center->height * TILE_HEIGHT_FACTOR);
				t->anim_height = t->height;
				t->animating = true;
				t->anim_delay = sqrt(pow((double)rx, 2) + pow((double)ry, 2)) * delay_factor;
				t->initial_elevation = t->height;
				if (mode == ABSOLUTE)
					t->final_elevation = target_elevation;
				else
					t->final_elevation = t->height + target_elevation;
			}
		}
		start_time = SDL_GetTicks();
	}

	void ElevationAnimator::do_stuff() {
		unsigned int ticks = SDL_GetTicks();
		unsigned int t = ticks - start_time;
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
					if (tile->initial_elevation > tile->final_elevation) {
						tile->anim_height = tile->final_elevation - (tile->initial_elevation - tile->final_elevation) *
							cos(2 * PI * this_t / 1000.0) / pow(2 * (tile->initial_elevation + 2), this_t / 1000.0);
						tile->anim_height = -4 - tile->anim_height;
					}
					else {
						tile->anim_height = tile->final_elevation - (tile->final_elevation - tile->initial_elevation)*cos(2*PI*this_t / 1000.0)/pow(2*(tile->final_elevation+2), this_t / 1000.0);
					}
				}

				did_stuff = true;
			}
		}

		if (!did_stuff) {
			assert(client->current_animator == this);
			client->current_animator = NULL;
			delete this;
		}
	}
}
