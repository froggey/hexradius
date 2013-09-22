#include <stdint.h>
#include <string>
#include <boost/bind.hpp>

#include "octradius.hpp"
#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "gui.hpp"
#include "menu.hpp"
#include "map.hpp"

using HexRadius::Position;

const unsigned int TOOLBAR_WIDTH  = 535;
const unsigned int TOOLBAR_HEIGHT = 38;

static GUI::TextBox *mw_text;
static GUI::TextBox *mh_text;
static GUI::TextBox *mp_text;

static HexRadius::Map map;

/* Screen size in pixels */

static unsigned int screen_width, screen_height;

/* Menu state, base position and tile indexes */

static bool menu_open;
static unsigned int menu_bx, menu_by;
static unsigned int menu_width, menu_height;
static unsigned int menu_tx, menu_ty;

static unsigned int tile_x_pos(unsigned int tile_x, unsigned int tile_y, int tile_height)
{
	return BOARD_OFFSET
		+ TILE_WOFF * tile_x
		+ (tile_y % 2) * TILE_ROFF
		+ (-1 * tile_height) * TILE_HEIGHT_FACTOR;
}

static unsigned int tile_y_pos(unsigned int, unsigned int tile_y, int tile_height)
{
	return BOARD_OFFSET
		+ TILE_HOFF * tile_y
		+ TOOLBAR_HEIGHT
		+ (-1 * tile_height) * TILE_HEIGHT_FACTOR;
}

static void map_resized()
{
	screen_width = std::max(
		2 * BOARD_OFFSET + (map.width() - 1) * TILE_WOFF + TILE_WIDTH + TILE_ROFF,
		TOOLBAR_WIDTH
	);
	
	screen_height = TOOLBAR_HEIGHT + 2 * BOARD_OFFSET + (map.height() - 1) * TILE_HOFF + TILE_HEIGHT;
	
	mw_text->set_text(to_string(map.width()));
	mh_text->set_text(to_string(map.height()));
	
	ImgStuff::set_mode(screen_width, screen_height);
}

static bool change_map_size_cb(const GUI::TextBox &, const SDL_Event &event)
{
	return isdigit(event.key.keysym.sym);
}

static void set_map_width_cb(const GUI::TextBox &text, const SDL_Event &)
{
	int width = atoi(text.text.c_str());
	
	/* Delete any tiles which are beyond the new right edge */
	
	for(std::map<Position,HexRadius::Map::Tile>::iterator t = map.tiles.begin(); t != map.tiles.end();)
	{
		std::map<Position,HexRadius::Map::Tile>::iterator d = ++t;
		
		if(d->second.pos.first >= width)
		{
			map.tiles.erase(d);
		}
	}
	
	/* Populate any new columns with blank tiles */
	
	for(int x = map.width(); x < width; ++x)
	{
		for(unsigned int y = 0; y < map.width(); ++y)
		{
			map.touch_tile(Position(x, y));
		}
	}
	
	map_resized();
}

static void set_map_height_cb(const GUI::TextBox &text, const SDL_Event &)
{
	int height = atoi(text.text.c_str());
	
	/* Delete any tiles which are beyond the new bottom edge */
	
	for(std::map<Position,HexRadius::Map::Tile>::iterator t = map.tiles.begin(); t != map.tiles.end();)
	{
		std::map<Position,HexRadius::Map::Tile>::iterator d = ++t;
		
		if(d->second.pos.second >= height)
		{
			map.tiles.erase(d);
		}
	}
	
	/* Populate any new rows with blank tiles */
	
	for(int y = map.height(); y < height; ++y)
	{
		for(unsigned int x = 0; x < map.width(); ++x)
		{
			map.touch_tile(Position(x, y));
		}
	}
	
	map_resized();
}

static void load_map_cb(const GUI::TextButton &, const SDL_Event &)
{
	std::string path = "scenario/" + mp_text->text;
	
	try {
		map.load(path);
	}
	catch(const std::runtime_error &e)
	{
		fprintf(stderr, "%s\n", e.what());
		return;
	}
	
	map_resized();
	
	printf("Loaded map from %s\n", path.c_str());
}

static void save_map_cb(const GUI::TextButton &, const SDL_Event &)
{
	std::string path = "scenario/" + mp_text->text;
	
	try {
		map.save(path);
	}
	catch(const std::runtime_error &e)
	{
		fprintf(stderr, "%s\n", e.what());
		return;
	}
	
	printf("Saved map to %s\n", path.c_str());
}

