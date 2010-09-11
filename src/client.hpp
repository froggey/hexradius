#ifndef OR_CLIENT_HPP
#define OR_CLIENT_HPP

#include <stdint.h>
#include <boost/asio.hpp>
#include <string>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <boost/shared_array.hpp>
#include <set>

#include "octradius.hpp"
#include "tile_anims.hpp"
#include "octradius.pb.h"
#include "gui.hpp"

class Client {
	public:
		bool quit, rfalse;
		
		Client(std::string host, uint16_t port, std::string name);
		
		bool DoStuff(void);
		
		TileAnimators::Animator* current_animator;
		
		void send_begin();
		
	private:
		typedef boost::shared_array<char> wbuf_ptr;
		
		struct Player {
			std::string name;
			PlayerColour colour;
			uint16_t id;
			
			bool operator()(const Player left, const Player right) {
				return left.id < right.id;
			}
		};
		
		typedef std::set<Player,Player> player_set;
		
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::socket socket;
		
		uint32_t msgsize;
		std::vector<char> msgbuf;
		
		Tile::List tiles;
		PlayerColour my_colour;
		uint16_t my_id, turn;
		player_set players;
		enum { CONNECTING, LOBBY, GAME } state;
		
		int screen_w, screen_h;
		uint last_redraw;
		int xd, yd;
		SDL_Rect board;
		
		Pawn *dpawn;
		Pawn *mpawn;
		Pawn *hpawn;
		
		struct pmenu_entry {
			SDL_Rect rect;
			int power;
		};
		
		std::vector<pmenu_entry> pmenu;
		SDL_Rect pmenu_area;
		
		GUI lobby_gui;
		std::vector<boost::shared_ptr<GUI::TextDisplay> > lobby_ptexts;
		std::vector<boost::shared_ptr<GUI::ImgButton> > lobby_buttons;
		
		std::string req_name;
		
		void connect_callback(const boost::system::error_code& error);
		
		void WriteProto(const protocol::message &msg);
		void WriteFinish(const boost::system::error_code& error, wbuf_ptr wb);
		
		void ReadSize(void);
		void ReadMessage(const boost::system::error_code& error);
		void ReadFinish(const boost::system::error_code& error);
		
		void DrawScreen(void);
		void DrawPawn(Pawn *pawn, SDL_Rect rect, uint torus_frame, double climb_offset);
		
		void lobby_regen();
		
		Player *get_player(uint16_t id) {
			Player key;
			key.id = id;
			
			player_set::iterator i = players.find(key);
			
			return i == players.end() ? NULL : (Player*)&(*i);
		}
};

#endif /* !OR_CLIENT_HPP */
