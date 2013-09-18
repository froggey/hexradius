#ifndef OR_GAMESTATE_HPP
#define OR_GAMESTATE_HPP

#include <vector>
#include <boost/utility.hpp>
#include "octradius.hpp"

class GameState : boost::noncopyable {
public:
	GameState();
	~GameState();
	std::vector<uint32_t> power_rand_vals;
	Tile::List tiles;

	/** Return all the pawns on the board. */
	std::vector<pawn_ptr> all_pawns();

	/** Return all the pawns belonging to a given player. */
	std::vector<pawn_ptr> player_pawns(PlayerColour colour);

	/** Return the tile at given board column & row,
	 * or null if there is no tile at that location. */
	Tile *tile_at(int column, int row);
	/** Return the pawn at given board column & row,
	 * or null if there is no pawn at that location. */
	pawn_ptr pawn_at(int column, int row);

	/** Return the "topmost" tile rendered at the given X,Y screen co-ordinates
	 * or null if there is no tile at that location. */
	Tile *tile_at_screen(int x, int y);
	/** Return the "topmost" pawn rendered at the given X,Y screen co-ordinates
	 * or null if there is no pawn at that location. */
	pawn_ptr pawn_at_screen(int x, int y);

	/** Destroy all the pawns on the given team. */
	void destroy_team_pawns(PlayerColour colour);
};

#endif
