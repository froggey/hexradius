#ifndef POWERS_HPP
#define POWERS_HPP

#include <stdint.h>
#include <boost/function.hpp>

#include "hexradius.hpp"

class ServerGameState;
class Tile;

const uint32_t PWR_SHIELD = 1<<0;
const uint32_t PWR_CLIMB = 1<<1;
const uint32_t PWR_JUMP = 1<<2;
const uint32_t PWR_INVISIBLE = 1<<3;
const uint32_t PWR_INFRAVISION = 1<<4;
const uint32_t PWR_CONFUSED = 1<<5;
const uint32_t PWR_BOMB = 1<<6;
const uint32_t PWR_GOOD = (PWR_SHIELD | PWR_CLIMB | PWR_INVISIBLE | PWR_INFRAVISION | PWR_JUMP | PWR_BOMB);

namespace Powers {
	const unsigned int REQ_FOG_OF_WAR = 1<<0;

	struct Power {
		const char *name;
		// Acually use the power.
		boost::function<void(pawn_ptr, const std::vector<Tile *> &, ServerGameState *)> func;
		// Verify that the power can be used and will do something.
		boost::function<bool(pawn_ptr, const std::vector<Tile *> &, ServerGameState *)> can_use;
		int spawn_rate;

		enum {
			undirected          = 0x000, // No tiles

			radial              = 0x001, // Radial tiles

			// Rows
			east_west           = 0x002, // - (e-w)
			northeast_southwest = 0x004, // / (ne-sw)
			northwest_southeast = 0x008, // \ (nw-se)

			// Adjacents
			east                = 0x010,
			southeast           = 0x020,
			southwest           = 0x040,
			west                = 0x080,
			northwest           = 0x100,
			northeast           = 0x200,

			targeted            = 0x400, // One tile, picked by the player
			point               = 0x800, // The pawn's tile

			// Tiles along one of the linear axes
			linear = east_west|northeast_southwest|northwest_southeast,
			// One adjacent tile
			adjacent = east|southeast|southwest|west|northwest|northeast,
		};
		unsigned int direction;

		// Spawn requirements.
		unsigned int requirements;
	};

	extern std::vector<Power> powers;
	void init_powers();

	int RandomPower(bool fog_of_war);
}

#endif /* !POWERS_HPP */
