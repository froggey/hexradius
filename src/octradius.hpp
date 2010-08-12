#ifndef OR_OCTRADIUS_HPP
#define OR_OCTRADIUS_HPP

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

const int PWR_ARMOUR = 1<<0;
const int PWR_CLIMB = 1<<1;

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
			int range, flags;
			
			Pawn(Colour c, TileList &tt, Tile *tile) : colour(c), m_tiles(tt), m_tile(tile), range(0), flags(0) {}
			
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
	SDL_Rect rect;
	const OctRadius::Power *power;
};

struct uistate {
	OctRadius::Pawn *dpawn;
	OctRadius::Pawn *mpawn;
	
	std::vector<struct pmenu_entry> pmenu;
	SDL_Rect pmenu_area;
	
	uistate() : dpawn(NULL), mpawn(NULL), pmenu_area((SDL_Rect){0,0,0,0}) {}
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

#endif