#include <algorithm>
#include <stdexcept>

#include "map.hpp"
#include "pawn.hpp"
#include "scenario.hpp"
#include "gamestate.hpp"

unsigned int Map::width() const
{
	unsigned int w = 0;
	
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t)
	{
		w = std::max(w, (unsigned int)(t->second.col + 1));
	}
	
	return w;
}

unsigned int Map::height() const
{
	unsigned int h = 0;
	
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t)
	{
		h = std::max(h, (unsigned int)(t->second.row + 1));
	}
	
	return h;
}

Tile *Map::get_tile(const Position &pos)
{
	std::map<Position,Tile>::iterator t = tiles.find(pos);
	
	return t != tiles.end() ? &(t->second) : NULL;
}

Tile *Map::touch_tile(const Position &pos)
{
	std::map<Position,Tile>::iterator t = tiles.insert(std::make_pair(pos, Tile(pos.first, pos.second, 0))).first;
	
	return &(t->second);
}

void Map::load(const std::string &filename)
{
	Scenario scn;
	scn.load_file(filename);

	std::map<Position,Tile> new_tiles;

	for(Tile::List::iterator i = scn.game_state->tiles.begin(); i != scn.game_state->tiles.end(); ++i) {
		std::map<Position,Tile>::iterator t = new_tiles.insert(std::make_pair(Position((*i)->col, (*i)->row), Tile((*i)->col, (*i)->row, 0))).first;
		t->second = **i;
		if(t->second.pawn) {
			t->second.pawn = pawn_ptr(new Pawn(t->second.pawn->colour, NULL, &t->second));
		}
	}

	if(new_tiles.empty())
	{
		throw std::runtime_error(filename + ": No tiles found in map");
	}

	tiles = new_tiles;

	// Fix pawn cur_tile pointers.
	for(std::map<Position,Tile>::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		if(i->second.pawn) {
			i->second.pawn->cur_tile = &i->second;
		}
	}
}

void Map::save(const std::string &filename) const
{
	Scenario scn;
	scn.game_state = new GameState;

	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t) {
		const Tile *tile = &t->second;
		scn.game_state->tiles.push_back(new Tile(*tile));
	}

	scn.save_file(filename);
}
