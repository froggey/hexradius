#include "powers.hpp"
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "client.hpp"
#include "network.hpp"

#undef ABSOLUTE
#undef RELATIVE

Powers::Power Powers::powers[] = {
	{"Destroy Row", &Powers::destroy_row, 50},
	{"Destroy Radial", &Powers::destroy_radial, 50},
	{"Destroy NW-SE", &Powers::destroy_bs, 50},
	{"Destroy NE-SW", &Powers::destroy_fs, 50},
	
	{"Raise Tile", &Powers::raise_tile, 100},
	{"Lower Tile", &Powers::lower_tile, 100},
	{"Increase Range", &Powers::increase_range, 20},
	{"Hover", &Powers::hover, 30},
	{"Shield", &Powers::shield, 30},
	{"Teleport", &Powers::teleport, 60},
	
	{"Elevate Row", &Powers::elevate_row, 70},
	{"Elevate Radial", &Powers::elevate_radial, 70},
	{"Elevate NW-SE", &Powers::elevate_bs, 70},
	{"Elevate NE-SW", &Powers::elevate_fs, 70},
	
	{"Dig Row", &Powers::dig_row, 70},
	{"Dig Radial", &Powers::dig_radial, 70},
	{"Dig NW-SE", &Powers::dig_bs, 70},
	{"Dig NE-SW", &Powers::dig_fs, 70},
	
	{"Purify Row", &Powers::purify_row, 50},
	{"Purify Radial", &Powers::purify_radial, 50},
	{"Purify NW-SE", &Powers::purify_bs, 50},
	{"Purify NE-SW", &Powers::purify_fs, 50}
};

const int Powers::num_powers = sizeof(Powers::powers) / sizeof(Powers::Power);

int Powers::RandomPower(void) {
	int total = 0;
	for (int i = 0; i < Powers::num_powers; i++) {
		total += Powers::powers[i].spawn_rate;
	}
	
	int n = rand() % total;
	for (int i = 0; i < Powers::num_powers; i++) {
		if(n < Powers::powers[i].spawn_rate) {
			return i;
		}
		
		n -= Powers::powers[i].spawn_rate;
	}
	
	abort();
}

namespace Powers {
	static bool destroy_enemies(Tile::List area, Pawn *pawn) {
		Tile::List::iterator i = area.begin();
		bool ret = false;
		
		while(i != area.end()) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
				delete (*i)->pawn;
				(*i)->pawn = NULL;
				
				ret = true;
			}
			
			i++;
		}
		
		return ret;
	}
	
	bool destroy_row(Pawn *pawn, Server *server, Client *client) {
		return destroy_enemies(pawn->RowTiles(), pawn);
	}
	
	bool destroy_radial(Pawn *pawn, Server *server, Client *client) {
		return destroy_enemies(pawn->RadialTiles(), pawn);
	}
	
	bool destroy_bs(Pawn *pawn, Server *server, Client *client) {
		return destroy_enemies(pawn->bs_tiles(), pawn);
	}
	
	bool destroy_fs(Pawn *pawn, Server *server, Client *client) {
		return destroy_enemies(pawn->fs_tiles(), pawn);
	}
	
	bool raise_tile(Pawn *pawn, Server *server, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, +1);
		return pawn->cur_tile->SetHeight(pawn->cur_tile->height+1);
	}
	
	bool lower_tile(Pawn *pawn, Server *server, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, -1);
		return pawn->cur_tile->SetHeight(pawn->cur_tile->height-1);
	}
	
	bool increase_range(Pawn *pawn, Server *server, Client *client) {
		if(pawn->range < 3) {
			pawn->range++;
			return true;
		}
		
		return false;
	}
	
	bool hover(Pawn *pawn, Server *server, Client *client) {
		if(pawn->flags & PWR_CLIMB) {
			return false;
		}
		
		pawn->flags |= PWR_CLIMB;
		return true;
	}
	
	static bool elevate_tiles(Tile::List &tiles) {
		Tile::List::iterator i = tiles.begin();
		bool ret = false;
		
		for(; i != tiles.end(); i++) {
			ret = (ret || (*i)->SetHeight(2));
		}
		
		return ret;
	}
	
	static bool dig_tiles(Tile::List &tiles) {
		Tile::List::iterator i = tiles.begin();
		bool ret = false;
		
		for(; i != tiles.end(); i++) {
			ret = (ret || (*i)->SetHeight(-2));
		}
		
		return ret;
	}
	
	bool elevate_row(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->RowTiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);
		
		return elevate_tiles(tiles);
	}
	
	bool elevate_radial(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->RadialTiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);
		
		return elevate_tiles(tiles);
	}
	
	bool elevate_bs(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->bs_tiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);
		
		return elevate_tiles(tiles);
	}
	
	bool elevate_fs(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->fs_tiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);
		
		return elevate_tiles(tiles);
	}
	
	bool dig_row(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->RowTiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);
		
		return dig_tiles(tiles);
	}
	
	bool dig_radial(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->RadialTiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);
		
		return dig_tiles(tiles);
	}
	
	bool dig_bs(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->bs_tiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);
		
		return dig_tiles(tiles);
	}
	
	bool dig_fs(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->fs_tiles();
		
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);
		
		return dig_tiles(tiles);
	}
	
	bool shield(Pawn *pawn, Server *server, Client *client) {
		if(pawn->flags & PWR_SHIELD) {
			return false;
		}
		
		pawn->flags |= PWR_SHIELD;
		return true;
	}
	
	static bool purify(Tile::List tiles, Pawn *pawn) {
		Tile::List::iterator i = tiles.begin();
		bool ret = false;
		
		for(; i != tiles.end(); i++) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
				(*i)->pawn->flags &= ~PWR_GOOD;
				(*i)->pawn->range = 0;
				
				ret = true;
			}
		}
		
		return ret;
	}
	
	bool purify_row(Pawn *pawn, Server *server, Client *client) {
		return purify(pawn->RowTiles(), pawn);
	}
	
	bool purify_radial(Pawn *pawn, Server *server, Client *client) {
		return purify(pawn->RadialTiles(), pawn);
	}
	
	bool purify_bs(Pawn *pawn, Server *server, Client *client) {
		return purify(pawn->bs_tiles(), pawn);
	}
	
	bool purify_fs(Pawn *pawn, Server *server, Client *client) {
		return purify(pawn->fs_tiles(), pawn);
	}
	
	bool teleport(Pawn *pawn, Server *server, Client *client) {
		if(server) {
			Tile::List tlist;
			Tile *tile;
			
			do {
				tlist = RandomTiles(server->tiles, 1, false);
				tile = *(tlist.begin());
			} while(tile->pawn);
			
			server->power_rand_vals.push_back(tile->col);
			server->power_rand_vals.push_back(tile->row);
			
			pawn->cur_tile->pawn = NULL;
			tile->pawn = pawn;
			pawn->cur_tile = tile;
		}else{
			if(client->power_rand_vals.size() != 2) {
				return false;
			}
			
			int col = client->power_rand_vals[0];
			int row = client->power_rand_vals[1];
			
			Tile *tile = FindTile(client->tiles, col, row);
			if(!tile || tile->pawn) {
				std::cerr << "Invalid teleport attempted, out of sync?" << std::endl;
				return false;
			}
			
			pawn->cur_tile->pawn = NULL;
			tile->pawn = pawn;
			pawn->cur_tile = tile;
		}
		
		return true;
	}
}
