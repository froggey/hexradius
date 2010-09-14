#ifndef OR_POWERS_HPP
#define OR_POWERS_HPP

#include <stdint.h>

#include "octradius.hpp"

class Server;
class Client;

const uint32_t PWR_SHIELD = 1<<0;
const uint32_t PWR_CLIMB = 1<<1;
const uint32_t HAS_POWER = 1<<2;
const uint32_t PWR_GOOD = (PWR_SHIELD | PWR_CLIMB);

namespace Powers {
	struct Power {
		const char *name;
		bool (*func)(Pawn*, Server*, Client*);
		int spawn_rate;
		
		/* Executing pawn is guarenteed to not be destroyed by power.
		 * This must be true if the pawn may be moved to another tile.
		*/
		
		bool pawn_survive;
	};
	
	extern Power powers[];
	extern const int num_powers;
	
	int RandomPower(void);
	
	bool destroy_row(Pawn *pawn, Server *server, Client *client);
	bool destroy_radial(Pawn *pawn, Server *server, Client *client);
	bool destroy_bs(Pawn *pawn, Server *server, Client *client);
	bool destroy_fs(Pawn *pawn, Server *server, Client *client);
	
	bool raise_tile(Pawn *pawn, Server *server, Client *client);
	bool lower_tile(Pawn *pawn, Server *server, Client *client);
	bool increase_range(Pawn *pawn, Server *server, Client *client);
	bool hover(Pawn *pawn, Server *server, Client *client);
	bool shield(Pawn *pawn, Server *server, Client *client);
	bool teleport(Pawn *pawn, Server *server, Client *client);
	
	bool elevate_row(Pawn *pawn, Server *server, Client *client);
	bool elevate_radial(Pawn *pawn, Server *server, Client *client);
	bool elevate_bs(Pawn *pawn, Server *server, Client *client);
	bool elevate_fs(Pawn *pawn, Server *server, Client *client);
	
	bool dig_row(Pawn *pawn, Server *server, Client *client);
	bool dig_radial(Pawn *pawn, Server *server, Client *client);
	bool dig_bs(Pawn *pawn, Server *server, Client *client);
	bool dig_fs(Pawn *pawn, Server *server, Client *client);
	
	bool purify_row(Pawn *pawn, Server *server, Client *client);
	bool purify_radial(Pawn *pawn, Server *server, Client *client);
	bool purify_bs(Pawn *pawn, Server *server, Client *client);
	bool purify_fs(Pawn *pawn, Server *server, Client *client);
}

#endif /* !OR_POWERS_HPP */
