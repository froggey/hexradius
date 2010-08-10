#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <fstream>
#include <string.h>
#include <map>

#include "loadimage.hpp"

namespace OctRadius {
	enum Colour { BLUE, RED, GREEN, YELLOW };
	
	struct Power {
		const char *name;
		int (*func)(void);
	};
	
	typedef std::map<const OctRadius::Power*,int> PowerList;
	
	class Tile;
	
	typedef std::vector<OctRadius::Tile*> TileList;
	typedef std::vector<OctRadius::TileList> TileTable;
	
	class Pawn {
		public:
			Colour colour;
			PowerList powers;
			
			Pawn(Colour c, TileTable &tt, Tile *tile) : colour(c), m_tiles(tt), m_tile(tile) {}
			
			void AddPower(const Power* power) {
				PowerList::iterator i = powers.find(power);
				if(i != powers.end()) {
					i->second++;
				}else{
					powers.insert(std::make_pair(power, 1));
				}
			}
			
			void Move(Tile *tile);
			Tile *OnTile(void) { return m_tile; }
			
		private:
			TileTable &m_tiles;
			Tile *m_tile;
	};
	
	class Tile {
		public:
			int col, row;
			int height;
			
			int screen_x, screen_y;
			
			Pawn* pawn;
			const Power *power;
			
			Tile(int c, int r) : col(c), row(r), height(0), pawn(NULL), power(NULL) {}
			
			~Tile() {
				delete pawn;
			}
	};
	
	class TileLoopThing {
		public:
			TileLoopThing(TileTable &ttable) : m_ttable(ttable), c(0), r(0) {
				m_cols = ttable.size();
				m_rows = ttable[0].size();
			}
			
			void first(void) {
				c = 0;
				r = 0;
			}
			
			void last(void) {
				c = m_cols-1;
				r = m_rows-1;
			}
			
			int valid(void) {
				return c >= 0 && c < m_cols && r >= 0 && r < m_rows;
			}
			
			TileLoopThing& operator++() {
				do {
					if(++c == m_cols) {
						c = 0;
						r++;
					}
				} while(valid() && !tile());
				
				return *this;
			}
			
			void operator++(int) {
				operator++();
			}
			
			TileLoopThing& operator--() {
				do {
					if(c-- == 0) {
						c = m_cols-1;
						r--;
					}
				} while(valid() && !tile());
				
				return *this;
			}
			
			void operator--(int) {
				operator--();
			}
			
			Tile *tile(void) {
				return m_ttable[c][r];
			}
			
		private:
			TileTable &m_ttable;
			int m_cols, m_rows, c, r;
	};
}

struct uistate {
	OctRadius::Pawn *dpawn;
	OctRadius::Pawn *mpawn;
	
	uistate() : dpawn(NULL), mpawn(NULL) {}
};

namespace OctRadius {
	void DrawBoard(TileTable &tiles, SDL_Surface *screen, struct uistate &uistate);
	OctRadius::Tile *TileAtXY(TileTable &tiles, int x, int y);
	void LoadScenario(std::string filename, TileTable &tiles);
	void SpawnPowers(TileTable &tiles, int num);
	TileList ChooseRandomTiles(TileList tiles, int num, int uniq);
}

void OctRadius::Pawn::Move(Tile *tile) {
	if(
		!(tile->col == m_tile->col && (tile->row == m_tile->row+1 || tile->row == m_tile->row-1)) &&
		!(tile->row == m_tile->row && (tile->col == m_tile->col+1 || tile->col == m_tile->col-1))
	) {
		std::cerr << "Square out of range" << std::endl;
		return;
	}
	
	if(tile->height-1 > m_tile->height) {
		std::cerr << "Square is too high" << std::endl;
		return;
	}
	
	if(tile->pawn) {
		if(tile->pawn->colour == colour) {
			std::cerr << "Square is blocked by friendly pawn" << std::endl;
			return;
		}else{
			std::cerr << "Deleting enemy pawn" << std::endl;
			delete tile->pawn;
		}
	}
	
	m_tile->pawn = NULL;
	m_tile = tile;
	
	tile->pawn = this;
	
	if(tile->power) {
		AddPower(tile->power);
		tile->power = NULL;
	}
}

const uint TILE_SIZE = 50;
const uint BOARD_OFFSET = 20;
const uint TORUS_FRAMES = 11;

const OctRadius::Power POWERS[] = {
	{"Do something", NULL},
	{"Do something else", NULL},
	{NULL, NULL}
};

