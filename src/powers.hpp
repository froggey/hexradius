#ifndef POWERS_HPP
#define POWERS_HPP

#include <stdint.h>
#include <boost/function.hpp>

#include "hexradius.hpp"

class ServerGameState;

const uint32_t PWR_SHIELD = 1<<0;
const uint32_t PWR_CLIMB = 1<<1;
const uint32_t PWR_JUMP = 1<<2;
const uint32_t PWR_INVISIBLE = 1<<3;
const uint32_t PWR_INFRAVISION = 1<<4;
const uint32_t PWR_CONFUSED = 1<<5;
const uint32_t PWR_GOOD = (PWR_SHIELD | PWR_CLIMB | PWR_INVISIBLE | PWR_INFRAVISION | PWR_JUMP);

namespace Powers {
	const unsigned int REQ_FOG_OF_WAR = 1<<0;

	struct Power {
		const char *name;
		// Acually use the power.
		boost::function<void(pawn_ptr, ServerGameState *)> func;
		// Verify that the power can be used and will do something.
		boost::function<bool(pawn_ptr, ServerGameState *)> can_use;
		int spawn_rate;

		enum Directionality {
			undirected, row, radial, nw_se, ne_sw
		} direction;

		// Spawn requirements.
		unsigned int requirements;
	};

	extern std::vector<Power> powers;
	void init_powers();

	int RandomPower(bool fog_of_war);
}

#endif /* !POWERS_HPP */
