#include <stdint.h>
#include <string>
#include <boost/bind.hpp>

#include "hexradius.hpp"
#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "gui.hpp"
#include "menu.hpp"
#include "map.hpp"
#include "pawn.hpp"

const unsigned int TOOLBAR_WIDTH  = 535;
const unsigned int TOOLBAR_HEIGHT = 38;

static GUI::TextBox *mw_text;
static GUI::TextBox *mh_text;
static GUI::TextBox *mp_text;

static Map map;

static unsigned int map_width, map_height;

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
		2 * BOARD_OFFSET + (map_width - 1) * TILE_WOFF + TILE_WIDTH + TILE_ROFF,
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

	/* Don't allow the map to grow beyond what can be stored */

	if(width > 255)
	{
		return;
	}

	/* Don't allow the map to shrink if it will remove all the tiles */

	bool found = false;

	for(std::map<Position,Tile>::iterator t = map.tiles.begin(); t != map.tiles.end(); ++t)
	{
		if(t->second.col < width)
		{
			found = true;
			break;
		}
	}

	if(!found)
	{
		return;
	}

	/* Delete any tiles which are beyond the new right edge */

	for(std::map<Position,Tile>::iterator t = map.tiles.begin(); t != map.tiles.end();)
	{
		std::map<Position,Tile>::iterator d = t++;

		if(d->second.col >= width)
		{
			map.tiles.erase(d);
		}
	}

	/* Populate any new columns with blank tiles */

	for(int x = map_width; x < width; ++x)
	{
		for(unsigned int y = 0; y < map_height; ++y)
		{
			map.touch_tile(Position(x, y));
		}
	}

	map_width = width;

	map_resized();
}

