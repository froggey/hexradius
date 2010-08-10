#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>

namespace OctRadius {
	class Tile {
		public:
			int col, row;
			int height;
			
			int screen_x, screen_y;
			
			Tile(int c, int r) : col(c), row(r), height(0) {}
	};
}

typedef std::vector<OctRadius::Tile> tile_list;
typedef std::vector<tile_list> tile_table;

namespace OctRadius {
	void DrawBoard(tile_table &tiles, SDL_Surface *screen);
	SDL_Surface *LoadImage(std::string filename);
	void FreeImages(void);
}

#define TILE_SIZE 100
#define BOARD_OFFSET 20

void OctRadius::DrawBoard(tile_table &tiles, SDL_Surface *screen) {
	int cols = tiles.size();
	int rows = tiles[0].size();
	
	SDL_Surface *square = OctRadius::LoadImage("graphics/tile.png");
	assert(square != NULL);
	
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
		}
	}
	
	SDL_UpdateRect(screen, 0,0,0,0);
}

static std::map<std::string,SDL_Surface*> image_cache;

SDL_Surface *OctRadius::LoadImage(std::string filename) {
	std::map<std::string,SDL_Surface*>::iterator i = image_cache.find(filename);
	if(i != image_cache.end()) {
		return i->second;
	}
	
	SDL_Surface *s = IMG_Load(filename.c_str());
	if(!s) {
		return NULL;
	}
	
	image_cache.insert(std::make_pair(filename, s));
	return s;
}

void OctRadius::FreeImages(void) {
	std::map<std::string,SDL_Surface*>::iterator i = image_cache.begin();
	
	while(i != image_cache.end()) {
		SDL_FreeSurface(i->second);
		i++;
	}
	
	image_cache.clear();
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
	
	while(1) {
		if(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				break;
			}
			
			if(event.type == SDL_MOUSEBUTTONDOWN) {
				std::cout << "Mouse #" << (int)event.button.button << " pressed at " << event.button.x << "," << event.button.y << std::endl;
				
				OctRadius::Tile *tile = NULL;
				
				for(int c = 0; c < width; c++) {
					for(int r = 0; r < height; r++) {
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
					}
					
					if(event.button.button == SDL_BUTTON_WHEELDOWN && tile->height > -2) {
						std::cout << "Lowering tile" << std::endl;
						
						tile->height--;
						OctRadius::DrawBoard(tiles, screen);
					}
				}
			}
		}
	}
	
	OctRadius::FreeImages();
	
	SDL_Quit();
	
	return 0;
}
