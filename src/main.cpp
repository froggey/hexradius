#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <fstream>
#include <string.h>
#include <map>
#include <SDL/SDL_ttf.h>

#include "loadimage.hpp"
#include "fontstuff.hpp"

namespace OctRadius {
	enum Colour { BLUE, RED, GREEN, YELLOW };
	
	class Pawn;
	
	struct Power {
		const char *name;
		int (*func)(OctRadius::Pawn*);
		int spawn_rate;
	};
	
	typedef std::map<const OctRadius::Power*,int> PowerList;
	
	class Tile;
	
	typedef std::vector<OctRadius::Tile*> TileList;
	typedef std::vector<OctRadius::TileList> TileTable;
	
	class Pawn {
		public:
			Colour colour;
			PowerList powers;
			
			Pawn(Colour c, TileList &tt, Tile *tile) : colour(c), m_tiles(tt), m_tile(tile) {}
			
			void AddPower(const Power* power) {
				PowerList::iterator i = powers.find(power);
				if(i != powers.end()) {
					i->second++;
				}else{
					powers.insert(std::make_pair(power, 1));
				}
			}
			
			void UsePower(const Power *power) {
				PowerList::iterator i = powers.find(power);
				if(i == powers.end()) {
					return;
				}
				
				if(!i->first->func(this)) {
					return;
				}
				
				if(i->second == 1) {
					powers.erase(i);
				}else{
					i->second--;
				}
			}
			
			void Move(Tile *tile);
			Tile *OnTile(void) { return m_tile; }
			
			TileList ColumnList(void);
			TileList RowList(void);
			TileList RadialList(void);
			
		private:
			TileList &m_tiles;
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
}

struct pmenu_entry {
	int x, y, w, h;
	const OctRadius::Power *power;
};

struct uistate {
	OctRadius::Pawn *dpawn;
	OctRadius::Pawn *mpawn;
	
	std::vector<struct pmenu_entry> pmenu;
	int pmx, pmy, pmw, pmh;
	
