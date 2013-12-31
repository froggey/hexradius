#include "powers.hpp"
#include "hexradius.hpp"
#include "tile_anims.hpp"
#include "animator.hpp"
#include "gamestate.hpp"
#include <boost/bind.hpp>
#include <algorithm>

#undef ABSOLUTE
#undef RELATIVE

using namespace Powers;

int Powers::RandomPower(bool fog_of_war) {
	int total = 0;
	for (size_t i = 0; i < powers.size(); i++) {
		if(!fog_of_war && (Powers::powers[i].requirements & Powers::REQ_FOG_OF_WAR)) {
			continue;
		}
		total += Powers::powers[i].spawn_rate;
	}

	int n = rand() % total;
	for (size_t i = 0; i < powers.size(); i++) {
		if(!fog_of_war && (Powers::powers[i].requirements & Powers::REQ_FOG_OF_WAR)) {
			continue;
		}
		if(n < Powers::powers[i].spawn_rate) {
			return i;
		}

		n -= Powers::powers[i].spawn_rate;
	}

	abort();
}

/// Functions for getting a list of tiles from a pawn.
typedef boost::function<Tile::List(pawn_ptr)> tile_area_function;

// Return the pawn's current tile, a single point.
/*
static Tile::List point_tile(pawn_ptr pawn)
{
	return Tile::List(1, pawn->cur_tile);
}
*/

// Return the surrounding radial tiles.
static tile_area_function radial_tiles = boost::bind(&Pawn::RadialTiles, _1);
// Functions that deal with the 3 linear directions.
static tile_area_function row_tiles = boost::bind(&Pawn::RowTiles, _1);
static tile_area_function fs_tiles = boost::bind(&Pawn::fs_tiles, _1);
static tile_area_function bs_tiles = boost::bind(&Pawn::bs_tiles, _1);

static void destroy_enemies(Tile::List area, pawn_ptr pawn, ServerGameState *state, Pawn::destroy_type dt, bool enemies_only, bool smash_tile) {
	for(Tile::List::iterator i = area.begin(); i != area.end(); ++i) {
		if((*i)->pawn && (!enemies_only || (*i)->pawn->colour != pawn->colour)) {
			state->destroy_pawn((*i)->pawn, dt, pawn);
			state->add_animator("pow", *i);
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

/// Destroy: Nice & simple, just destroy enemy pawns in the target area.
static bool test_destroy_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state)
{
	return can_destroy_enemies(area_fn(pawn), pawn, state, true);
}

static void use_destroy_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state)
{
	destroy_enemies(area_fn(pawn), pawn, state, Pawn::PWR_DESTROY, true, false);
}

/// Confuse: Confused pawns have a chance to move in the wrong direction when moved.
static bool test_confuse_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState */*state*/) {
	Tile::List area = area_fn(pawn);
	for(Tile::List::iterator i = area.begin(); i != area.end(); ++i) {
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour && !((*i)->pawn->flags & PWR_CONFUSED)) {
			return true;
		}
	}
	return false;
}

static void use_confuse_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	Tile::List area = area_fn(pawn);
	for(Tile::List::iterator i = area.begin(); i != area.end(); ++i) {
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
			(*i)->pawn->flags |= PWR_CONFUSED;
			state->update_pawn((*i)->pawn);
		}
	}
}

/// Hijack: Recruit pawns.
static bool test_hijack_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState */*state*/) {
	Tile::List area = area_fn(pawn);
	for(Tile::List::iterator i = area.begin(); i != area.end(); ++i) {
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
			return true;
		}
	}
	return false;
}

static void use_hijack_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	Tile::List area = area_fn(pawn);
	for(Tile::List::iterator i = area.begin(); i != area.end(); ++i) {
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
			(*i)->pawn->colour = pawn->colour;
			state->update_pawn((*i)->pawn);
		}
	}
}

/// Annihilate: Destroy *all* pawns in the target area.
static bool test_annihilate_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(area_fn(pawn), pawn, state, false);
}

static void use_annihilate_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(area_fn(pawn), pawn, state, Pawn::PWR_ANNIHILATE, false, false);
}

/// Smash: Destroy enemy pawns in the target area and smash the tiles they're on.
static bool test_smash_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	return can_destroy_enemies(area_fn(pawn), pawn, state, true);
}

static void use_smash_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	destroy_enemies(area_fn(pawn), pawn, state, Pawn::PWR_SMASH, true, true);
}

