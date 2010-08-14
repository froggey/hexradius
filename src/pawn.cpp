#include "octradius.hpp"
#include "powers.hpp"
#include "octradius.pb.h"

bool Pawn::Move(Tile *tile) {
	if(
		!(tile->col == cur_tile->col && (tile->row == cur_tile->row+1 || tile->row == cur_tile->row-1)) &&
		!(tile->row == cur_tile->row && (tile->col == cur_tile->col+1 || tile->col == cur_tile->col-1))
	) {
		return false;
	}
	
	if(tile->height-1 > cur_tile->height && !(flags & PWR_CLIMB)) {
		return false;
	}
	
	if(tile->pawn) {
		if(tile->pawn->colour == colour) {
			return false;
		}else if(tile->pawn->flags & PWR_ARMOUR) {
			return false;
		}else{
			delete tile->pawn;
		}
	}
	
	cur_tile->pawn = NULL;
	cur_tile = tile;
	
	tile->pawn = this;
	
	if(tile->power >= 0) {
		AddPower(tile->power);
		tile->power = -1;
	}
	
	return true;
}

void Pawn::AddPower(int power) {
	PowerList::iterator p = powers.find(power);
	
	if(p != powers.end()) {
		p->second++;
	}else{
		powers.insert(std::make_pair(power, 1));
	}
}

bool Pawn::UsePower(int power) {
	PowerList::iterator p = powers.find(power);
	if(p == powers.end()) {
		return false;
	}
	
	if(!Powers::powers[power].func(this)) {
		return false;
	}
	
	if(p->second > 1) {
		p->second--;
	}else{
		powers.erase(p);
	}
	
	return true;
}

Tile::List Pawn::ColTiles(void) {
	Tile::List tiles;
	Tile::List::iterator i = all_tiles.begin();
	
	int min = cur_tile->col-range;
	int max = cur_tile->col+range;
	
	for(; i != all_tiles.end(); i++) {
		if((*i)->col >= min && (*i)->col <= max) {
			tiles.push_back(*i);
		}
	}
	
	return tiles;
}

Tile::List Pawn::RowTiles(void) {
	Tile::List tiles;
	Tile::List::iterator i = all_tiles.begin();
	
	int min = cur_tile->row-range;
	int max = cur_tile->row+range;
	
	for(; i != all_tiles.end(); i++) {
		if((*i)->row >= min && (*i)->row <= max) {
			tiles.push_back(*i);
		}
	}
	
	return tiles;
}

Tile::List Pawn::RadialTiles(void) {
	Tile::List tiles;
	Tile::List::iterator i = all_tiles.begin();
	
	int c_min = cur_tile->col-range-1;
	int c_max = cur_tile->col+range+1;
	int r_min = cur_tile->row-range-1;
	int r_max = cur_tile->row+range+1;
	
	for(; i != all_tiles.end(); i++) {
		if((*i)->col >= c_min && (*i)->col <= c_max && (*i)->row >= r_min && (*i)->row <= r_max) {
			tiles.push_back(*i);
		}
	}
	
	return tiles;
}

void Pawn::CopyToProto(protocol::pawn *p, bool copy_powers) {
	p->set_col(cur_tile->col);
	p->set_row(cur_tile->row);
	p->set_colour((protocol::colour)colour);
	p->set_range(range);
	p->set_flags(flags);
	p->set_has_powers(powers.size() ? true : false);
	
	if(copy_powers) {
		p->clear_powers();
		PowerList::iterator i = powers.begin();
		
		for(; i != powers.end(); i++) {
			int index = p->powers_size();
			p->add_powers();
			
			p->mutable_powers(index)->set_index(i->first);
			p->mutable_powers(index)->set_num(i->second);
		}
	}
}
