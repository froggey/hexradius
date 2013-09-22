#ifndef PAWN_HPP
#define PAWN_HPP

class Pawn : public boost::enable_shared_from_this<Pawn>, boost::noncopyable {
private:
	Tile::List &all_tiles;

public:
	enum destroy_type { OK, STOMP, PWR_DESTROY, PWR_ANNIHILATE, PWR_SMASH, MINED, FELL_OUT_OF_THE_WORLD, BLACKHOLE };
	typedef std::map<int,int> PowerList;

	Tile *cur_tile;

	PlayerColour colour;
	PowerList powers;
	int range;
	uint32_t flags;
	destroy_type destroyed_by;

	Tile *last_tile;
	Uint32 teleport_time;
	struct PowerMessage {
		int power;
		bool added;
		float time;

		PowerMessage(int p, bool a) : power(p), added(a), time(2) {}
	};
	std::list<PowerMessage> power_messages;

	Pawn(PlayerColour c, Tile::List &at, Tile *ct);

	void destroy(destroy_type dt);
	bool destroyed();

	void CopyToProto(protocol::pawn *p, bool copy_powers);

	bool Move(Tile *new_tile, ServerGameState *state);
	// Perform a move without performing the move checks.
	// Moving on to a friendly pawn will still smash it!
	void force_move(Tile *new_tile, ServerGameState *state);

	void AddPower(int power);
	bool UsePower(int power, ServerGameState *state);

	/* Test if the pawn moved (or fell) onto a mine. */
	void maybe_step_on_mine(ServerGameState *state);

	Tile::List RowTiles(void);
	Tile::List RadialTiles(void);
	Tile::List bs_tiles();
	Tile::List fs_tiles();

	bool has_power();
};

#endif
