#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>

#include "loadimage.hpp"

namespace OctRadius {
	enum Color { BLUE, RED, GREEN, YELLOW };
	
	struct Pawn {
		Color color;
		
		Pawn(Color c) : color(c) {}
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
	void DrawBoard(tile_table &tiles, SDL_Surface *screen);
}

const uint TILE_SIZE = 50;
const int BOARD_OFFSET = 20;
const uint TORUS_FRAMES = 11;

void OctRadius::DrawBoard(tile_table &tiles, SDL_Surface *screen) {
	int cols = tiles.size();
	int rows = tiles[0].size();

	int torus_frame = SDL_GetTicks() / 100 % (TORUS_FRAMES * 2);
	if (torus_frame >= TORUS_FRAMES)
		torus_frame = 2 * TORUS_FRAMES - torus_frame - 1;
	
	SDL_Surface *square = OctRadius::LoadImage("graphics/tile.png");
	assert(square != NULL);
	
	SDL_Surface *blue_torii = OctRadius::LoadImage("graphics/pawns/blue.png");
	SDL_Surface *red_torii = OctRadius::LoadImage("graphics/pawns/red.png");
	SDL_Surface *green_torii = OctRadius::LoadImage("graphics/pawns/green.png");
	SDL_Surface *yellow_torii = OctRadius::LoadImage("graphics/pawns/yellow.png");
	assert(blue_torii != NULL && red_torii != NULL && green_torii != NULL && yellow_torii != NULL);
	
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
			
			if (tiles[c][r].pawn) {
				SDL_Rect srect = { torus_frame * 50, 0, 50, 50 };
				switch (tiles[c][r].pawn->color) {
				case BLUE:
					assert(SDL_BlitSurface(blue_torii, &srect, screen, &rect) == 0);
					break;
				case RED:
					assert(SDL_BlitSurface(red_torii, &srect, screen, &rect) == 0);
					break;
				case GREEN:
					assert(SDL_BlitSurface(green_torii, &srect, screen, &rect) == 0);
					break;
				case YELLOW:
					assert(SDL_BlitSurface(yellow_torii, &srect, screen, &rect) == 0);
					break;
				}
			}
		}
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
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
	
	SDL_Surface *screen = SDL_SetVideoMode((width*TILE_SIZE) + (2*BOARD_OFFSET), (width*TILE_SIZE) + (2*BOARD_OFFSET), 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	SDL_WM_SetCaption("OctRadius", "OctRadius");
	
	OctRadius::DrawBoard(tiles, screen);
	
	SDL_Event event;
	
	int last_redraw = SDL_GetTicks();
	
	while(1) {
		if(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				break;
			}
			else if(event.type == SDL_MOUSEBUTTONDOWN) {
				std::cout << "Mouse #" << (int)event.button.button << " pressed at " << event.button.x << "," << event.button.y << std::endl;
				
				OctRadius::Tile *tile = NULL;
				
				for(int c = width-1; c >= 0 && !tile; c--) {
					for(int r = height-1; r >= 0 && !tile; r--) {
						int tile_x = tiles[c][r].screen_x;
						int tile_y = tiles[c][r].screen_y;
						
						if(tile_x <= event.button.x && tile_x+TILE_SIZE > event.button.x && tile_y <= event.button.y && tile_y+TILE_SIZE > event.button.y) {
							tile = &tiles[c][r];
						}
					}
				}
				
				if(tile) {
					std::cout << "Matched tile: " << tile->col << "," << tile->row << std::endl;
					
					if(event.button.button == SDL_BUTTON_WHEELUP && tile->height < 2) {
						std::cout << "Raising tile" << std::endl;
						
						tile->height++;
						OctRadius::DrawBoard(tiles, screen);
						last_redraw = SDL_GetTicks();
					}
					else if(event.button.button == SDL_BUTTON_WHEELDOWN && tile->height > -2) {
						std::cout << "Lowering tile" << std::endl;
						
						tile->height--;
						OctRadius::DrawBoard(tiles, screen);
						last_redraw = SDL_GetTicks();
					}
					else if (event.button.button == SDL_BUTTON_LEFT && tile->pawn == NULL) {
						tile->pawn = new OctRadius::Pawn((OctRadius::Color)(rand() % 4));
					}
				}
			}
		}
		
		// force a redraw if it's been too long (for animations)
		if (SDL_GetTicks() >= last_redraw + 50) {
			OctRadius::DrawBoard(tiles, screen);
			last_redraw = SDL_GetTicks();
		}
	}
	
	OctRadius::FreeImages();
	
	SDL_Quit();
	
	return 0;
}
