#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <unistd.h>
#include <assert.h>
#include <vector>

namespace OctRadius {
	class Tile {
		public:
			int row;
			int col;
			
			int height;
			
			Tile(int r, int c) {
				row = r;
				col = c;
				height = 0;
			}
	};
}

typedef std::vector<OctRadius::Tile> tile_list;
typedef std::vector<tile_list> tile_table;

int main(int argc, char **argv) {
	if(argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <width> <height>" << std::endl;
		return 1;
	}
	
	int width = atoi(argv[1]), height = atoi(argv[2]);
	
	tile_table tiles;
	int row, col;
	
	for(row = 0; row < height; row++) {
		tile_list rlist;
		
		for(col = 0; col < width; col++) {
			rlist.push_back(OctRadius::Tile(row, col));
		}
		
		tiles.push_back(rlist);
	}
	
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	
	SDL_Surface *screen = SDL_SetVideoMode(width*100, height*100, 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	SDL_WM_SetCaption("OctRadius", "OctRadius");
	
	SDL_Surface *square = IMG_Load("graphics/tile.png");
	assert(square != NULL);
	
	for(row = 0; row < height; row++) {
		for(col = 0; col < width; col++) {
			SDL_Rect rect;
			rect.x = 100*col;
			rect.y = 100*row;
			rect.w = rect.h = 0;
			
			if(row == col) {
				tiles[row][col].height += 2;
			}
			
			rect.x += (-1 * tiles[row][col].height) * 10;
			rect.y += (-1 * tiles[row][col].height) * 10;
			
			assert(SDL_BlitSurface(square, NULL, screen, &rect) == 0);
		}
	}
	
	SDL_UpdateRect(screen, 0,0,0,0);
	
	SDL_Event event;
	
	while(1) {
		if(SDL_PollEvent(&event)) {
			if(event.type == SDL_QUIT) {
				break;
			}
		}
	}
	
	SDL_Quit();
	
	return 0;
}
