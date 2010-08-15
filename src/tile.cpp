#include "octradius.hpp"

bool Tile::SetHeight(int h) {
	if(h != height && h <= 2 && h >= -2) {
		height = h;
		return true;
	}else{
		return false;
	}
}

void Tile::CopyToProto(protocol::tile *t) {
	t->set_col(col);
	t->set_row(row);
	t->set_height(height);
	t->set_power(power >= 0 ? true : false);
}

Tile::~Tile() {
	delete pawn;
}

Tile *FindTile(Tile::List &list, int c, int r) {
	Tile::List::iterator i = list.begin();
	
	while(i != list.end()) {
		if((*i)->col == c && (*i)->row == r) {
			return *i;
		}
		
		i++;
	}
	
	return NULL;
}

Tile::List RandomTiles(Tile::List tiles, int num, bool uniq) {
	Tile::List ret;
	
	while(tiles.size() && num) {
		Tile::List::iterator i = tiles.begin();
		i += rand() % tiles.size();
		
		ret.push_back(*i);
		
		if(uniq) {
			tiles.erase(i);
		}
		
		num--;
	}
	
	return ret;
}

/* Return the "topmost" tile rendered at the given X,Y co-ordinates or NULL if
 * there is no tile at that location.
*/
Tile *TileAtXY(Tile::List &tiles, int x, int y) {
	Tile::List::iterator ti = tiles.end();
	
	do {
		ti--;
		
		int tx = (*ti)->screen_x;
		int ty = (*ti)->screen_y;
		
		if(tx <= x && tx+(int)TILE_SIZE > x && ty <= y && ty+(int)TILE_SIZE > y) {
			return *ti;
		}
	} while(ti != tiles.begin());
	
	return NULL;
}

void FreeTiles(Tile::List &tiles) {
	Tile::List::iterator i = tiles.begin();
	
	for(; i != tiles.end(); i++) {
		delete *i;
	}
	
	tiles.clear();
}
