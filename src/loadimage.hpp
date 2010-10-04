#ifndef OR_LOADIMAGE_HPP
#define OR_LOADIMAGE_HPP

#include <SDL/SDL.h>

extern SDL_Surface *screen;
extern int screen_w, screen_h;

namespace ImgStuff {
	struct TintValues {
		int r, g, b, a;
		
		TintValues() : r(0), g(0), b(0), a(0) {}
		TintValues(int ar, int ag, int ab, int aa = 0) : r(ar), g(ag), b(ab), a(aa) {}
		
		void Tint(int ar, int ag, int ab, int aa = 0) {
			r += ar;
			g += ag;
			b += ab;
			a += aa;
		}
		
		void Tint(TintValues &t) {
			Tint(t.r, t.g, t.b, t.a);
		}
		
		bool HazTint(void) const {
			return (r || g || b || a);
		}
	};
	
	struct Colour : SDL_Colour {
		Colour() {}
		
		Colour(int ar, int ag, int ab) {
			r = ar;
			g = ag;
			b = ab;
		}
	};
	
	SDL_Surface *GetImage(std::string filename, const TintValues &tint = TintValues(0,0,0,0));
	SDL_Surface *GetImageNC(std::string filename);
	void FreeImages(void);
	Uint32 GetPixel(SDL_Surface *surface, int x, int y);
	void SetPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
	void TintSurface(SDL_Surface *surface, const TintValues &tint);
	Uint32 MapColour(SDL_Colour &colour);
	
	void draw_rect(SDL_Rect rect, const SDL_Colour &colour, uint8_t alpha = 255);
	void set_mode(int w, int h);
}

#endif /* !OR_LOADIMAGE_HPP */
