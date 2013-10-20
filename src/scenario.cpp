#include <iostream>
#include <fstream>
#include <stdexcept>

#include "scenario.hpp"
#include "powers.hpp"
#include "gamestate.hpp"
#include "map.hpp"

using HexRadius::Position;

Scenario::Scenario(Server &server) :
	game_state(0), server(&server)
{
}
Scenario::Scenario() :
	game_state(0), server(0)
{
}

Scenario::~Scenario() {
	delete game_state;
}

#define LINE_ERR(s) \
	throw std::runtime_error(filename + ":" + to_string(lnum) + ": " + s);

void Scenario::load_file(std::string filename) {
	last_filename = filename;
	delete game_state;
	game_state = server ?
		static_cast<GameState*>(new ServerGameState(*server)) :
		new GameState;
	colours.clear();

	HexRadius::Map map;
	map.load(filename);

	for(std::map<Position,HexRadius::Map::Tile>::iterator t = map.tiles.begin(); t != map.tiles.end(); ++t)
	{
		game_state->tiles.push_back(new Tile(t->second.pos.first, t->second.pos.second, t->second.height));
		Tile *tile = game_state->tile_at(t->second.pos.first, t->second.pos.second);
		
		if(t->second.type == HexRadius::Map::Tile::BROKEN)
		{
			tile->smashed = true;
		}
		else if(t->second.type == HexRadius::Map::Tile::BHOLE)
		{
			tile->has_black_hole   = true;
			tile->black_hole_power = 1;
		}
		
		if(t->second.has_pawn)
		{
			tile->pawn = pawn_ptr(new Pawn(t->second.pawn_colour, game_state, tile));
			colours.insert(t->second.pawn_colour);
		}
		
		if(t->second.has_landing_pad)
		{
			tile->has_landing_pad    = true;
			tile->landing_pad_colour = t->second.landing_pad_colour;
		}
		
		if(t->second.has_mine)
		{
			tile->has_mine    = true;
			tile->mine_colour = t->second.mine_colour;
		}
	}
}

void Scenario::store_proto(protocol::message &msg) {
	if(!game_state) {
		if(!last_filename.empty()) {
			load_file(last_filename);
		} else {
			load_proto(saved_msg);
		}
	}
	assert(game_state);
	for(Tile::List::iterator t = game_state->tiles.begin(); t != game_state->tiles.end(); t++) {
		msg.add_tiles();
		(*t)->CopyToProto(msg.mutable_tiles(msg.tiles_size()-1));

		if((*t)->pawn) {
			msg.add_pawns();
			(*t)->pawn->CopyToProto(msg.mutable_pawns(msg.pawns_size()-1), false);
		}
	}
}

void Scenario::load_proto(const protocol::message &msg) {
	saved_msg = msg;
	last_filename = std::string();
	delete game_state;
	game_state = server ?
		static_cast<GameState*>(new ServerGameState(*server)) :
		new GameState;
	colours.clear();

	for(int i = 0; i < msg.tiles_size(); i++) {
		game_state->tiles.push_back(new Tile(msg.tiles(i).col(), msg.tiles(i).row(), msg.tiles(i).height()));
		
		Tile *tile = game_state->tile_at(msg.tiles(i).col(), msg.tiles(i).row());
		
		tile->smashed = msg.tiles(i).smashed();
		
		tile->has_mine    = msg.tiles(i).has_mine();
		tile->mine_colour = (PlayerColour)(msg.tiles(i).mine_colour());
		
		tile->has_landing_pad    = msg.tiles(i).has_landing_pad();
		tile->landing_pad_colour = (PlayerColour)(msg.tiles(i).landing_pad_colour());
		
		tile->has_black_hole   = msg.tiles(i).has_black_hole();
		tile->black_hole_power = msg.tiles(i).black_hole_power();
	}

	for(int i = 0; i < msg.pawns_size(); i++) {
		PlayerColour c = (PlayerColour)msg.pawns(i).colour();

		if(c < BLUE || c > ORANGE) {
			std::cerr << "Recieved pawn with invalid colour, ignoring" << std::endl;
			continue;
		}

		Tile *tile = game_state->tile_at(msg.pawns(i).col(), msg.pawns(i).row());

		if(!tile) {
			std::cerr << "Recieved pawn with invalid location, ignoring" << std::endl;
			continue;
		}

		if(tile->pawn) {
			std::cerr << "Recieved multiple pawns on a tile, ignoring" << std::endl;
			continue;
		}

		tile->pawn = pawn_ptr(new Pawn(c, game_state, tile));
		colours.insert(c);
	}
}

GameState *Scenario::init_game(std::set<PlayerColour> spawn_colours) {
	if(!game_state) {
		if(!last_filename.empty()) {
			load_file(last_filename);
		} else {
			load_proto(saved_msg);
		}
	}
	assert(game_state);
	GameState *g = game_state;
	game_state = 0;

	for(Tile::List::iterator t = g->tiles.begin(); t != g->tiles.end(); t++) {
		if((*t)->pawn && spawn_colours.find((*t)->pawn->colour) == spawn_colours.end()) {
			(*t)->pawn.reset();
		}
	}

	return g;
}
