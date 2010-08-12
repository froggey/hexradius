#ifndef OR_POWERS_HPP
#define OR_POWERS_HPP

#include "octradius.hpp"

namespace Powers {
	int destroy_column(OctRadius::Pawn *pawn);
	int destroy_row(OctRadius::Pawn *pawn);
	int destroy_radial(OctRadius::Pawn *pawn);
	int raise_tile(OctRadius::Pawn *pawn);
	int lower_tile(OctRadius::Pawn *pawn);
	int moar_range(OctRadius::Pawn *pawn);
	int climb_tile(OctRadius::Pawn *pawn);
	int wall_column(OctRadius::Pawn *pawn);
	int wall_row(OctRadius::Pawn *pawn);
	int armour(OctRadius::Pawn *pawn);
	int purify_row(OctRadius::Pawn *pawn);
	int purify_column(OctRadius::Pawn *pawn);
	int purify_radial(OctRadius::Pawn *pawn);
}

#endif /* !OR_POWERS_HPP */
