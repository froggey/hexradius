#include "powers.hpp"
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "client.hpp"
#include "network.hpp"

#undef ABSOLUTE
#undef RELATIVE

Powers::Power Powers::powers[] = {
	{"Destroy Row", &Powers::destroy_row, 50, true},
	{"Destroy Radial", &Powers::destroy_radial, 50, true},
	{"Destroy NW-SE", &Powers::destroy_bs, 50, true},
	{"Destroy NE-SW", &Powers::destroy_fs, 50, true},
	
	{"Raise Tile", &Powers::raise_tile, 100, true},
	{"Lower Tile", &Powers::lower_tile, 100, true},
	{"Increase Range", &Powers::increase_range, 20, true},
	{"Hover", &Powers::hover, 30, true},
	{"Shield", &Powers::shield, 30, true},
	{"Teleport", &Powers::teleport, 60, true},
	
	{"Elevate Row", &Powers::elevate_row, 70, true},
	{"Elevate Radial", &Powers::elevate_radial, 70, true},
	{"Elevate NW-SE", &Powers::elevate_bs, 70, true},
	{"Elevate NE-SW", &Powers::elevate_fs, 70, true},
	
	{"Dig Row", &Powers::dig_row, 70, true},
	{"Dig Radial", &Powers::dig_radial, 70, true},
	{"Dig NW-SE", &Powers::dig_bs, 70, true},
	{"Dig NE-SW", &Powers::dig_fs, 70, true},
	
	{"Purify Row", &Powers::purify_row, 50, true},
	{"Purify Radial", &Powers::purify_radial, 50, true},
	{"Purify NW-SE", &Powers::purify_bs, 50, true},
	{"Purify NE-SW", &Powers::purify_fs, 50, true},
	
	{"Annihilate Row", &Powers::annihilate_row, 50, false},
	{"Annihilate Radial", &Powers::annihilate_radial, 50, false},
	{"Annihilate NW-SE", &Powers::annihilate_bs, 50, false},
	{"Annihilate NE-SW", &Powers::annihilate_fs, 50, false}
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
	
	static bool annihilate(Tile::List tiles) {
		bool ret = false;
		
		for(Tile::List::iterator tile = tiles.begin(); tile != tiles.end(); tile++) {
			if((*tile)->pawn) {
				delete (*tile)->pawn;
				(*tile)->pawn = NULL;
				
				ret = true;
			}
		}
		
		return ret;
	}
	
	bool annihilate_row(Pawn *pawn, Server *server, Client *client) {
		return annihilate(pawn->RowTiles());
	}
	
	bool annihilate_radial(Pawn *pawn, Server *server, Client *client) {
		return annihilate(pawn->RadialTiles());
	}
	
	bool annihilate_bs(Pawn *pawn, Server *server, Client *client) {
		return annihilate(pawn->bs_tiles());
	}
	
	bool annihilate_fs(Pawn *pawn, Server *server, Client *client) {
		return annihilate(pawn->fs_tiles());
	}
}
