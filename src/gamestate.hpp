#ifndef OR_GAMESTATE_HPP
#define OR_GAMESTATE_HPP

#include <vector>
#include <boost/utility.hpp>
#include "octradius.hpp"

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

	virtual void add_animator(TileAnimators::Animator *animator);
	virtual void add_animator(Animators::Generic *animator);
	virtual bool teleport_hack(pawn_ptr pawn);
	virtual bool grant_upgrade(pawn_ptr pawn, uint32_t upgrade);
	virtual void add_power_notification(pawn_ptr pawn, int power);
};

class ServerGameState : public GameState {
public:
	ServerGameState(Server &server);
	virtual void add_animator(Animators::Generic *animator);
	virtual void add_animator(TileAnimators::Animator *animator);
	virtual bool teleport_hack(pawn_ptr pawn);
	virtual bool grant_upgrade(pawn_ptr pawn, uint32_t upgrade);
	virtual void add_power_notification(pawn_ptr pawn, int power);
private:
	Server &server;
};

class ClientGameState : public GameState {
public:
	ClientGameState(Client &client);
private:
	Client &client;
};

#endif
