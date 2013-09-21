#include <set>

#include "octradius.hpp"
#include "powers.hpp"
#include "octradius.pb.h"
#include "client.hpp"

Pawn::Pawn(PlayerColour c, Tile::List &at, Tile *ct) :
	all_tiles(at), cur_tile(ct), colour(c),
	range(0), flags(0), destroyed_by(OK),
	last_tile(0), teleport_time(0)
{
}

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
	// Move only onto adjacent tiles or friendly landing pads.
	if(
		!(tile->row == cur_tile->row && (tile->col == cur_tile->col+1 || tile->col == cur_tile->col-1)) &&
		!((tile->row == cur_tile->row+1 || tile->row == cur_tile->row-1) &&
		  (tile->col == cur_tile->col + (cur_tile->row % 2) ||
		   tile->col+1 == cur_tile->col + (cur_tile->row % 2))) &&
		!(tile->has_landing_pad && tile->landing_pad_colour == colour)
	) {
		return false;
	}

	// Avoid moving onto black holes.
	if(tile->has_black_hole) {
		return false;
	}

	// Don't walk up cliffs or onto smashed tiles unless hovering or the tile has a landing pad.
	if((tile->height > cur_tile->height + 1 || tile->smashed) && !(tile->has_landing_pad || (flags & PWR_CLIMB))) {
		return false;
	}

	// Don't smash friendly or shielded pawns.
	if(tile->pawn && (tile->pawn->colour == colour || (tile->pawn->flags & PWR_SHIELD))) {
		return false;
	}

	force_move(tile, server, client);

	return true;
}

void Pawn::force_move(Tile *tile, Server *, Client *client) {
	assert(tile);

	if(tile->pawn) {
		if(client) {
			client->add_animator(new Animators::PawnCrush(tile->screen_x, tile->screen_y));
		}
		tile->pawn->destroy(STOMP);
	}

	tile->pawn.swap(cur_tile->pawn);
	cur_tile = tile;

	if(tile->has_black_hole) {
		destroy(Pawn::BLACKHOLE);
		if(client) {
			client->add_animator(new Animators::PawnOhShitIFellDownAHole(tile->screen_x, tile->screen_y));
		}
		return;
	}

	if(tile->smashed && !(flags & PWR_CLIMB)) {
		destroy(Pawn::FELL_OUT_OF_THE_WORLD);
		if(client) {
			client->add_animator(new Animators::PawnOhShitIFellDownAHole(tile->screen_x, tile->screen_y));
		}
		return;
	}

	if(tile->has_power) {
		if(tile->power >= 0) {
			AddPower(tile->power);
		}
		tile->has_power = false;
	}
	maybe_step_on_mine(client);
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
	}

	power_messages.push_back(PowerMessage(power, false));
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

void Pawn::maybe_step_on_mine(Client *client)
{
	if(cur_tile->has_mine && cur_tile->mine_colour != colour && !(flags & PWR_CLIMB)) {
		if(client) {
			client->add_animator(new Animators::PawnBoom(cur_tile->screen_x, cur_tile->screen_y));
		}
		cur_tile->has_mine = false;
		if(!(flags & PWR_SHIELD)) {
			this->destroy(MINED);
		}
	}
}

bool Pawn::has_power()
{
	return !powers.empty();
}
