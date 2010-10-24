#ifndef OR_SCENARIO_HPP
#define OR_SCENARIO_HPP

#include "octradius.hpp"
#include "octradius.pb.h"

struct Scenario {
	Tile::List tiles;
	std::set<PlayerColour> colours;
	
	Scenario() {}
	
	~Scenario() {
		FreeTiles(tiles);
	}
	
	Scenario &operator=(const Scenario &s) {
		if(this != &s) {
			CopyTiles(tiles, s.tiles);
		}
		
		return *this;
	}
	
	void load_file(std::string filename);
	
	void store_proto(protocol::message &msg);
	void load_proto(const protocol::message &msg);
	
	Tile::List init_game(std::set<PlayerColour> spawn_colours);
	
	Scenario(const Scenario &s) {
		CopyTiles(tiles, s.tiles);
	}
};

#endif /* !OR_SCENARIO_HPP */
