#include "gamestate.hpp"
#include "loadimage.hpp"
#include "network.hpp"
#include "client.hpp"
#include "animator.hpp"
#include "tile_anims.hpp"
#include <stdexcept>

GameState::GameState() {
}

GameState::~GameState() {
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		delete *i;
	}
}

std::vector<pawn_ptr> GameState::all_pawns() {
	std::vector<pawn_ptr> pawns;

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		if((*i)->pawn) {
			pawns.push_back((*i)->pawn);
		}
	}

	return pawns;
}

/** Return all the pawns belonging to a given player. */
std::vector<pawn_ptr> GameState::player_pawns(PlayerColour colour) {
	std::vector<pawn_ptr> pawns;

	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		if((*i)->pawn && (*i)->pawn->colour == colour) {
			pawns.push_back((*i)->pawn);
		}
	}

	return pawns;
}

Tile *GameState::tile_at(int column, int row) {
	for(Tile::List::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		if((*i)->col == column && (*i)->row == row) {
			return *i;
		}
	}

	return 0;
}

Tile *GameState::tile_left_of_coords(int column, int row) { return tile_at(column - 1, row); }
Tile *GameState::tile_right_of_coords(int column, int row) { return tile_at(column + 1, row); }
Tile *GameState::tile_ne_of_coords(int column, int row) { return tile_at(column + (row % 2), row - 1); }
Tile *GameState::tile_nw_of_coords(int column, int row) { return tile_at(column - !(row % 2), row - 1); }
Tile *GameState::tile_se_of_coords(int column, int row) { return tile_at(column + (row % 2), row + 1); }
Tile *GameState::tile_sw_of_coords(int column, int row) { return tile_at(column - !(row % 2), row + 1); }
Tile *GameState::tile_left_of(Tile *t) { return tile_left_of_coords(t->col, t->row); }
Tile *GameState::tile_right_of(Tile *t) { return tile_right_of_coords(t->col, t->row); }
Tile *GameState::tile_ne_of(Tile *t) { return tile_ne_of_coords(t->col, t->row); }
Tile *GameState::tile_nw_of(Tile *t) { return tile_nw_of_coords(t->col, t->row); }
Tile *GameState::tile_se_of(Tile *t) { return tile_se_of_coords(t->col, t->row); }
Tile *GameState::tile_sw_of(Tile *t) { return tile_sw_of_coords(t->col, t->row); }

Tile::List GameState::row_tiles(Tile *t, int range) {
	Tile::List tiles;
	Tile::List::iterator i = this->tiles.begin();

	int min = t->row-range;
	int max = t->row+range;

	for(; i != this->tiles.end(); i++) {
		if((*i)->row >= min && (*i)->row <= max) {
			tiles.push_back(*i);
		}
	}

	return tiles;
}

Tile::List GameState::radial_tiles(Tile *t, int range)
{
	/* Build a set of coordinates that fall within the pawn's radial power
	 * area by initialising the set with the current tile and then
	 * calculating the surrounding coordinates of each pair repeatedly
	 * until the pawn's range has been satisfied.
	*/

	std::set< std::pair<int,int> > coords;

	coords.insert(std::make_pair(t->col, t->row));

	for(int i = 0; i <= range; ++i)
	{
		std::set< std::pair<int,int> > new_coords = coords;

		for(std::set< std::pair<int,int> >::iterator ci = coords.begin(); ci != coords.end(); ++ci)
		{
			int c_min   = ci->first  - 1 + (ci->second % 2) * 1;
			int c_max   = ci->first  + (ci->second % 2) * 1;
			int c_extra = ci->first  + (ci->second % 2 ? -1 : 1);
			int r_min   = ci->second - 1;
			int r_max   = ci->second + 1;

			for(int c = c_min; c <= c_max; ++c)
			{
				for(int r = r_min; r <= r_max; ++r)
				{
					new_coords.insert(std::make_pair(c, r));
				}
			}

			new_coords.insert(std::make_pair(c_extra, ci->second));
		}

		coords.insert(new_coords.begin(), new_coords.end());
	}

	/* Build a list of any tiles that fall within the set of coordinates to
	 * be returned.
	*/

	Tile::List ret;

	for(Tile::List::iterator ti = this->tiles.begin(); ti != this->tiles.end(); ++ti)
	{
		if(coords.find(std::make_pair((*ti)->col, (*ti)->row)) != coords.end())
		{
			ret.push_back(*ti);
		}
	}

	return ret;
}

