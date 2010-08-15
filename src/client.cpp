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

static int within_rect(SDL_Rect rect, int x, int y) {
	return (x >= rect.x && x < rect.x+rect.w && y >= rect.y && y < rect.y+rect.h);
}

Client::Client(std::string host, uint16_t port, std::string name) : socket(io_service), grid_cols(0), grid_rows(0), turn(NOINIT), screen(NULL), last_redraw(0), dpawn(NULL), mpawn(NULL), pmenu_area((SDL_Rect){0,0,0,0}) {
	boost::asio::ip::tcp::endpoint ep;
	boost::asio::ip::address ip;
	
	ip.from_string(host);
	ep.address(ip);
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
		screen = SDL_SetVideoMode((grid_cols*TILE_SIZE) + (2*BOARD_OFFSET), (grid_rows*TILE_SIZE) + (2*BOARD_OFFSET), 0, SDL_SWSURFACE);
		assert(screen != NULL);
		
		SDL_WM_SetCaption("OctRadius", "OctRadius");
	}
	
	if(!screen) {
		return true;
	}
	
	SDL_Event event;
	
	int xd, yd;
	
	if(SDL_PollEvent(&event)) {
		if(event.type == SDL_QUIT) {
			return false;
		}else if(event.type == SDL_MOUSEBUTTONDOWN && turn == mycolour) {
			Tile *tile = TileAtXY(tiles, event.button.x, event.button.y);
			
			if(event.button.button == SDL_BUTTON_LEFT) {
				xd = event.button.x;
				yd = event.button.y;
				
				if(tile && tile->pawn) {
					dpawn = tile->pawn;
				}
			}
		}else if(event.type == SDL_MOUSEBUTTONUP && turn == mycolour) {
			Tile *tile = TileAtXY(tiles, event.button.x, event.button.y);
			
			if(event.button.button == SDL_BUTTON_LEFT && xd == event.button.x && yd == event.button.y) {
				if(within_rect(pmenu_area, event.button.x, event.button.y)) {
					std::vector<struct pmenu_entry>::iterator i = pmenu.begin();
					
					while(i != pmenu.end()) {
						if(within_rect((*i).rect, event.button.x, event.button.y)) {
							mpawn->UsePower((*i).power);
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
				}else if(tile && tile != dpawn->GetTile()) {
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
		}else if(event.type == SDL_MOUSEMOTION && dpawn) {
			last_redraw = 0;
		}
	}
	
	// force a redraw if it's been too long (for animations)
	if(SDL_GetTicks() >= last_redraw + 50) {
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
		
		mycolour = (PlayerColour)msg.colour();
	}
	if(msg.msg() == protocol::TURN) {
		turn = (PlayerColour)msg.colour();
		std::cout << "Turn for colour " << turn << std::endl;
	}
	if(msg.msg() == protocol::MOVE && msg.pawns_size() == 1) {
		Tile *tile = FindTile(tiles, msg.pawns(0).col(), msg.pawns(0).row());
		Tile *ntile = FindTile(tiles, msg.pawns(0).new_col(), msg.pawns(0).new_row());
		
		if(!(tile && tile->pawn && ntile && tile->pawn->Move(ntile))) {
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
	}
	
	ReadSize();
}

void Client::DrawScreen(void) {
	int torus_frame = SDL_GetTicks() / 100 % (TORUS_FRAMES * 2);
	if (torus_frame >= TORUS_FRAMES)
		torus_frame = 2 * TORUS_FRAMES - torus_frame - 1;
	
	double climb_offset = 2.5+(2.0*sin(SDL_GetTicks() / 300.0));
	
	SDL_Surface *square = OctRadius::LoadImage("graphics/tile.png");
	SDL_Surface *pawn_graphics = OctRadius::LoadImage("graphics/pawns.png");
	SDL_Surface *pickup = OctRadius::LoadImage("graphics/pickup.png");
	SDL_Surface *range_overlay = OctRadius::LoadImage("graphics/upgrades/range.png");
	SDL_Surface *shadow = OctRadius::LoadImage("graphics/shadow.png");
	SDL_Surface *armour = OctRadius::LoadImage("graphics/upgrades/armour.png");
	
	assert(SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
	
	Tile::List::iterator ti = tiles.begin();
	
	for(; ti != tiles.end(); ti++) {
		SDL_Rect rect;
		rect.x = BOARD_OFFSET + TILE_SIZE * (*ti)->col;
		rect.y = BOARD_OFFSET + TILE_SIZE * (*ti)->row;
		rect.w = rect.h = 0;
		
		rect.x += (-1 * (*ti)->height) * 10;
		rect.y += (-1 * (*ti)->height) * 10;
		
		(*ti)->screen_x = rect.x;
		(*ti)->screen_y = rect.y;
		
		assert(SDL_BlitSurface(square, NULL, screen, &rect) == 0);
		
		if((*ti)->has_power) {
			assert(SDL_BlitSurface(pickup, NULL, screen, &rect) == 0);
		}
		
		if ((*ti)->pawn && (*ti)->pawn != dpawn) {
			assert(SDL_BlitSurface(shadow, NULL, screen, &rect) == 0);
			
			if((*ti)->pawn->flags & PWR_CLIMB) {
				rect.x -= climb_offset;
				rect.y -= climb_offset;
			}
			
			SDL_Rect srect = { (*ti)->pawn->powers.size() ? (torus_frame * 50) : 0, (*ti)->pawn->colour * 50, 50, 50 };
			assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
			
			if((*ti)->pawn->flags & PWR_ARMOUR) {
				assert(SDL_BlitSurface(armour, NULL, screen, &rect) == 0);
			}
			
			if((*ti)->pawn->flags & PWR_CLIMB) {
				rect.x += climb_offset;
				rect.y += climb_offset;
			}
			
			srect.x = (*ti)->pawn->range * 50;
			srect.y = 0;
			assert(SDL_BlitSurface(range_overlay, &srect, screen, &rect) == 0);
		}
	}
	
	if(dpawn) {
		int mouse_x, mouse_y;
		SDL_GetMouseState(&mouse_x, &mouse_y);
		
		SDL_Rect srect = { dpawn->powers.size() ? (torus_frame * 50) : 0, dpawn->colour*50, 50, 50 };
		SDL_Rect rect = { mouse_x-30, mouse_y-30, 0, 0 };
		
		assert(SDL_BlitSurface(pawn_graphics, &srect, screen, &rect) == 0);
	}
	
	pmenu.clear();
	pmenu_area.w = 0;
	pmenu_area.h = 0;
	
	if(mpawn) {
		TTF_Font *font = FontStuff::LoadFont("fonts/DejaVuSansMono.ttf", 14);
		
		int fh = TTF_FontLineSkip(font), fw = 0;
		
		Pawn::PowerList::iterator i = mpawn->powers.begin();
		
		for(; i != mpawn->powers.end(); i++) {
			int w;
			TTF_SizeText(font, Powers::powers[i->first].name, &w, NULL);
			
			if(w > fw) {
				fw = w;
			}
		}
		
		SDL_Rect rect = { mpawn->GetTile()->screen_x+TILE_SIZE, mpawn->GetTile()->screen_y, fw+30, mpawn->powers.size() * fh };
		assert(SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format, 0, 0, 0)) != -1);
		
		pmenu_area = rect;
		rect.h = fh;
		
		SDL_Color colour = {255,0,0};
		
		for(i = mpawn->powers.begin(); i != mpawn->powers.end(); i++) {
			char ns[4];
			sprintf(ns, "%d", i->second);
			
			pmenu.push_back((struct pmenu_entry){rect, i->first});
			
			FontStuff::BlitText(screen, rect, font, colour, ns);
			
			rect.x += 30;
			
			FontStuff::BlitText(screen, rect, font, colour, Powers::powers[i->first].name);
			
			rect.x -= 30;
			rect.y += fh;
		}
	}
	
	SDL_UpdateRect(screen, 0, 0, 0, 0);
}
