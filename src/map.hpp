#ifndef MAP_HPP
#define MAP_HPP

#include <utility>
#include <map>
#include <string>
#include <vector>
#include <functional>

#include "hexradius.hpp"
#include "tile.hpp"

typedef std::pair<int,int> Position;

class Map
{
public:
	std::map<Position,Tile> tiles;

	unsigned int width() const;
	unsigned int height() const;

	Tile *get_tile(const Position &pos);
	Tile *touch_tile(const Position &pos);

	void load(const std::string &filename);
	void save(const std::string &filename) const;
private:
	void load_old_style(const std::string &filename);
};

#endif /* !MAP_HPP */
