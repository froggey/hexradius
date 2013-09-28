#ifndef HR_MAP_HPP
#define HR_MAP_HPP

#include <utility>
#include <map>
#include <string>
#include <vector>
#include <functional>

#include "octradius.hpp"

namespace HexRadius
{
	typedef std::pair<int,int> Position;
	
	class Map
	{
		public:
			class Tile
			{
				public:
					enum Type {
						NORMAL,
						BROKEN,
						BHOLE
					};
					
					const Position pos;
					int height;
					
					Type type;
					
					bool has_pawn;
					PlayerColour pawn_colour;
					
					bool has_landing_pad;
					PlayerColour landing_pad_colour;
					
					bool has_mine;
					PlayerColour mine_colour;
					
					Tile(const Position &_pos, int _height = 0);
			};
			
			std::map<Position,Tile> tiles;
			
			unsigned int width() const;
			unsigned int height() const;
			
			Tile *get_tile(const Position &pos);
			Tile *touch_tile(const Position &pos);
			
			void load(const std::string &filename);
			void save(const std::string &filename) const;
	};
}

#endif /* !HR_MAP_HPP */
