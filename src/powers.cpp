#include "powers.hpp"
#include "octradius.hpp"

namespace Powers {
	int destroy_column(OctRadius::Pawn *pawn) {
		OctRadius::TileList tiles = pawn->ColumnList();
		OctRadius::TileList::iterator i = tiles.begin();
		int ret = 0;
		
		while(i != tiles.end()) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
				delete (*i)->pawn;
				(*i)->pawn = NULL;
				
				ret = 1;
			}
			
			i++;
		}
		
		return ret;
	}

	int moar_range(OctRadius::Pawn *pawn) {
		if (pawn->range < 3) {
			pawn->range++;
			return 1;
		}
		else return 0;
	}
}