#include <iostream>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <unistd.h>
#include <assert.h>

int main(int argc, char **argv) {
	if(argc != 3) {
		std::cerr << "Usage: " << argv[0] << " <width> <height>" << std::endl;
		return 1;
	}
	
	int width = atoi(argv[1]), height = atoi(argv[2]);
	
	assert(SDL_Init(SDL_INIT_VIDEO) == 0);
	
	SDL_Surface *screen = SDL_SetVideoMode(width*100, height*100, 0, SDL_SWSURFACE);
	assert(screen != NULL);
	
	SDL_WM_SetCaption("OctRadius", "OctRadius");
	
	SDL_Surface *square = IMG_Load("graphics/square.png");
	assert(square != NULL);
	
	int w, h;
	
	for(w = 0; w < width; w++) {
		for(h = 0; h < height; h++) {
			SDL_Rect rect;
			rect.x = 100*w;
			rect.y = 100*h;
			rect.w = rect.h = 0;
			
			assert(SDL_BlitSurface(square, NULL, screen, &rect) == 0);
		}
	}
	
	SDL_UpdateRect(screen, 0,0,0,0);
	
	SDL_Event event;
	
	while(1) {
		if(SDL_PollEvent(&event)) {
			if(event.type == SDL_KEYDOWN) {
				break;
			}
		}
	}
	
	SDL_Quit();
	
	return 0;
}