static const SDL_Color active_text_colour = { 255, 0,   0, 0 };
static const SDL_Color normal_text_colour = { 0,   255, 0, 0 };

static void _draw_team_list(TTF_Font *font, int fh, SDL_Rect &rect, bool has, int is)
{
	FontStuff::BlitText(screen, rect, font, (has ? active_text_colour : normal_text_colour), "None");
	rect.y += fh;
	
	for(int i = 0; i < 6; ++i)
	{
		FontStuff::BlitText(screen, rect, font, (has && is == i ? active_text_colour : normal_text_colour), std::string("Team " + to_string(i + 1)).c_str());
		rect.y += fh;
	}
}

static void redraw()
{
	/* Load images */
	
	SDL_Surface *tile_img          = ImgStuff::GetImage("graphics/hextile.png");
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
	
	for(unsigned int tx = 0; tx < map.width(); ++tx)
	{
		for(unsigned int ty = 0; ty < map.height(); ++ty)
		{
			HexRadius::Map::Tile *tile = map.get_tile(Position(tx, ty));
			if(!tile)
			{
				continue;
			}
			
			SDL_Rect rect;
			rect.x = tile_x_pos(tx, ty, tile->height);
			rect.y = tile_y_pos(tx, ty, tile->height);
			rect.w = rect.h = 0;
			
			bool highlight = (menu_open && menu_tx == tx && menu_ty == ty);
			
			if(tile->type == HexRadius::Map::Tile::BROKEN)
			{
				ensure_SDL_BlitSurface(highlight ? smashed_tint_tile : smashed_tile, NULL, screen, &rect);
			}
			else{
				ensure_SDL_BlitSurface(highlight ? tint_tile : tile_img, NULL, screen, &rect);
			}
			
			if(tile->type == HexRadius::Map::Tile::BHOLE)
			{
				ensure_SDL_BlitSurface(blackhole, NULL, screen, &rect);
			}
			
			if(tile->has_mine)
			{
				SDL_Rect s;
				s.x = 0;
				s.y = tile->mine_colour * 50;
				s.w = s.h = 50;
				
				ensure_SDL_BlitSurface(mine, &s, screen, &rect);
			}
			
			if(tile->has_landing_pad)
			{
				SDL_Rect s;
				s.x = 0;
				s.y = tile->landing_pad_colour * 50;
				s.w = s.h = 50;
				
				ensure_SDL_BlitSurface(landing_pad, &s, screen, &rect);
			}
			
			if(tile->has_pawn)
			{
				SDL_Rect s;
				s.x = 0;
				s.y = tile->pawn_colour * 50;
				s.w = s.h = 50;
				
				ensure_SDL_BlitSurface(pawns, &s, screen, &rect);
			}
		}
	}
	
	/* Draw the menu (if open) */
	
	if(menu_open)
	{
		HexRadius::Map::Tile *tile = map.get_tile(Position(menu_tx, menu_ty));
		
		/* Calculate the size of the menu area */
		
		int menu_w = strlen("Black hole");
		int menu_h = 6;
		
		if(tile)
		{
			menu_w += strlen(" | Height");
			menu_h += 1;
			
			if(tile->type == HexRadius::Map::Tile::NORMAL)
			{
				menu_w += strlen(" | Spawn  | Landing pad  |       ");
				menu_h += 2;
			}
		}
		
		menu_width  = menu_w * fw;
		menu_height = menu_h * fh;
		
		menu_bx = std::min(menu_bx, screen_width  - menu_width);
		menu_by = std::min(menu_by, screen_height - menu_height);
		
		SDL_Rect rect = { menu_bx, menu_by, menu_width, menu_height };
		
		ImgStuff::draw_rect(rect, ImgStuff::Colour(0,0,0), 178);
		
		{
			/* Tile type */
			
			SDL_Rect t_rect = rect;
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Tile type");
			t_rect.y += 2 * fh;
			
			FontStuff::BlitText(screen, t_rect, font, (!tile ? active_text_colour : normal_text_colour), "None");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile && tile->type == HexRadius::Map::Tile::NORMAL ? active_text_colour : normal_text_colour), "Normal");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile && tile->type == HexRadius::Map::Tile::BROKEN ? active_text_colour : normal_text_colour), "Broken");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile && tile->type == HexRadius::Map::Tile::BHOLE ? active_text_colour : normal_text_colour), "Black hole");
			t_rect.y += fh;
		}
		
		if(tile)
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
			
			FontStuff::BlitText(screen, t_rect, font, (tile->height == 2 ? active_text_colour : normal_text_colour), "+ 2");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile->height == 1 ? active_text_colour : normal_text_colour), "+ 1");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile->height == 0 ? active_text_colour : normal_text_colour), "  0");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile->height == -1 ? active_text_colour : normal_text_colour), "- 1");
			t_rect.y += fh;
			
			FontStuff::BlitText(screen, t_rect, font, (tile->height == -2 ? active_text_colour : normal_text_colour), "- 2");
			t_rect.y += fh;
		}
		
		if(tile && tile->type == HexRadius::Map::Tile::NORMAL)
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
			
			_draw_team_list(font, fh, t_rect, tile->has_pawn, tile->pawn_colour);
			
			/* Landing pad */
			
			t_rect = rect;
			t_rect.x += fw * strlen("Tile type  | Height | Spawn  | ");
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Landing pad");
			t_rect.y += 2 * fh;
			
			_draw_team_list(font, fh, t_rect, tile->has_landing_pad, tile->landing_pad_colour);
			
			/* Mine */
			
			t_rect = rect;
			t_rect.x += fw * strlen("Tile type  | Height | Spawn  | Landing pad | ");
			
			FontStuff::BlitText(screen, t_rect, bfont, normal_text_colour, "Mine");
			t_rect.y += 2 * fh;
			
			_draw_team_list(font, fh, t_rect, tile->has_mine, tile->mine_colour);
		}
	}
}

