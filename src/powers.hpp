#ifndef OR_POWERS_HPP
#define OR_POWERS_HPP

#include "octradius.hpp"

const uint PWR_SHIELD = 1<<0;
const uint PWR_CLIMB = 1<<1;
const uint HAS_POWER = 1<<2;
const uint PWR_GOOD = (PWR_SHIELD | PWR_CLIMB);

namespace Powers {
	struct Power {
		const char *name;
		int (*func)(Pawn*);
		int spawn_rate;
	};
	
	extern const Power powers[];
	extern const int num_powers;
	
	int RandomPower(void);
	
	int destroy_row(Pawn *pawn);
	int destroy_radial(Pawn *pawn);
	int raise_tile(Pawn *pawn);
	int lower_tile(Pawn *pawn);
	int increase_range(Pawn *pawn);
	int hover(Pawn *pawn);
	int wall_row(Pawn *pawn);
	int shield(Pawn *pawn);
	int purify_row(Pawn *pawn);
	int purify_radial(Pawn *pawn);
}

#endif /* !OR_POWERS_HPP */
