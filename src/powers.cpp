#include "powers.hpp"
#include "octradius.hpp"
#include "tile_anims.hpp"
#include "client.hpp"
#include "network.hpp"
#include "gamestate.hpp"

#undef ABSOLUTE
#undef RELATIVE

Powers::Power Powers::powers[] = {
	{"Destroy", &Powers::destroy_row, 50, true, Powers::Power::row},
	{"Destroy", &Powers::destroy_radial, 50, true, Powers::Power::radial},
	{"Destroy", &Powers::destroy_bs, 50, true, Powers::Power::nw_se},
	{"Destroy", &Powers::destroy_fs, 50, true, Powers::Power::ne_sw},

	{"Raise Tile", &Powers::raise_tile, 100, true, Powers::Power::undirected},
	{"Lower Tile", &Powers::lower_tile, 100, true, Powers::Power::undirected},
	{"Increase Range", &Powers::increase_range, 20, true, Powers::Power::undirected},
	{"Hover", &Powers::hover, 30, true, Powers::Power::undirected},
	{"Shield", &Powers::shield, 30, true, Powers::Power::undirected},
	{"Invisibility", &Powers::invisibility, 30, true, Powers::Power::undirected},
	{"Teleport", &Powers::teleport, 60, true, Powers::Power::undirected},

	{"Elevate", &Powers::elevate_row, 70, true, Powers::Power::row},
	{"Elevate", &Powers::elevate_radial, 70, true, Powers::Power::radial},
	{"Elevate", &Powers::elevate_bs, 70, true, Powers::Power::nw_se},
	{"Elevate", &Powers::elevate_fs, 70, true, Powers::Power::ne_sw},

	{"Dig", &Powers::dig_row, 70, true, Powers::Power::row},
	{"Dig", &Powers::dig_radial, 70, true, Powers::Power::radial},
	{"Dig", &Powers::dig_bs, 70, true, Powers::Power::nw_se},
	{"Dig", &Powers::dig_fs, 70, true, Powers::Power::ne_sw},

	{"Purify", &Powers::purify_row, 50, true, Powers::Power::row},
	{"Purify", &Powers::purify_radial, 50, true, Powers::Power::radial},
	{"Purify", &Powers::purify_bs, 50, true, Powers::Power::nw_se},
	{"Purify", &Powers::purify_fs, 50, true, Powers::Power::ne_sw},

	{"Annihilate", &Powers::annihilate_row, 50, false, Powers::Power::row},
	{"Annihilate", &Powers::annihilate_radial, 50, false, Powers::Power::radial},
	{"Annihilate", &Powers::annihilate_bs, 50, false, Powers::Power::nw_se},
	{"Annihilate", &Powers::annihilate_fs, 50, false, Powers::Power::ne_sw}
};

const int Powers::num_powers = sizeof(Powers::powers) / sizeof(Powers::Power);

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

namespace Powers {
	static bool destroy_enemies(Tile::List area, pawn_ptr pawn) {
		Tile::List::iterator i = area.begin();
		bool ret = false;

		while(i != area.end()) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
				(*i)->pawn->destroy(Pawn::PWR_DESTROY);
				ret = true;
			}

