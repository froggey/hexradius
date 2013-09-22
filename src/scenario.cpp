#include <iostream>
#include <fstream>
#include <stdexcept>

#include "scenario.hpp"
#include "powers.hpp"
#include "gamestate.hpp"
#include "editor.hpp"

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

	FILE *fh = fopen(filename.c_str(), "rb");
	if(!fh)
	{
		throw std::runtime_error("Unable to open scenario file");
	}

	hr_map_cmd cmd;
	
	while(fread(&cmd, sizeof(cmd), 1, fh))
	{
		PlayerColour colour = (PlayerColour)(cmd.extra);
		
		Tile *tile = game_state->tile_at(cmd.x, cmd.y);
		
		if(cmd.cmd != HR_MAP_CMD_TILE && !tile)
		{
			printf("Ignoring command %d with bad position %d,%d\n", (int)(cmd.cmd), (int)(cmd.x), (int)(cmd.y));
			continue;
		}
		
		switch(cmd.cmd)
		{
			case HR_MAP_CMD_TILE:
			{
				game_state->tiles.push_back(new Tile(cmd.x, cmd.y, cmd.extra));
				break;
			}
			
			case HR_MAP_CMD_BREAK:
			{
				tile->smashed = true;
				break;
			}
			
			case HR_MAP_CMD_BHOLE:
			{
				tile->has_black_hole = true;
				break;
			}
			
			case HR_MAP_CMD_PAWN:
			{
				tile->pawn = pawn_ptr(new Pawn(colour, game_state->tiles, tile));
				colours.insert(colour);
				break;
			}
			
			case HR_MAP_CMD_PAD:
			{
				tile->has_landing_pad    = true;
				tile->landing_pad_colour = colour;
				break;
			}
			
			case HR_MAP_CMD_MINE:
			{
				tile->has_mine    = true;
				tile->mine_colour = colour;
				break;
			}
			
			default:
			{
				printf("Ignoring unknown command %d\n", (int)(cmd.cmd));
				break;
			}
		}
	}
	
	fclose(fh);
	
	if(game_state->tiles.empty())
	{
		throw std::runtime_error(filename + ": No HR_MAP_CMD_TILE command used");
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
