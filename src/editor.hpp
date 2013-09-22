#ifndef HR_EDITOR_HPP
#define HR_EDITOR_HPP

#include <stdlib.h>

const unsigned int HR_MAP_CMD_TILE  = 1;
const unsigned int HR_MAP_CMD_BREAK = 2;
const unsigned int HR_MAP_CMD_BHOLE = 3;
const unsigned int HR_MAP_CMD_PAWN  = 4;
const unsigned int HR_MAP_CMD_PAD   = 5;
const unsigned int HR_MAP_CMD_MINE  = 6;

struct hr_map_cmd
{
	unsigned char cmd;
	
	unsigned char x;
	unsigned char y;
	char extra;
	
	unsigned char pad[28];
	
	hr_map_cmd()
	{
		memset(this, 0, sizeof(*this));
	}
	
	hr_map_cmd(unsigned char _cmd, unsigned char _x, unsigned char _y, char _extra = 0)
	{
		cmd   = _cmd;
		x     = _x;
		y     = _y;
		extra = _extra;
		
		memset(pad, 0, sizeof(pad));
	}
} __attribute((__packed__));

#endif /* !HR_EDITOR_HPP */
