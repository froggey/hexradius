#ifndef OR_TILE_ANIMS_HPP
#define OR_TILE_ANIMS_HPP

#include "octradius.hpp"

class Client;

namespace TileAnimators {
	struct Animator {
		Tile::List tiles;
		uint start_time;
		uint last_time;
		Client* client;
	
		Animator(Client* _client, Tile::List _tiles);
		virtual void do_stuff() = 0;
	};
	
	struct ElevationAnimator: public Animator {
		ElevationAnimator(Client* _client, Tile::List _tiles, Tile* center, float delay_factor, uint target_elevation);
		virtual void do_stuff();
	};
}

#endif