static std::pair<int,int> tile_by_screen(int screen_x, int screen_y)
{
	SDL_Surface *tile_img = ImgStuff::GetImage("graphics/hextile.png");
	ensure_SDL_LockSurface(tile_img);
	
	for(int y = map.height() - 1; y >= 0; --y)
	{
		for(int x = map.width() - 1; x >= 0; --x)
		{
			HexRadius::Map::Tile *tile = map.get_tile(Position(x, y));
			
			int tx = tile_x_pos(x, y, tile ? tile->height : 0);
			int ty = tile_y_pos(x, y, tile ? tile->height : 0);
			
			if(tx <= screen_x && tx + (int)TILE_WIDTH > screen_x && ty <= screen_y && ty + (int)TILE_HEIGHT > screen_y)
			{
				Uint8 alpha, blah;
				Uint32 pixel = ImgStuff::GetPixel(tile_img, screen_x - tx, screen_y - ty);
				
				SDL_GetRGBA(pixel, tile_img->format, &blah, &blah, &blah, &alpha);
				
				if(alpha)
				{
					SDL_UnlockSurface(tile_img);
					return std::make_pair(x, y);
				}
			}
		}
	}
	
	SDL_UnlockSurface(tile_img);
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

				/* Pointer to the Tile that the menu was opened
				 * on. Will be NULL for tiles of type "None".
				*/
				HexRadius::Map::Tile *tile = map.get_tile(Position(menu_tx, menu_ty));

				if((unsigned int)mouse_x < menu_bx + fw * strlen("Black hole") &&
				   row >= 2 && row <= 5)
				{
					if(row == 2)
					{
						map.tiles.erase(Position(menu_tx, menu_ty));
					}
					else{
						HexRadius::Map::Tile *tile = map.touch_tile(Position(menu_tx, menu_ty));
						
						tile->type = (HexRadius::Map::Tile::Type)(row - 3);
						
						if(tile->type != HexRadius::Map::Tile::NORMAL)
						{
							tile->has_pawn        = false;
							tile->has_landing_pad = false;
							tile->has_mine        = false;
						}
					}
					
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole |") &&
					(unsigned int)mouse_x < menu_bx + fw * strlen("Black hole | Height ") &&
					row >= 2 && row <= 6)
				{
					tile->height = -1 * (row - 4);
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height |") &&
					(unsigned int)mouse_x < menu_bx + fw * strlen("Black hole | Height | Spawn  ") &&
					row >= 2)
				{
					if((tile->has_pawn = row > 2))
					{
						tile->pawn_colour = (PlayerColour)(row - 3);
					}
					
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height | Spawn  |") &&
					(unsigned int)mouse_x < menu_bx + fw * strlen("Black hole | Height | Spawn  | Landing pad ") &&
					row >= 2)
				{
					if((tile->has_landing_pad = row > 2))
					{
						tile->landing_pad_colour = (PlayerColour)(row - 3);
					}
					
					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height | Spawn  | Landing pad |") &&
					row >= 2)
				{
					if((tile->has_mine = row > 2))
					{
						tile->mine_colour = (PlayerColour)(row - 3);
					}
					
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