			i++;
		}

		return ret;
	}

	bool destroy_row(pawn_ptr pawn, Server *, Client *) {
		return destroy_enemies(pawn->RowTiles(), pawn);
	}

	bool destroy_radial(pawn_ptr pawn, Server *, Client *) {
		return destroy_enemies(pawn->RadialTiles(), pawn);
	}

	bool destroy_bs(pawn_ptr pawn, Server *, Client *) {
		return destroy_enemies(pawn->bs_tiles(), pawn);
	}

	bool destroy_fs(pawn_ptr pawn, Server *, Client *) {
		return destroy_enemies(pawn->fs_tiles(), pawn);
	}

	bool raise_tile(pawn_ptr pawn, Server *, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(
				client, Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, +1);
		return pawn->cur_tile->SetHeight(pawn->cur_tile->height + 1);
	}

	bool lower_tile(pawn_ptr pawn, Server *, Client *client) {
		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(
				client, Tile::List(1, pawn->cur_tile), pawn->cur_tile, 3.0, TileAnimators::RELATIVE, -1);
		return pawn->cur_tile->SetHeight(pawn->cur_tile->height - 1);
	}

	bool increase_range(pawn_ptr pawn, Server *, Client *) {
		if(pawn->range < 3) {
			pawn->range++;
			return true;
		}

		return false;
	}

	bool hover(pawn_ptr pawn, Server *, Client *) {
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

	bool elevate_row(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->RowTiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

		return elevate_tiles(tiles);
	}

	bool elevate_radial(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->RadialTiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

		return elevate_tiles(tiles);
	}

	bool elevate_bs(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->bs_tiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

		return elevate_tiles(tiles);
	}

	bool elevate_fs(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->fs_tiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, 2);

		return elevate_tiles(tiles);
	}

	bool dig_row(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->RowTiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

		return dig_tiles(tiles);
	}

	bool dig_radial(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->RadialTiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

		return dig_tiles(tiles);
	}

	bool dig_bs(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->bs_tiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

		return dig_tiles(tiles);
	}

	bool dig_fs(pawn_ptr pawn, Server *, Client *client) {
		Tile::List tiles = pawn->fs_tiles();

		if (client && !client->current_animator)
			client->current_animator = new TileAnimators::ElevationAnimator(client, tiles, pawn->cur_tile, 3.0, TileAnimators::ABSOLUTE, -2);

		return dig_tiles(tiles);
	}

	bool shield(pawn_ptr pawn, Server *, Client *) {
		if(pawn->flags & PWR_SHIELD) {
			return false;
		}

		pawn->flags |= PWR_SHIELD;
		return true;
	}

	bool invisibility(pawn_ptr pawn, Server *, Client *) {
		if (pawn->flags & PWR_INVISIBLE) {
			return false;
		}

		pawn->flags |= PWR_INVISIBLE;
		return true;
	}

	static bool purify(Tile::List tiles, pawn_ptr pawn) {
		Tile::List::iterator i = tiles.begin();
		bool ret = false;

		for(; i != tiles.end(); i++) {
			if((*i)->pawn && (*i)->pawn->colour != pawn->colour && ((*i)->pawn->flags & PWR_GOOD || (*i)->pawn->range > 0)) {
				(*i)->pawn->flags &= ~PWR_GOOD;
				(*i)->pawn->range = 0;

				ret = true;
			}
		}

		return ret;
	}

	bool purify_row(pawn_ptr pawn, Server *, Client *) {
		return purify(pawn->RowTiles(), pawn);
	}

	bool purify_radial(pawn_ptr pawn, Server *, Client *) {
		return purify(pawn->RadialTiles(), pawn);
	}

	bool purify_bs(pawn_ptr pawn, Server *, Client *) {
		return purify(pawn->bs_tiles(), pawn);
	}

	bool purify_fs(pawn_ptr pawn, Server *, Client *) {
		return purify(pawn->fs_tiles(), pawn);
	}

	bool teleport(pawn_ptr pawn, Server *server, Client *client) {
		if(server) {
			Tile::List tlist;
			Tile *tile;

			do {
				tlist = RandomTiles(server->game_state->tiles, 1, false);
				tile = *(tlist.begin());
			} while(tile->pawn);

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

	static bool annihilate(Tile::List tiles) {
		bool ret = false;

		for(Tile::List::iterator tile = tiles.begin(); tile != tiles.end(); tile++) {
			if((*tile)->pawn) {
				(*tile)->pawn->destroy(Pawn::PWR_ANNIHILATE);
				ret = true;
			}
		}

		return ret;
	}

	bool annihilate_row(pawn_ptr pawn, Server *, Client *) {
		return annihilate(pawn->RowTiles());
	}

	bool annihilate_radial(pawn_ptr pawn, Server *, Client *) {
		return annihilate(pawn->RadialTiles());
	}

	bool annihilate_bs(pawn_ptr pawn, Server *, Client *) {
		return annihilate(pawn->bs_tiles());
	}

	bool annihilate_fs(pawn_ptr pawn, Server *, Client *) {
		return annihilate(pawn->fs_tiles());
	}
}
