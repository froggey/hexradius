#ifndef OR_CLIENT_HPP
#define OR_CLIENT_HPP

#include <stdint.h>
#include <boost/asio.hpp>
#include <string>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <boost/shared_array.hpp>
#include <set>
#include <boost/thread.hpp>
#include <queue>

#include "octradius.hpp"
#include "tile_anims.hpp"
#include "octradius.pb.h"
#include "gui.hpp"
#include "animator.hpp"
#include "scenario.hpp"

class GameState;

class Client {
	public:
		bool quit;	// Game returned due to application quit

		Client(std::string host, uint16_t port);
		~Client();

		void run();

		TileAnimators::Animator* current_animator;

		void send_begin();
		void change_colour(uint16_t id, PlayerColour colour);
		void add_animator(Animators::Generic* anim);

		GameState *game_state;

	private:
		struct Player {
			std::string name;
			PlayerColour colour;
			uint16_t id;

			bool operator()(const Player left, const Player right) {
				return left.id < right.id;
			}
		};

		typedef std::set<Player,Player> player_set;
		typedef std::set<Animators::Generic*> anim_set;

		boost::asio::io_service io_service;
		boost::asio::ip::tcp::socket socket;
		SDL_TimerID redraw_timer;
		boost::thread network_thread;
		boost::mutex the_mutex;

		uint32_t msgsize;
		std::vector<char> msgbuf;

		std::queue<protocol::message> recv_queue;
		std::queue<send_buf> send_queue;

		PlayerColour my_colour;
		uint16_t my_id, turn;
		player_set players;
		enum { CONNECTING, LOBBY, GAME } state;
		Scenario scenario;

		int screen_w, screen_h;
		unsigned int last_redraw;
		/* Drag origin. */
		int xd, yd;
		SDL_Rect board;
		anim_set animators;
		unsigned int torus_frame;
		double climb_offset;

		/* Pawn currently being dragged. */
		pawn_ptr dpawn;
		/* Pawn currently selected, for the purpose of power usage. */
		pawn_ptr mpawn;
		/* Pawn the mouse is currently over,
		 * only valid if no dpawn or mpawn. */
		pawn_ptr hpawn;

		struct pmenu_entry {
			SDL_Rect rect;
			int power;
		};

		std::vector<pmenu_entry> pmenu;
		SDL_Rect pmenu_area;

		GUI lobby_gui;
		std::vector<boost::shared_ptr<GUI::TextButton> > lobby_players;
		std::vector<boost::shared_ptr<GUI::TextButton> > lobby_buttons;
		std::vector<boost::shared_ptr<GUI::DropDown> > lobby_drops;

		void net_thread_main();
		void connect_callback(const boost::system::error_code& error);

		void WriteProto(const protocol::message &msg);
		void WriteFinish(const boost::system::error_code& error);

		void ReadSize(void);
		void ReadMessage(const boost::system::error_code& error);
		void ReadFinish(const boost::system::error_code& error);

		void handle_message(const protocol::message &msg);
		void handle_message_lobby(const protocol::message &msg);
		void handle_message_game(const protocol::message &msg);

		void DrawScreen(void);
		void DrawPawn(pawn_ptr pawn, SDL_Rect rect, SDL_Rect base);
		void draw_pawn_tile(pawn_ptr pawn, Tile *tile);
		void diag_cols(Tile *htile, int row, int &bs_col, int &fs_col);
		void draw_pmenu(pawn_ptr pawn);
		void draw_power_message(pawn_ptr pawn, Pawn::PowerMessage& pm);

		void lobby_regen();

		Player *get_player(uint16_t id) {
			Player key;
			key.id = id;

			player_set::iterator i = players.find(key);

			return i == players.end() ? NULL : (Player*)&(*i);
		}
};

#endif /* !OR_CLIENT_HPP */
