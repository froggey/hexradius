#include "powers.hpp"
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "client.hpp"
#include "network.hpp"
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

static bool destroy_enemies(Tile::List area, pawn_ptr pawn, Client *client) {
	Tile::List::iterator i = area.begin();
	bool ret = false;

	while(i != area.end()) {
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
			(*i)->pawn->destroy(Pawn::PWR_DESTROY);
			if(client) {
				client->add_animator(new Animators::PawnPow((*i)->screen_x, (*i)->screen_y));
			}
			ret = true;
		}

		i++;
	}

	return ret;
}

static bool destroy_row(pawn_ptr pawn, Server *, Client *client) {
	return destroy_enemies(pawn->RowTiles(), pawn, client);
}

static bool destroy_radial(pawn_ptr pawn, Server *, Client *client) {
	return destroy_enemies(pawn->RadialTiles(), pawn, client);
}

static bool destroy_bs(pawn_ptr pawn, Server *, Client *client) {
	return destroy_enemies(pawn->bs_tiles(), pawn, client);
}

static bool destroy_fs(pawn_ptr pawn, Server *, Client *client) {
	return destroy_enemies(pawn->fs_tiles(), pawn, client);
}

static bool raise_tile(pawn_ptr pawn, Server *, Client *client) {
	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(
			client, Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, +1);
	return pawn->cur_tile->SetHeight(pawn->cur_tile->height + 1);
}

static bool lower_tile(pawn_ptr pawn, Server *, Client *client) {
	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(
			client, Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, -1);
	return pawn->cur_tile->SetHeight(pawn->cur_tile->height - 1);
}

static bool increase_range(pawn_ptr pawn, Server *, Client *) {
	if(pawn->range < 3) {
		pawn->range++;
		return true;
	}

	return false;
}

static bool hover(pawn_ptr pawn, Server *, Client *) {
	if(pawn->flags & PWR_CLIMB) {
		return false;
	}

	pawn->flags |= PWR_CLIMB;
	return true;
}

static bool elevate_tiles(Tile::List &tiles) {
	Tile::List::iterator i = tiles.begin();
	bool ret = false;

	for(; i != tiles.end(); i++) {
		ret = ((*i)->SetHeight(2) || ret);
	}

	return ret;
}

static bool dig_tiles(Tile::List &tiles) {
	Tile::List::iterator i = tiles.begin();
	bool ret = false;

	for(; i != tiles.end(); i++) {
		ret = ((*i)->SetHeight(-2) || ret);
	}

	return ret;
}

static bool elevate_row(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->RowTiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

	return elevate_tiles(tiles);
}

static bool elevate_radial(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->RadialTiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

	return elevate_tiles(tiles);
}

static bool elevate_bs(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->bs_tiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

	return elevate_tiles(tiles);
}

static bool elevate_fs(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->fs_tiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

	return elevate_tiles(tiles);
}

static bool dig_row(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->RowTiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

	return dig_tiles(tiles);
}

static bool dig_radial(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->RadialTiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

	return dig_tiles(tiles);
}

static bool dig_bs(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->bs_tiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

	return dig_tiles(tiles);
}

static bool dig_fs(pawn_ptr pawn, Server *, Client *client) {
	Tile::List tiles = pawn->fs_tiles();

	if (client && !client->current_animator)
		client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

	return dig_tiles(tiles);
}

static bool shield(pawn_ptr pawn, Server *, Client *) {
	if(pawn->flags & PWR_SHIELD) {
		return false;
	}

	pawn->flags |= PWR_SHIELD;
	return true;
}

static bool invisibility(pawn_ptr pawn, Server *, Client *) {
	if (pawn->flags & PWR_INVISIBLE) {
		return false;
	}

	pawn->flags |= PWR_INVISIBLE;
	return true;
}

static bool purify(Tile::List tiles, pawn_ptr pawn, Client *client) {
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
			(*i)->pawn->maybe_step_on_mine(client);
			// And falling onto a smashed tile is bad.
			if((*i)->smashed) {
				if(client) {
					client->add_animator(new Animators::PawnOhShitIFellDownAHole((*i)->screen_x, (*i)->screen_y));
				}
				(*i)->pawn->destroy(Pawn::FELL_OUT_OF_THE_WORLD);
			}
			ret = true;
		}
	}

	return ret;
}

static bool purify_row(pawn_ptr pawn, Server *, Client *client) {
	return purify(pawn->RowTiles(), pawn, client);
}

static bool purify_radial(pawn_ptr pawn, Server *, Client *client) {
	return purify(pawn->RadialTiles(), pawn, client);
}

static bool purify_bs(pawn_ptr pawn, Server *, Client *client) {
	return purify(pawn->bs_tiles(), pawn, client);
}

static bool purify_fs(pawn_ptr pawn, Server *, Client *client) {
	return purify(pawn->fs_tiles(), pawn, client);
}

