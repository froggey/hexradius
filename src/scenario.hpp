#ifndef SCENARIO_HPP
#define SCENARIO_HPP

#include "hexradius.hpp"
#include "hexradius.pb.h"
#include <boost/utility.hpp>

class GameState;

struct Scenario : boost::noncopyable {
	GameState *game_state;
	std::set<PlayerColour> colours;

	// Blech.
	std::string last_filename;
	protocol::message saved_msg;

	Scenario();
	Scenario(Server &server);
	~Scenario();

	/** Load a scenario from the given file.
	 * Throws an exception if the file doesn't exist. */
	void load_file(const std::string &filename);
	/** Save the current scenario to the given file */
	void save_file(const std::string &filename);

	/** Load a scenario from a network message. */
	void load_proto(const protocol::message &msg);
	/** Serialize the loaded scenerio into a network message */
	void store_proto(protocol::message &msg);

	/** Return the currently loaded game state, after
	 * converting pawn colours to their appropriate player colours.
	 * Caller takes ownership of the GameState and init_game
	 * cannot be called again until a new scenario is loaded. */
	GameState *init_game(std::set<PlayerColour> spawn_colours);
private:
	Server *server;
};

#endif /* !SCENARIO_HPP */
