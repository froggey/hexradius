#include <algorithm>
#include <stdexcept>

#include "map.hpp"

const unsigned char MAP_OP_TILE  = 1;
const unsigned char MAP_OP_BREAK = 2;
const unsigned char MAP_OP_BHOLE = 3;
const unsigned char MAP_OP_SPAWN = 4;
const unsigned char MAP_OP_PAD   = 5;
const unsigned char MAP_OP_MINE  = 6;

struct map_op
{
	unsigned char op;
	
	unsigned char x;
	unsigned char y;
	char extra;
	
	unsigned char pad[28];
	
	map_op()
	{
		memset(this, 0, sizeof(*this));
	}
	
	map_op(unsigned char _op, unsigned char _x, unsigned char _y, char _extra = 0)
	{
		op    = _op;
		x     = _x;
		y     = _y;
		extra = _extra;
		
		memset(pad, 0, sizeof(pad));
	}
} __attribute((__packed__));

unsigned int HexRadius::Map::width() const
{
	unsigned int w = 0;
	
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t)
	{
		w = std::max(w, (unsigned int)(t->second.pos.first + 1));
	}
	
	return w;
}

unsigned int HexRadius::Map::height() const
{
	unsigned int h = 0;
	
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t)
	{
		h = std::max(h, (unsigned int)(t->second.pos.second + 1));
	}
	
	return h;
}

HexRadius::Map::Tile *HexRadius::Map::get_tile(const Position &pos)
{
	std::map<Position,Tile>::iterator t = tiles.find(pos);
	
	return t != tiles.end() ? &(t->second) : NULL;
}

HexRadius::Map::Tile *HexRadius::Map::touch_tile(const Position &pos)
{
	std::map<Position,Tile>::iterator t = tiles.insert(std::make_pair(pos, Tile(pos))).first;
	
	return &(t->second);
}

void HexRadius::Map::load(const std::string &filename)
{
	FILE *fh = fopen(filename.c_str(), "rb");
	if(!fh)
	{
		throw std::runtime_error("Could not open " + filename);
	}
	
	std::map<Position,Tile> new_tiles;
	
	map_op op;
	while(fread(&op, sizeof(op), 1, fh))
	{
		std::map<Position,Tile>::iterator t = new_tiles.find(Position(op.x, op.y));
		
		if(op.op != MAP_OP_TILE && t == tiles.end())
		{
			fprintf(stderr, "Ignoring map_op %d with bad position %d,%d\n", (int)(op.op), (int)(op.x), (int)(op.y));
			continue;
		}
		
		if((op.op == MAP_OP_SPAWN || op.op == MAP_OP_PAD || op.op == MAP_OP_MINE) && (op.extra < 0 || op.extra > 5))
		{
			fprintf(stderr, "Ignoring map_op %d with bad colour %d\n", (int)(op.op), (int)(op.extra));
			continue;
		}
		
		Tile *tile = &(t->second);
		
		switch(op.op)
		{
			case MAP_OP_TILE:
			{
				if(op.extra < -2 || op.extra > 2)
				{
					fprintf(stderr, "Ignoring tile with bad height\n");
					break;
				}
				
				new_tiles.insert(std::make_pair(Position(op.x, op.y), Tile(Position(op.x, op.y), op.extra)));
				break;
			}
			
			case MAP_OP_BREAK:
			{
				tile->type = Tile::BROKEN;
				break;
			}
			
			case MAP_OP_BHOLE:
			{
				tile->type = Tile::BHOLE;
				break;
			}
			
			case MAP_OP_SPAWN:
			{
				tile->has_pawn    = true;
				tile->pawn_colour = (PlayerColour)(op.extra);
				break;
			}
			
			case MAP_OP_PAD:
			{
				tile->has_landing_pad    = true;
				tile->landing_pad_colour = (PlayerColour)(op.extra);
				break;
			}
			
			case MAP_OP_MINE:
			{
				tile->has_mine    = true;
				tile->mine_colour = (PlayerColour)(op.extra);
				break;
			}
			
			default:
			{
				fprintf(stderr, "Ignoring unknown map_op %d\n", (int)(op.op));
				break;
			}
		}
	}
	
	fclose(fh);
	
	if(new_tiles.empty())
	{
		throw std::runtime_error(filename + ": No tiles found in map");
	}
	
	tiles = new_tiles;
}

void HexRadius::Map::save(const std::string &filename) const
{
	FILE *fh = fopen(filename.c_str(), "wb");
	if(!fh)
	{
		throw std::runtime_error("Could not open " + filename);
	}
	
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t)
	{
		const Tile &tile = t->second;
		
		map_op tile_op(MAP_OP_TILE, tile.pos.first, tile.pos.second, tile.height);
		fwrite(&tile_op, sizeof(tile_op), 1, fh);
		
		if(tile.type == Tile::BROKEN)
		{
			map_op break_op(MAP_OP_BREAK, tile.pos.first, tile.pos.second);
			fwrite(&break_op, sizeof(break_op), 1, fh);
		}
		else if(tile.type == Tile::BHOLE)
		{
			map_op bhole_op(MAP_OP_BHOLE, tile.pos.first, tile.pos.second);
			fwrite(&bhole_op, sizeof(bhole_op), 1, fh);
		}
		
		if(tile.has_pawn)
		{
			map_op spawn_op(MAP_OP_SPAWN, tile.pos.first, tile.pos.second, tile.pawn_colour);
			fwrite(&spawn_op, sizeof(spawn_op), 1, fh);
		}
		
		if(tile.has_landing_pad)
		{
			map_op pad_op(MAP_OP_PAD, tile.pos.first, tile.pos.second, tile.landing_pad_colour);
			fwrite(&pad_op, sizeof(pad_op), 1, fh);
		}
		
		if(tile.has_mine)
		{
			map_op mine_op(MAP_OP_MINE, tile.pos.first, tile.pos.second, tile.mine_colour);
			fwrite(&mine_op, sizeof(mine_op), 1, fh);
		}
	}
	
	fclose(fh);
}

HexRadius::Map::Tile::Tile(const Position &_pos, int _height): pos(_pos)
{
	height = _height;
	type   = NORMAL;
	
	has_pawn        = false;
	has_landing_pad = false;
	has_mine        = false;
}
