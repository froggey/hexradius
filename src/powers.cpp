#include "powers.hpp"
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "animator.hpp"
#include "gamestate.hpp"
#include <boost/bind.hpp>

#undef ABSOLUTE
#undef RELATIVE

int Powers::RandomPower(void) {
	int total = 0;
	for (int i = 0; i < Powers::num_powers; i++) {
		total += Powers::powers[i].spawn_rate;
	}

	int n = rand() % total;
	for (int i = 0; i < Powers::num_powers; i++) {
		if(n < Powers::powers[i].spawn_rate) {
			return i;
		}

		n -= Powers::powers[i].spawn_rate;
	}

	abort();
}

static void destroy_enemies(Tile::List area, pawn_ptr pawn, ServerGameState *state, Pawn::destroy_type dt, bool enemies_only, bool smash_tile) {
	for(Tile::List::iterator i = area.begin(); i != area.end(); ++i) {
		if((*i)->pawn && (!enemies_only || (*i)->pawn->colour != pawn->colour)) {
			state->destroy_pawn((*i)->pawn, dt, pawn);
			state->add_animator(new Animators::PawnPow((*i)->screen_x, (*i)->screen_y));
			if(smash_tile) {
				(*i)->smashed = true;
				(*i)->has_mine = false;
				(*i)->has_power = false;
				(*i)->has_landing_pad = false;
				state->update_tile(*i);
			}
		}
	}
}

static bool can_destroy_enemies(Tile::List area, pawn_ptr pawn, ServerGameState *, bool enemies_only) {
	for(Tile::List::iterator i = area.begin(); i != area.end(); ++i) {
		if((*i)->pawn && (!enemies_only || (*i)->pawn->colour != pawn->colour)) {
			return true;
		}
	}
	return false;
}

static void destroy_row(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->RowTiles(), pawn, state, Pawn::PWR_DESTROY, true, false);
}

static void destroy_radial(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->RadialTiles(), pawn, state, Pawn::PWR_DESTROY, true, false);
}

static void destroy_bs(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->bs_tiles(), pawn, state, Pawn::PWR_DESTROY, true, false);
}

static void destroy_fs(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->fs_tiles(), pawn, state, Pawn::PWR_DESTROY, true, false);
}

static bool can_destroy_row(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->RowTiles(), pawn, state, true);
}

static bool can_destroy_radial(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->RadialTiles(), pawn, state, true);
}

static bool can_destroy_bs(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->bs_tiles(), pawn, state, true);
}

static bool can_destroy_fs(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->fs_tiles(), pawn, state, true);
}


static void raise_tile(pawn_ptr pawn, ServerGameState *state) {
	state->add_animator(new TileAnimators::ElevationAnimator(
				    Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, +1));
	state->set_tile_height(pawn->cur_tile, pawn->cur_tile->height + 1);
}

static void lower_tile(pawn_ptr pawn, ServerGameState *state) {
	state->add_animator(new TileAnimators::ElevationAnimator(
				    Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, -1));
	state->set_tile_height(pawn->cur_tile, pawn->cur_tile->height - 1);
}

static bool can_raise_tile(pawn_ptr pawn, ServerGameState *) {
	return pawn->cur_tile->height != 2;
}

static bool can_lower_tile(pawn_ptr pawn, ServerGameState *) {
	return pawn->cur_tile->height != -2;
}


static void increase_range(pawn_ptr pawn, ServerGameState *state) {
	assert(pawn->range < 3);
	pawn->range++;
	state->update_pawn(pawn);
}

static bool can_increase_range(pawn_ptr pawn, ServerGameState *) {
	return (pawn->range < 3);
}

static void hover(pawn_ptr pawn, ServerGameState *state) {
	state->grant_upgrade(pawn, PWR_CLIMB);
}

static void dig_elevate_tiles(const Tile::List &tiles, pawn_ptr pawn, ServerGameState *state, int target_elevation) {
	state->add_animator(new TileAnimators::ElevationAnimator(tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, target_elevation));

	for(Tile::List::const_iterator i = tiles.begin(); i != tiles.end(); i++) {
		state->set_tile_height(*i, target_elevation);
	}
}

