#include <stdint.h>
#include <boost/asio.hpp>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <stdexcept>

#include "octradius.hpp"
#include "octradius.pb.h"
#include "client.hpp"
#include "loadimage.hpp"
#include "fontstuff.hpp"
#include "powers.hpp"
#include "tile_anims.hpp"

static int within_rect(SDL_Rect rect, int x, int y) {
	return (x >= rect.x && x < rect.x+rect.w && y >= rect.y && y < rect.y+rect.h);
}

Client::Client(std::string host, uint16_t port, std::string name) : socket(io_service), grid_cols(0), grid_rows(0), turn(SPECTATE), screen(NULL), last_redraw(0), board(SDL_Rect()), dpawn(NULL), mpawn(NULL), hpawn(NULL), pmenu_area(SDL_Rect()), current_animator(NULL) {
	boost::asio::ip::tcp::resolver resolver(io_service);
	boost::asio::ip::tcp::resolver::query query(host, "");
	boost::asio::ip::tcp::resolver::iterator it = resolver.resolve(query);
	
	boost::asio::ip::tcp::endpoint ep = *it;
	ep.port(port);
	socket.connect(ep);
	
	protocol::message msg;
	msg.set_msg(protocol::INIT);
	msg.set_player_name(name);
	
	WriteProto(msg);
	
	ReadSize();
}

void Client::WriteProto(const protocol::message &msg) {
	std::string pb;
	msg.SerializeToString(&pb);
	
	uint32_t psize = htonl(pb.size());
	wbuf_ptr wb(new char[pb.size()+sizeof(psize)]);
	
	memcpy(wb.get(), &psize, sizeof(psize));
	memcpy(wb.get()+sizeof(psize), pb.data(), pb.size());
	
	async_write(socket, boost::asio::buffer(wb.get(), pb.size()+sizeof(psize)), boost::bind(&Client::WriteFinish, this, boost::asio::placeholders::error, wb));
}

void Client::WriteFinish(const boost::system::error_code& error, wbuf_ptr wb) {
	if(error) {
		throw std::runtime_error("Write error: " + error.message());
	}
}

bool Client::DoStuff(void) {
	io_service.poll();
	
	if(!screen && grid_cols && grid_rows) {
		TTF_Font *bfont = FontStuff::LoadFont("fonts/DejaVuSansMono-Bold.ttf", 14);
		int bskip = TTF_FontLineSkip(bfont);
		
		Tile::List::iterator tile = tiles.begin();
		
		board.x = 0;
		board.y = bskip;
		
		for(; tile != tiles.end(); tile++) {
			int w = 2*BOARD_OFFSET + (*tile)->col * TILE_WOFF + TILE_WIDTH + (((*tile)->row % 2) * TILE_ROFF);
			int h = 2*BOARD_OFFSET + (*tile)->row * TILE_HOFF + TILE_HEIGHT;
			
			if(board.w < w) {
				board.w = w;
			}
			if(board.h < h) {
				board.h = h;
			}
		}
		
		screen = SDL_SetVideoMode(board.w, board.h+bskip, 0, SDL_SWSURFACE);
		assert(screen != NULL);
		
		SDL_WM_SetCaption("OctRadius", "OctRadius");
	}
	
	if(!screen) {
		return true;
	}
	
	SDL_Event event;
	
	if(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			exit(0);
		}
		else if(event.type == SDL_MOUSEBUTTONDOWN && turn == mycolour && !current_animator) {
			Tile *tile = TileAtXY(tiles, event.button.x, event.button.y);
			
			if(event.button.button == SDL_BUTTON_LEFT) {
				xd = event.button.x;
				yd = event.button.y;
				
				if(tile && tile->pawn && tile->pawn->colour == mycolour) {
					dpawn = tile->pawn;
				}
			}
		}
		else if(event.type == SDL_MOUSEBUTTONUP && turn == mycolour && !current_animator) {
			Tile *tile = TileAtXY(tiles, event.button.x, event.button.y);
			
			if(event.button.button == SDL_BUTTON_LEFT && xd == event.button.x && yd == event.button.y) {
				if(within_rect(pmenu_area, event.button.x, event.button.y)) {
					std::vector<pmenu_entry>::iterator i = pmenu.begin();
					
					while(i != pmenu.end()) {
						if(within_rect((*i).rect, event.button.x, event.button.y)) {
							protocol::message msg;
							msg.set_msg(protocol::USE);
							
							msg.add_pawns();
							mpawn->CopyToProto(msg.mutable_pawns(0), false);
							msg.mutable_pawns(0)->set_use_power((*i).power);
							
							WriteProto(msg);
							break;
						}
						
						i++;
					}
					
					dpawn = NULL;
				}
			}
			
			mpawn = NULL;
			
			if(event.button.button == SDL_BUTTON_LEFT && dpawn) {
				if(xd == event.button.x && yd == event.button.y) {
					if(tile->pawn->powers.size()) {
						mpawn = tile->pawn;
					}
				}
				else if(tile && tile != dpawn->GetTile()) {
					protocol::message msg;
					msg.set_msg(protocol::MOVE);
					msg.add_pawns();
					msg.mutable_pawns(0)->set_col(dpawn->GetTile()->col);
					msg.mutable_pawns(0)->set_row(dpawn->GetTile()->row);
					msg.mutable_pawns(0)->set_new_col(tile->col);
					msg.mutable_pawns(0)->set_new_row(tile->row);
					
					WriteProto(msg);
				}
				
				dpawn = NULL;
			}
		}
		else if(event.type == SDL_MOUSEMOTION) {
			Tile *tile = TileAtXY(tiles, event.motion.x, event.motion.y);
			
			if(dpawn) {
				last_redraw = 0;
			}
			else if(!mpawn && tile && tile->pawn) {
				if(hpawn != tile->pawn) {
					hpawn = tile->pawn;
					last_redraw = 0;
				}
			}
			else{
				hpawn = NULL;
			}
		}
		else if(event.type == SDL_KEYDOWN) {
			if(event.key.keysym.scancode == 49) {
				int mouse_x, mouse_y;
				SDL_GetMouseState(&mouse_x, &mouse_y);
				
				Tile *tile = TileAtXY(tiles, mouse_x, mouse_y);
				if(tile) {
					std::cout << "Mouse is over tile " << tile->col << "," << tile->row << std::endl;
				}else{
					std::cout << "The mouse isn't over a tile, you idiot." << std::endl;
				}
			}
		}
	}
	
	// force a redraw if it's been too long (for animations)
	if(SDL_GetTicks() >= last_redraw + 25) {
		DrawScreen();
		last_redraw = SDL_GetTicks();
	}
	
	return true;
}

