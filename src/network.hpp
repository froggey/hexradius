#ifndef OR_NETWORK_HPP
#define OR_NETWORK_HPP

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

#include "octradius.pb.h"
#include "octradius.hpp"
#include "scenario.hpp"

class Server {
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
		Server(uint16_t port, Scenario &s);
		~Server();
		
		std::vector<uint32_t> power_rand_vals;
		
		Tile::List tiles;
		
	private:
		typedef std::set<Server::Client::ptr,client_compare> client_set;
		typedef client_set::iterator client_iterator;
		
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor;
		boost::thread worker;
		
		client_set clients;
		uint16_t idcounter;
		
		Scenario scenario;
		
		client_iterator turn;
		enum { LOBBY, GAME } state;
		
		int pspawn_turns;
		int pspawn_num;
		
		void worker_main();
		
		void StartAccept(void);
		void HandleAccept(Server::Client::ptr client, const boost::system::error_code& err);
		bool HandleMessage(Server::Client::ptr client, const protocol::message &msg);
		
		typedef boost::shared_array<char> wbuf_ptr;
		
		void WriteAll(const protocol::message &msg, Server::Client *exempt = NULL);
		
		void StartGame(void);
		
		void NextTurn(void);
		void SpawnPowers(void);
		
		bool handle_msg_lobby(Server::Client::ptr client, const protocol::message &msg);
		bool handle_msg_game(Server::Client::ptr client, const protocol::message &msg);
		
		Server::Client *get_client(uint16_t id);
};

#endif /* !OR_NETWORK_HPP */
