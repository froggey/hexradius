#ifndef TILE_HPP
#define TILE_HPP

#include <vector>
#include "octradius.hpp"

struct Tile {
	typedef std::vector<Tile*> List;

	int col, row;
	int height;
	int power;
	bool has_power;
	bool smashed;
	pawn_ptr pawn;

	bool animating;
	float anim_height;
	int anim_delay;
	int initial_elevation;
	int final_elevation;

	int screen_x, screen_y;
	pawn_ptr render_pawn;

	bool has_mine;
	PlayerColour mine_colour;

	bool has_landing_pad;
	PlayerColour landing_pad_colour;

	bool has_black_hole;
	int black_hole_power;
	
	struct PowerMessage {
		int power;
		bool added;
		float time;

		PowerMessage(int p, bool a) : power(p), added(a), time(2) {}
	};
	std::list<PowerMessage> power_messages;
	
	uint32_t wrap;
	enum wrap_direction { WRAP_RIGHT, WRAP_LEFT, WRAP_UP_RIGHT, WRAP_DOWN_RIGHT, WRAP_UP_LEFT, WRAP_DOWN_LEFT };

	Tile(int c, int r, int h);

	bool SetHeight(int h);

	void CopyToProto(protocol::tile *t);
};

Tile::List RandomTiles(Tile::List tiles, int num, bool unique, bool include_mines, bool include_holes, bool include_occupied);

#endif
