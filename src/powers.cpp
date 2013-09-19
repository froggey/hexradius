#include "powers.hpp"
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "animator.hpp"
#include "gamestate.hpp"

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

static bool destroy_enemies(Tile::List area, pawn_ptr pawn, GameState *state) {
	Tile::List::iterator i = area.begin();
	bool ret = false;

	while(i != area.end()) {
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
			(*i)->pawn->destroy(Pawn::PWR_DESTROY);
			state->add_animator(new Animators::PawnPow((*i)->screen_x, (*i)->screen_y));
			ret = true;
		}

		i++;
	}

	return ret;
}

static bool destroy_row(pawn_ptr pawn, GameState *state) {
	return destroy_enemies(pawn->RowTiles(), pawn, state);
}

static bool destroy_radial(pawn_ptr pawn, GameState *state) {
	return destroy_enemies(pawn->RadialTiles(), pawn, state);
}

static bool destroy_bs(pawn_ptr pawn, GameState *state) {
	return destroy_enemies(pawn->bs_tiles(), pawn, state);
}

static bool destroy_fs(pawn_ptr pawn, GameState *state) {
	return destroy_enemies(pawn->fs_tiles(), pawn, state);
}


static bool raise_tile(pawn_ptr pawn, GameState *state) {
	state->add_animator(new TileAnimators::ElevationAnimator(
				    Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, +1));
	return pawn->cur_tile->SetHeight(pawn->cur_tile->height + 1);
}

static bool lower_tile(pawn_ptr pawn, GameState *state) {
	state->add_animator(new TileAnimators::ElevationAnimator(
				    Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, -1));
	return pawn->cur_tile->SetHeight(pawn->cur_tile->height - 1);
}

static bool increase_range(pawn_ptr pawn, GameState *) {
	if(pawn->range < 3) {
		pawn->range++;
		return true;
	}

	return false;
}

static bool hover(pawn_ptr pawn, GameState *) {
	if(pawn->flags & PWR_CLIMB) {
		return false;
	}

	pawn->flags |= PWR_CLIMB;
	return true;
}

static bool elevate_tiles(const Tile::List &tiles, pawn_ptr pawn, GameState *state) {
	bool ret = false;

	for(Tile::List::const_iterator i = tiles.begin(); i != tiles.end(); i++) {
		ret = ((*i)->SetHeight(2) || ret);
	}

	state->add_animator(new TileAnimators::ElevationAnimator(tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2));

	return ret;
}

static bool dig_tiles(const Tile::List &tiles, pawn_ptr pawn, GameState *state) {
	bool ret = false;

	for(Tile::List::const_iterator i = tiles.begin(); i != tiles.end(); i++) {
		ret = ((*i)->SetHeight(-2) || ret);
	}

	state->add_animator(new TileAnimators::ElevationAnimator(tiles, pawn->cur_tile, -3.0, TileAnimators::ABSOLUTE, 2));

	return ret;
}

static bool elevate_row(pawn_ptr pawn, GameState *state) {
	return elevate_tiles(pawn->RowTiles(), pawn, state);
}

static bool elevate_radial(pawn_ptr pawn, GameState *state) {
	return elevate_tiles(pawn->RadialTiles(), pawn, state);
}

static bool elevate_bs(pawn_ptr pawn, GameState *state) {
	return elevate_tiles(pawn->bs_tiles(), pawn, state);
}

static bool elevate_fs(pawn_ptr pawn, GameState *state) {
	return elevate_tiles(pawn->fs_tiles(), pawn, state);
}

static bool dig_row(pawn_ptr pawn, GameState *state) {
	return dig_tiles(pawn->RowTiles(), pawn, state);
}

static bool dig_radial(pawn_ptr pawn, GameState *state) {
	return dig_tiles(pawn->RadialTiles(), pawn, state);
}

static bool dig_bs(pawn_ptr pawn, GameState *state) {
	return dig_tiles(pawn->bs_tiles(), pawn, state);
}

static bool dig_fs(pawn_ptr pawn, GameState *state) {
	return dig_tiles(pawn->fs_tiles(), pawn, state);
}

static bool shield(pawn_ptr pawn, GameState *) {
	if(pawn->flags & PWR_SHIELD) {
		return false;
	}

	pawn->flags |= PWR_SHIELD;
	return true;
}

static bool invisibility(pawn_ptr pawn, GameState *) {
	if (pawn->flags & PWR_INVISIBLE) {
		return false;
	}

	pawn->flags |= PWR_INVISIBLE;
	return true;
}

