#ifndef OR_POWERS_HPP
#define OR_POWERS_HPP

#include <stdint.h>

#include "octradius.hpp"

class Server;
class Client;

const uint32_t PWR_SHIELD = 1<<0;
const uint32_t PWR_CLIMB = 1<<1;
const uint32_t HAS_POWER = 1<<2;
const uint32_t PWR_INVISIBLE = 1<<3;
const uint32_t PWR_INFRAVISION = 1<<4;
const uint32_t PWR_GOOD = (PWR_SHIELD | PWR_CLIMB | PWR_INVISIBLE | PWR_INFRAVISION);

namespace Powers {
	struct Power {
		const char *name;
		bool (*func)(pawn_ptr, Server*, Client*);
		int spawn_rate;

		/* Executing pawn is guarenteed to not be destroyed by power.
		 * This must be true if the pawn may be moved to another tile.
		*/

		bool pawn_survive;
		enum Directionality {
			undirected, row, radial, nw_se, ne_sw
		} direction;
	};

	extern Power powers[];
	extern const int num_powers;

	int RandomPower(void);
}

#endif /* !OR_POWERS_HPP */
