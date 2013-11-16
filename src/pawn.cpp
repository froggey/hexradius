#include <set>

#include "hexradius.hpp"
#include "powers.hpp"
#include "hexradius.pb.h"
#include "client.hpp"
#include "network.hpp"
#include "gamestate.hpp"

Pawn::Pawn(PlayerColour c, GameState *game_state, Tile *ct) :
	game_state(game_state), cur_tile(ct), colour(c),
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

Tile::List Pawn::move_tiles() {
	return RadialTiles((flags & PWR_JUMP) ? range + 1 : 0);
}

static Tile* line_end(Tile* start, boost::function<Tile*(Tile*)> move_fn) {
	Tile* rv = start;
	for (;;) {
		Tile* temp = move_fn(rv);
		if (temp)
			rv = temp;
		else
			return rv;
	}
}

bool Pawn::can_move(Tile *tile, ServerGameState *state) {
	// Move only onto adjacent tiles or friendly landing pads.
	Tile::List adjacent_tiles = move_tiles();

	if (cur_tile->wrap & (1 << Tile::WRAP_LEFT))
		adjacent_tiles.push_back(line_end(cur_tile, boost::bind(&GameState::tile_right_of, state, _1)));
	if (cur_tile->wrap & (1 << Tile::WRAP_RIGHT))
		adjacent_tiles.push_back(line_end(cur_tile, boost::bind(&GameState::tile_left_of, state, _1)));
	if (cur_tile->wrap & (1 << Tile::WRAP_UP_LEFT))
		adjacent_tiles.push_back(line_end(cur_tile, boost::bind(&GameState::tile_se_of, state, _1)));
	if (cur_tile->wrap & (1 << Tile::WRAP_DOWN_LEFT))
		adjacent_tiles.push_back(line_end(cur_tile, boost::bind(&GameState::tile_ne_of, state, _1)));
	if (cur_tile->wrap & (1 << Tile::WRAP_UP_RIGHT))
		adjacent_tiles.push_back(line_end(cur_tile, boost::bind(&GameState::tile_sw_of, state, _1)));
	if (cur_tile->wrap & (1 << Tile::WRAP_DOWN_RIGHT))
		adjacent_tiles.push_back(line_end(cur_tile, boost::bind(&GameState::tile_nw_of, state, _1)));

	if((std::find(adjacent_tiles.begin(), adjacent_tiles.end(), tile) == adjacent_tiles.end()) &&
	   !(tile->has_landing_pad && tile->landing_pad_colour == colour)) {
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

	return true;
}

void Pawn::force_move(Tile *tile, ServerGameState *state) {
	assert(tile);

	// force_move is also called to recheck tile effects when a pawn's upgrade state changes.
	if(cur_tile != tile) {
		tile->pawn.swap(cur_tile->pawn);
		cur_tile = tile;
	}

	if(tile->has_black_hole) {
		state->destroy_pawn(shared_from_this(), Pawn::BLACKHOLE);
		state->add_animator(new Animators::PawnOhShitIFellDownAHole(tile->screen_x, tile->screen_y));
		return;
	}

	if(tile->smashed && !(flags & PWR_CLIMB)) {
		state->destroy_pawn(shared_from_this(), Pawn::FELL_OUT_OF_THE_WORLD);
		state->add_animator(new Animators::PawnOhShitIFellDownAHole(tile->screen_x, tile->screen_y));
		return;
	}

	if(tile->has_power) {
		if(tile->power >= 0 && !destroyed()) {
			state->add_power_notification(shared_from_this(), tile->power);
			AddPower(tile->power);
		}
		tile->has_power = false;
	}

	if(cur_tile->has_mine && cur_tile->mine_colour != colour && !(flags & PWR_CLIMB)) {
		state->add_animator(new Animators::PawnBoom(cur_tile->screen_x, cur_tile->screen_y));
		cur_tile->has_mine = false;
		state->update_tile(cur_tile);
		// Shield protects from one mine.
		if(flags & PWR_SHIELD) {
			flags &= ~PWR_SHIELD;
			state->update_pawn(shared_from_this());
		} else {
			state->destroy_pawn(shared_from_this(), MINED);
		}
	}
}

void Pawn::AddPower(int power) {
	PowerList::iterator p = powers.find(power);

	if(p != powers.end()) {
		p->second++;
	}else{
		powers.insert(std::make_pair(power, 1));
	}
}

bool Pawn::UsePower(int power, ServerGameState *state) {
	PowerList::iterator p = powers.find(power);

	// Huh?
	if(p == powers.end()) {
		return false;
	}

	if(!Powers::powers[power].can_use(shared_from_this(), state)) {
		return false;
	}

	state->use_power_notification(shared_from_this(), power);

	Powers::powers[power].func(shared_from_this(), state);

	if(p != powers.end() && --p->second == 0) {
		powers.erase(p);
	}

	return true;
}

Tile::List Pawn::RowTiles(int range)
{ return game_state->row_tiles(cur_tile, range); }
Tile::List Pawn::RadialTiles(int range)
{ return game_state->radial_tiles(cur_tile, range); }
Tile::List Pawn::bs_tiles(int range)
{ return game_state->bs_tiles(cur_tile, range); }
Tile::List Pawn::fs_tiles(int range)
{ return game_state->fs_tiles(cur_tile, range); }

Tile::List Pawn::linear_tiles(int range)
{ return game_state->linear_tiles(cur_tile, range); }

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

bool Pawn::has_power()
{
	return !powers.empty();
}
