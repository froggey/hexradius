#include <stdint.h>
#include <string>
#include <boost/bind.hpp>

#include "octradius.hpp"
#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "gui.hpp"
#include "menu.hpp"
#include "editor.hpp"

const int TILE_TYPE_NONE   = 0;
const int TILE_TYPE_NORMAL = 1;
const int TILE_TYPE_BROKEN = 2;
const int TILE_TYPE_BHOLE  = 3;

struct editor_tile
{
	int type;
	int height;
	int pawn_team;
	int pad_team;
	int mine_team;
	
	int screen_x;
	int screen_y;
	
	editor_tile(): type(TILE_TYPE_NORMAL), height(0), pawn_team(0), pad_team(0), mine_team(0) {}
};

const unsigned int TOOLBAR_WIDTH  = 535;
const unsigned int TOOLBAR_HEIGHT = 38;

static GUI::TextBox *mw_text;
static GUI::TextBox *mh_text;
static GUI::TextBox *mp_text;

/* A vector of vectors containing editor_tile structures for each tile within
 * the map boundaries.
 *
 * Contains map_width elements in the first level and map_height in each second
 * level once resize_map has been called.
*/

static std::vector< std::vector<editor_tile> > tiles;

/* Size of map, in tiles */

static unsigned int map_width;
static unsigned int map_height;

/* Screen size in pixels */

static unsigned int screen_width, screen_height;

/* Menu state, base position and tile indexes */

static bool menu_open;
static unsigned int menu_bx, menu_by;
static unsigned int menu_width, menu_height;
static unsigned int menu_tx, menu_ty;

static void resize_map(unsigned int width, unsigned int height)
{
	map_width  = width;
	map_height = height;
	
	tiles.resize(map_width);
	
	for(std::vector< std::vector<editor_tile> >::iterator i = tiles.begin(); i != tiles.end(); ++i)
	{
		i->resize(map_height);
	}
	
	screen_width = std::max(
		2 * BOARD_OFFSET + (map_width  - 1) * TILE_WOFF + TILE_WIDTH + TILE_ROFF,
		TOOLBAR_WIDTH
	);
	
	screen_height = TOOLBAR_HEIGHT + 2 * BOARD_OFFSET + (map_height - 1) * TILE_HOFF + TILE_HEIGHT;
	
	mw_text->set_text(to_string(map_width));
	mh_text->set_text(to_string(map_height));
	
	ImgStuff::set_mode(screen_width, screen_height);
}

static bool change_map_size_cb(const GUI::TextBox &, const SDL_Event &event)
{
	return isdigit(event.key.keysym.sym);
}

static void set_map_width_cb(const GUI::TextBox &text, const SDL_Event &)
{
	int width = atoi(text.text.c_str());
	
	if(width > 0 && width < 256)
	{
		resize_map(width, map_height);
	}
}

static void set_map_height_cb(const GUI::TextBox &text, const SDL_Event &)
{
	int height = atoi(text.text.c_str());
	
	if(height > 0 && height < 256)
	{
		resize_map(map_width, height);
	}
}

static void load_map_cb(const GUI::TextButton &, const SDL_Event &)
{
	std::string path = "scenario/" + mp_text->text;
	
	FILE *fh = fopen(path.c_str(), "rb");
	if(!fh)
	{
		fprintf(stderr, "Could not read %s\n", path.c_str());
		return;
	}
	
	std::vector<hr_map_cmd> cmds;
	hr_map_cmd cmd;
	
	int new_w = 0;
	int new_h = 0;
	
	while(fread(&cmd, sizeof(cmd), 1, fh))
	{
		cmds.push_back(cmd);
	}
	
	for(std::vector<hr_map_cmd>::iterator cmd = cmds.begin(); cmd != cmds.end(); ++cmd)
	{
		if(cmd->cmd == HR_MAP_CMD_TILE)
		{
			new_w = std::max(new_w, (int)(cmd->x + 1));
			new_h = std::max(new_h, (int)(cmd->y + 1));
		}
	}
	
	tiles.clear();
	resize_map(new_w, new_h);
	
	for(unsigned int x = 0; x < map_width; ++x)
	{
		for(unsigned int y = 0; y < map_height; ++y)
		{
			tiles[x][y].type = TILE_TYPE_NONE;
		}
	}
	
	for(std::vector<hr_map_cmd>::iterator cmd = cmds.begin(); cmd != cmds.end(); ++cmd)
	{
		if(cmd->x >= map_width || cmd->y >= map_height)
		{
			printf("Ignoring command %d with bad position %d,%d\n", (int)(cmd->cmd), (int)(cmd->x), (int)(cmd->y));
			continue;
		}
		
		switch(cmd->cmd)
		{
			case HR_MAP_CMD_TILE:
			{
				tiles[cmd->x][cmd->y].type   = TILE_TYPE_NORMAL;
				tiles[cmd->x][cmd->y].height = std::max(-2, std::min(2, (int)(cmd->extra)));
				break;
			}
			
			case HR_MAP_CMD_BREAK:
			{
				tiles[cmd->x][cmd->y].type = TILE_TYPE_BROKEN;
				break;
			}
			
			case HR_MAP_CMD_BHOLE:
			{
				tiles[cmd->x][cmd->y].type = TILE_TYPE_BHOLE;
				break;
			}
			
			case HR_MAP_CMD_PAWN:
			{
				tiles[cmd->x][cmd->y].pawn_team = cmd->extra + 1;
				break;
			}
			
			case HR_MAP_CMD_PAD:
			{
				tiles[cmd->x][cmd->y].pad_team = cmd->extra + 1;
				break;
			}
			
			case HR_MAP_CMD_MINE:
			{
				tiles[cmd->x][cmd->y].mine_team = cmd->extra + 1;
				break;
			}
			
			default:
			{
				printf("Ignoring unknown command %d\n", (int)(cmd->cmd));
				break;
			}
		}
	}
	
	fclose(fh);
	
	printf("Loaded map from %s\n", path.c_str());
}

