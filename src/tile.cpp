#include "octradius.hpp"

bool Tile::SetHeight(int h) {
	if(h != height && h < 2 && h > -2) {
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
