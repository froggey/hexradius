#include <iostream>
#include <fstream>
#include <stdexcept>

#include "scenario.hpp"
#include "powers.hpp"
#include "gamestate.hpp"
#include "map.hpp"

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

void Scenario::load_file(const std::string &filename) {
	delete game_state;
	game_state = server ?
		static_cast<GameState*>(new ServerGameState(*server)) :
		new GameState;
	game_state->load_file(filename);
	last_filename = filename;
}

void Scenario::save_file(const std::string &filename)
{
	assert(game_state);
	game_state->save_file(filename);
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
	game_state->serialize(msg);
}

void Scenario::load_proto(const protocol::message &msg) {
	saved_msg = msg;
	last_filename = std::string();
	delete game_state;
	game_state = server ?
		static_cast<GameState*>(new ServerGameState(*server)) :
		new GameState;

	game_state->deserialize(msg);
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