Tile::List GameState::bs_tiles(Tile *t, int range) {
	Tile::List tiles;

	int base = t->col;

	for(int r = t->row; r > 0; r--) {
		if(r % 2 == 0) {
			base--;
		}
	}

	for(Tile::List::iterator tile = this->tiles.begin(); tile != this->tiles.end(); tile++) {
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

Tile::List GameState::fs_tiles(Tile *t, int range) {
	Tile::List tiles;

	int base = t->col;

	for(int r = t->row; r > 0; r--) {
		if(r % 2) {
			base++;
		}
	}

	for(Tile::List::iterator tile = this->tiles.begin(); tile != this->tiles.end(); tile++) {
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

Tile::List GameState::linear_tiles(Tile *t, int range) {
	Tile::List tiles, result;
	tiles = row_tiles(t, range);
	result.insert(result.end(), tiles.begin(), tiles.end());
	tiles = bs_tiles(t, range);
	result.insert(result.end(), tiles.begin(), tiles.end());
	tiles = fs_tiles(t, range);
	result.insert(result.end(), tiles.begin(), tiles.end());
	return result;
}

pawn_ptr GameState::pawn_at(int column, int row)
{
	Tile *tile = tile_at(column, row);
	return tile ? tile->pawn : pawn_ptr();
}

Tile *GameState::tile_at_screen(int x, int y) {
	Tile::List::iterator ti = tiles.end();

	SDL_Surface *tile = ImgStuff::GetImage("graphics/hextile.png");
	ensure_SDL_LockSurface(tile);

	do {
		ti--;

		int tx = (*ti)->screen_x;
		int ty = (*ti)->screen_y;

		if(tx <= x && tx+(int)TILE_WIDTH > x && ty <= y && ty+(int)TILE_HEIGHT > y) {
			Uint8 alpha, blah;
			Uint32 pixel = ImgStuff::GetPixel(tile, x-(*ti)->screen_x, y-(*ti)->screen_y);

			SDL_GetRGBA(pixel, tile->format, &blah, &blah, &blah, &alpha);

			if(alpha) {
				SDL_UnlockSurface(tile);
				return *ti;
			}
		}
	} while(ti != tiles.begin());

	SDL_UnlockSurface(tile);

	return 0;
}

pawn_ptr GameState::pawn_at_screen(int x, int y) {
	Tile *tile = tile_at_screen(x, y);
	return tile ? tile->pawn : pawn_ptr();
}

void GameState::destroy_team_pawns(PlayerColour colour) {
	for(Tile::List::iterator t = tiles.begin(); t != tiles.end(); t++) {
		if((*t)->pawn && (*t)->pawn->colour == colour) {
			(*t)->pawn.reset();
		}
	}
}

ServerGameState::ServerGameState(Server &server) : server(server) {}

void ServerGameState::add_animator(TileAnimators::Animator *ani) {
	protocol::message msg = ani->serialize();
	delete ani;
	server.WriteAll(msg);
}

void ServerGameState::add_animator(Animators::Generic *ani) {
	protocol::message msg = ani->serialize();
	delete ani;
	server.WriteAll(msg);
}

void ServerGameState::teleport_hack(pawn_ptr pawn)
{
	Tile::List targets = RandomTiles(tiles, 1, false, false, false, false);
	assert(!targets.empty());
	Tile *target = *targets.begin();

	// Play the teleport animation, then move the pawn.
	{
		protocol::message msg;
		msg.set_msg(protocol::PAWN_ANIMATION);
		msg.add_pawns();
		msg.mutable_pawns(0)->set_col(pawn->cur_tile->col);
		msg.mutable_pawns(0)->set_row(pawn->cur_tile->row);
		msg.mutable_pawns(0)->set_new_col(target->col);
		msg.mutable_pawns(0)->set_new_row(target->row);
		msg.set_animation_name("teleport");
		server.WriteAll(msg);
	}

	move_pawn_to(pawn, target);
}

void ServerGameState::add_power_notification(pawn_ptr pawn, int power) {
	for(Server::client_set::iterator i = server.clients.begin(); i != server.clients.end(); i++) {
		Server::Client::ptr client = *i;
		if(client->colour == NOINIT) continue;
		protocol::message msg;
		msg.set_msg(protocol::ADD_POWER_NOTIFICATION);
		msg.add_pawns();
		msg.mutable_pawns(0)->set_col(pawn->cur_tile->col);
		msg.mutable_pawns(0)->set_row(pawn->cur_tile->row);
		if(client->colour == SPECTATE || client->colour == pawn->colour) {
			msg.mutable_pawns(0)->set_use_power(power);
		}
		client->Write(msg);
	}
}

void ServerGameState::use_power_notification(pawn_ptr pawn, int power) {
	for(Server::client_set::iterator i = server.clients.begin(); i != server.clients.end(); i++) {
		Server::Client::ptr client = *i;
		if(client->colour == NOINIT) continue;
		protocol::message msg;
		msg.set_msg(protocol::USE_POWER_NOTIFICATION);
		msg.add_pawns();
		msg.mutable_pawns(0)->set_col(pawn->cur_tile->col);
		msg.mutable_pawns(0)->set_row(pawn->cur_tile->row);
		msg.mutable_pawns(0)->set_use_power(power);
		client->Write(msg);
	}
}

void ServerGameState::grant_upgrade(pawn_ptr pawn, uint32_t upgrade) {
	assert((pawn->flags & upgrade) == 0);
	pawn->flags |= upgrade;
	server.update_one_pawn(pawn);
}

void ServerGameState::set_tile_height(Tile *tile, int height) {
	tile->SetHeight(height);
	server.update_one_tile(tile);
}

void ServerGameState::destroy_pawn(pawn_ptr target, Pawn::destroy_type reason, pawn_ptr)
{
	protocol::message msg;
	msg.set_msg(protocol::DESTROY);
	msg.add_pawns();
	msg.mutable_pawns(0)->set_col(target->cur_tile->col);
	msg.mutable_pawns(0)->set_row(target->cur_tile->row);
	server.WriteAll(msg);
	target->destroy(reason);
}

void ServerGameState::update_pawn(pawn_ptr pawn)
{
	server.update_one_pawn(pawn);
}

void ServerGameState::update_tile(Tile *tile)
{
	server.update_one_tile(tile);
}

void ServerGameState::move_pawn_to(pawn_ptr pawn, Tile *target)
{
	// Do smashy smashy before moving the pawn.
	// move_pawn_to is also called to recheck tile effects when a pawn's upgrade state changes.
	if(pawn->cur_tile != target) {
		if(target->pawn) {
			add_animator(new Animators::PawnCrush(target->screen_x, target->screen_y));
			destroy_pawn(target->pawn, Pawn::STOMP, pawn);
		}

		// Notify clients of the move.
		protocol::message msg;
		msg.set_msg(protocol::FORCE_MOVE);
		msg.add_pawns();
		msg.mutable_pawns(0)->set_col(pawn->cur_tile->col);
		msg.mutable_pawns(0)->set_row(pawn->cur_tile->row);
		msg.mutable_pawns(0)->set_new_col(target->col);
		msg.mutable_pawns(0)->set_new_row(target->row);
		server.WriteAll(msg);
	}

	bool hp = target->has_power;

	// Move the pawn on our end. This also performs various
	// tile-related effects.
	pawn->force_move(target, this);

	if(hp) {
		server.update_one_tile(target);
		if(!pawn->destroyed()) {
			server.update_one_pawn(pawn);
		}
	}
}

void ServerGameState::run_worm_stuff(pawn_ptr pawn, int range)
{
	server.worm_pawn = pawn;
	server.worm_tile = pawn->cur_tile;
	server.worm_range = range;

	server.doing_worm_stuff = true;
	server.worm_timer.expires_from_now(boost::posix_time::milliseconds(0));
	server.worm_timer.async_wait(boost::bind(&Server::worm_tick, &server, boost::asio::placeholders::error));
}
