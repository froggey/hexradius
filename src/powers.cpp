#include "powers.hpp"
#include "octradius.hpp"

namespace Powers {
	static int destroy_enemies(OctRadius::TileList &area, OctRadius::Pawn *pawn) {
		OctRadius::TileList::iterator i = area.begin();
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
	
	int destroy_column(OctRadius::Pawn *pawn) {
		OctRadius::TileList tiles = pawn->ColumnList();
		return destroy_enemies(tiles, pawn);
	}
	
	int destroy_row(OctRadius::Pawn *pawn) {
		OctRadius::TileList tiles = pawn->RowList();
		return destroy_enemies(tiles, pawn);
	}
	
	int destroy_radial(OctRadius::Pawn *pawn) {
		OctRadius::TileList tiles = pawn->RadialList();
		return destroy_enemies(tiles, pawn);
	}
	
	int raise_tile(OctRadius::Pawn *pawn) {
		if(pawn->OnTile()->height < 2) {
			pawn->OnTile()->height++;
			return 1;
		}else{
			return 0;
		}
	}
	
	int lower_tile(OctRadius::Pawn *pawn) {
		if(pawn->OnTile()->height > -2) {
			pawn->OnTile()->height--;
			return 1;
		}else{
			return 0;
		}
	}
	
	int moar_range(OctRadius::Pawn *pawn) {
		if (pawn->range < 3) {
			pawn->range++;
			return 1;
		}
		else return 0;
	}
	
	int climb_tile(OctRadius::Pawn *pawn) {
		if(pawn->flags & PWR_CLIMB) {
			return 0;
		}else{
			pawn->flags |= PWR_CLIMB;
			return 1;
		}
	}
}
