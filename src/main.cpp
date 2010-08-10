#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>

#include "loadimage.hpp"

namespace OctRadius {
	enum Colour { BLUE, RED, GREEN, YELLOW };
	
	struct Pawn {
		Colour colour;
		
		Pawn(Colour c) : colour(c) {}
	};
	
	class Tile {
		public:
			int col, row;
			int height;
			
			int screen_x, screen_y;
			
			Pawn* pawn;
			
			Tile(int c, int r) : col(c), row(r), height(0), pawn(NULL) {}
	};
}

typedef std::vector<OctRadius::Tile> tile_list;
typedef std::vector<tile_list> tile_table;

namespace OctRadius {
	void DrawBoard(tile_table &tiles, SDL_Surface *screen, OctRadius::Pawn *dpawn);
	OctRadius::Tile *TileAtXY(tile_table &tiles, int x, int y);
}

const uint TILE_SIZE = 50;
const uint BOARD_OFFSET = 20;
const uint TORUS_FRAMES = 11;

void OctRadius::DrawBoard(tile_table &tiles, SDL_Surface *screen, OctRadius::Pawn *dpawn) {
	int cols = tiles.size();
	int rows = tiles[0].size();

	int torus_frame = SDL_GetTicks() / 100 % (TORUS_FRAMES * 2);
	if (torus_frame >= TORUS_FRAMES)
		torus_frame = 2 * TORUS_FRAMES - torus_frame - 1;
	
	SDL_Surface *square = OctRadius::LoadImage("graphics/tile.png");
	assert(square != NULL);
	
	SDL_Surface *pawn_graphics = OctRadius::LoadImage("graphics/pawns.png");
	assert(pawn_graphics);
	
	assert(SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
	
	for(int r = 0; r < rows; r++) {
		for(int c = 0; c < cols; c++) {
			SDL_Rect rect;
			rect.x = BOARD_OFFSET + TILE_SIZE * c;
			rect.y = BOARD_OFFSET + TILE_SIZE * r;
			rect.w = rect.h = 0;
			
			rect.x += (-1 * tiles[c][r].height) * 10;
			rect.y += (-1 * tiles[c][r].height) * 10;
			
			tiles[c][r].screen_x = rect.x;
			tiles[c][r].screen_y = rect.y;
			
			assert(SDL_BlitSurface(square, NULL, screen, &rect) == 0);
			
			if (tiles[c][r].pawn && tiles[c][r].pawn != dpawn) {
				SDL_Rect srect = { torus_frame * 50, tiles[c][r].pawn->colour*50, 50, 50 };
				assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
			}
		}
	}
	
	if(dpawn) {
		int mouse_x, mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);
		
		SDL_Rect srect = { torus_frame * 50, dpawn->colour*50, 50, 50 };
		SDL_Rect rect = { mouse_x-30, mouse_y-30, 0, 0 };
		
		assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

/* Return the "topmost" tile rendered at the given X,Y co-ordinates or NULL if
 * there is no tile at that location.
*/
OctRadius::Tile *OctRadius::TileAtXY(tile_table &tiles, int x, int y) {
	int cols = tiles.size();
	int rows = tiles[0].size();
	
	for(int c = cols-1; c >= 0; c--) {
		for(int r = rows-1; r >= 0; r--) {
			int tx = tiles[c][r].screen_x;
			int ty = tiles[c][r].screen_y;
			
			if(tx <= x && tx+(int)TILE_SIZE > x && ty <= y && ty+(int)TILE_SIZE > y) {
				return &tiles[c][r];
			}
		}
	}
	
	return NULL;
}

int main(int argc, char **argv) {
	if(argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <width> <height>" << std::endl;
		return 1;
	}
	
	int width = atoi(argv[1]), height = atoi(argv[2]);
	
	tile_table tiles;
	int row, col;
	
	for(col = 0; col < width; col++) {
		tile_list rlist;
		
		for(row = 0; row < height; row++) {
			rlist.push_back(OctRadius::Tile(col, row));
		}
		
		tiles.push_back(rlist);
	}
	
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	
	SDL_Surface *screen = SDL_SetVideoMode((width*TILE_SIZE) + (2*BOARD_OFFSET), (height*TILE_SIZE) + (2*BOARD_OFFSET), 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	SDL_WM_SetCaption("OctRadius", "OctRadius");
	
	//OctRadius::DrawBoard(tiles, screen);
	
	SDL_Event event;
	
	uint last_redraw = 0;
	OctRadius::Pawn *dpawn = NULL;
	
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
						OctRadius::DrawBoard(tiles, screen, dpawn);
						last_redraw = SDL_GetTicks();
					}
					else if(event.button.button == SDL_BUTTON_WHEELDOWN && tile->height > -2) {
						tile->height--;
						OctRadius::DrawBoard(tiles, screen, dpawn);
						last_redraw = SDL_GetTicks();
					}
					else if (event.button.button == SDL_BUTTON_LEFT) {
						if(tile->pawn) {
							dpawn = tile->pawn;
						}else{
							tile->pawn = new OctRadius::Pawn((OctRadius::Colour)(rand() % 4));
						}
					}
				}
			}else if(event.type == SDL_MOUSEBUTTONUP) {
				OctRadius::Tile *tile = OctRadius::TileAtXY(tiles, event.button.x, event.button.y);
				
				if(event.button.button == SDL_BUTTON_LEFT && dpawn) {
					if(tile && (!tile->pawn || tile->pawn->colour != dpawn->colour)) {
						if(tile->pawn && tile->pawn->colour != dpawn->colour) {
							std::cout << "Pawn at (" << tile->col << "," << tile->row << ") destroyed" << std::endl;
							
							delete tile->pawn;
							tile->pawn = NULL;
						}
						
						for(int c = 0; c < width; c++) {
							for(int r = 0; r < height; r++) {
								if(tiles[c][r].pawn == dpawn) {
									tiles[c][r].pawn = NULL;
								}
							}
						}
						
						tile->pawn = dpawn;
					}
					
					dpawn = NULL;
				}
			}else if(event.type == SDL_MOUSEMOTION && dpawn) {
				OctRadius::DrawBoard(tiles, screen, dpawn);
				last_redraw = SDL_GetTicks();
			}
		}
		
		// force a redraw if it's been too long (for animations)
		if (SDL_GetTicks() >= last_redraw + 50) {
			OctRadius::DrawBoard(tiles, screen, dpawn);
			last_redraw = SDL_GetTicks();
		}
	}
	
	OctRadius::FreeImages();
	
	SDL_Quit();
	
	return 0;
}