static void set_map_height_cb(const GUI::TextBox &text, const SDL_Event &)
{
	int height = atoi(text.text.c_str());

	/* Don't allow the map to grow beyond what can be stored */

	if(height > 255)
	{
		return;
	}

	/* Don't allow the map to shrink if it will remove all the tiles */

	bool found = false;

	for(std::map<Position,Tile>::iterator t = map.tiles.begin(); t != map.tiles.end(); ++t)
	{
		if(t->second.row < height)
		{
			found = true;
			break;
		}
	}

	if(!found)
	{
		return;
	}

	/* Delete any tiles which are beyond the new bottom edge */

	for(std::map<Position,Tile>::iterator t = map.tiles.begin(); t != map.tiles.end();)
	{
		std::map<Position,Tile>::iterator d = t++;

		if(d->second.row >= height)
		{
			map.tiles.erase(d);
		}
	}

	/* Populate any new rows with blank tiles */

	for(int y = map_height; y < height; ++y)
	{
		for(unsigned int x = 0; x < map_width; ++x)
		{
			map.touch_tile(Position(x, y));
		}
	}

	map_height = height;

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

	map_width  = map.width();
	map_height = map.height();

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

static bool is_none(Tile *tile)
{
	return !tile;
}

static bool is_normal(Tile *tile)
{
	return tile && !tile->smashed && !tile->has_black_hole;
}

static bool is_black_hole(Tile *tile)
{
	return tile && tile->has_black_hole;
}

static bool is_broken(Tile *tile)
{
	return tile && tile->smashed;
}

static bool has_pawn(Tile *tile)
{
	return tile && tile->pawn;
}

static PlayerColour pawn_colour(Tile *tile)
{
	assert(has_pawn(tile));
	return tile->pawn->colour;
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
			Tile *tile = map.get_tile(Position(tx, ty));
			if(!tile)
			{
				continue;
			}

			SDL_Rect rect;
			rect.x = tile_x_pos(tx, ty, tile->height);
			rect.y = tile_y_pos(tx, ty, tile->height);
			rect.w = rect.h = 0;

			bool highlight = (menu_open && menu_tx == tx && menu_ty == ty);

			if(tile->smashed)
			{
				ensure_SDL_BlitSurface(highlight ? smashed_tint_tile : smashed_tile, NULL, screen, &rect);
			}
			else{
				ensure_SDL_BlitSurface(highlight ? tint_tile : tile_img, NULL, screen, &rect);
			}

			if(tile->has_black_hole)
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

			if(tile->pawn)
			{
				SDL_Rect s;
				s.x = 0;
				s.y = pawn_colour(tile) * 50;
				s.w = s.h = 50;

				ensure_SDL_BlitSurface(pawns, &s, screen, &rect);
			}
		}
	}

	/* Draw the menu (if open) */

	if(menu_open)
	{
		Tile *tile = map.get_tile(Position(menu_tx, menu_ty));

		/* Calculate the size of the menu area */

		int menu_w = strlen("Black hole");
		int menu_h = 6;

		if(tile)
		{
			menu_w += strlen(" | Height");
			menu_h += 1;

			if(!tile->smashed && !tile->has_black_hole)
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

			FontStuff::BlitText(screen, t_rect, font, (is_none(tile) ? active_text_colour : normal_text_colour), "None");
			t_rect.y += fh;

			FontStuff::BlitText(screen, t_rect, font, (is_normal(tile) ? active_text_colour : normal_text_colour), "Normal");
			t_rect.y += fh;

			FontStuff::BlitText(screen, t_rect, font, (is_broken(tile) ? active_text_colour : normal_text_colour), "Broken");
			t_rect.y += fh;

			FontStuff::BlitText(screen, t_rect, font, (is_black_hole(tile) ? active_text_colour : normal_text_colour), "Black hole");
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

		if(is_normal(tile))
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

			_draw_team_list(font, fh, t_rect, has_pawn(tile), has_pawn(tile) ? pawn_colour(tile) : 0);

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

	for(int y = map_height - 1; y >= 0; --y)
	{
		for(int x = map_width - 1; x >= 0; --x)
		{
			Tile *tile = map.get_tile(Position(x, y));

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
	mp_text->set_text("hex-2p");

	GUI::TextButton load_button(toolbar, 425, 5, 50, 25, 3, "Load", &load_map_cb);
	GUI::TextButton save_button(toolbar, 480, 5, 50, 25, 4, "Save", &save_map_cb);

	load_map_cb(load_button, SDL_Event());

	menu_open = false;

	bool use_keyboard_controls = false;

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
				Tile *tile = map.get_tile(Position(menu_tx, menu_ty));

				if((unsigned int)mouse_x < menu_bx + fw * strlen("Black hole") &&
				   row >= 2 && row <= 5)
				{
					if(row == 2)
					{
						map.tiles.erase(Position(menu_tx, menu_ty));
					}
					else{
						Tile *tile = map.touch_tile(Position(menu_tx, menu_ty));

						if(row - 3 == 0) { // normal
							tile->has_black_hole = false;
							tile->smashed = false;
						} else if(row - 3 == 1) { // broken
							tile->has_black_hole = false;
							tile->smashed = true;
						} else if(row - 3 == 2) { // black hole
							tile->has_black_hole = true;
							tile->black_hole_power = 1;
							tile->smashed = false;
						} else assert(!"Bad tile type?");

						if(!is_normal(tile))
						{
							tile->pawn.reset();
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
					if(row == 2) { // Spawn None
						tile->pawn.reset();
					} else {
						tile->pawn = pawn_ptr(new Pawn((PlayerColour)(row - 3), NULL, tile));
					}

					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height | Spawn  |") &&
					(unsigned int)mouse_x < menu_bx + fw * strlen("Black hole | Height | Spawn  | Landing pad ") &&
					row >= 2)
				{
					if(row == 2) { // Landing pad none
						tile->has_landing_pad = false;
					} else {
						tile->has_landing_pad = true;
						tile->landing_pad_colour = (PlayerColour)(row - 3);
					}

					menu_open = false;
				}
				else if((unsigned int)mouse_x >= menu_bx + fw * strlen("Black hole | Height | Spawn  | Landing pad |") &&
					row >= 2)
				{
					if(row == 2) { // Mine none
						tile->has_mine = false;
					} else {
						tile->has_mine = true;
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
		else if (event.type == SDL_KEYDOWN) {
			if (event.key.keysym.unicode == '`') {
				use_keyboard_controls = !use_keyboard_controls;
			}
			else if (use_keyboard_controls) {
				int mouse_x, mouse_y;
				SDL_GetMouseState(&mouse_x, &mouse_y);
				std::pair<int,int> tile_coords = tile_by_screen(mouse_x, mouse_y);
				if (tile_coords.first >= 0) {
					if (event.key.keysym.unicode == 'd') {
						Tile *tile = map.get_tile(Position(tile_coords.first, tile_coords.second));
						if (tile)
							map.tiles.erase(Position(tile_coords.first, tile_coords.second));
					}
					else if (event.key.keysym.unicode == 'a') {
						std::pair<int,int> tile_coords = tile_by_screen(mouse_x, mouse_y);
						Tile *tile = map.touch_tile(Position(tile_coords.first, tile_coords.second));

						tile->smashed         = false;
						tile->has_black_hole  = false;
						tile->has_landing_pad = false;
						tile->has_mine        = false;
						tile->pawn.reset();
					}
					else if (event.key.keysym.unicode >= '0' && event.key.keysym.unicode <= '6') {
						int t = event.key.keysym.unicode - '0';
						std::pair<int,int> tile_coords = tile_by_screen(mouse_x, mouse_y);
						Tile *tile = map.get_tile(Position(tile_coords.first, tile_coords.second));
						if (tile) {
							tile->pawn = pawn_ptr(new Pawn(PlayerColour(t - 1), NULL, tile));
						}
					}
				}
			}
		}

		if (!use_keyboard_controls)
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
