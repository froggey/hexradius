#ifndef OR_TILE_ANIMS_HPP
#define OR_TILE_ANIMS_HPP

#include "octradius.hpp"

namespace TileAnimators {
	struct Animator {
		Tile::List tiles;
		uint start_time;
		uint last_time;
	
		Animator(Tile::List _tiles);
		virtual void do_stuff() = 0;
	};
	
	struct ElevationAnimator: public Animator {
		ElevationAnimator(Tile::List _tiles, Tile* center, uint delay, uint target_elevation);
		virtual void do_stuff();
	};
	
	extern Animator* current_animator;
}

#endif