static void elevate_tiles(const Tile::List &tiles, pawn_ptr pawn, ServerGameState *state) {
	dig_elevate_tiles(tiles, pawn, state, +2);
}

static void dig_tiles(const Tile::List &tiles, pawn_ptr pawn, ServerGameState *state) {
	dig_elevate_tiles(tiles, pawn, state, -2);
}

static void elevate_row(pawn_ptr pawn, ServerGameState *state) {
	elevate_tiles(pawn->RowTiles(), pawn, state);
}

static void elevate_radial(pawn_ptr pawn, ServerGameState *state) {
	elevate_tiles(pawn->RadialTiles(), pawn, state);
}

static void elevate_bs(pawn_ptr pawn, ServerGameState *state) {
	elevate_tiles(pawn->bs_tiles(), pawn, state);
}

static void elevate_fs(pawn_ptr pawn, ServerGameState *state) {
	elevate_tiles(pawn->fs_tiles(), pawn, state);
}

static void dig_row(pawn_ptr pawn, ServerGameState *state) {
	dig_tiles(pawn->RowTiles(), pawn, state);
}

static void dig_radial(pawn_ptr pawn, ServerGameState *state) {
	dig_tiles(pawn->RadialTiles(), pawn, state);
}

static void dig_bs(pawn_ptr pawn, ServerGameState *state) {
	dig_tiles(pawn->bs_tiles(), pawn, state);
}

static void dig_fs(pawn_ptr pawn, ServerGameState *state) {
	dig_tiles(pawn->fs_tiles(), pawn, state);
}

static bool can_dig_elevate_tiles(const Tile::List &tiles, pawn_ptr, ServerGameState *, int target_elevation) {
	for(Tile::List::const_iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->height != target_elevation) {
			return true;
		}
	}

	return false;
}

static bool can_elevate_tiles(const Tile::List &tiles, pawn_ptr pawn, ServerGameState *state) {
	return can_dig_elevate_tiles(tiles, pawn, state, +2);
}

static bool can_dig_tiles(const Tile::List &tiles, pawn_ptr pawn, ServerGameState *state) {
	return can_dig_elevate_tiles(tiles, pawn, state, -2);
}

static bool can_elevate_row(pawn_ptr pawn, ServerGameState *state) {
	return can_elevate_tiles(pawn->RowTiles(), pawn, state);
}

static bool can_elevate_radial(pawn_ptr pawn, ServerGameState *state) {
	return can_elevate_tiles(pawn->RadialTiles(), pawn, state);
}

static bool can_elevate_bs(pawn_ptr pawn, ServerGameState *state) {
	return can_elevate_tiles(pawn->bs_tiles(), pawn, state);
}

static bool can_elevate_fs(pawn_ptr pawn, ServerGameState *state) {
	return can_elevate_tiles(pawn->fs_tiles(), pawn, state);
}

static bool can_dig_row(pawn_ptr pawn, ServerGameState *state) {
	return can_dig_tiles(pawn->RowTiles(), pawn, state);
}

static bool can_dig_radial(pawn_ptr pawn, ServerGameState *state) {
	return can_dig_tiles(pawn->RadialTiles(), pawn, state);
}

static bool can_dig_bs(pawn_ptr pawn, ServerGameState *state) {
	return can_dig_tiles(pawn->bs_tiles(), pawn, state);
}

static bool can_dig_fs(pawn_ptr pawn, ServerGameState *state) {
	return can_dig_tiles(pawn->fs_tiles(), pawn, state);
}

static void shield(pawn_ptr pawn, ServerGameState *state) {
	state->grant_upgrade(pawn, PWR_SHIELD);
}

static void invisibility(pawn_ptr pawn, ServerGameState *state) {
	state->grant_upgrade(pawn, PWR_INVISIBLE);
}

