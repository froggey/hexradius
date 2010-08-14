#include "powers.hpp"
#include "octradius.hpp"

const Powers::Power Powers::powers[] = {
	{"Destroy Column", &Powers::destroy_column, 100},
	{"Destroy Row", &Powers::destroy_row, 100},
	{"Destroy Radial", &Powers::destroy_radial, 100},
	{"Raise Tile", &Powers::raise_tile, 100},
	{"Lower Tile", &Powers::lower_tile, 100},
	{"Moar Range", &Powers::moar_range, 20},
	{"Climb Tile", &Powers::climb_tile, 100},
	{"Wall Row", &Powers::wall_row, 100},
	{"Wall column", &Powers::wall_column, 100},
	{"Armour", &Powers::armour, 100},
	{"Purify Row", &Powers::purify_row, 100},
	{"Purify Column", &Powers::purify_column, 100},
	{"Purify Radial", &Powers::purify_radial, 100}
};

const int Powers::num_powers = sizeof(powers) / sizeof(Power);

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
	
	int destroy_column(Pawn *pawn) {
		Tile::List tiles = pawn->ColTiles();
		return destroy_enemies(tiles, pawn);
	}
	
	int destroy_row(Pawn *pawn) {
		Tile::List tiles = pawn->RowTiles();
		return destroy_enemies(tiles, pawn);
	}
	
	int destroy_radial(Pawn *pawn) {
		Tile::List tiles = pawn->RadialTiles();
		return destroy_enemies(tiles, pawn);
	}
	
	int raise_tile(Pawn *pawn) {
		if(pawn->GetTile()->height < 2) {
			pawn->GetTile()->height++;
			return 1;
		}else{
			return 0;
		}
	}
	
	int lower_tile(Pawn *pawn) {
		if(pawn->GetTile()->height > -2) {
			pawn->GetTile()->height--;
			return 1;
		}else{
			return 0;
		}
	}
	
	int moar_range(Pawn *pawn) {
		if (pawn->range < 3) {
			pawn->range++;
			return 1;
		}
		else return 0;
	}
	
	int climb_tile(Pawn *pawn) {
		if(pawn->flags & PWR_CLIMB) {
			return 0;
		}else{
			pawn->flags |= PWR_CLIMB;
			return 1;
		}
	}
	
	static int wall_tiles(Tile::List tiles) {
		Tile::List::iterator i = tiles.begin();
		int ret = 0;
		
		for(; i != tiles.end(); i++) {
			if((*i)->height != 2) {
				ret = 1;
				(*i)->height = 2;
			}
		}
		
		return ret;
	}
	
	int wall_column(Pawn *pawn) {
		return wall_tiles(pawn->ColTiles());
	}
	
	int wall_row(Pawn *pawn) {
		return wall_tiles(pawn->RowTiles());
	}
	
	int armour(Pawn *pawn) {
		if(pawn->flags & PWR_ARMOUR) {
			return 0;
		}
		
		pawn->flags |= PWR_ARMOUR;
		return 1;
	}
	
	static int purify(Tile::List tiles, Pawn *pawn) {
		Tile::List::iterator i = tiles.begin();
		int ret = 0;
		
		for(; i != tiles.end(); i++) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour && (*i)->pawn->flags & PWR_GOOD) {
				(*i)->pawn->flags &= ~PWR_GOOD;
				ret = 1;
			}
		}
		
		return ret;
	}
	
	int purify_row(Pawn *pawn) {
		return purify(pawn->RowTiles(), pawn);
	}
	
	int purify_column(Pawn *pawn) {
		return purify(pawn->ColTiles(), pawn);
	}
	
	int purify_radial(Pawn *pawn) {
		return purify(pawn->RadialTiles(), pawn);
	}
}
