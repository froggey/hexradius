#include "powers.hpp"
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "client.hpp"

Powers::Power Powers::powers[] = {
	{"Destroy Row", &Powers::destroy_row, 50},
	{"Destroy Radial", &Powers::destroy_radial, 50},
	{"Raise Tile", &Powers::raise_tile, 100},
	{"Lower Tile", &Powers::lower_tile, 100},
	{"Increase Range", &Powers::increase_range, 20},
	{"Hover", &Powers::hover, 30},
	{"Elevate Row", &Powers::elevate_row, 70},
	{"Elevate Radial", &Powers::elevate_radial, 70},
	{"Dig Row", &Powers::dig_row, 70},
	{"Shield", &Powers::shield, 30},
	{"Purify Row", &Powers::purify_row, 50},
	{"Purify Radial", &Powers::purify_radial, 50},
};

const int Powers::num_powers = sizeof(powers) / sizeof(Power);

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
	static int destroy_enemies(Tile::List &area, Pawn *pawn) {
		Tile::List::iterator i = area.begin();
		int ret = 0;
		
		while(i != area.end()) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
				delete (*i)->pawn;
				(*i)->pawn = NULL;
				
				ret = 1;
			}
			
			i++;
		}
		
		return ret;
	}
	
	int destroy_row(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->RowTiles();
		return destroy_enemies(tiles, pawn);
	}
	
	int destroy_radial(Pawn *pawn, Server *server, Client *client) {
		Tile::List tiles = pawn->RadialTiles();
		return destroy_enemies(tiles, pawn);
	}
	
	int raise_tile(Pawn *pawn, Server *server, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, Tile::List(1, pawn->GetTile()), pawn->GetTile(), 3.0, TileAnimators::RELATIVE, +1);
		return pawn->GetTile()->SetHeight(pawn->GetTile()->height+1);
	}
	
	int lower_tile(Pawn *pawn, Server *server, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, Tile::List(1, pawn->GetTile()), pawn->GetTile(), 3.0, TileAnimators::RELATIVE, -1);
		return pawn->GetTile()->SetHeight(pawn->GetTile()->height-1);
	}
	
	int increase_range(Pawn *pawn, Server *server, Client *client) {
		if (pawn->range < 3) {
			pawn->range++;
			return 1;
		}
		else return 0;
	}
	
	int hover(Pawn *pawn, Server *server, Client *client) {
		if(pawn->flags & PWR_CLIMB) {
			return 0;
		}
		else{
			pawn->flags |= PWR_CLIMB;
			return 1;
		}
	}
	
	static int elevate_tiles(Tile::List tiles) {
		Tile::List::iterator i = tiles.begin();
		int ret = 0;
		
		for(; i != tiles.end(); i++) {
			ret |= (*i)->SetHeight(2);
		}
		
		return ret;
	}
	
	static int dig_tiles(Tile::List tiles) {
		Tile::List::iterator i = tiles.begin();
		int ret = 0;
		
		for(; i != tiles.end(); i++) {
			ret |= (*i)->SetHeight(-2);
		}
		
		return ret;
	}
	
	int elevate_row(Pawn *pawn, Server *server, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, pawn->RowTiles(), pawn->GetTile(), 3.0, TileAnimators::ABSOLUTE, 2);
		return elevate_tiles(pawn->RowTiles());
	}
	
	int elevate_radial(Pawn *pawn, Server *server, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, pawn->RadialTiles(), pawn->GetTile(), 3.0, TileAnimators::ABSOLUTE, 2);
		return elevate_tiles(pawn->RadialTiles());
	}
	
	int dig_row(Pawn *pawn, Server *server, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, pawn->RowTiles(), pawn->GetTile(), 3.0, TileAnimators::ABSOLUTE, -2);
		return dig_tiles(pawn->RowTiles());
	}
	
	int shield(Pawn *pawn, Server *server, Client *client) {
		if(pawn->flags & PWR_SHIELD) {
			return 0;
		}
		
		pawn->flags |= PWR_SHIELD;
		return 1;
	}
	
	static int purify(Tile::List tiles, Pawn *pawn) {
		Tile::List::iterator i = tiles.begin();
		int ret = 0;
		
		for(; i != tiles.end(); i++) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
				(*i)->pawn->flags &= ~PWR_GOOD;
				(*i)->pawn->range = 0;
				ret = 1;
			}
		}
		
		return ret;
	}
	
	int purify_row(Pawn *pawn, Server *server, Client *client) {
		return purify(pawn->RowTiles(), pawn);
	}
	
	int purify_radial(Pawn *pawn, Server *server, Client *client) {
		return purify(pawn->RadialTiles(), pawn);
	}
}