static void save_map_cb(const GUI::TextButton &, const SDL_Event &)
{
	std::string path = "scenario/" + mp_text->text;
	
	FILE *fh = fopen(path.c_str(), "wb");
	if(!fh)
	{
		fprintf(stderr, "Could not write to %s\n", path.c_str());
		return;
	}
	
	for(unsigned int x = 0; x < map_width; ++x)
	{
		for(unsigned int y = 0; y < map_width; ++y)
		{
			if(tiles[x][y].type != TILE_TYPE_NONE)
			{
				hr_map_cmd cmd(HR_MAP_CMD_TILE, x, y, tiles[x][y].height);
				fwrite(&cmd, sizeof(cmd), 1, fh);
			}
			
			if(tiles[x][y].type == TILE_TYPE_BROKEN)
			{
				hr_map_cmd cmd(HR_MAP_CMD_BREAK, x, y);
				fwrite(&cmd, sizeof(cmd), 1, fh);
			}
			else if(tiles[x][y].type == TILE_TYPE_BHOLE)
			{
				hr_map_cmd cmd(HR_MAP_CMD_BHOLE, x, y);
				fwrite(&cmd, sizeof(cmd), 1, fh);
			}
			
			if(tiles[x][y].pawn_team)
			{
				hr_map_cmd cmd(HR_MAP_CMD_PAWN, x, y, tiles[x][y].pawn_team - 1);
				fwrite(&cmd, sizeof(cmd), 1, fh);
			}
			
			if(tiles[x][y].pad_team)
			{
				hr_map_cmd cmd(HR_MAP_CMD_PAD, x, y, tiles[x][y].pad_team - 1);
				fwrite(&cmd, sizeof(cmd), 1, fh);
			}
			
			if(tiles[x][y].mine_team)
			{
				hr_map_cmd cmd(HR_MAP_CMD_MINE, x, y, tiles[x][y].mine_team - 1);
				fwrite(&cmd, sizeof(cmd), 1, fh);
			}
		}
	}
	
	fclose(fh);
	
	printf("Saved map to %s\n", path.c_str());
}

