#ifndef OR_SCENARIO_HPP
#define OR_SCENARIO_HPP

#include "octradius.hpp"

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
	
	Scenario(const Scenario &s) {
		CopyTiles(tiles, s.tiles);
	}
};

#endif /* !OR_SCENARIO_HPP */
