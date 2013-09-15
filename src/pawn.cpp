#include <set>

#include "octradius.hpp"
#include "powers.hpp"
#include "octradius.pb.h"
#include "client.hpp"

void Pawn::destroy(destroy_type dt) {
	destroyed_by = dt;

	if(last_tile) {
		last_tile->render_pawn.reset();
	}

	cur_tile->pawn.reset();
	cur_tile = NULL;
}

bool Pawn::destroyed() {
	return destroyed_by != OK;
}

bool Pawn::Move(Tile *tile, Server *server, Client *client) {
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
			if(client) {
				client->add_animator(new Animators::PawnCrush(tile->screen_x, tile->screen_y));
			}

			tile->pawn->destroy(STOMP);
		}
	}

	tile->pawn.swap(cur_tile->pawn);
	cur_tile = tile;

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

bool Pawn::UsePower(int power, Server *server, Client *client) {
	PowerList::iterator p = powers.find(power);
	if(server && p == powers.end()) {
		return false;
	}

	if(!Powers::powers[power].func(shared_from_this(), server, client)) {
		return false;
	}

	if(p != powers.end() && --p->second == 0) {
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

Tile::List Pawn::bs_tiles() {
	Tile::List tiles;

	int base = cur_tile->col;

	for(int r = cur_tile->row; r > 0; r--) {
		if(r % 2 == 0) {
			base--;
		}
	}

	for(Tile::List::iterator tile = all_tiles.begin(); tile != all_tiles.end(); tile++) {
		int col = (*tile)->col, row = (*tile)->row;
		int mcol = base;

		for(int i = 1; i <= row; i++) {
			if(i % 2 == 0) {
				mcol++;
			}
		}

		int min = mcol-range, max = mcol+range;

		if(col >= min && col <= max) {
			tiles.push_back(*tile);
		}
	}

	return tiles;
}

Tile::List Pawn::fs_tiles() {
	Tile::List tiles;

	int base = cur_tile->col;

	for(int r = cur_tile->row; r > 0; r--) {
		if(r % 2) {
			base++;
		}
	}

	for(Tile::List::iterator tile = all_tiles.begin(); tile != all_tiles.end(); tile++) {
		int col = (*tile)->col, row = (*tile)->row;
		int mcol = base;

		for(int i = 0; i <= row; i++) {
			if(i % 2) {
				mcol--;
			}
		}

		int min = mcol-range, max = mcol+range;

		if(col >= min && col <= max) {
			tiles.push_back(*tile);
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
