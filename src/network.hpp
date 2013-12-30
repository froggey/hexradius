#ifndef NETWORK_HPP
#define NETWORK_HPP

#include <queue>
#include <stdint.h>
#include <string>
#include <set>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <vector>
#include <boost/thread.hpp>

#include "hexradius.pb.h"
#include "hexradius.hpp"

class ServerGameState;
class Tile;

class Server {
	friend class ServerGameState;
	struct Client : public boost::enable_shared_from_this<Server::Client> {
		typedef boost::shared_ptr<Server::Client> ptr;
		typedef void (Server::Client::*write_cb)(const boost::system::error_code&, ptr);

		struct server_send_buf : send_buf {
			write_cb callback;

			server_send_buf(const protocol::message &msg) : send_buf(msg) {}
		};

		Client(boost::asio::io_service &io_service, Server &s) : socket(io_service), server(s), colour(NOINIT), qcalled(false) {}

		boost::asio::ip::tcp::socket socket;
		Server &server;

		uint32_t msgsize;
		std::vector<char> msgbuf;

		std::queue<server_send_buf> send_queue;

		uint16_t id;
		std::string playername;
		PlayerColour colour;

		bool qcalled;

		void BeginRead();
		void BeginRead2(const boost::system::error_code& error, ptr cptr);
		void FinishRead(const boost::system::error_code& error, ptr cptr);

		void FinishWrite(const boost::system::error_code& error, ptr cptr);
		void Write(const protocol::message &msg, write_cb callback = &Server::Client::FinishWrite);
		void WriteBasic(protocol::msgtype type);

		void Quit(const std::string &msg, bool send_to_client = true);
		void FinishQuit(const boost::system::error_code& error, ptr cptr);
	};

	struct client_compare {
		bool operator()(const Server::Client::ptr left, const Server::Client::ptr right) {
			return left->id < right->id;
		}
	};

public:
	Server(uint16_t port, const std::string &scenario_file);
	~Server();

	ServerGameState *game_state;

private:
	typedef std::set<Server::Client::ptr,client_compare> client_set;
	typedef client_set::iterator client_iterator;

	boost::asio::io_service io_service;
	boost::asio::ip::tcp::acceptor acceptor;
	boost::thread worker;

	client_set clients;
	uint16_t idcounter;

	std::string map_name;

	client_iterator turn;
	enum { LOBBY, GAME } state;

	int pspawn_turns;
	int pspawn_num;

	bool fog_of_war;

	bool doing_worm_stuff;
	pawn_ptr worm_pawn;
	Tile *worm_tile;
	int worm_range;
	boost::asio::deadline_timer worm_timer;
	void worm_tick(const boost::system::error_code &/*ec*/);

	void worker_main();

	void StartAccept();
	void HandleAccept(Server::Client::ptr client, const boost::system::error_code& err);
	bool HandleMessage(Server::Client::ptr client, const protocol::message &msg);

	typedef boost::shared_array<char> wbuf_ptr;

	void WriteAll(const protocol::message &msg, Server::Client *exempt = NULL);

	void StartGame();
	bool CheckForGameOver();

	void NextTurn();
	void SpawnPowers();
	void black_hole_suck();
	// Suck pawn towards the black hole's tile.
	void black_hole_suck_pawn(Tile *tile, pawn_ptr pawn);

	bool handle_msg_lobby(Server::Client::ptr client, const protocol::message &msg);
	bool handle_msg_game(Server::Client::ptr client, const protocol::message &msg);

	Server::Client *get_client(uint16_t id);

	// Send an UPDATE message for one pawn.
	void update_one_pawn(pawn_ptr pawn);
	// Send an UPDATE message for one tile.
	void update_one_tile(Tile *tile);
};

#endif /* !NETWORK_HPP */