static void purify(Tile::List tiles, pawn_ptr pawn, ServerGameState *state) {
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->has_mine && (*i)->mine_colour != pawn->colour) {
			(*i)->has_mine = false;
		}
		if((*i)->has_landing_pad && (*i)->landing_pad_colour != pawn->colour) {
			(*i)->has_landing_pad = false;
		}
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
			(*i)->pawn->flags &= ~PWR_GOOD;
			(*i)->pawn->range = 0;
			state->update_pawn((*i)->pawn);
			// This pawn got changed, rerun the tile effects.
			state->move_pawn_to((*i)->pawn, (*i)->pawn->cur_tile);
		}
		state->update_tile(*i);
	}
}

static void purify_row(pawn_ptr pawn, ServerGameState *state) {
	purify(pawn->RowTiles(), pawn, state);
}

static void purify_radial(pawn_ptr pawn, ServerGameState *state) {
	purify(pawn->RadialTiles(), pawn, state);
}

static void purify_bs(pawn_ptr pawn, ServerGameState *state) {
	purify(pawn->bs_tiles(), pawn, state);
}

static void purify_fs(pawn_ptr pawn, ServerGameState *state) {
	purify(pawn->fs_tiles(), pawn, state);
}

static bool can_purify(Tile::List tiles, pawn_ptr pawn, ServerGameState *) {
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->has_mine && (*i)->mine_colour != pawn->colour) {
			return true;
		}
		if((*i)->has_landing_pad && (*i)->landing_pad_colour != pawn->colour) {
			return true;
		}
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
			return true;
		}
	}
	return false;
}

static bool can_purify_row(pawn_ptr pawn, ServerGameState *state) {
	return can_purify(pawn->RowTiles(), pawn, state);
}

static bool can_purify_radial(pawn_ptr pawn, ServerGameState *state) {
	return can_purify(pawn->RadialTiles(), pawn, state);
}

static bool can_purify_bs(pawn_ptr pawn, ServerGameState *state) {
	return can_purify(pawn->bs_tiles(), pawn, state);
}

static bool can_purify_fs(pawn_ptr pawn, ServerGameState *state) {
	return can_purify(pawn->fs_tiles(), pawn, state);
}

static void teleport(pawn_ptr pawn, ServerGameState *state) {
	state->teleport_hack(pawn);
}

static bool can_teleport(pawn_ptr, ServerGameState *state) {
	Tile::List targets = RandomTiles(state->tiles, 1, false, false, false, false);
	return !targets.empty();
}

static void annihilate_row(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->RowTiles(), pawn, state, Pawn::PWR_ANNIHILATE, false, false);
}

static void annihilate_radial(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->RadialTiles(), pawn, state, Pawn::PWR_ANNIHILATE, false, false);
}

static void annihilate_bs(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->bs_tiles(), pawn, state, Pawn::PWR_ANNIHILATE, false, false);
}

static void annihilate_fs(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->fs_tiles(), pawn, state, Pawn::PWR_ANNIHILATE, false, false);
}

static bool can_annihilate_row(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->RowTiles(), pawn, state, false);
}

static bool can_annihilate_radial(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->RadialTiles(), pawn, state, false);
}

static bool can_annihilate_bs(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->bs_tiles(), pawn, state, false);
}

static bool can_annihilate_fs(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->fs_tiles(), pawn, state, false);
}


static void smash_row(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->RowTiles(), pawn, state, Pawn::PWR_SMASH, true, true);
}

static void smash_radial(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->RadialTiles(), pawn, state, Pawn::PWR_SMASH, true, true);
}

static void smash_bs(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->bs_tiles(), pawn, state, Pawn::PWR_SMASH, true, true);
}

static void smash_fs(pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(pawn->fs_tiles(), pawn, state, Pawn::PWR_SMASH, true, true);
}

static bool can_smash_row(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->RowTiles(), pawn, state, true);
}

static bool can_smash_radial(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->RadialTiles(), pawn, state, true);
}

static bool can_smash_bs(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->bs_tiles(), pawn, state, true);
}

static bool can_smash_fs(pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(pawn->fs_tiles(), pawn, state, true);
}



