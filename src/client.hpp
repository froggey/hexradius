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

class Client {
	public:
		Client(std::string host, uint16_t port, std::string name);
		bool DoStuff(void);
		
		TileAnimators::Animator* current_animator;
		
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
		
		int grid_cols, grid_rows;
		Tile::List tiles;
		PlayerColour my_colour;
		uint16_t my_id, turn;
		player_set players;
		enum { LOBBY, GAME } state;
		
		bool screen_set;
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
		
		void WriteProto(const protocol::message &msg);
		void WriteFinish(const boost::system::error_code& error, wbuf_ptr wb);
		
		void ReadSize(void);
		void ReadMessage(const boost::system::error_code& error);
		void ReadFinish(const boost::system::error_code& error);
		
		void DrawScreen(void);
		void DrawPawn(Pawn *pawn, SDL_Rect rect, uint torus_frame, double climb_offset);
};

#endif /* !OR_CLIENT_HPP */
