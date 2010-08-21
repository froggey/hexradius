#ifndef OR_OCTRADIUS_HPP
#define OR_OCTRADIUS_HPP

#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <assert.h>
#include <vector>
#include <fstream>
#include <string.h>
#include <map>
#include <set>
#include <SDL/SDL_ttf.h>

#include "octradius.pb.h"

const uint MAX_MSGSIZE = 8192; // YAAAAAAAAAAAAAAAAY I CAN USE LINE COMMENTS PROPERLY TOO!

const int BOARD_OFFSET = 10;
const uint TORUS_FRAMES = 11;

const uint TILE_WIDTH = 50;
const uint TILE_HEIGHT = 51;
const uint TILE_WOFF = 50;
const uint TILE_HOFF = 38;
const uint TILE_ROFF = 25;
const uint TILE_HEIGHT_FACTOR = 5;

enum PlayerColour { BLUE, RED, GREEN, YELLOW, PURPLE, ORANGE, SPECTATE };

class Pawn;

struct Tile {
	typedef std::vector<Tile*> List;
	
	int col, row;
	int height;
	int power;
	bool has_power;
	Pawn *pawn;
	
	bool use_anim_height;
	float anim_height;
	int anim_delay;
	int initial_elevation;
	int final_elevation;
	
	int screen_x, screen_y;
	
	Tile(int c, int r, int h) : col(c), row(r), height(h), power(-1), has_power(false), pawn(NULL), screen_x(0), screen_y(0), use_anim_height(false) {}
	~Tile();
	
	bool SetHeight(int h);
	
	void CopyToProto(protocol::tile *t);
};

class Pawn {
	private:
		Tile *cur_tile;
		Tile::List &all_tiles;
		
	public:
		typedef std::map<int,int> PowerList;
		
		PlayerColour colour;
		PowerList powers;
		int range, flags;
		
		Pawn(PlayerColour c, Tile::List &at, Tile *ct) : cur_tile(ct), all_tiles(at), colour(c), range(0), flags(0) {}
		
		Tile *GetTile(void) { return cur_tile; }
		void CopyToProto(protocol::pawn *p, bool copy_powers);
		
		bool Move(Tile *new_tile);
		
		void AddPower(int power);
		bool UsePower(int power);
		
		Tile::List RowTiles(void);
		Tile::List RadialTiles(void);
};

Tile *FindTile(Tile::List &list, int c, int r);
Pawn *FindPawn(Tile::List &list, int c, int r);
Tile::List RandomTiles(Tile::List tiles, int num, bool uniq);
Tile *TileAtXY(Tile::List &tiles, int x, int y);
Pawn *PawnAtXY(Tile::List &tiles, int x, int y);
void FreeTiles(Tile::List &tiles);
void CopyTiles(Tile::List &dest, const Tile::List &src);
void DestroyTeamPawns(Tile::List &tiles, PlayerColour colour);

struct Scenario {
	int cols;
	int rows;
	Tile::List tiles;
	
	Scenario() : cols(0), rows(0) {}
	
	~Scenario() {
		FreeTiles(tiles);
	}
	
	Scenario &operator=(const Scenario &s) {
		if(this != &s) {
			cols = s.cols;
			rows = s.rows;
			CopyTiles(tiles, s.tiles);
		}
		
		return *this;
	}
	
	Scenario(const Scenario &s) {
		cols = s.cols;
		rows = s.rows;
		CopyTiles(tiles, s.tiles);
	}
};
void LoadScenario(std::string filename, Scenario &sc);

#endif
