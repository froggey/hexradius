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
#include <list>
#include <SDL/SDL_ttf.h>
#include <sstream>
#include <boost/shared_array.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "octradius.pb.h"

const unsigned int MAX_MSGSIZE = 8192;
const uint16_t DEFAULT_PORT = 9012;

const int BOARD_OFFSET = 10;
const unsigned int TORUS_FRAMES = 11;

const unsigned int FRAME_RATE = 30;
const unsigned int FRAME_DELAY = 1000 / FRAME_RATE;

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

typedef boost::shared_ptr<Pawn> pawn_ptr;

struct Tile {
	typedef std::vector<Tile*> List;

	int col, row;
	int height;
	int power;
	bool has_power;
	pawn_ptr pawn;

	bool animating;
	float anim_height;
	int anim_delay;
	int initial_elevation;
	int final_elevation;

	int screen_x, screen_y;
	pawn_ptr render_pawn;

	Tile(int c, int r, int h) :
		col(c), row(r), height(h),
		power(-1), has_power(false),
		pawn(pawn_ptr()),
		animating(false), screen_x(0), screen_y(0),
		render_pawn(pawn_ptr())
	{}

	bool SetHeight(int h);

	void CopyToProto(protocol::tile *t);
};

class Pawn : public boost::enable_shared_from_this<Pawn> {
	private:
		Tile::List &all_tiles;

	public:
		enum destroy_type { OK, STOMP, PWR_DESTROY, PWR_ANNIHILATE };
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

		Pawn(PlayerColour c, Tile::List &at, Tile *ct) : all_tiles(at) {
			cur_tile = ct;

			colour = c;
			range = 0;
			flags = 0;

			last_tile = NULL;
		}

		Pawn(pawn_ptr pawn, Tile::List &at, Tile *ct) : all_tiles(at) {
			cur_tile = ct;

			colour = pawn->colour;
			powers = pawn->powers;
			range = pawn->range;
			flags = pawn->flags;

			last_tile = NULL;
		}

		void destroy(destroy_type dt);
		bool destroyed();

		void CopyToProto(protocol::pawn *p, bool copy_powers);

		bool Move(Tile *new_tile, Server *server, Client *client);

		void AddPower(int power);
		bool UsePower(int power, Server *server, Client *client);

		Tile::List RowTiles(void);
		Tile::List RadialTiles(void);
		Tile::List bs_tiles();
		Tile::List fs_tiles();
};

Tile::List RandomTiles(Tile::List tiles, int num, bool uniq);
void DestroyTeamPawns(Tile::List &tiles, PlayerColour colour);

struct options {
	std::string username;
	bool show_lines;

	options();

	void load(std::string filename);
	void save(std::string filename);
};

extern struct options options;

struct send_buf {
	typedef boost::shared_array<char> buf_ptr;

	buf_ptr buf;
	uint32_t size;

	send_buf(const protocol::message &message);
};

/* Exception-throwing versions of some SDL functions. */
void ensure_SDL_BlitSurface(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dst, SDL_Rect *dstrect);
void ensure_SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color);
void ensure_SDL_LockSurface(SDL_Surface *surface);
void ensure_SDL_SetAlpha(SDL_Surface *surface, Uint32 flags, Uint8 alpha);



#endif
