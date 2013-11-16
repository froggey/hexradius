#include <cmath>
#include <iostream>
#include <boost/foreach.hpp>
#include "hexradius.hpp"
#include "tile_anims.hpp"
#include "client.hpp"

int sign(int n) { return (n == 0)? n : (abs(n) / n); }
// Are these defined somewhere already?
const double PI = 3.14159265358979323846;
const double E = 2.71828182845904523536;

namespace TileAnimators {
	Animator* current_animator = NULL;

	Animator::Animator(Tile::List _tiles) :
		tiles(_tiles) {}

	Animator::~Animator() {
	}

	ElevationAnimator::ElevationAnimator(Tile::List _tiles, Tile* center, float delay_factor, ElevationMode mode, int target_elevation):
		Animator(_tiles), center(center), delay_factor(delay_factor), mode(mode), target_elevation(target_elevation) {
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

	bool ElevationAnimator::do_stuff() {
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
				if (this_t > 1500) {
					tile->animating = false;
				}
				else if (this_t >= 0) {
					tile->anim_height = tile->final_elevation
						+ (tile->initial_elevation - tile->final_elevation)
						* cos(2 * PI * this_t / 1000.0) / pow(12, this_t / 1000.0);
				}

				did_stuff = true;
			}
		}

		return did_stuff;
	}
}

protocol::message TileAnimators::ElevationAnimator::serialize()
{
	protocol::message msg;
	msg.set_msg(protocol::TILE_ANIMATION);
	msg.set_animation_name("elevation");

	center->CopyToProto(msg.add_tiles());
	for(Tile::List::iterator t = tiles.begin(); t != tiles.end(); ++t) {
		(*t)->CopyToProto(msg.add_tiles());
	}

	protocol::key_value *delay_factor_kv = msg.add_misc();
	delay_factor_kv->set_key("delay-factor");
	delay_factor_kv->set_float_value(delay_factor);
	protocol::key_value *mode_kv = msg.add_misc();
	mode_kv->set_key("mode");
	switch(mode) {
	case ABSOLUTE:
		mode_kv->set_string_value("absolute");
		break;
	case RELATIVE:
		mode_kv->set_string_value("relative");
		break;
	default: abort();
	}
	protocol::key_value *target_elevation_kv = msg.add_misc();
	target_elevation_kv->set_key("target-elevation");
	target_elevation_kv->set_int_value(target_elevation);
	return msg;
}
