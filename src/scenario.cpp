#include <iostream>
#include <fstream>
#include <stdexcept>

#include "scenario.hpp"
#include "powers.hpp"
#include "gamestate.hpp"

static char *next_value(char *str) {
	char *r = str+strcspn(str, "\t ");

	if(r[0]) {
		r[0] = '\0';
		r += strspn(r+1, "\t ")+1;
	}

	return r;
}

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

	std::fstream file(filename.c_str(), std::fstream::in);
	if(!file.is_open()) {
		throw std::runtime_error("Unable to open scenario file");
	}

	char buf[1024], *bp;
	unsigned int lnum = 0;

	while(file.good()) {
		file.getline(buf, sizeof(buf));
		buf[strcspn(buf, "\n")] = '\0';
		lnum++;

		bp = next_value(buf);
		std::string name = buf;

		if(bp[0] == '#' || bp[0] == '\0') {
			continue;
		}

		if(name == "GRID") {
			int cols = atoi(bp);
			int rows = atoi(next_value(bp));

			if(cols <= 0 || rows <= 0) {
				LINE_ERR("Grid size must be greater than 0x0");
			}

			for(int r = 0; r < rows; r++) {
				for(int c = 0; c < cols; c++) {
					game_state->tiles.push_back(new Tile(c, r, 0));
				}
			}
		}else if(name == "SPAWN") {
			/* SPAWN x y c */

			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int c = atoi((bp = next_value(bp)));

			Tile *tile = game_state->tile_at(x, y);

			if(!tile) {
				LINE_ERR("Tile not found (" + to_string(x) + "," + to_string(y) + ")");
			}
			if(tile->pawn) {
				LINE_ERR("Cannot spawn multiple pawns on the same tile");
			}
			if(c < BLUE || c > ORANGE) {
				LINE_ERR("Invalid pawn colour");
			}

			tile->pawn = pawn_ptr(new Pawn((PlayerColour)c, game_state->tiles, tile));
			colours.insert((PlayerColour)c);
		}else if(name == "HOLE") {
			int x = atoi(bp);
			int y = atoi(next_value(bp));

			Tile::List::iterator i = game_state->tiles.begin();

			while(i != game_state->tiles.end()) {
				if((*i)->col == x && (*i)->row == y) {
					delete *i;
					game_state->tiles.erase(i);

					break;
				}

				i++;
			}
		}else if(name == "POWER") {
			int i = atoi(bp);
			int p = atoi(next_value(bp));

			if(i < 0 || i >= Powers::num_powers) {
				LINE_ERR("Unknown power (" + to_string(i) + ")");
			}

			Powers::powers[i].spawn_rate = p;
		}else if(name == "HEIGHT") {
			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int h = atoi((bp = next_value(bp)));

			Tile *tile = game_state->tile_at(x, y);

			if(!tile) {
				LINE_ERR("Tile not found (" + to_string(x) + "," + to_string(y) + ")");
			}
			if(h > 2 || h < -2) {
				LINE_ERR("Tile height must be in the range -2 ... 2");
			}

			tile->height = h;
		}else{
			LINE_ERR("Unknown directive '" + name + "'");
		}
	}

	if(game_state->tiles.empty()) {
		throw std::runtime_error(filename + ": No GRID directive used");
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

		tile->pawn = pawn_ptr(new Pawn(c, game_state->tiles, tile));
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