static void lay_mines(Tile::List tiles, PlayerColour colour, ServerGameState *state) {
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		Tile *tile = *i;
		if(tile->has_mine || tile->smashed || tile->has_black_hole) continue;
		tile->has_mine = true;
		tile->mine_colour = colour;
		state->update_tile(tile);
	}
}

static void mine(pawn_ptr pawn, ServerGameState *state) {
	lay_mines(Tile::List(1, pawn->cur_tile), pawn->colour, state);
}

static void mine_row(pawn_ptr pawn, ServerGameState *state) {
	lay_mines(pawn->RowTiles(), pawn->colour, state);
}

static void mine_radial(pawn_ptr pawn, ServerGameState *state) {
	lay_mines(pawn->RadialTiles(), pawn->colour, state);
}

static void mine_bs(pawn_ptr pawn, ServerGameState *state) {
	lay_mines(pawn->bs_tiles(), pawn->colour, state);
}

static void mine_fs(pawn_ptr pawn, ServerGameState *state) {
	lay_mines(pawn->fs_tiles(), pawn->colour, state);
}

static int can_lay_mines(Tile::List tiles, PlayerColour, ServerGameState *) {
	int n_mines = 0;
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		Tile *tile = *i;
		if(tile->has_mine || tile->smashed || tile->has_black_hole) continue;
		n_mines += 1;
	}

	return n_mines;
}

static bool can_mine(pawn_ptr pawn, ServerGameState *state) {
	return can_lay_mines(Tile::List(1, pawn->cur_tile), pawn->colour, state) == 1;
}

static bool can_mine_row(pawn_ptr pawn, ServerGameState *state) {
	return can_lay_mines(pawn->RowTiles(), pawn->colour, state) != 0;
}

static bool can_mine_radial(pawn_ptr pawn, ServerGameState *state) {
	return can_lay_mines(pawn->RadialTiles(), pawn->colour, state) != 0;
}

static bool can_mine_bs(pawn_ptr pawn, ServerGameState *state) {
	return can_lay_mines(pawn->bs_tiles(), pawn->colour, state) != 0;
}

static bool can_mine_fs(pawn_ptr pawn, ServerGameState *state) {
	return can_lay_mines(pawn->fs_tiles(), pawn->colour, state) != 0;
}

static void landing_pad(pawn_ptr pawn, ServerGameState *state) {
	pawn->cur_tile->has_landing_pad = true;
	pawn->cur_tile->landing_pad_colour = pawn->colour;
	state->update_tile(pawn->cur_tile);
}

static bool can_landing_pad(pawn_ptr pawn, ServerGameState *) {
	if(pawn->cur_tile->smashed) return false;
	if(pawn->cur_tile->has_landing_pad && pawn->cur_tile->landing_pad_colour == pawn->colour) return false;
	if(pawn->cur_tile->has_black_hole) return false;
	return true;
}

static void infravision(pawn_ptr pawn, ServerGameState *state) {
	state->grant_upgrade(pawn, PWR_INFRAVISION);
}

static void black_hole(pawn_ptr pawn, ServerGameState *state) {
	Tile *tile = pawn->cur_tile;
	state->destroy_pawn(pawn, Pawn::BLACKHOLE, pawn);
	state->add_animator(new Animators::PawnOhShitIFellDownAHole(tile->screen_x, tile->screen_y));
	tile->has_black_hole = true;
	tile->black_hole_power = pawn->range + 1;
	tile->has_mine = false;
	tile->has_power = false;
	tile->has_landing_pad = false;
	state->update_tile(tile);
}

static bool can_black_hole(pawn_ptr, ServerGameState *) {
	return true;
}

static bool can_use_upgrade(pawn_ptr pawn, ServerGameState *, uint32_t upgrade)
{
	return !(pawn->flags & upgrade);
}