/// Raise Tile: Raise the pawn's current tile up one level.
static void raise_tile(pawn_ptr pawn, ServerGameState *state) {
	state->add_animator(new TileAnimators::ElevationAnimator(
				    Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, +1));
	state->set_tile_height(pawn->cur_tile, pawn->cur_tile->height + 1);
}

static bool can_raise_tile(pawn_ptr pawn, ServerGameState *) {
	return pawn->cur_tile->height != 2;
}

/// Lower Tile: Lower the pawn's current tile down one level.
static void lower_tile(pawn_ptr pawn, ServerGameState *state) {
	state->add_animator(new TileAnimators::ElevationAnimator(
				    Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, -1));
	state->set_tile_height(pawn->cur_tile, pawn->cur_tile->height - 1);
}

static bool can_lower_tile(pawn_ptr pawn, ServerGameState *) {
	return pawn->cur_tile->height != -2;
}

// Common test function for dig & elevate.
static bool can_dig_elevate_tiles(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *, int target_elevation) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::const_iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->height != target_elevation) {
			return true;
		}
	}

	return false;
}

// Common use function for dig & elevate.
static void dig_elevate_tiles(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state, int target_elevation) {
	Tile::List tiles = area_fn(pawn);

	state->add_animator(new TileAnimators::ElevationAnimator(tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, target_elevation));

	for(Tile::List::const_iterator i = tiles.begin(); i != tiles.end(); i++) {
		state->set_tile_height(*i, target_elevation);
	}
}

/// Elevate: Raise the tiles up to the maximum elevation.
static void use_elevate_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	dig_elevate_tiles(area_fn, pawn, state, +2);
}

static bool test_elevate_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	return can_dig_elevate_tiles(area_fn, pawn, state, +2);
}

/// Dig: Lower the tiles down to the minimum elevation.
static void use_dig_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	dig_elevate_tiles(area_fn, pawn, state, -2);
}

static bool test_dig_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	return can_dig_elevate_tiles(area_fn, pawn, state, -2);
}

/// Purify: Clear bad upgrades from friendly pawns, good upgrades from enemy pawns and remove enemy tile modifications.
static void use_purify_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->has_mine && (*i)->mine_colour != pawn->colour) {
			(*i)->has_mine = false;
		}
		if((*i)->has_landing_pad && (*i)->landing_pad_colour != pawn->colour) {
			(*i)->has_landing_pad = false;
		}
		if((*i)->has_eye && (*i)->eye_colour != pawn->colour) {
			(*i)->has_eye = false;
		}
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
			(*i)->pawn->flags &= ~PWR_GOOD;
			(*i)->pawn->range = 0;
			state->update_pawn((*i)->pawn);
			// This pawn got changed, rerun the tile effects.
			state->move_pawn_to((*i)->pawn, (*i)->pawn->cur_tile);
		}
		if ((*i)->pawn && (*i)->pawn->colour == pawn->colour) {
			(*i)->pawn->flags &= PWR_GOOD;
			state->update_pawn((*i)->pawn);
		}
		state->update_tile(*i);
	}
}

static bool test_purify_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->has_mine && (*i)->mine_colour != pawn->colour) {
			return true;
		}
		if((*i)->has_landing_pad && (*i)->landing_pad_colour != pawn->colour) {
			return true;
		}
		if((*i)->has_eye && (*i)->eye_colour != pawn->colour) {
			return true;
		}
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
			return true;
		}
		if((*i)->pawn && (*i)->pawn->colour == pawn->colour && ((*i)->pawn->flags & ~PWR_GOOD)) {
			return true;
		}
	}

	return false;
}

/// Pick Up: Pick up all orbs from the area.
static void use_pickup_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->has_power) {
			state->add_power_notification(pawn, (*i)->power);
			pawn->AddPower((*i)->power);
			(*i)->has_power = false;
			state->update_tile(*i);
		}
	}
}

static bool test_pickup_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if((*i)->has_power) {
			return true;
		}
	}

	return false;
}

/// Repaint: Change the color of all colored terrain features (mines, landing pads) to the user's color.
static void use_repaint_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if ((*i)->has_mine)
			(*i)->mine_colour = pawn->colour;
		if ((*i)->has_landing_pad)
			(*i)->landing_pad_colour = pawn->colour;
		if ((*i)->has_eye)
			(*i)->eye_colour = pawn->colour;

		state->update_tile(*i);
	}
}

