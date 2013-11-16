#ifndef TILE_ANIMS_HPP
#define TILE_ANIMS_HPP

#include "hexradius.hpp"
#include "tile.hpp"

#undef ABSOLUTE
#undef RELATIVE

namespace TileAnimators {
	struct Animator {
		Tile::List tiles;
		unsigned int start_time;
		unsigned int last_time;

		Animator(Tile::List _tiles);
		// Returns true if the animation still has stuff to do.
		// If false, the client will stop running & delete the animation.
		virtual bool do_stuff() = 0;
		virtual ~Animator();
		virtual protocol::message serialize() = 0;
	};

	enum ElevationMode { ABSOLUTE, RELATIVE };

	struct ElevationAnimator: public Animator {
		ElevationAnimator(Tile::List _tiles, Tile* center, float delay_factor, ElevationMode mode, int target_elevation);
		virtual bool do_stuff();
		virtual protocol::message serialize();

		Tile *center;
		float delay_factor;
		ElevationMode mode;
		int target_elevation;
	};
}

#endif
