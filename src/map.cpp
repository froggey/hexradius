#include <algorithm>
#include <stdexcept>

#include "map.hpp"
#include "pawn.hpp"
#include "scenario.hpp"
#include "gamestate.hpp"

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

unsigned int Map::width() const
{
	unsigned int w = 0;
	
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t)
	{
		w = std::max(w, (unsigned int)(t->second.col + 1));
	}
	
	return w;
}

unsigned int Map::height() const
{
	unsigned int h = 0;
	
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t)
	{
		h = std::max(h, (unsigned int)(t->second.row + 1));
	}
	
	return h;
}

Tile *Map::get_tile(const Position &pos)
{
	std::map<Position,Tile>::iterator t = tiles.find(pos);
	
	return t != tiles.end() ? &(t->second) : NULL;
}

Tile *Map::touch_tile(const Position &pos)
{
	std::map<Position,Tile>::iterator t = tiles.insert(std::make_pair(pos, Tile(pos.first, pos.second, 0))).first;
	
	return &(t->second);
}

void Map::load(const std::string &filename)
{
	FILE *fh = fopen(filename.c_str(), "rb");
	if(!fh) {
		throw std::runtime_error("Could not open " + filename);
	}

	// Read map header.
	char hdr[4];
	if(fread(hdr, 4, 1, fh) != 1) {
		fclose(fh);
		throw std::runtime_error("Cannot read header from map " + filename);
	}
	if(memcmp(hdr, "HRM1", 4) != 0) {
		// Not a map1 format, punt to the old loader.
		fclose(fh);
		load_old_style(filename);
		return;
	}

	unsigned char len_bits[4];
	if(fread(len_bits, 4, 1, fh) != 1) {
		fclose(fh);
		throw std::runtime_error("Cannot read length from map " + filename);
	}
	size_t len =
		len_bits[0] |
		(len_bits[1] << 8) |
		(len_bits[2] << 16) |
		(len_bits[3] << 24);
	std::string pb(len, 0);
	if(fread(&pb[0], len, 1, fh) != 1) {
		fclose(fh);
		throw std::runtime_error("Cannot read data from map " + filename);
	}
	fclose(fh);

	Scenario scn;
	protocol::message msg;
	msg.ParseFromString(pb);
	scn.load_proto(msg);

	std::map<Position,Tile> new_tiles;

	for(Tile::List::iterator i = scn.game_state->tiles.begin(); i != scn.game_state->tiles.end(); ++i) {
		std::map<Position,Tile>::iterator t = new_tiles.insert(std::make_pair(Position((*i)->col, (*i)->row), Tile((*i)->col, (*i)->row, 0))).first;
		t->second = **i;
		if(t->second.pawn) {
			t->second.pawn = pawn_ptr(new Pawn(t->second.pawn->colour, NULL, &t->second));
		}
	}

	if(new_tiles.empty())
	{
		throw std::runtime_error(filename + ": No tiles found in map");
	}

	tiles = new_tiles;

	// Fix pawn cur_tile pointers.
	for(std::map<Position,Tile>::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		if(i->second.pawn) {
			i->second.pawn->cur_tile = &i->second;
		}
	}
}

void Map::load_old_style(const std::string &filename)
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

				new_tiles.insert(std::make_pair(Position(op.x, op.y), Tile(op.x, op.y, op.extra)));
				break;
			}
			
			case MAP_OP_BREAK:
			{
				tile->smashed = true;
				break;
			}
			
			case MAP_OP_BHOLE:
			{
				tile->has_black_hole = true;
				tile->black_hole_power = 1;
				break;
			}
			
			case MAP_OP_SPAWN:
			{
				tile->pawn = pawn_ptr(new Pawn((PlayerColour)(op.extra), NULL, tile));
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

	// Fix pawn cur_tile pointers.
	for(std::map<Position,Tile>::iterator i = tiles.begin(); i != tiles.end(); ++i) {
		if(i->second.pawn) {
			i->second.pawn->cur_tile = &i->second;
		}
	}
}

void Map::save(const std::string &filename) const
{
	FILE *fh = fopen(filename.c_str(), "wb");
	if(!fh)
	{
		throw std::runtime_error("Could not open " + filename);
	}

	if(fwrite("HRM1", 4, 1, fh) != 1) {
		fclose(fh);
		throw std::runtime_error("Failed to write magic to map " + filename);
	}

	// Same format as what scenario uses.
	protocol::message msg;
	msg.set_msg(protocol::MAP_DEFINITION);
	for(std::map<Position,Tile>::const_iterator t = tiles.begin(); t != tiles.end(); ++t) {
		const Tile *tile = &t->second;
		msg.add_tiles();
		tile->CopyToProto(msg.mutable_tiles(msg.tiles_size()-1));

		if(tile->pawn) {
			msg.add_pawns();
			tile->pawn->CopyToProto(msg.mutable_pawns(msg.pawns_size()-1), false);
			assert(tile->pawn->cur_tile == tile);
		}
	}

	std::string pb;
	msg.SerializeToString(&pb);

	unsigned char len[4];
	len[0] = pb.size();
	len[1] = (pb.size() >> 8);
	len[2] = (pb.size() >> 16);
	len[3] = (pb.size() >> 24);

	if(fwrite(len, 4, 1, fh) != 1) {
		fclose(fh);
		throw std::runtime_error("Failed to write length to map " + filename);
	}

	if(fwrite(pb.data(), pb.size(), 1, fh) != 1) {
		fclose(fh);
		throw std::runtime_error("Failed to write to map " + filename);
	}

	if(fclose(fh)) {
		throw std::runtime_error("Failed to save map " + filename);
	}
}