static bool test_repaint_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); i++) {
		if ((*i)->has_mine && (*i)->mine_colour != pawn->colour)
			return true;
		if ((*i)->has_landing_pad && (*i)->landing_pad_colour != pawn->colour)
			return true;
		if ((*i)->has_eye && (*i)->eye_colour != pawn->colour)
			return true;
	}

	return false;
}

/// Wrap: Allow jumping from one side of the board to the other.
static void use_wrap_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state, Powers::Power::Directionality direction) {
	Tile::List tiles = area_fn(pawn);

	for (Tile::List::iterator it = tiles.begin(); it != tiles.end(); it++) {
		if (direction == Powers::Power::row) {
			if (!state->tile_left_of(*it))
				(*it)->wrap |= 1 << Tile::WRAP_LEFT;
			if (!state->tile_right_of(*it))
				(*it)->wrap |= 1 << Tile::WRAP_RIGHT;
		}
		else if (direction == Powers::Power::nw_se) {
			if (!state->tile_nw_of(*it))
				(*it)->wrap |= 1 << Tile::WRAP_UP_LEFT;
			if (!state->tile_se_of(*it))
				(*it)->wrap |= 1 << Tile::WRAP_DOWN_RIGHT;
		}
		else {
			if (!state->tile_ne_of(*it))
				(*it)->wrap |= 1 << Tile::WRAP_UP_RIGHT;
			if (!state->tile_sw_of(*it))
				(*it)->wrap |= 1 << Tile::WRAP_DOWN_LEFT;
		}

		state->update_tile(*it);
	}
}

static bool test_wrap_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state, Powers::Power::Directionality direction) {
	Tile::List tiles = area_fn(pawn);

	for (Tile::List::iterator it = tiles.begin(); it != tiles.end(); it++) {
		if (direction == Powers::Power::row) {
			if (!state->tile_left_of(*it) && !((*it)->wrap & (1 << Tile::WRAP_LEFT)))
				return true;
			if (!state->tile_right_of(*it) && !((*it)->wrap & (1 << Tile::WRAP_RIGHT)))
				return true;
		}
		else if (direction == Powers::Power::nw_se) {
			if (!state->tile_nw_of(*it) && !((*it)->wrap & (1 << Tile::WRAP_UP_LEFT)))
				return true;
			if (!state->tile_se_of(*it) && !((*it)->wrap & (1 << Tile::WRAP_DOWN_RIGHT)))
				return true;
		}
		else {
			if (!state->tile_ne_of(*it) && !((*it)->wrap & (1 << Tile::WRAP_UP_RIGHT)))
				return true;
			if (!state->tile_sw_of(*it) && !((*it)->wrap & (1 << Tile::WRAP_DOWN_LEFT)))
				return true;
		}
	}
	return false;
}

/// Teleport: Move to a random location on the board, will not land on a mine, smashed/black hole tile or existing pawn.
static void teleport(pawn_ptr pawn, ServerGameState *state) {
	state->teleport_hack(pawn);
}

static bool can_teleport(pawn_ptr, ServerGameState *state) {
	Tile::List targets = RandomTiles(state->tiles, 1, false, false, false, false);
	return !targets.empty();
}

/// Mine: Add a mine modification to the targeted area.
static bool can_mine_tile(Tile *tile) {
	if(tile->has_mine) return false;
	if(tile->smashed) return false;
	if(tile->has_black_hole) return false;
	return true;
}

static bool test_mine_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		if(can_mine_tile(*i)) {
			return true;
		}
	}

	return false;
}

static void use_mine_power(tile_area_function area_fn, pawn_ptr pawn, ServerGameState *state) {
	Tile::List tiles = area_fn(pawn);

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		Tile *tile = *i;
		if(!can_mine_tile(*i)) continue;
		tile->has_mine = true;
		tile->mine_colour = pawn->colour;
		state->update_tile(tile);
	}
}

/// Landing Pad: Add a landing pad modification to the current tile.
/// Pawns can move to landing pads from anywhere on the board.
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