void OctRadius::DrawBoard(TileTable &tiles, SDL_Surface *screen, struct uistate &uistate) {
	int torus_frame = SDL_GetTicks() / 100 % (TORUS_FRAMES * 2);
	if (torus_frame >= TORUS_FRAMES)
		torus_frame = 2 * TORUS_FRAMES - torus_frame - 1;
	
	SDL_Surface *square = OctRadius::LoadImage("graphics/tile.png");
	assert(square != NULL);
	
	SDL_Surface *pawn_graphics = OctRadius::LoadImage("graphics/pawns.png");
	SDL_Surface *pickup = OctRadius::LoadImage("graphics/pickup.png");
	assert(pawn_graphics && pickup);
	
	assert(SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
	
	OctRadius::TileLoopThing tl(tiles);
	
	for(; tl.valid(); tl++) {
		SDL_Rect rect;
		rect.x = BOARD_OFFSET + TILE_SIZE * tl.tile()->col;
		rect.y = BOARD_OFFSET + TILE_SIZE * tl.tile()->row;
		rect.w = rect.h = 0;
		
		rect.x += (-1 * tl.tile()->height) * 10;
		rect.y += (-1 * tl.tile()->height) * 10;
		
		tl.tile()->screen_x = rect.x;
		tl.tile()->screen_y = rect.y;
		
		assert(SDL_BlitSurface(square, NULL, screen, &rect) == 0);
		
		if(tl.tile()->power) {
			assert(SDL_BlitSurface(pickup, NULL, screen, &rect) == 0);
		}
		
		if (tl.tile()->pawn && tl.tile()->pawn != uistate.dpawn) {
			SDL_Rect srect = { tl.tile()->pawn->powers.size() ? (torus_frame * 50) : 0, tl.tile()->pawn->colour * 50, 50, 50 };
			assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
		}
	}
	
	if(uistate.dpawn) {
		int mouse_x, mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);
		
		SDL_Rect srect = { uistate.dpawn->powers.size() ? (torus_frame * 50) : 0, uistate.dpawn->colour*50, 50, 50 };
		SDL_Rect rect = { mouse_x-30, mouse_y-30, 0, 0 };
		
		assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
	}
	
	if(uistate.mpawn) {
		SDL_Rect rect = { uistate.mpawn->OnTile()->screen_x+TILE_SIZE, uistate.mpawn->OnTile()->screen_y, 100, 140 };
		assert(SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

/* Return the "topmost" tile rendered at the given X,Y co-ordinates or NULL if
 * there is no tile at that location.
*/
OctRadius::Tile *OctRadius::TileAtXY(TileTable &tiles, int x, int y) {
	OctRadius::TileLoopThing tl(tiles);
	
	for(tl.last(); tl.valid(); tl--) {
		int tx = tl.tile()->screen_x;
		int ty = tl.tile()->screen_y;
		
		if(tx <= x && tx+(int)TILE_SIZE > x && ty <= y && ty+(int)TILE_SIZE > y) {
			return tl.tile();
		}
	}
	
	return NULL;
}

static char *next_value(char *str) {
	char *r = str+strcspn(str, "\t ");
	
	if(r[0]) {
		r[0] = '\0';
		r += strspn(r+1, "\t ")+1;
	}
	
	return r;
}

void OctRadius::LoadScenario(std::string filename, TileTable &tiles) {
	std::fstream file(filename.c_str(), std::fstream::in);
	assert(file.is_open());
	
	char buf[1024], *bp;
	
	int grid_cols = 0, grid_rows = 0;
	
	while(file.good()) {
		file.getline(buf, sizeof(buf));
		buf[strcspn(buf, "\n")] = '\0';
		
		bp = next_value(buf);
		std::string name = buf;
		
		if(name == "GRID") {
			grid_cols = atoi(bp);
			grid_rows = atoi(next_value(bp));
			
			assert(grid_cols > 0 && grid_rows > 0);
	
			for(int c = 0; c < grid_cols; c++) {
				TileList col;
				
				for(int r = 0; r < grid_rows; r++) {
					col.push_back(new OctRadius::Tile(c, r));
				}
				
				tiles.push_back(col);
			}
		}
		if(name == "SPAWN") {
			assert(grid_cols > 0 && grid_rows > 0);
			
			/* SPAWN x y c */
			
			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int c = atoi((bp = next_value(bp)));
			
			assert(x >= 0 && x < grid_cols && y >= 0 && y < grid_rows);
			assert(c >= 0 && c < 4);
			
			tiles[x][y]->pawn = new Pawn((OctRadius::Colour)c, tiles, tiles[x][y]);
		}
		if(name == "HOLE") {
			int x = atoi(bp);
			int y = atoi(next_value(bp));
			
			assert(x >= 0 && x < grid_cols && y >= 0 && y < grid_rows);
			
			delete tiles[x][y];
			tiles[x][y] = NULL;
		}
	}
}

void OctRadius::SpawnPowers(TileTable &tiles, int num) {
	TileLoopThing tl(tiles);
	TileList ctiles;
	
	for(; tl.valid(); tl++) {
		if(!tl.tile()->pawn && !tl.tile()->power) {
			ctiles.push_back(tl.tile());
		}
	}
	
	TileList stiles = ChooseRandomTiles(ctiles, num, 1);
	TileList::iterator i = stiles.begin();
	
	static int pcount = 0;
	while(POWERS[pcount].name) { pcount++; }
	
	for(; i != stiles.end(); i++) {
		(*i)->power = &POWERS[rand() % pcount];
		std::cout << "Spawned " << (*i)->power->name << " at (" << (*i)->col << "," << (*i)->row << ")" << std::endl;
	}
}

OctRadius::TileList OctRadius::ChooseRandomTiles(TileList tiles, int num, int uniq) {
	TileList ret;
	
	while(tiles.size() && num) {
		TileList::iterator i = tiles.begin();
		i += rand() % tiles.size();
		
		ret.push_back(*i);
		
		if(uniq) {
			tiles.erase(i);
		}
		
		num--;
	}
	
	return ret;
}

int main(int argc, char **argv) {
	if(argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [scenario]" << std::endl;
		return 1;
	}
	
	OctRadius::TileTable tiles;
	
	if(argc == 1) {
		OctRadius::LoadScenario("scenario/default.txt", tiles);
	}else{
		OctRadius::LoadScenario(argv[1], tiles);
	}
	
	int cols = tiles.size();
	int rows = tiles[0].size();
	
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	
	SDL_Surface *screen = SDL_SetVideoMode((cols*TILE_SIZE) + (2*BOARD_OFFSET), (rows*TILE_SIZE) + (2*BOARD_OFFSET), 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	SDL_WM_SetCaption("OctRadius", "OctRadius");
	
	//OctRadius::DrawBoard(tiles, screen);
	
	SDL_Event event;
	
	uint last_redraw = 0;
	struct uistate uistate;
	int xd, yd;
	
	while(1) {
		if(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				break;
			}
			else if(event.type == SDL_MOUSEBUTTONDOWN) {
				OctRadius::Tile *tile = OctRadius::TileAtXY(tiles, event.button.x, event.button.y);
				
				if(tile) {
					if(event.button.button == SDL_BUTTON_WHEELUP && tile->height < 2) {
						tile->height++;
						OctRadius::DrawBoard(tiles, screen, uistate);
						last_redraw = SDL_GetTicks();
					}
					else if(event.button.button == SDL_BUTTON_WHEELDOWN && tile->height > -2) {
						tile->height--;
						OctRadius::DrawBoard(tiles, screen, uistate);
						last_redraw = SDL_GetTicks();
					}
					else if (event.button.button == SDL_BUTTON_LEFT) {
						uistate.mpawn = NULL;
						xd = event.button.x;
						yd = event.button.y;
						
						if(tile->pawn) {
							uistate.dpawn = tile->pawn;
						}
					}
				}
			}else if(event.type == SDL_MOUSEBUTTONUP) {
				OctRadius::Tile *tile = OctRadius::TileAtXY(tiles, event.button.x, event.button.y);
				
				if(event.button.button == SDL_BUTTON_LEFT && uistate.dpawn) {
					if(xd == event.button.x && yd == event.button.y) {
						uistate.mpawn = tile->pawn;
					}else if(tile != uistate.dpawn->OnTile()) {
						uistate.dpawn->Move(tile);
						
						OctRadius::SpawnPowers(tiles, 1);
					}
					
					uistate.dpawn = NULL;
				}
			}else if(event.type == SDL_MOUSEMOTION && uistate.dpawn) {
				OctRadius::DrawBoard(tiles, screen, uistate);
				last_redraw = SDL_GetTicks();
			}
		}
		
		// force a redraw if it's been too long (for animations)
		if (SDL_GetTicks() >= last_redraw + 50) {
			OctRadius::DrawBoard(tiles, screen, uistate);
			last_redraw = SDL_GetTicks();
		}
	}
	
	OctRadius::FreeImages();
	
	SDL_Quit();
	
	return 0;
}
