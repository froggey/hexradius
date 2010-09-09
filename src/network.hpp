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

#include "octradius.pb.h"
#include "octradius.hpp"

class Server {
	struct Client : public boost::enable_shared_from_this<Server::Client> {
		typedef boost::shared_ptr<Server::Client> ptr;
		typedef boost::shared_array<char> wbuf_ptr;
		typedef void (Server::Client::*write_cb)(const boost::system::error_code&, wbuf_ptr, ptr);
		
		Client(boost::asio::io_service &io_service, Server &s) : socket(io_service), server(s), colour(NOINIT), qcalled(false) {}
		
		boost::asio::ip::tcp::socket socket;
		Server &server;
		
		uint32_t msgsize;
		std::vector<char> msgbuf;
		
		uint16_t id;
		std::string playername;
		PlayerColour colour;
		
		bool qcalled;
		
		void BeginRead();
		void BeginRead2(const boost::system::error_code& error, ptr cptr);
		void FinishRead(const boost::system::error_code& error, ptr cptr);
		
		void FinishWrite(const boost::system::error_code& error, wbuf_ptr wb, ptr cptr);
		void Write(const protocol::message &msg, write_cb callback = &Server::Client::FinishWrite);
		void WriteBasic(protocol::msgtype type);
		
		void Quit(const std::string &msg, bool send_to_client = true);
		void FinishQuit(const boost::system::error_code& error, wbuf_ptr wb, ptr cptr);
	};
	
	struct client_compare {
		bool operator()(const Server::Client::ptr left, const Server::Client::ptr right) {
			return left->id < right->id;
		}
	};
	
	public:
		Server(uint16_t port, Scenario &s);
		void DoStuff(void);
		
		~Server() {
			FreeTiles(tiles);
		}
		
	private:
		typedef std::set<Server::Client::ptr,client_compare> client_set;
		
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor;
		
		client_set clients;
		Tile::List tiles;
		
		Scenario scenario;
		
		std::set<Server::Client::ptr>::iterator turn;
		enum { LOBBY, GAME } state;
		
		int pspawn_turns;
		int pspawn_num;
		
		void StartAccept(void);
		void HandleAccept(Server::Client::ptr client, const boost::system::error_code& err);
		bool HandleMessage(Server::Client::ptr client, const protocol::message &msg);
		
		typedef boost::shared_array<char> wbuf_ptr;
		
		void WriteAll(const protocol::message &msg, Server::Client *exempt = NULL);
		
		void StartGame(void);
		
		void NextTurn(void);
		void SpawnPowers(void);
};

#endif /* !OR_NETWORK_HPP */
