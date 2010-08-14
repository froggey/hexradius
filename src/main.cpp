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
#include <math.h>

#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "powers.hpp"
#include "octradius.hpp"
#include "network.hpp"
#include "client.hpp"

int within_rect(SDL_Rect rect, int x, int y) {
	return (x >= rect.x && x < rect.x+rect.w && y >= rect.y && y < rect.y+rect.h);
}

#if 0
void OctRadius::Pawn::ToProto(protocol::pawn *p, bool copy_powers) {
	p->set_col(m_tile->col);
	p->set_row(m_tile->row);
	
	p->set_colour((protocol::colour)colour);
	p->set_range(range);
	p->set_flags(flags);
	p->set_has_powers(powers.size() ? true : false);
}
#endif

const uint TILE_SIZE = 50;
const int BOARD_OFFSET = 20;
const int TORUS_FRAMES = 11;

struct pmenu_entry {
	SDL_Rect rect;
	int power;
};

struct uistate {
	Pawn *dpawn;
	Pawn *mpawn;
	
	std::vector<struct pmenu_entry> pmenu;
	SDL_Rect pmenu_area;
	
	uistate() : dpawn(NULL), mpawn(NULL), pmenu_area((SDL_Rect){0,0,0,0}) {}
};

void DrawBoard(Tile::List &tiles, SDL_Surface *screen, uistate &uistate) {
	int torus_frame = SDL_GetTicks() / 100 % (TORUS_FRAMES * 2);
	if (torus_frame >= TORUS_FRAMES)
		torus_frame = 2 * TORUS_FRAMES - torus_frame - 1;
	
	double climb_offset = 2.5+(2.0*sin(SDL_GetTicks() / 300.0));
	
	SDL_Surface *square = OctRadius::LoadImage("graphics/tile.png");
	SDL_Surface *pawn_graphics = OctRadius::LoadImage("graphics/pawns.png");
	SDL_Surface *pickup = OctRadius::LoadImage("graphics/pickup.png");
	SDL_Surface *range_overlay = OctRadius::LoadImage("graphics/upgrades/range.png");
	SDL_Surface *shadow = OctRadius::LoadImage("graphics/shadow.png");
	SDL_Surface *armour = OctRadius::LoadImage("graphics/upgrades/armour.png");
	
	assert(SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
	
	Tile::List::iterator ti = tiles.begin();
	
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
		
		if((*ti)->power >= 0) {
			assert(SDL_BlitSurface(pickup, NULL, screen, &rect) == 0);
		}
		
		if ((*ti)->pawn && (*ti)->pawn != uistate.dpawn) {
			assert(SDL_BlitSurface(shadow, NULL, screen, &rect) == 0);
			
			if((*ti)->pawn->flags & PWR_CLIMB) {
				rect.x -= climb_offset;
				rect.y -= climb_offset;
			}
			
			SDL_Rect srect = { (*ti)->pawn->powers.size() ? (torus_frame * 50) : 0, (*ti)->pawn->colour * 50, 50, 50 };
			assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
			
			if((*ti)->pawn->flags & PWR_ARMOUR) {
				assert(SDL_BlitSurface(armour, NULL, screen, &rect) == 0);
			}
			
			if((*ti)->pawn->flags & PWR_CLIMB) {
				rect.x += climb_offset;
				rect.y += climb_offset;
			}
			
			srect.x = (*ti)->pawn->range * 50;
			srect.y = 0;
			assert(SDL_BlitSurface(range_overlay, &srect, screen, &rect) == 0);
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
	uistate.pmenu_area.w = 0;
	uistate.pmenu_area.h = 0;
	
	if(uistate.mpawn) {
		TTF_Font *font = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 14);
		
		int fh = TTF_FontLineSkip(font), fw = 0;
		
		Pawn::PowerList::iterator i = uistate.mpawn->powers.begin();
		
		for(; i != uistate.mpawn->powers.end(); i++) {
			int w;
			TTF_SizeText(font, Powers::powers[i->first].name, &w, NULL);
			
			if(w > fw) {
				fw = w;
			}
		}
		
		SDL_Rect rect = { uistate.mpawn->GetTile()->screen_x+TILE_SIZE, uistate.mpawn->GetTile()->screen_y, fw+30, uistate.mpawn->powers.size() * fh };
		assert(SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
		
		uistate.pmenu_area = rect;
		rect.h = fh;
		
		SDL_Color colour = {255,0,0};
		
		for(i = uistate.mpawn->powers.begin(); i != uistate.mpawn->powers.end(); i++) {
			char ns[4];
			sprintf(ns, "%d", i->second);
			
			uistate.pmenu.push_back((struct pmenu_entry){rect, i->first});
			
			FontStuff::BlitText(screen, rect, font, colour, ns);
			
			rect.x += 30;
			
			FontStuff::BlitText(screen, rect, font, colour, Powers::powers[i->first].name);
			
			rect.x -= 30;
			rect.y += fh;
		}
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

/* Return the "topmost" tile rendered at the given X,Y co-ordinates or NULL if
 * there is no tile at that location.
*/
Tile *TileAtXY(Tile::List &tiles, int x, int y) {
	Tile::List::iterator ti = tiles.end();
	
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

void LoadScenario(std::string filename, Scenario &sc) {
	std::fstream file(filename.c_str(), std::fstream::in);
	assert(file.is_open());
	
	char buf[1024], *bp;
	
	while(file.good()) {
		file.getline(buf, sizeof(buf));
		buf[strcspn(buf, "\n")] = '\0';
		
		bp = next_value(buf);
		std::string name = buf;
		
		if(name == "GRID") {
			sc.cols = atoi(bp);
			sc.rows = atoi(next_value(bp));
			
			assert(sc.cols > 0 && sc.rows > 0);
			
			for(int c = 0; c < sc.cols; c++) {
				for(int r = 0; r < sc.rows; r++) {
					sc.tiles.push_back(new Tile(c, r, 0));
				}
			}
		}
		if(name == "SPAWN") {
			assert(sc.cols > 0 && sc.rows > 0);
			
			/* SPAWN x y c */
			
			int x = atoi(bp);
			int y = atoi((bp = next_value(bp)));
			int c = atoi((bp = next_value(bp)));
			
			Tile *tile = FindTile(sc.tiles, x, y);
			assert(tile);
			
			tile->pawn = new Pawn((PlayerColour)c, sc.tiles, tile);
		}
		if(name == "HOLE") {
			int x = atoi(bp);
			int y = atoi(next_value(bp));
			
			Tile::List::iterator i = sc.tiles.begin();
			
			while(i != sc.tiles.end()) {
				if((*i)->col == x && (*i)->row == y) {
					delete *i;
					sc.tiles.erase(i);
					
					break;
				}
				
				i++;
			}
		}
	}
}

int ChooseRandomPower(void) {
	int total = 0;
	for (int i = 0; i < Powers::num_powers; i++) {
		total += Powers::powers[i].spawn_rate;
	}
	
	int n = rand() % total;
	for (int i = 0; i < Powers::num_powers; i++) {
		if(n < Powers::powers[i].spawn_rate) {
			return i;
		}
		
		n -= Powers::powers[i].spawn_rate;
	}
	
	abort();
}

void OctRadius::SpawnPowers(Tile::List &tiles, int num) {
	Tile::List::iterator ti = tiles.begin();
	Tile::List ctiles;
	
	for(; ti != tiles.end(); ti++) {
		if(!(*ti)->pawn && (*ti)->power < 0) {
			ctiles.push_back(*ti);
		}
	}
	
	Tile::List stiles = RandomTiles(ctiles, num, 1);
	Tile::List::iterator i = stiles.begin();
	
	for(; i != stiles.end(); i++) {
		(*i)->power = ChooseRandomPower();
	}
}

int main(int argc, char **argv) {
	if(argc > 2) {
		std::cerr << "Usage: " << argv[0] << " [scenario]" << std::endl;
		return 1;
	}
	
	srand(time(NULL));
	
	Scenario scn;
	
	if(argc == 1) {
		LoadScenario("scenario/default.txt", scn);
	}else{
		LoadScenario(argv[1], scn);
	}
	
	int cols = scn.cols, rows = scn.rows;
	Tile::List tiles = scn.tiles;
	
	Server server(9001, scn, 1);
	Client client("127.0.0.1", 9001, "test");
	
	while(1) {
		server.DoStuff();
		client.DoStuff();
	}
	
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	assert(TTF_Init() == 0);
	
	SDL_Surface *screen = SDL_SetVideoMode((cols*TILE_SIZE) + (2*BOARD_OFFSET), (rows*TILE_SIZE) + (2*BOARD_OFFSET), 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	SDL_WM_SetCaption("OctRadius", "OctRadius");
	
	SDL_Event event;
	
	uint last_redraw = 0;
	struct uistate uistate;
	int xd, yd;
	
	while(1) {
		if(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				break;
			}else if(event.type == SDL_MOUSEBUTTONDOWN) {
				Tile *tile = TileAtXY(tiles, event.button.x, event.button.y);
				
				if(event.button.button == SDL_BUTTON_LEFT) {
					xd = event.button.x;
					yd = event.button.y;
					
					if(tile && tile->pawn) {
						uistate.dpawn = tile->pawn;
					}
				}
			}else if(event.type == SDL_MOUSEBUTTONUP) {
				Tile *tile = TileAtXY(tiles, event.button.x, event.button.y);
				
				if(event.button.button == SDL_BUTTON_LEFT && xd == event.button.x && yd == event.button.y) {
					if(within_rect(uistate.pmenu_area, event.button.x, event.button.y)) {
						std::vector<struct pmenu_entry>::iterator i = uistate.pmenu.begin();
						
						while(i != uistate.pmenu.end()) {
							if(within_rect((*i).rect, event.button.x, event.button.y)) {
								uistate.mpawn->UsePower((*i).power);
							}
							
							i++;
						}
						
						uistate.dpawn = NULL;
					}
				}
				
				uistate.mpawn = NULL;
				
				if(event.button.button == SDL_BUTTON_LEFT && uistate.dpawn) {
					if(xd == event.button.x && yd == event.button.y) {
						if(tile->pawn->powers.size()) {
							uistate.mpawn = tile->pawn;
						}
					}else if(tile && tile != uistate.dpawn->GetTile()) {
						uistate.dpawn->Move(tile);
						
						OctRadius::SpawnPowers(tiles, 1);
					}
					
					uistate.dpawn = NULL;
				}
			}else if(event.type == SDL_MOUSEMOTION && uistate.dpawn) {
				last_redraw = 0;
			}
		}
		
		// force a redraw if it's been too long (for animations)
		if (SDL_GetTicks() >= last_redraw + 50) {
			DrawBoard(tiles, screen, uistate);
			last_redraw = SDL_GetTicks();
		}
	}
	
	OctRadius::FreeImages();
	FontStuff::FreeFonts();
	
	SDL_Quit();
	
	return 0;
}