static void redraw()
{
	/* Load images */
	
	SDL_Surface *tile              = ImgStuff::GetImage("graphics/hextile.png");
	SDL_Surface *smashed_tile      = ImgStuff::GetImage("graphics/hextile-broken.png");
	SDL_Surface *tint_tile         = ImgStuff::GetImage("graphics/hextile.png", ImgStuff::TintValues(0,100,0));
	SDL_Surface *smashed_tint_tile = ImgStuff::GetImage("graphics/hextile-broken.png", ImgStuff::TintValues(0,100,0));
	SDL_Surface *mine              = ImgStuff::GetImage("graphics/mines.png");
	SDL_Surface *landing_pad       = ImgStuff::GetImage("graphics/landingpad.png");
	SDL_Surface *blackhole         = ImgStuff::GetImage("graphics/blackhole.png");
	SDL_Surface *pawns             = ImgStuff::GetImage("graphics/pawns.png");
	
	/* Load fonts, get width/height of a character */
	
	TTF_Font *font  = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 14);
	TTF_Font *bfont = FontStuff::LoadFont("fonts/DejaVuSansMono-Bold.ttf", 14);
	
	int fh = std::max(TTF_FontLineSkip(font), TTF_FontLineSkip(bfont));
	int fw = FontStuff::TextWidth(font, "0");
	
	/* Clear the screen */
	
	ensure_SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
	
	/* Draw the tiles/pawns */
	
	for(unsigned int tx = 0; tx < tiles.size(); ++tx)
	{
		for(unsigned int ty = 0; ty < tiles[tx].size(); ++ty)
		{
			SDL_Rect rect;
			rect.x = BOARD_OFFSET + TILE_WOFF * tx + ((ty % 2) * TILE_ROFF);
			rect.y = BOARD_OFFSET + TILE_HOFF * ty + TOOLBAR_HEIGHT;
			rect.w = rect.h = 0;
			
			rect.x += (-1 * tiles[tx][ty].height) * TILE_HEIGHT_FACTOR;
			rect.y += (-1 * tiles[tx][ty].height) * TILE_HEIGHT_FACTOR;
			
			tiles[tx][ty].screen_x = rect.x;
			tiles[tx][ty].screen_y = rect.y;
			
			bool highlight = (menu_open && menu_tx == tx && menu_ty == ty);
			
			if(tiles[tx][ty].type == TILE_TYPE_BROKEN)
			{
				ensure_SDL_BlitSurface(highlight ? smashed_tint_tile : smashed_tile, NULL, screen, &rect);
			}
			else if(tiles[tx][ty].type != TILE_TYPE_NONE)
			{
				ensure_SDL_BlitSurface(highlight ? tint_tile : tile, NULL, screen, &rect);
			}
			
			if(tiles[tx][ty].type == TILE_TYPE_BHOLE)
			{
				ensure_SDL_BlitSurface(blackhole, NULL, screen, &rect);
			}
			
			if(tiles[tx][ty].mine_team)
			{
				SDL_Rect s;
				s.x = 0;
				s.y = (tiles[tx][ty].mine_team - 1) * 50;
				s.w = s.h = 50;
				
				ensure_SDL_BlitSurface(mine, &s, screen, &rect);
			}
			
			if(tiles[tx][ty].pad_team)
			{
				SDL_Rect s;
				s.x = 0;
				s.y = (tiles[tx][ty].pad_team - 1) * 50;
				s.w = s.h = 50;
				
				ensure_SDL_BlitSurface(landing_pad, &s, screen, &rect);
			}
			
			if(tiles[tx][ty].pawn_team)
			{
				SDL_Rect s;
				s.x = 0;
				s.y = (tiles[tx][ty].pawn_team - 1) * 50;
				s.w = s.h = 50;
				
				ensure_SDL_BlitSurface(pawns, &s, screen, &rect);
			}
		}
	}
	
	/* Draw the menu (if open) */
	
	if(menu_open)
	{
		int tile_type   = tiles[menu_tx][menu_ty].type;
		int tile_height = tiles[menu_tx][menu_ty].height;
		
		/* Calculate the size of the menu area */
		
		int menu_w = strlen("Black hole");
		int menu_h = 6;
		
		if(tiles[menu_tx][menu_ty].type)
		{
			menu_w += strlen(" | Height");
			menu_h += 1;
		}
		
		if(tiles[menu_tx][menu_ty].type == TILE_TYPE_NORMAL)
		{
			menu_w += strlen(" | Spawn  | Landing pad  |       ");
			menu_h += 2;
		}
		
		menu_width  = menu_w * fw;
		menu_height = menu_h * fh;
		
		menu_bx = std::min(menu_bx, screen_width  - menu_width);
		menu_by = std::min(menu_by, screen_height - menu_height);
		
		SDL_Rect rect = { menu_bx, menu_by, menu_width, menu_height };
		
		SDL_Color active_text_colour = { 255, 0,   0, 0 };
		SDL_Color normal_text_colour = { 0,   255, 0, 0 };
		
		ImgStuff::draw_rect(rect, ImgStuff::Colour(0,0,0), 178);
		
		{
			/* Tile type */
			
			SDL_Rect t_rect = rect;
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Tile type");
			t_rect.y += 2 * fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_type == TILE_TYPE_NONE ? active_text_colour : normal_text_colour), "None");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_type == TILE_TYPE_NORMAL ? active_text_colour : normal_text_colour), "Normal");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_type == TILE_TYPE_BROKEN ? active_text_colour : normal_text_colour), "Broken");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_type == TILE_TYPE_BHOLE ? active_text_colour : normal_text_colour), "Black hole");
			t_rect.y += fh;
		}
		
		if(tile_type)
		{
			SDL_Rect t_rect = rect;
			t_rect.x += fw * strlen("Tile type  ");
			
			for(int i = 0; i < menu_h; ++i)
			{
				FontStuff::BlitText(screen, t_rect, font, normal_text_colour, "|");
				t_rect.y += fh;
			}
			
			/* Height */
			
			t_rect = rect;
			t_rect.x += fw * strlen("Tile type  | ");
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Height");
			t_rect.y += 2 * fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_height == 2 ? active_text_colour : normal_text_colour), "+ 2");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_height == 1 ? active_text_colour : normal_text_colour), "+ 1");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_height == 0 ? active_text_colour : normal_text_colour), "  0");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_height == -1 ? active_text_colour : normal_text_colour), "- 1");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile_height == -2 ? active_text_colour : normal_text_colour), "- 2");
			t_rect.y += fh;
		}
		
		if(tile_type == TILE_TYPE_NORMAL)
		{
			SDL_Rect t_rect = rect;
			t_rect.x += fw * strlen("Tile type  | Height ");
			
			for(int i = 0; i < menu_h; ++i)
			{
				FontStuff::BlitText(screen, t_rect, font, normal_text_colour, "|        |             |");
				t_rect.y += fh;
			}
			
			/* Spawn */
			
			t_rect = rect;
			t_rect.x += fw * strlen("Tile type  | Height | ");
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Spawn");
			t_rect.y += 2 * fh;
			
			for(int i = 0; i <= 6; ++i)
			{
				FontStuff::BlitText(screen, t_rect, font, (tiles[menu_tx][menu_ty].pawn_team == i ? active_text_colour : normal_text_colour), (i == 0 ? "None" : std::string("Team " + to_string(i)).c_str()));
				t_rect.y += fh;
			}
			
			/* Landing pad */
			
			t_rect = rect;
			t_rect.x += fw * strlen("Tile type  | Height | Spawn  | ");
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Landing pad");
			t_rect.y += 2 * fh;
			
			for(int i = 0; i <= 6; ++i)
			{
				FontStuff::BlitText(screen, t_rect, font, (tiles[menu_tx][menu_ty].pad_team == i ? active_text_colour : normal_text_colour), (i == 0 ? "None" : std::string("Team " + to_string(i)).c_str()));
				t_rect.y += fh;
			}
			
			/* Mine */
			
			t_rect = rect;
			t_rect.x += fw * strlen("Tile type  | Height | Spawn  | Landing pad | ");
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Mine");
			t_rect.y += 2 * fh;
			
			for(int i = 0; i <= 6; ++i)
			{
				FontStuff::BlitText(screen, t_rect, font, (tiles[menu_tx][menu_ty].mine_team == i ? active_text_colour : normal_text_colour), (i == 0 ? "None" : std::string("Team " + to_string(i)).c_str()));
				t_rect.y += fh;
			}
		}
	}
}

