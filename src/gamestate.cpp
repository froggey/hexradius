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

void GameState::add_animator(TileAnimators::Animator *ani) {
	delete ani;
}
void GameState::add_animator(Animators::Generic *ani) {
	delete ani;
}
bool GameState::teleport_hack(pawn_ptr) {
	return true;
}
bool GameState::grant_upgrade(pawn_ptr, uint32_t) {
	return true;
}
void GameState::add_power_notification(pawn_ptr, int) {
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

bool ServerGameState::teleport_hack(pawn_ptr pawn)
{
	Tile::List targets = RandomTiles(tiles, 1, false, false, false, false);
	if(targets.empty()) {
		return false;
	}
	Tile *target = *targets.begin();

	// Play the teleport animation, then move the pawn.
	// Small problem: Clients get sent a bad USE message after the power gets used.
	// the pawn is no longer at the correct location after that. (wait, what?)
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

	{
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
	pawn->force_move(target, server.game_state);

	if(hp) {
		server.update_one_pawn(pawn);
	}

	return true;
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

bool ServerGameState::grant_upgrade(pawn_ptr pawn, uint32_t upgrade) {
	if(pawn->flags & upgrade) {
		return false;
	}
	pawn->flags |= upgrade;
	server.update_one_pawn(pawn);
	return false;
}

ClientGameState::ClientGameState(Client &client) : client(client) {}
