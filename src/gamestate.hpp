#ifndef OR_GAMESTATE_HPP
#define OR_GAMESTATE_HPP

#include <vector>
#include "octradius.hpp"

class GameState {
public:
	~GameState();
	std::vector<uint32_t> power_rand_vals;
	Tile::List tiles;
};

#endif
