#ifndef OR_GAMESTATE_HPP
#define OR_GAMESTATE_HPP

#include <vector>
#include <boost/utility.hpp>
#include "octradius.hpp"
#include "tile.hpp"
#include "pawn.hpp"

namespace TileAnimators { class Animator; }
namespace Animators { class Generic; }

class GameState : boost::noncopyable {
public:
	GameState();
	virtual ~GameState();
	Tile::List tiles;

	/** Return all the pawns on the board. */
	std::vector<pawn_ptr> all_pawns();

	/** Return all the pawns belonging to a given player. */
	std::vector<pawn_ptr> player_pawns(PlayerColour colour);

	/** Return the tile at given board column & row,
	 * or null if there is no tile at that location. */
	Tile *tile_at(int column, int row);
	
	Tile *tile_left_of_coords(int column, int row);
	Tile *tile_right_of_coords(int column, int row);
	Tile *tile_ne_of_coords(int column, int row);
	Tile *tile_nw_of_coords(int column, int row);
	Tile *tile_se_of_coords(int column, int row);
	Tile *tile_sw_of_coords(int column, int row);
	Tile *tile_left_of(Tile *t);
	Tile *tile_right_of(Tile *t);
	Tile *tile_ne_of(Tile *t);
	Tile *tile_nw_of(Tile *t);
	Tile *tile_se_of(Tile *t);
	Tile *tile_sw_of(Tile *t);
	
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

class ServerGameState : public GameState {
public:
	ServerGameState(Server &server);
	void add_animator(Animators::Generic *animator);
	void add_animator(TileAnimators::Animator *animator);
	void teleport_hack(pawn_ptr pawn);
	void grant_upgrade(pawn_ptr pawn, uint32_t upgrade);
	void add_power_notification(pawn_ptr pawn, int power);
	void use_power_notification(pawn_ptr pawn, int power);
	void set_tile_height(Tile *tile, int height);
	void destroy_pawn(pawn_ptr target, Pawn::destroy_type reason, pawn_ptr killer = pawn_ptr());
	void update_pawn(pawn_ptr pawn);
	void update_tile(Tile *tile);
	// Move a pawn onto the target tile, for effect.
	void move_pawn_to(pawn_ptr pawn, Tile *target);
	void run_worm_stuff(pawn_ptr pawn, int range);
private:
	Server &server;
};

#endif