static bool purify(Tile::List tiles, pawn_ptr pawn, GameState *state) {
	bool ret = false;

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->has_mine && (*i)->mine_colour != pawn->colour) {
			(*i)->has_mine = false;
			ret = true;
		}
		if((*i)->has_landing_pad && (*i)->landing_pad_colour != pawn->colour) {
			(*i)->has_landing_pad = false;
			ret = true;
		}
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
			(*i)->pawn->flags &= ~PWR_GOOD;
			(*i)->pawn->range = 0;
			// Hovering pawns that fall on a mine trigger it.
			(*i)->pawn->maybe_step_on_mine(state);
			// And falling onto a smashed tile is bad.
			if((*i)->smashed) {
				state->add_animator(new Animators::PawnOhShitIFellDownAHole((*i)->screen_x, (*i)->screen_y));
				(*i)->pawn->destroy(Pawn::FELL_OUT_OF_THE_WORLD);
			}
			ret = true;
		}
	}

	return ret;
}

static bool purify_row(pawn_ptr pawn, GameState *state) {
	return purify(pawn->RowTiles(), pawn, state);
}

static bool purify_radial(pawn_ptr pawn, GameState *state) {
	return purify(pawn->RadialTiles(), pawn, state);
}

static bool purify_bs(pawn_ptr pawn, GameState *state) {
	return purify(pawn->bs_tiles(), pawn, state);
}

static bool purify_fs(pawn_ptr pawn, GameState *state) {
	return purify(pawn->fs_tiles(), pawn, state);
}

static bool teleport(pawn_ptr pawn, GameState *state) {
	Tile *tile = state->teleport_hack(pawn);

	tile->pawn.swap(pawn->cur_tile->pawn);
	pawn->cur_tile = tile;

	if(tile->has_power) {
		if(tile->power >= 0) {
			pawn->AddPower(tile->power);
		}
		tile->has_power = false;
	}
	return true;
}

static bool annihilate(Tile::List tiles, GameState *state) {
	bool ret = false;

	for(Tile::List::iterator tile = tiles.begin(); tile != tiles.end(); tile++) {
		if((*tile)->pawn) {
			(*tile)->pawn->destroy(Pawn::PWR_ANNIHILATE);
			state->add_animator(new Animators::PawnPow((*tile)->screen_x, (*tile)->screen_y));
			ret = true;
		}
	}

	return ret;
}

static bool annihilate_row(pawn_ptr pawn, GameState *state) {
	return annihilate(pawn->RowTiles(), state);
}

static bool annihilate_radial(pawn_ptr pawn, GameState *state) {
	return annihilate(pawn->RadialTiles(), state);
}

static bool annihilate_bs(pawn_ptr pawn, GameState *state) {
	return annihilate(pawn->bs_tiles(), state);
}

static bool annihilate_fs(pawn_ptr pawn, GameState *state) {
	return annihilate(pawn->fs_tiles(), state);
}

static bool smash(Tile::List tiles, pawn_ptr pawn, GameState *state) {
	bool ret = false;

	for(Tile::List::iterator tile = tiles.begin(); tile != tiles.end(); tile++) {
		if((*tile)->pawn && (*tile)->pawn->colour != pawn->colour) {
			(*tile)->pawn->destroy(Pawn::PWR_SMASH);
			state->add_animator(new Animators::PawnPow((*tile)->screen_x, (*tile)->screen_y));
			ret = true;
			(*tile)->smashed = true;
			(*tile)->has_mine = false;
			(*tile)->has_power = false;
			(*tile)->has_landing_pad = false;
		}
	}

	return ret;
}

static bool smash_row(pawn_ptr pawn, GameState *state) {
	return smash(pawn->RowTiles(), pawn, state);
}

static bool smash_radial(pawn_ptr pawn, GameState *state) {
	return smash(pawn->RadialTiles(), pawn, state);
}

static bool smash_bs(pawn_ptr pawn, GameState *state) {
	return smash(pawn->bs_tiles(), pawn, state);
}

static bool smash_fs(pawn_ptr pawn, GameState *state) {
	return smash(pawn->fs_tiles(), pawn, state);
}


static int lay_mines(Tile::List tiles, PlayerColour colour) {
	int n_mines = 0;
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		Tile *tile = *i;
		if(tile->has_mine || tile->smashed || tile->has_black_hole) continue;
		tile->has_mine = true;
		tile->mine_colour = colour;
		n_mines += 1;
	}

	return n_mines;
}

static bool mine(pawn_ptr pawn, GameState *) {
	return lay_mines(Tile::List(1, pawn->cur_tile), pawn->colour) == 1;
}

static bool mine_row(pawn_ptr pawn, GameState *) {
	return lay_mines(pawn->RowTiles(), pawn->colour) != 0;
}

static bool mine_radial(pawn_ptr pawn, GameState *) {
	return lay_mines(pawn->RadialTiles(), pawn->colour) != 0;
}