void Client::ReadSize(void) {
	async_read(socket, boost::asio::buffer(&msgsize, sizeof(uint32_t)), boost::bind(&Client::ReadMessage, this, boost::asio::placeholders::error));
}

void Client::ReadMessage(const boost::system::error_code& error) {
	if(error) {
		throw std::runtime_error("Read error: " + error.message());
	}
	
	msgsize = ntohl(msgsize);
	
	if(msgsize > MAX_MSGSIZE) {
		throw std::runtime_error("Recieved oversized message from server");
	}
	
	msgbuf.resize(msgsize);
	async_read(socket, boost::asio::buffer(&(msgbuf[0]), msgsize), boost::bind(&Client::ReadFinish, this, boost::asio::placeholders::error));
}

void Client::ReadFinish(const boost::system::error_code& error) {
	if(error) {
		throw std::runtime_error("Read error: " + error.message());
	}
	
	std::string msgstring(msgbuf.begin(), msgbuf.end());
	
	protocol::message msg;
	if(!msg.ParseFromString(msgstring)) {
		throw std::runtime_error("Invalid protobuf recieved from server");
	}
	
	if(msg.msg() == protocol::BEGIN) {
		grid_cols = msg.grid_cols();
		grid_rows = msg.grid_rows();
		
		for(int i = 0; i < msg.tiles_size(); i++) {
			tiles.push_back(new Tile(msg.tiles(i).col(), msg.tiles(i).row(), msg.tiles(i).height()));
		}
		
		for(int i = 0; i < msg.pawns_size(); i++) {
			Tile *tile = FindTile(tiles, msg.pawns(i).col(), msg.pawns(i).row());
			if(!tile) {
				continue;
			}
			
			tile->pawn = new Pawn((PlayerColour)msg.pawns(i).colour(), tiles, tile);
		}
		
		for(int i = 0; i < msg.players_size(); i++) {
			Player p;
			p.name = msg.players(i).name();
			p.colour = (PlayerColour)msg.players(i).colour();
			
			players.push_back(p);
		}
		
		mycolour = (PlayerColour)msg.colour();
	}
	if(msg.msg() == protocol::TURN) {
		turn = (PlayerColour)msg.colour();
		std::cout << "Turn for colour " << turn << std::endl;
	}
	if(msg.msg() == protocol::MOVE && msg.pawns_size() == 1) {
		Pawn *pawn = FindPawn(tiles, msg.pawns(0).col(), msg.pawns(0).row());
		Tile *tile = FindTile(tiles, msg.pawns(0).new_col(), msg.pawns(0).new_row());
		
		if(!(pawn && tile && pawn->Move(tile))) {
			std::cerr << "Invalid move recieved from server! Out of sync?" << std::endl;
		}
	}
	if(msg.msg() == protocol::UPDATE) {
		for(int i = 0; i < msg.tiles_size(); i++) {
			Tile *tile = FindTile(tiles, msg.tiles(i).col(), msg.tiles(i).row());
			if(!tile) {
				continue;
			}
			
			tile->height = msg.tiles(i).height();
			tile->has_power = msg.tiles(i).power();
		}
		
		for(int i = 0; i < msg.pawns_size(); i++) {
			Pawn *pawn = FindPawn(tiles, msg.pawns(i).col(), msg.pawns(i).row());
			if(!pawn) {
				continue;
			}
			
			pawn->flags = msg.pawns(i).flags();
			pawn->powers.clear();
			
			for(int p = 0; p < msg.pawns(i).powers_size(); p++) {
				int index = msg.pawns(i).powers(p).index();
				int num = msg.pawns(i).powers(p).num();
				
				if(index < 0 || index >= Powers::num_powers || num <= 0) {
					continue;
				}
				
				pawn->powers.insert(std::make_pair(index, num));
			}
		}
	}
	if(msg.msg() == protocol::USE && msg.pawns_size() == 1) {
		Pawn *pawn = FindPawn(tiles, msg.pawns(0).col(), msg.pawns(0).row());
		
		if(pawn) {
			int power = msg.pawns(0).use_power();
			
			if(pawn->powers.size()) {
				pawn->UsePower(power, NULL, this);
			}
			else if(power >= 0 && power < Powers::num_powers) {
				Powers::powers[power].func(pawn, NULL, this);
			}
		}
	}
	if(msg.msg() == protocol::PQUIT && msg.players_size() == 1) {
		PlayerColour c = (PlayerColour)msg.players(0).colour();
		
		std::cout << "Team " << c << " quit (" << msg.quit_msg() << ")" << std::endl;
		DestroyTeamPawns(tiles, c);
		
		for(std::vector<Player>::iterator p = players.begin(); p != players.end(); p++) {
			if((*p).colour == c) {
				players.erase(p);
				break;
			}
		}
	}
	if(msg.msg() == protocol::QUIT) {
		std::cout << "You have been disconnected by the server (" << msg.quit_msg() << ")" << std::endl;
		abort();
	}
	
	ReadSize();
}

