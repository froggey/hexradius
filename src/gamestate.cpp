#include "gamestate.hpp"

GameState::~GameState()
{
	FreeTiles(tiles);
}