static bool mine_bs(pawn_ptr pawn, GameState *) {
	return lay_mines(pawn->bs_tiles(), pawn->colour) != 0;
}

static bool mine_fs(pawn_ptr pawn, GameState *) {
	return lay_mines(pawn->fs_tiles(), pawn->colour) != 0;
}

static bool landing_pad(pawn_ptr pawn, GameState *) {
	if(pawn->cur_tile->smashed) return false;
	if(pawn->cur_tile->has_landing_pad && pawn->cur_tile->landing_pad_colour == pawn->colour) return false;
	if(pawn->cur_tile->has_black_hole) return false;
	pawn->cur_tile->has_landing_pad = true;
	pawn->cur_tile->landing_pad_colour = pawn->colour;
	return true;
}

static bool infravision(pawn_ptr pawn, GameState *) {
	if (pawn->flags & PWR_INFRAVISION) {
		return false;
	}

	pawn->flags |= PWR_INFRAVISION;
	return true;
}

static bool black_hole(pawn_ptr pawn, GameState *state) {
	Tile *tile = pawn->cur_tile;
	pawn->destroy(Pawn::BLACKHOLE);
	state->add_animator(new Animators::PawnOhShitIFellDownAHole(tile->screen_x, tile->screen_y));
	tile->has_black_hole = true;
	tile->black_hole_power = pawn->range + 1;
	tile->has_mine = false;
	tile->has_power = false;
	tile->has_landing_pad = false;
	return true;
}

Powers::Power Powers::powers[] = {
	{"Destroy", &destroy_row, 50, true, Powers::Power::row},
	{"Destroy", &destroy_radial, 50, true, Powers::Power::radial},
	{"Destroy", &destroy_bs, 50, true, Powers::Power::nw_se},
	{"Destroy", &destroy_fs, 50, true, Powers::Power::ne_sw},

	{"Raise Tile", &raise_tile, 50, true, Powers::Power::undirected},
	{"Lower Tile", &lower_tile, 50, true, Powers::Power::undirected},
	{"Increase Range", &increase_range, 20, true, Powers::Power::undirected},
	{"Hover", &hover, 30, true, Powers::Power::undirected},
	{"Shield", &shield, 30, true, Powers::Power::undirected},
	{"Invisibility", &invisibility, 40, true, Powers::Power::undirected},
	{"Infravision", &infravision, 40, true, Powers::Power::undirected},
	{"Teleport", &teleport, 60, true, Powers::Power::undirected},
	{"Landing Pad", &landing_pad, 60, true, Powers::Power::undirected},
	{"Black Hole", &black_hole, 15, true, Powers::Power::undirected},

	{"Elevate", &elevate_row, 35, true, Powers::Power::row},
	{"Elevate", &elevate_radial, 35, true, Powers::Power::radial},
	{"Elevate", &elevate_bs, 35, true, Powers::Power::nw_se},
	{"Elevate", &elevate_fs, 35, true, Powers::Power::ne_sw},

	{"Dig", &dig_row, 35, true, Powers::Power::row},
	{"Dig", &dig_radial, 35, true, Powers::Power::radial},
	{"Dig", &dig_bs, 35, true, Powers::Power::nw_se},
	{"Dig", &dig_fs, 35, true, Powers::Power::ne_sw},

	{"Purify", &purify_row, 50, true, Powers::Power::row},
	{"Purify", &purify_radial, 50, true, Powers::Power::radial},
	{"Purify", &purify_bs, 50, true, Powers::Power::nw_se},
	{"Purify", &purify_fs, 50, true, Powers::Power::ne_sw},

	{"Annihilate", &annihilate_row, 50, false, Powers::Power::row},
	{"Annihilate", &annihilate_radial, 50, false, Powers::Power::radial},
	{"Annihilate", &annihilate_bs, 50, false, Powers::Power::nw_se},
	{"Annihilate", &annihilate_fs, 50, false, Powers::Power::ne_sw},

	{"Mine", &mine, 60, true, Powers::Power::undirected},
	{"Mine", &mine_row, 20, false, Powers::Power::row},
	{"Mine", &mine_radial, 40, false, Powers::Power::radial},
	{"Mine", &mine_bs, 20, false, Powers::Power::nw_se},
	{"Mine", &mine_fs, 20, false, Powers::Power::ne_sw},

	{"Smash", &smash_row, 50, false, Powers::Power::row},
	{"Smash", &smash_radial, 50, false, Powers::Power::radial},
	{"Smash", &smash_bs, 50, false, Powers::Power::nw_se},
	{"Smash", &smash_fs, 50, false, Powers::Power::ne_sw},
};

const int Powers::num_powers = sizeof(Powers::powers) / sizeof(Powers::Power);
