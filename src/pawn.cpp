#include <set>

#include "octradius.hpp"
#include "powers.hpp"
#include "octradius.pb.h"

bool Pawn::Move(Tile *tile) {
	if(
		!(tile->row == cur_tile->row && (tile->col == cur_tile->col+1 || tile->col == cur_tile->col-1)) &&
		!((tile->row == cur_tile->row+1 || tile->row == cur_tile->row-1) && (tile->col == cur_tile->col + (cur_tile->row % 2) || tile->col+1 == cur_tile->col + (cur_tile->row % 2)))
	) {
		return false;
	}
	
	if(tile->height-1 > cur_tile->height && !(flags & PWR_CLIMB)) {
		return false;
	}
	
	if(tile->pawn) {
		if(tile->pawn->colour == colour) {
			return false;
		}else if(tile->pawn->flags & PWR_SHIELD) {
			return false;
		}else{
			delete tile->pawn;
		}
	}
	
	cur_tile->pawn = NULL;
	cur_tile = tile;
	
	tile->pawn = this;
	
	if(tile->has_power) {
		if(tile->power >= 0) {
			AddPower(tile->power);
		}
		
		flags |= HAS_POWER;
		tile->has_power = false;
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
		
		if(powers.empty()) {
			flags &= ~HAS_POWER;
		}
	}
	
	return true;
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

typedef std::set<Tile*> tile_set;

static void radial_loop(Tile::List &all, tile_set &tiles, Tile *base) {
	int c_min = base->col - 1 + (base->row % 2) * 1;
	int c_max = base->col + (base->row % 2) * 1;
	int c_extra = base->col + (base->row % 2 ? -1 : 1);
	int r_min = base->row-1;
	int r_max = base->row+1;
	
	for(Tile::List::iterator i = all.begin(); i != all.end(); i++) {
		if(((*i)->col >= c_min && (*i)->col <= c_max && (*i)->row >= r_min && (*i)->row <= r_max) || ((*i)->row == base->row && (*i)->col == c_extra)) {
			tiles.insert(*i);
		}
	}
}

Tile::List Pawn::RadialTiles(void) {
	tile_set tiles;
	radial_loop(all_tiles, tiles, cur_tile);
	
	tile_set outer = tiles;
	
	for(int i = 0; i < range; i++) {
		tile_set new_outer;
		
		for(tile_set::iterator t = outer.begin(); t != outer.end(); t++) {
			radial_loop(all_tiles, new_outer, *t);
		}
		
		tiles.insert(new_outer.begin(), new_outer.end());
		outer = new_outer;
	}
	
	Tile::List ret;
	
	for(tile_set::iterator t = tiles.begin(); t != tiles.end(); t++) {
		ret.push_back(*t);
	}
	
	return ret;
}

void Pawn::CopyToProto(protocol::pawn *p, bool copy_powers) {
	p->set_col(cur_tile->col);
	p->set_row(cur_tile->row);
	p->set_colour((protocol::colour)colour);
	p->set_range(range);
	p->set_flags(flags);
	
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
