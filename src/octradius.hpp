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
#include <sstream>

#include "octradius.pb.h"

const unsigned int MAX_MSGSIZE = 8192;
const uint16_t DEFAULT_PORT = 9012;

const int BOARD_OFFSET = 10;
const unsigned int TORUS_FRAMES = 11;

const unsigned int TILE_WIDTH = 50;
const unsigned int TILE_HEIGHT = 51;
const unsigned int TILE_WOFF = 50;
const unsigned int TILE_HOFF = 38;
const unsigned int TILE_ROFF = 25;
const unsigned int TILE_HEIGHT_FACTOR = 5;

const uint16_t ADMIN_ID = 0;

extern const char *team_names[];
extern const SDL_Colour team_colours[];

template <class T> std::string to_string(const T &t) {
	std::ostringstream ss;
	ss << t;
	return ss.str();
}

enum PlayerColour { BLUE, RED, GREEN, YELLOW, PURPLE, ORANGE, SPECTATE, NOINIT };

class Pawn;
class Server;
class Client;

struct Tile {
	typedef std::vector<Tile*> List;
	
	int col, row;
	int height;
	int power;
	bool has_power;
	Pawn *pawn;
	
	bool animating;
	float anim_height;
	int anim_delay;
	int initial_elevation;
	int final_elevation;
	
	int screen_x, screen_y;
	
	Tile(int c, int r, int h) : col(c), row(r), height(h), power(-1), has_power(false), pawn(NULL), screen_x(0), screen_y(0), animating(false) {}
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
		int range;
		uint32_t flags;
		
		Pawn(PlayerColour c, Tile::List &at, Tile *ct) : cur_tile(ct), all_tiles(at), colour(c), range(0), flags(0) {}
		
		Tile *GetTile(void) { return cur_tile; }
		void CopyToProto(protocol::pawn *p, bool copy_powers);
		
		bool Move(Tile *new_tile);
		
		void AddPower(int power);
		bool UsePower(int power, Server *server, Client *client);
		
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
	Tile::List tiles;
	
	Scenario() {}
	
	~Scenario() {
		FreeTiles(tiles);
	}
	
	Scenario &operator=(const Scenario &s) {
		if(this != &s) {
			CopyTiles(tiles, s.tiles);
		}
		
		return *this;
	}
	
	Scenario(const Scenario &s) {
		CopyTiles(tiles, s.tiles);
	}
};
void LoadScenario(std::string filename, Scenario &sc);

#endif