/// Black Hole: Overload the pawn's warp core to create a dangerous gravitational anomaly.
/// Black holes will pull pawns in from far & wide, and gain power when
/// created by a pawn with increased range.
static void black_hole(pawn_ptr pawn, ServerGameState *state) {
	Tile *tile = pawn->cur_tile;
	state->destroy_pawn(pawn, Pawn::BLACKHOLE, pawn);
	state->add_animator("ohshitifelldownahole", tile);
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

/// Increase Range: Power up a pawn and increase the range of various powers.
static void increase_range(pawn_ptr pawn, ServerGameState *state) {
	assert(pawn->range < 3);
	pawn->range++;
	state->update_pawn(pawn);
}

static bool can_increase_range(pawn_ptr pawn, ServerGameState *) {
	return (pawn->range < 3);
}

static bool can_use_upgrade(pawn_ptr pawn, ServerGameState *, uint32_t upgrade)
{
	return !(pawn->flags & upgrade);
}

static void use_upgrade_power(pawn_ptr pawn, ServerGameState *state, uint32_t upgrade)
{
	state->grant_upgrade(pawn, upgrade);
}

static void use_eye(pawn_ptr pawn, ServerGameState *state)
{
	pawn->cur_tile->has_eye = true;
	pawn->cur_tile->eye_colour = pawn->colour;
	state->update_tile(pawn->cur_tile);
}

static bool can_eye(pawn_ptr pawn, ServerGameState *) {
	if(pawn->cur_tile->has_eye && pawn->cur_tile->eye_colour == pawn->colour) return false;
	if(pawn->cur_tile->has_black_hole) return false;
	return true;
}

static bool can_worm(pawn_ptr, ServerGameState *) {
	return true;
}

static void use_worm(pawn_ptr pawn, ServerGameState *state) {
	state->run_worm_stuff(pawn, 15 * (pawn->range + 1));
}

/// Prod: Do thing.
static bool can_prod(int, pawn_ptr, ServerGameState *) {
	return true;
}

/// Scramble Powers: Change a pawn's powers into the same number of randomly chosen ones
static bool can_scramble(pawn_ptr pawn, ServerGameState *) {
	return pawn->powers.size() > 1; // Not including the scramble power.
}

static void use_scramble(pawn_ptr pawn, ServerGameState *state) {
	int total_powers = -1; // Minus the scramble power.
	for(Pawn::PowerList::iterator itr = pawn->powers.begin(); itr != pawn->powers.end(); ++itr) {
		total_powers += itr->second;
	}

	// Remove everything but one scramble.
	// Pawn::UsePower keeps an iterator into the power list.
	for(size_t i = 0; i < Powers::powers.size(); ++i) {
		if(Powers::powers[i].name == std::string("Scramble Powers")) {
			pawn->powers[i] = 1;
		} else {
			pawn->powers.erase(int(i));
		}
	}

	// Add new powers.
	for(int i = 0; i < total_powers; ++i) {
		pawn->AddPower(Powers::RandomPower(false));
	}
	state->update_pawn(pawn);
}

//typedef boost::function<Tile*(GameState *, Tile *)> direction_fn;
static Tile *(GameState::*directions[])(Tile *) = {
	&GameState::tile_ne_of,
	&GameState::tile_right_of,
	&GameState::tile_se_of,
	&GameState::tile_sw_of,
	&GameState::tile_left_of,
	&GameState::tile_nw_of,
};

static void use_prod(int dir, pawn_ptr pawn, ServerGameState *state) {
	Tile *tile = (state->*(directions[dir]))(pawn->cur_tile);
	if(!tile) return;
	if(tile->has_mine) {
		// ### This should probably be part of Tile.
		if(tile->pawn) {
			tile->pawn->detonate_mine(state);
		} else {
			state->add_animator("boom", tile);
			tile->has_mine = false;
			state->update_tile(tile);
		}
	}
	if(tile->pawn) {
		state->play_prod_animation(pawn, tile->pawn);
	}
}

static void def_power(const char *name,
		      boost::function<void(pawn_ptr, ServerGameState *)> use_fn,
		      boost::function<bool(pawn_ptr, ServerGameState *)> test_fn,
		      int probability, Power::Directionality direction,
		      unsigned int requirements = 0)
{
	powers.push_back((Power){name, use_fn, test_fn, probability, direction, requirements});
}


// Define powers for each direction.
static void def_directional_power(const char *name,
				  boost::function<void(tile_area_function, pawn_ptr, ServerGameState *)> use_fn,
				  boost::function<bool(tile_area_function, pawn_ptr, ServerGameState *)> test_fn,
				  int probability,
				  unsigned int requirements = 0)
{
	def_power(name,
		  boost::bind(use_fn, radial_tiles, _1, _2),
		  boost::bind(test_fn, radial_tiles, _1, _2),
		  probability/4,
		  Powers::Power::radial,
		  requirements);
	def_power(name,
		  boost::bind(use_fn, row_tiles, _1, _2),
		  boost::bind(test_fn, row_tiles, _1, _2),
		  probability/4,
		  Powers::Power::row,
		  requirements);
	def_power(name,
		  boost::bind(use_fn, bs_tiles, _1, _2),
		  boost::bind(test_fn, bs_tiles, _1, _2),
		  probability/4,
		  Powers::Power::nw_se,
		  requirements);
	def_power(name,
		  boost::bind(use_fn, fs_tiles, _1, _2),
		  boost::bind(test_fn, fs_tiles, _1, _2),
		  probability/4,
		  Powers::Power::ne_sw,
		  requirements);
}

static void def_upgrade_power(const char *name, uint32_t upgrade, int probability, unsigned int requirements = 0)
{
	def_power(name,
		  boost::bind(use_upgrade_power, _1, _2, upgrade),
		  boost::bind(can_use_upgrade, _1, _2, upgrade),
		  probability,
		  Powers::Power::undirected,
		  requirements);
}

std::vector<Powers::Power> Powers::powers;
void Powers::init_powers()
{
	def_directional_power("Destroy", use_destroy_power, test_destroy_power, 100);
	def_directional_power("Annihilate", use_annihilate_power, test_annihilate_power, 100);
	def_directional_power("Smash", use_smash_power, test_smash_power, 100);
	def_directional_power("Elevate", use_elevate_power, test_elevate_power, 70);
	def_directional_power("Dig", use_dig_power, test_dig_power, 70);
	def_directional_power("Purify", use_purify_power, test_purify_power, 100);
	def_directional_power("Mine", use_mine_power, test_mine_power, 80);
	def_directional_power("Pick Up", use_pickup_power, test_pickup_power, 100);
	def_directional_power("Repaint", use_repaint_power, test_repaint_power, 100);
	def_directional_power("Confuse", use_confuse_power, test_confuse_power, 60);
	def_directional_power("Hijack", use_hijack_power, test_hijack_power, 50);
	int wrap_prob = 100;
	def_power("Wrap",
		  boost::bind(use_wrap_power, row_tiles, _1, _2, Powers::Power::row),
		  boost::bind(test_wrap_power, row_tiles, _1, _2, Powers::Power::row),
		  wrap_prob/3,
		  Powers::Power::row);
	def_power("Wrap",
		  boost::bind(use_wrap_power, bs_tiles, _1, _2, Powers::Power::nw_se),
		  boost::bind(test_wrap_power, bs_tiles, _1, _2, Powers::Power::nw_se),
		  wrap_prob/3,
		  Powers::Power::nw_se);
	def_power("Wrap",
		  boost::bind(use_wrap_power, fs_tiles, _1, _2, Powers::Power::ne_sw),
		  boost::bind(test_wrap_power, fs_tiles, _1, _2, Powers::Power::ne_sw),
		  wrap_prob/3,
		  Powers::Power::ne_sw);

	def_upgrade_power("Hover", PWR_CLIMB, 30);
	def_upgrade_power("Shield", PWR_SHIELD, 30);
	def_upgrade_power("Invisibility", PWR_INVISIBLE, 40);
	def_upgrade_power("Infravision", PWR_INFRAVISION, 40);
	def_upgrade_power("Jump", PWR_JUMP, 50);

	def_power("Raise Tile", &raise_tile, can_raise_tile, 50, Powers::Power::undirected);
	def_power("Lower Tile", &lower_tile, can_lower_tile, 50, Powers::Power::undirected);
	def_power("Increase Range", &increase_range, can_increase_range, 20, Powers::Power::undirected);
	def_power("Teleport", &teleport, can_teleport, 60, Powers::Power::undirected);
	def_power("Landing Pad", &landing_pad, can_landing_pad, 60, Powers::Power::undirected);
	def_power("Black Hole", &black_hole, can_black_hole, 15, Powers::Power::undirected);
	def_power("Worm", &use_worm, can_worm, 40, Powers::Power::undirected);

	int prod_prob = 100;
	for(int i = 0; i < 6; ++i) {
		def_power("Prod",
			  boost::bind(use_prod, i, _1, _2),
			  boost::bind(can_prod, i, _1, _2),
			  prod_prob/6,
			  Powers::Power::Directionality(Powers::Power::northeast + i));
	}

	def_power("Watchful Eye", &use_eye, can_eye, 20, Powers::Power::undirected, Powers::REQ_FOG_OF_WAR);
	def_power("Scramble Powers", &use_scramble, can_scramble, 40, Powers::Power::undirected);
}