void Client::DrawScreen() {
	uint torus_frame = SDL_GetTicks() / 100 % (TORUS_FRAMES * 2);
	if (torus_frame >= TORUS_FRAMES)
		torus_frame = 2 * TORUS_FRAMES - torus_frame - 1;
	
	double climb_offset = 2.5+(2.0*sin(SDL_GetTicks() / 300.0));
	
	SDL_Surface *tile = ImgStuff::GetImage("graphics/hextile.png");
	SDL_Surface *tint_tile = ImgStuff::GetImage("graphics/hextile.png", ImgStuff::TintValues(0,100,0));
	SDL_Surface *pickup = ImgStuff::GetImage("graphics/pickup.png");
	
	TTF_Font *font = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 14);
	TTF_Font *bfont = FontStuff::LoadFont("fonts/DejaVuSansMono-Bold.ttf", 14);
	
	if (current_animator) {
		current_animator->do_stuff();
	}
	
	assert(SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
	
	{
		SDL_Rect rect = {0,0,0,0};
		
		SDL_Colour c = {255, 255, 255};
		FontStuff::BlitText(screen, rect, font, c, "Players: ");
		rect.x += FontStuff::TextWidth(font, "Players: ");
		
		std::vector<Player>::iterator p = players.begin();
		
		const SDL_Colour colours[] = {{0,0,255},{255,0,0},{0,255,0},{255,255,0},{160,32,240},{255,165,0}};
		
		for(; p != players.end(); p++) {
			TTF_Font *f = (*p).colour == turn ? bfont : font;
			
			FontStuff::BlitText(screen, rect, f, colours[(*p).colour], (*p).name + " ");
			rect.x += FontStuff::TextWidth(f, (*p).name + " ");
		}
	}
	
	Tile::List::iterator ti = tiles.begin();
	
	int mouse_x, mouse_y;
	SDL_GetMouseState(&mouse_x, &mouse_y);
	Tile *htile = TileAtXY(tiles, mouse_x, mouse_y);
	
	for(; ti != tiles.end(); ti++) {
		SDL_Rect rect;
		rect.x = board.x + BOARD_OFFSET + TILE_WOFF * (*ti)->col + (((*ti)->row % 2) * TILE_ROFF);
		rect.y = board.y + BOARD_OFFSET + TILE_HOFF * (*ti)->row;
		rect.w = rect.h = 0;
		
		if ((*ti)->animating) {
			rect.x += (-1 * (*ti)->anim_height) * TILE_HEIGHT_FACTOR;
			rect.y += (-1 * (*ti)->anim_height) * TILE_HEIGHT_FACTOR;
		}
		else {
			rect.x += (-1 * (*ti)->height) * TILE_HEIGHT_FACTOR;
			rect.y += (-1 * (*ti)->height) * TILE_HEIGHT_FACTOR;
		}
		
		(*ti)->screen_x = rect.x;
		(*ti)->screen_y = rect.y;
		
		assert(SDL_BlitSurface(htile == *ti ? tint_tile : tile, NULL, screen, &rect) == 0);
		
		if((*ti)->has_power) {
			assert(SDL_BlitSurface(pickup, NULL, screen, &rect) == 0);
		}
		
		if((*ti)->pawn && (*ti)->pawn != dpawn) {
			DrawPawn((*ti)->pawn, rect, torus_frame, climb_offset);
		}
	}
	
	if(dpawn) {
		SDL_Rect rect = {mouse_x-30, mouse_y-30, 0, 0};
		DrawPawn(dpawn, rect, torus_frame, climb_offset);
	}
	
	pmenu.clear();
	pmenu_area.w = 0;
	pmenu_area.h = 0;
	
	if(mpawn) {
		int fh = TTF_FontLineSkip(font), fw = 0;
		
		Pawn::PowerList::iterator i = mpawn->powers.begin();
		
		for(; i != mpawn->powers.end(); i++) {
			int w = FontStuff::TextWidth(font, Powers::powers[i->first].name);
			
			if(w > fw) {
				fw = w;
			}
		}
		
		SDL_Rect rect = { mpawn->GetTile()->screen_x+TILE_WIDTH, mpawn->GetTile()->screen_y, fw+30, mpawn->powers.size() * fh };
		assert(SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
		
		pmenu_area = rect;
		rect.h = fh;
		
		SDL_Color colour = {255,0,0};
		
		for(i = mpawn->powers.begin(); i != mpawn->powers.end(); i++) {
			char ns[4];
			sprintf(ns, "%d", i->second);
			
			pmenu_entry foobar = {rect, i->first};
			pmenu.push_back(foobar);
			
			FontStuff::BlitText(screen, rect, font, colour, ns);
			
			rect.x += 30;
			
			FontStuff::BlitText(screen, rect, font, colour, Powers::powers[i->first].name);
			
			rect.x -= 30;
			rect.y += fh;
		}
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}

void Client::DrawPawn(Pawn *pawn, SDL_Rect rect, uint torus_frame, double climb_offset) {
	SDL_Surface *pawn_graphics = ImgStuff::GetImage("graphics/pawns.png");
	SDL_Surface *range_overlay = ImgStuff::GetImage("graphics/upgrades/range.png");
	SDL_Surface *shadow = ImgStuff::GetImage("graphics/shadow.png");
	SDL_Surface *shield = ImgStuff::GetImage("graphics/upgrades/shield.png");
	
	assert(SDL_BlitSurface(shadow, NULL, screen, &rect) == 0);
	
	if(pawn->flags & PWR_CLIMB && pawn != dpawn) {
		rect.x -= climb_offset;
		rect.y -= climb_offset;
	}
	
	if(pawn == hpawn && pawn->colour == mycolour) {
		torus_frame = 10;
	}
	else if(!(pawn->flags & HAS_POWER)) {
		torus_frame = 0;
	}
	
	SDL_Rect srect = { torus_frame * 50, pawn->colour * 50, 50, 50 };
	assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
	
	srect.x = pawn->range * 50;
	srect.y = pawn->colour * 50;
	assert(SDL_BlitSurface(range_overlay, &srect, screen, &rect) == 0);
	
	if(pawn->flags & PWR_SHIELD) {
		assert(SDL_BlitSurface(shield, NULL, screen, &rect) == 0);
	}
	
	if(pawn->flags & PWR_CLIMB && pawn != dpawn) {
		rect.x += climb_offset;
		rect.y += climb_offset;
	}
}
