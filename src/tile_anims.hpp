#ifndef OR_TILE_ANIMS_HPP
#define OR_TILE_ANIMS_HPP

#include "octradius.hpp"

#undef ABSOLUTE
#undef RELATIVE

class Client;

namespace TileAnimators {
	struct Animator {
		Tile::List tiles;
		unsigned int start_time;
		unsigned int last_time;
		Client* client;
	
		Animator(Client* _client, Tile::List _tiles);
		virtual void do_stuff() = 0;
	};
	
	enum ElevationMode { ABSOLUTE, RELATIVE };
	
	struct ElevationAnimator: public Animator {
		ElevationAnimator(Client* _client, Tile::List _tiles, Tile* center, float delay_factor, ElevationMode mode, int target_elevation);
		virtual void do_stuff();
	};
}

#endif
