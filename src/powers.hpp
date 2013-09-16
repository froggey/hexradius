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
const uint32_t PWR_GOOD = (PWR_SHIELD | PWR_CLIMB | PWR_INVISIBLE);

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

	bool destroy_row(pawn_ptr pawn, Server *server, Client *client);
	bool destroy_radial(pawn_ptr pawn, Server *server, Client *client);
	bool destroy_bs(pawn_ptr pawn, Server *server, Client *client);
	bool destroy_fs(pawn_ptr pawn, Server *server, Client *client);

	bool raise_tile(pawn_ptr pawn, Server *server, Client *client);
	bool lower_tile(pawn_ptr pawn, Server *server, Client *client);
	bool increase_range(pawn_ptr pawn, Server *server, Client *client);
	bool hover(pawn_ptr pawn, Server *server, Client *client);
	bool shield(pawn_ptr pawn, Server *server, Client *client);
	bool invisibility(pawn_ptr pawn, Server *server, Client *client);
	bool teleport(pawn_ptr pawn, Server *server, Client *client);
	bool mine(pawn_ptr pawn, Server *server, Client *client);

	bool elevate_row(pawn_ptr pawn, Server *server, Client *client);
	bool elevate_radial(pawn_ptr pawn, Server *server, Client *client);
	bool elevate_bs(pawn_ptr pawn, Server *server, Client *client);
	bool elevate_fs(pawn_ptr pawn, Server *server, Client *client);

	bool dig_row(pawn_ptr pawn, Server *server, Client *client);
	bool dig_radial(pawn_ptr pawn, Server *server, Client *client);
	bool dig_bs(pawn_ptr pawn, Server *server, Client *client);
	bool dig_fs(pawn_ptr pawn, Server *server, Client *client);

	bool purify_row(pawn_ptr pawn, Server *server, Client *client);
	bool purify_radial(pawn_ptr pawn, Server *server, Client *client);
	bool purify_bs(pawn_ptr pawn, Server *server, Client *client);
	bool purify_fs(pawn_ptr pawn, Server *server, Client *client);

	bool annihilate_row(pawn_ptr pawn, Server *server, Client *client);
	bool annihilate_radial(pawn_ptr pawn, Server *server, Client *client);
	bool annihilate_bs(pawn_ptr pawn, Server *server, Client *client);
	bool annihilate_fs(pawn_ptr pawn, Server *server, Client *client);
}

#endif /* !OR_POWERS_HPP */