static bool teleport(pawn_ptr pawn, Server *server, Client *client) {
	if(server) {
		Tile *tile;

		tile = *(RandomTiles(server->game_state->tiles, 1, false, false, false, false).begin());

		server->game_state->power_rand_vals.push_back(tile->col);
		server->game_state->power_rand_vals.push_back(tile->row);

		tile->pawn.swap(pawn->cur_tile->pawn);
		pawn->cur_tile = tile;
	}else{
		if(client->game_state->power_rand_vals.size() != 2) {
			return false;
		}

		int col = client->game_state->power_rand_vals[0];
		int row = client->game_state->power_rand_vals[1];

		Tile *tile = client->game_state->tile_at(col, row);
		if(!tile || tile->pawn) {
			std::cerr << "Invalid teleport attempted, out of sync?" << std::endl;
			return false;
		}

		pawn->last_tile = pawn->cur_tile;
		pawn->last_tile->render_pawn = pawn;
		pawn->teleport_time = SDL_GetTicks();

		tile->pawn.swap(pawn->cur_tile->pawn);
		pawn->cur_tile = tile;
	}

	return true;
}

static bool annihilate(Tile::List tiles, Client *client) {
	bool ret = false;

	for(Tile::List::iterator tile = tiles.begin(); tile != tiles.end(); tile++) {
		if((*tile)->pawn) {
			(*tile)->pawn->destroy(Pawn::PWR_ANNIHILATE);
			if(client) {
				client->add_animator(new Animators::PawnPow((*tile)->screen_x, (*tile)->screen_y));
			}
			ret = true;
		}
	}

	return ret;
}

static bool annihilate_row(pawn_ptr pawn, Server *, Client *client) {
	return annihilate(pawn->RowTiles(), client);
}

static bool annihilate_radial(pawn_ptr pawn, Server *, Client *client) {
	return annihilate(pawn->RadialTiles(), client);
}

static bool annihilate_bs(pawn_ptr pawn, Server *, Client *client) {
	return annihilate(pawn->bs_tiles(), client);
}

static bool annihilate_fs(pawn_ptr pawn, Server *, Client *client) {
	return annihilate(pawn->fs_tiles(), client);
}

static bool smash(Tile::List tiles, pawn_ptr pawn, Client *client) {
	bool ret = false;

	for(Tile::List::iterator tile = tiles.begin(); tile != tiles.end(); tile++) {
		if((*tile)->pawn && (*tile)->pawn->colour != pawn->colour) {
			(*tile)->pawn->destroy(Pawn::PWR_SMASH);
			if(client) {
				client->add_animator(new Animators::PawnPow((*tile)->screen_x, (*tile)->screen_y));
			}
			ret = true;
			(*tile)->smashed = true;
			(*tile)->has_mine = false;
			(*tile)->has_power = false;
			(*tile)->has_landing_pad = false;
		}
	}

	return ret;
}

static bool smash_row(pawn_ptr pawn, Server *, Client *client) {
	return smash(pawn->RowTiles(), pawn, client);
}

static bool smash_radial(pawn_ptr pawn, Server *, Client *client) {
	return smash(pawn->RadialTiles(), pawn, client);
}

static bool smash_bs(pawn_ptr pawn, Server *, Client *client) {
	return smash(pawn->bs_tiles(), pawn, client);
}

static bool smash_fs(pawn_ptr pawn, Server *, Client *client) {
	return smash(pawn->fs_tiles(), pawn, client);
}


static int lay_mines(Tile::List tiles, PlayerColour colour) {
	int n_mines = 0;
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		Tile *tile = *i;
		if(tile->has_mine || tile->smashed) continue;
		tile->has_mine = true;
		tile->mine_colour = colour;
		n_mines += 1;
	}

	return n_mines;
}

static bool mine(pawn_ptr pawn, Server *, Client *) {
	return lay_mines(Tile::List(1, pawn->cur_tile), pawn->colour) == 1;
}

static bool mine_row(pawn_ptr pawn, Server *, Client *) {
	return lay_mines(pawn->RowTiles(), pawn->colour) != 0;
}

static bool mine_radial(pawn_ptr pawn, Server *, Client *) {
	return lay_mines(pawn->RadialTiles(), pawn->colour) != 0;
}

static bool mine_bs(pawn_ptr pawn, Server *, Client *) {
	return lay_mines(pawn->bs_tiles(), pawn->colour) != 0;
}

static bool mine_fs(pawn_ptr pawn, Server *, Client *) {
	return lay_mines(pawn->fs_tiles(), pawn->colour) != 0;
}

static bool landing_pad(pawn_ptr pawn, Server *, Client *) {
	if(pawn->cur_tile->smashed) return false;
	pawn->cur_tile->has_landing_pad = true;
	pawn->cur_tile->landing_pad_colour = pawn->colour;
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
	{"Invisibility", &invisibility, 30, true, Powers::Power::undirected},
	{"Teleport", &teleport, 60, true, Powers::Power::undirected},
	{"Landing Pad", &landing_pad, 60, true, Powers::Power::undirected},

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