	uistate() : dpawn(NULL), mpawn(NULL), pmw(0), pmh(0) {}
};

namespace OctRadius {
	void DrawBoard(TileList &tiles, SDL_Surface *screen, struct uistate &uistate);
	OctRadius::Tile *TileAtXY(TileList &tiles, int x, int y);
	void LoadScenario(std::string filename, TileList &tiles, int &cols, int &rows);
	void SpawnPowers(TileList &tiles, int num);
	TileList ChooseRandomTiles(TileList tiles, int num, bool uniq);
	Tile *FindTile(TileList &list, int c, int r);
	const Power* ChooseRandomPower();
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

OctRadius::TileList OctRadius::Pawn::ColumnList(void) {
	TileList tiles;
	TileList::iterator i = m_tiles.begin();
	
	while(i != m_tiles.end()) {
		if((*i)->col == m_tile->col) {
			tiles.push_back(*i);
		}
		
		i++;
	}
	
	return tiles;
}

OctRadius::TileList OctRadius::Pawn::RowList(void) {
	TileList tiles;
	TileList::iterator i = m_tiles.begin();
	
	while(i != m_tiles.end()) {
		if((*i)->row == m_tile->row) {
			tiles.push_back(*i);
		}
		
		i++;
	}
	
	return tiles;
}

OctRadius::TileList OctRadius::Pawn::RadialList(void) {
	TileList tiles;
	TileList::iterator i = m_tiles.begin();
	
	while(i != m_tiles.end()) {
		if((*i)->row >= m_tile->row-1 && (*i)->row <= m_tile->row+1 && (*i)->col >= m_tile->col-1 && (*i)->col <= m_tile->col+1) {
			tiles.push_back(*i);
		}
		
		i++;
	}
	
	return tiles;
}

const uint TILE_SIZE = 50;
const uint BOARD_OFFSET = 20;
const uint TORUS_FRAMES = 11;

static int destroy_column(OctRadius::Pawn *pawn) {
	OctRadius::TileList tiles = pawn->ColumnList();
	OctRadius::TileList::iterator i = tiles.begin();
	int ret = 0;
	
	while(i != tiles.end()) {
		if((*i)->pawn && (*i)->pawn->colour != pawn->colour) {
			delete (*i)->pawn;
			(*i)->pawn = NULL;
			
			ret = 1;
		}
		
		i++;
	}
	
	return ret;
}

const OctRadius::Power POWERS[] = {
	{"Destroy Column", &destroy_column, 100}
};
const int N_POWERS = sizeof(POWERS) / sizeof(OctRadius::Power);


void OctRadius::DrawBoard(TileList &tiles, SDL_Surface *screen, struct uistate &uistate) {
	int torus_frame = SDL_GetTicks() / 100 % (TORUS_FRAMES * 2);
	if (torus_frame >= TORUS_FRAMES)
		torus_frame = 2 * TORUS_FRAMES - torus_frame - 1;
	
	SDL_Surface *square = OctRadius::LoadImage("graphics/tile.png");
	assert(square != NULL);
	
	SDL_Surface *pawn_graphics = OctRadius::LoadImage("graphics/pawns.png");
	SDL_Surface *pickup = OctRadius::LoadImage("graphics/pickup.png");
	assert(pawn_graphics && pickup);
	
	assert(SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
	
	TileList::iterator ti = tiles.begin();
	
	for(; ti != tiles.end(); ti++) {
		SDL_Rect rect;
		rect.x = BOARD_OFFSET + TILE_SIZE * (*ti)->col;
		rect.y = BOARD_OFFSET + TILE_SIZE * (*ti)->row;
		rect.w = rect.h = 0;
		
		rect.x += (-1 * (*ti)->height) * 10;
		rect.y += (-1 * (*ti)->height) * 10;
		
		(*ti)->screen_x = rect.x;
		(*ti)->screen_y = rect.y;
		
		assert(SDL_BlitSurface(square, NULL, screen, &rect) == 0);
		
		if((*ti)->power) {
			assert(SDL_BlitSurface(pickup, NULL, screen, &rect) == 0);
		}
		
		if ((*ti)->pawn && (*ti)->pawn != uistate.dpawn) {
			SDL_Rect srect = { (*ti)->pawn->powers.size() ? (torus_frame * 50) : 0, (*ti)->pawn->colour * 50, 50, 50 };
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
	
	uistate.pmenu.clear();
	uistate.pmw = 0;
	uistate.pmh = 0;
	
	if(uistate.mpawn) {
		TTF_Font *font = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 14);
		
		int fh = TTF_FontLineSkip(font), fw = 0;
		
		PowerList::iterator i = uistate.mpawn->powers.begin();
		
		for(; i != uistate.mpawn->powers.end(); i++) {
			int w;
			TTF_SizeText(font, i->first->name, &w, NULL);
			
			if(w > fw) {
				fw = w;
			}
		}
		
		SDL_Rect rect = { uistate.mpawn->OnTile()->screen_x+TILE_SIZE, uistate.mpawn->OnTile()->screen_y, fw+30, uistate.mpawn->powers.size() * fh };
		assert(SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
		
		uistate.pmx = rect.x;
		uistate.pmy = rect.y;
		uistate.pmw = rect.w;
		uistate.pmh = rect.h;
		
		SDL_Color colour = {255,0,0};
		
		for(i = uistate.mpawn->powers.begin(); i != uistate.mpawn->powers.end(); i++) {
			char ns[4];
			sprintf(ns, "%d", i->second);
			
			uistate.pmenu.push_back((struct pmenu_entry){rect.x, rect.y, fw+30, fh, i->first});
			
			FontStuff::BlitText(screen, rect, font, colour, ns);
			
			rect.x += 30;
			
			FontStuff::BlitText(screen, rect, font, colour, i->first->name);
			
			rect.x -= 30;
			rect.y += fh;
		}
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

/* Return the "topmost" tile rendered at the given X,Y co-ordinates or NULL if
 * there is no tile at that location.
*/
OctRadius::Tile *OctRadius::TileAtXY(TileList &tiles, int x, int y) {
	TileList::iterator ti = tiles.end();
	
	do {
		ti--;
		
		int tx = (*ti)->screen_x;
		int ty = (*ti)->screen_y;
		
		if(tx <= x && tx+(int)TILE_SIZE > x && ty <= y && ty+(int)TILE_SIZE > y) {
			return *ti;
		}
	} while(ti != tiles.begin());
	
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

void OctRadius::LoadScenario(std::string filename, TileList &tiles, int &cols, int &rows) {
	std::fstream file(filename.c_str(), std::fstream::in);
	assert(file.is_open());
	
	char buf[1024], *bp;
	
	while(file.good()) {
		file.getline(buf, sizeof(buf));
		buf[strcspn(buf, "\n")] = '\0';
		
		bp = next_value(buf);
		std::string name = buf;
		
		if(name == "GRID") {
			cols = atoi(bp);
			rows = atoi(next_value(bp));
			
			assert(cols > 0 && rows > 0);
			
			for(int c = 0; c < cols; c++) {
				for(int r = 0; r < rows; r++) {
					tiles.push_back(new OctRadius::Tile(c, r));
				}
			}
		}
		if(name == "SPAWN") {
			assert(cols > 0 && rows > 0);
			
			/* SPAWN x y c */
			
			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int c = atoi((bp = next_value(bp)));
			
			Tile *tile = FindTile(tiles, x, y);
			assert(tile);
			
			tile->pawn = new Pawn((OctRadius::Colour)c, tiles, tile);
		}
		if(name == "HOLE") {
			int x = atoi(bp);
			int y = atoi(next_value(bp));
			
			TileList::iterator i = tiles.begin();
			
			while(i != tiles.end()) {
				if((*i)->col == x && (*i)->row == y) {
					delete *i;
					tiles.erase(i);
					
					break;
				}
				
				i++;
			}
		}
	}
}

const OctRadius::Power* OctRadius::ChooseRandomPower() {
	int total = 0;
	for (int i = 0; i < N_POWERS; i++) {
		total += POWERS[i].spawn_rate;
	}
	
	int n = rand() % total;
	for (int i = 0; i < N_POWERS; i++) {
		if (n < POWERS[i].spawn_rate)
			return &POWERS[i];
		n -= POWERS[i].spawn_rate;
	}
}

void OctRadius::SpawnPowers(TileList &tiles, int num) {
	TileList::iterator ti = tiles.begin();
	TileList ctiles;
	
	for(; ti != tiles.end(); ti++) {
		if(!(*ti)->pawn && !(*ti)->power) {
			ctiles.push_back(*ti);
		}
	}
	
	TileList stiles = ChooseRandomTiles(ctiles, num, 1);
	TileList::iterator i = stiles.begin();
	
	for(; i != stiles.end(); i++) {
		(*i)->power = ChooseRandomPower();
		std::cout << "Spawned " << (*i)->power->name << " at (" << (*i)->col << "," << (*i)->row << ")" << std::endl;
	}
}

OctRadius::TileList OctRadius::ChooseRandomTiles(TileList tiles, int num, bool uniq) {
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

OctRadius::Tile *OctRadius::FindTile(TileList &list, int c, int r) {
	TileList::iterator i = list.begin();
	
	while(i != list.end()) {
		if((*i)->col == c && (*i)->row == r) {
			return *i;
		}
		
		i++;
	}
	
	return NULL;
}

int main(int argc, char **argv) {
	if(argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [scenario]" << std::endl;
		return 1;
	}
	
	OctRadius::TileList tiles;
	int cols, rows;
	
	if(argc == 1) {
		OctRadius::LoadScenario("scenario/default.txt", tiles, cols, rows);
	}else{
		OctRadius::LoadScenario(argv[1], tiles, cols, rows);
	}
	
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	assert(TTF_Init() == 0);
	
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
						xd = event.button.x;
						yd = event.button.y;
						
						if(tile->pawn) {
							uistate.dpawn = tile->pawn;
						}
					}
				}
			}else if(event.type == SDL_MOUSEBUTTONUP) {
				OctRadius::Tile *tile = OctRadius::TileAtXY(tiles, event.button.x, event.button.y);
				
				if(event.button.button == SDL_BUTTON_LEFT && xd == event.button.x && yd == event.button.y) {
					if(event.button.x >= uistate.pmx && event.button.x < uistate.pmx+uistate.pmw && event.button.y >= uistate.pmy && event.button.y < uistate.pmy+uistate.pmh) {
						std::vector<struct pmenu_entry>::iterator i = uistate.pmenu.begin();
						
						while(i != uistate.pmenu.end()) {
							if(event.button.x >= (*i).x && event.button.x < (*i).x+(*i).w && event.button.y >= (*i).y && event.button.y < (*i).y+(*i).h) {
								uistate.mpawn->UsePower((*i).power);
							}
							
							i++;
						}
					}
				}
				
				uistate.mpawn = NULL;
				
				if(event.button.button == SDL_BUTTON_LEFT && uistate.dpawn) {
					if(xd == event.button.x && yd == event.button.y) {
						if(tile->pawn->powers.size()) {
							uistate.mpawn = tile->pawn;
						}
					}else if(tile && tile != uistate.dpawn->OnTile()) {
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
	FontStuff::FreeFonts();
	
	SDL_Quit();
	
	return 0;
}
