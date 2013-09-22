#include <boost/foreach.hpp>

#include "tile.hpp"
#include "octradius.hpp"
#include "loadimage.hpp"

Tile::Tile(int c, int r, int h) :
	col(c), row(r), height(h),
	power(-1), has_power(false), smashed(false),
	pawn(pawn_ptr()),
	animating(false), screen_x(0), screen_y(0),
	render_pawn(pawn_ptr()),
	has_mine(false), has_landing_pad(false),
	has_black_hole(false) {
}

bool Tile::SetHeight(int h) {
	if(h != height && h <= 2 && h >= -2) {
		height = h;
		return true;
	}
	else {
		return false;
	}
}

void Tile::CopyToProto(protocol::tile *t) {
	t->set_col(col);
	t->set_row(row);
	t->set_height(height);
	t->set_power(has_power);
	t->set_smashed(smashed);
	t->set_has_mine(has_mine);
	t->set_mine_colour(mine_colour);
	t->set_has_landing_pad(has_landing_pad);
	t->set_landing_pad_colour(landing_pad_colour);
	t->set_has_black_hole(has_black_hole);
	t->set_black_hole_power(black_hole_power);
}

Tile::List RandomTiles(Tile::List _tiles, int num, bool unique, bool include_mines, bool include_holes, bool include_occupied) {
	Tile::List ret, tiles;

	BOOST_FOREACH(Tile *tile, _tiles) {
		if ((include_holes || !(tile->smashed || tile->has_black_hole)) &&
		    (include_mines || !tile->has_mine) &&
		    (include_occupied || !tile->pawn))
			tiles.push_back(tile);
	}

	while(tiles.size() && num) {
		Tile::List::iterator i = tiles.begin();
		i += rand() % tiles.size();

		ret.push_back(*i);

		if(unique) {
			tiles.erase(i);
		}

		num--;
	}

	return ret;
}
