#ifndef OR_LOADIMAGE_HPP
#define OR_LOADIMAGE_HPP

#include <SDL/SDL.h>

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
	};
	
	void TintSurface(SDL_Surface *surface, TintValues &tint);
}

namespace OctRadius {
	SDL_Surface *LoadImage(std::string filename, bool usecache = true);
	void FreeImages(void);
	Uint32 GetPixel(SDL_Surface *surface, int x, int y);
	void SetPixel(SDL_Surface *surface, int x, int y, Uint32 pixel);
}

#endif /* !OR_LOADIMAGE_HPP */
