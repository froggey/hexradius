#ifndef OR_FONTSTUFF_HPP
#define OR_FONTSTUFF_HPP
#include <SDL/SDL_ttf.h>
#include <string>

namespace FontStuff {
	TTF_Font *LoadFont(std::string filename, int size);
	void FreeFonts(void);
	int BlitText(SDL_Surface *surface, SDL_Rect rect, TTF_Font *font, SDL_Color colour, std::string text);
	int TextWidth(TTF_Font *font, const std::string &text);
};

#endif /* !OR_FONTSTUFF_HPP */