static std::pair<int,int> tile_by_screen(int screen_x, int screen_y)
{
	SDL_Surface *tile = ImgStuff::GetImage("graphics/hextile.png");
	ensure_SDL_LockSurface(tile);
	
	for(int y = map_height - 1; y >= 0; --y)
	{
		for(int x = map_width - 1; x >= 0; --x)
		{
			int tx = tiles[x][y].screen_x;
			int ty = tiles[x][y].screen_y;
			
			if(tx <= screen_x && tx + (int)TILE_WIDTH > screen_x && ty <= screen_y && ty + (int)TILE_HEIGHT > screen_y)
			{
				Uint8 alpha, blah;
				Uint32 pixel = ImgStuff::GetPixel(tile, screen_x - tx, screen_y - ty);
				
				SDL_GetRGBA(pixel, tile->format, &blah, &blah, &blah, &alpha);
				
				if(alpha)
				{
					SDL_UnlockSurface(tile);
					return std::make_pair(x, y);
				}
			}
		}
	}
	
	SDL_UnlockSurface(tile);
	return std::make_pair(-1, -1);
}

void editor_main(const GUI::TextButton &, const SDL_Event &)
{
	/* Get size of a character in the menu */
	
	TTF_Font *font  = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 14);
	int fh          = TTF_FontLineSkip(font);
	int fw          = FontStuff::TextWidth(font, "0");
	
	/* Create the toolbar */
	
	GUI toolbar(0, 0, TOOLBAR_WIDTH, TOOLBAR_HEIGHT);
	toolbar.set_bg_colour(204, 204, 204);
	
	GUI::TextDisplay mw_label(toolbar, 5, 8, "Width:");
	mw_label.colour = ImgStuff::Colour(0, 0, 0);
	
	mw_text = new GUI::TextBox(toolbar, 50, 5, 50, 25, 0);
	mw_text->set_enter_callback(&set_map_width_cb);
	mw_text->set_input_callback(&change_map_size_cb);
	
	GUI::TextDisplay mh_label(toolbar, 120, 8, "Height:");
	mh_label.colour = ImgStuff::Colour(0, 0, 0);
	
	mh_text = new GUI::TextBox(toolbar, 170, 5, 50, 25, 1);
	mh_text->set_enter_callback(&set_map_height_cb);
	mh_text->set_input_callback(&change_map_size_cb);
	
	GUI::TextDisplay mp_label(toolbar, 230, 8, "File:");
	mp_label.colour = ImgStuff::Colour(0, 0, 0);
	
	mp_text = new GUI::TextBox(toolbar, 270, 5, 150, 25, 2);
	mp_text->set_text("hex_2p");
	
	GUI::TextButton load_button(toolbar, 425, 5, 50, 25, 3, "Load", &load_map_cb);
	GUI::TextButton save_button(toolbar, 480, 5, 50, 25, 4, "Save", &save_map_cb);
	
	load_map_cb(load_button, SDL_Event());
	
	menu_open = false;
	
	/* Do all the things */
	
	SDL_Event event;
	
	while(SDL_WaitEvent(&event))
	{
		if(event.type == SDL_QUIT)
		{
			running = false;
			goto END;
		}
		
		if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT)
		{
			int mouse_x, mouse_y;
			SDL_GetMouseState(&mouse_x, &mouse_y);

			if(menu_open &&
			   (unsigned int)mouse_x >= menu_bx &&
			   (unsigned int)mouse_x < menu_bx + menu_width &&
			   (unsigned int)mouse_y >= menu_by &&
			   (unsigned int)mouse_y < menu_by + menu_height)
			{
				/* Click within the menu boundaries */

				int row = (mouse_y - menu_by) / fh;

				if((unsigned int)mouse_x < menu_bx + fw * strlen("Black hole") &&
				   row >= 2 && row <= 5)
				{
					tiles[menu_tx][menu_ty].type = row - 2;
					
					if(tiles[menu_tx][menu_ty].type == TILE_TYPE_NONE)
					{
						tiles[menu_tx][menu_ty].height    = 0;
					}
					
					if(tiles[menu_tx][menu_ty].type != TILE_TYPE_NORMAL)
					{
						tiles[menu_tx][menu_ty].pawn_team = 0;
						tiles[menu_tx][menu_ty].pad_team  = 0;
						tiles[menu_tx][menu_ty].mine_team = 0;
					}
					
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole |") &&
					(unsigned int)mouse_x < menu_bx + fw * strlen("Black hole | Height ") &&
					row >= 2 && row <= 6)
				{
					tiles[menu_tx][menu_ty].height = -1 * (row - 4);
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height |") &&
					(unsigned int)mouse_x < menu_bx + fw * strlen("Black hole | Height | Spawn  ") &&
					row >= 2)
				{
					tiles[menu_tx][menu_ty].pawn_team = row - 2;
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height | Spawn  |") &&
					(unsigned int)mouse_x < menu_bx + fw * strlen("Black hole | Height | Spawn  | Landing pad ") &&
					row >= 2)
				{
					tiles[menu_tx][menu_ty].pad_team = row - 2;
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height | Spawn  | Landing pad |") &&
					row >= 2)
				{
					tiles[menu_tx][menu_ty].mine_team = row - 2;
					menu_open = false;
				}
			}
			else{
				std::pair<int,int> tile_coords = tile_by_screen(mouse_x, mouse_y);
				
				if(tile_coords.first >= 0)
				{
					menu_open = true;
					
					menu_by = mouse_y;
					menu_bx = mouse_x;
					
					menu_tx = tile_coords.first;
					menu_ty = tile_coords.second;
				}
			}
		}
		
		toolbar.handle_event(event);
		
		SDL_Rect tb_rect = {
			0, 0,
			screen_width, TOOLBAR_HEIGHT
		};
		
		ensure_SDL_FillRect(screen, &tb_rect, SDL_MapRGB(screen->format, 204, 204, 204));
		toolbar.redraw();
		
		redraw();
	}
	
	END:
	
	delete mp_text;
	delete mh_text;
	delete mw_text;
}
