#ifndef PAWN_HPP
#define PAWN_HPP

class GameState;

class Pawn : public boost::enable_shared_from_this<Pawn>, boost::noncopyable {
private:
	GameState *game_state;

public:
	enum destroy_type { OK, STOMP, PWR_DESTROY, PWR_ANNIHILATE, PWR_SMASH, MINED, FELL_OUT_OF_THE_WORLD, BLACKHOLE, ANT_ATTACK };
	typedef std::map<int,int> PowerList;

	Tile *cur_tile;

	PlayerColour colour;
	PowerList powers;
	int range;
	uint32_t flags;
	destroy_type destroyed_by;

	Tile *last_tile;
	Uint32 teleport_time;

	Pawn(PlayerColour c, GameState *game_state, Tile *ct);

	void destroy(destroy_type dt);
	bool destroyed();

	void CopyToProto(protocol::pawn *p, bool copy_powers);

	bool can_move(Tile *new_tile, ServerGameState *state);
	// Perform a move without performing the move checks.
	// Moving on to a friendly pawn will still smash it!
	void force_move(Tile *new_tile, ServerGameState *state);
	// A list of tiles that the pawn can reach when moving.
	Tile::List move_tiles();

	void AddPower(int power);
	bool UsePower(int power, ServerGameState *state);

	Tile::List RowTiles() { return RowTiles(this->range); }
	Tile::List RowTiles(int range);
	Tile::List RadialTiles() { return RadialTiles(this->range); }
	Tile::List RadialTiles(int range);
	Tile::List bs_tiles() { return bs_tiles(this->range); }
	Tile::List bs_tiles(int range);
	Tile::List fs_tiles() { return fs_tiles(this->range); }
	Tile::List fs_tiles(int range);

	Tile::List linear_tiles() { return linear_tiles(this->range); }
	Tile::List linear_tiles(int range);

	bool has_power();
};

#endif
