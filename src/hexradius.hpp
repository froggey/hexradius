#ifndef HEXRADIUS_HPP
#define HEXRADIUS_HPP

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
#include <boost/utility.hpp>

#include "hexradius.pb.h"

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
class GameState;
class ServerGameState;

typedef boost::shared_ptr<Pawn> pawn_ptr;

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