Powers::Power Powers::powers[] = {
	{"Destroy", &destroy_row, can_destroy_row, 50, true, Powers::Power::row},
	{"Destroy", &destroy_radial, can_destroy_radial, 50, true, Powers::Power::radial},
	{"Destroy", &destroy_bs, can_destroy_bs, 50, true, Powers::Power::nw_se},
	{"Destroy", &destroy_fs, can_destroy_fs, 50, true, Powers::Power::ne_sw},

	{"Raise Tile", &raise_tile, can_raise_tile, 50, true, Powers::Power::undirected},
	{"Lower Tile", &lower_tile, can_lower_tile, 50, true, Powers::Power::undirected},
	{"Increase Range", &increase_range, can_increase_range, 20, true, Powers::Power::undirected},
	{"Hover", &hover, boost::bind(can_use_upgrade, _1, _2, PWR_CLIMB), 30, true, Powers::Power::undirected},
	{"Shield", &shield, boost::bind(can_use_upgrade, _1, _2, PWR_SHIELD), 30, true, Powers::Power::undirected},
	{"Invisibility", &invisibility, boost::bind(can_use_upgrade, _1, _2, PWR_INVISIBLE), 40, true, Powers::Power::undirected},
	{"Infravision", &infravision, boost::bind(can_use_upgrade, _1, _2, PWR_INFRAVISION), 40, true, Powers::Power::undirected},
	{"Teleport", &teleport, can_teleport, 60, true, Powers::Power::undirected},
	{"Landing Pad", &landing_pad, can_landing_pad, 60, true, Powers::Power::undirected},
	{"Black Hole", &black_hole, can_black_hole, 15, true, Powers::Power::undirected},

	{"Elevate", &elevate_row, can_elevate_row, 35, true, Powers::Power::row},
	{"Elevate", &elevate_radial, can_elevate_radial, 35, true, Powers::Power::radial},
	{"Elevate", &elevate_bs, can_elevate_bs, 35, true, Powers::Power::nw_se},
	{"Elevate", &elevate_fs, can_elevate_fs, 35, true, Powers::Power::ne_sw},

	{"Dig", &dig_row, can_dig_row, 35, true, Powers::Power::row},
	{"Dig", &dig_radial, can_dig_radial, 35, true, Powers::Power::radial},
	{"Dig", &dig_bs, can_dig_bs, 35, true, Powers::Power::nw_se},
	{"Dig", &dig_fs, can_dig_fs, 35, true, Powers::Power::ne_sw},

	{"Purify", &purify_row, can_purify_row, 50, true, Powers::Power::row},
	{"Purify", &purify_radial, can_purify_radial, 50, true, Powers::Power::radial},
	{"Purify", &purify_bs, can_purify_bs, 50, true, Powers::Power::nw_se},
	{"Purify", &purify_fs, can_purify_fs, 50, true, Powers::Power::ne_sw},

	{"Annihilate", &annihilate_row, can_annihilate_row, 50, false, Powers::Power::row},
	{"Annihilate", &annihilate_radial, can_annihilate_radial, 50, false, Powers::Power::radial},
	{"Annihilate", &annihilate_bs, can_annihilate_bs, 50, false, Powers::Power::nw_se},
	{"Annihilate", &annihilate_fs, can_annihilate_fs, 50, false, Powers::Power::ne_sw},

	{"Mine", &mine, can_mine, 60, true, Powers::Power::undirected},
	{"Mine", &mine_row, can_mine_row, 20, false, Powers::Power::row},
	{"Mine", &mine_radial, can_mine_radial, 40, false, Powers::Power::radial},
	{"Mine", &mine_bs, can_mine_bs, 20, false, Powers::Power::nw_se},
	{"Mine", &mine_fs, can_mine_fs, 20, false, Powers::Power::ne_sw},

	{"Smash", &smash_row, can_smash_row, 50, false, Powers::Power::row},
	{"Smash", &smash_radial, can_smash_radial, 50, false, Powers::Power::radial},
	{"Smash", &smash_bs, can_smash_bs, 50, false, Powers::Power::nw_se},
	{"Smash", &smash_fs, can_smash_fs, 50, false, Powers::Power::ne_sw},
};

const int Powers::num_powers = sizeof(Powers::powers) / sizeof(Powers::Power);
