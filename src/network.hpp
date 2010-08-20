#ifndef OR_NETWORK_HPP
#define OR_NETWORK_HPP

#include <queue>
#include <stdint.h>
#include <string>
#include <set>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

#include "octradius.pb.h"
#include "octradius.hpp"

class Server {
	struct Client {
		typedef boost::shared_ptr<Server::Client> ptr;
		
		Client(boost::asio::io_service &io_service) : socket(io_service), colour(SPECTATE) {}
		
		boost::asio::ip::tcp::socket socket;
		
		uint32_t msgsize;
		std::vector<char> msgbuf;
		
		std::string playername;
		PlayerColour colour;
	};
	
	public:
		Server(uint16_t port, Scenario &s, uint players);
		void DoStuff(void);
		
		~Server() {
			FreeTiles(tiles);
		}
		
	private:
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor;
		
		std::set<Server::Client::ptr> clients;
		Tile::List tiles;
		
		Scenario scenario;
		uint req_players;
		
		std::set<Server::Client::ptr>::iterator turn;
		
		int pspawn_turns;
		int pspawn_num;
		
		void StartAccept(void);
		void HandleAccept(Server::Client::ptr client, const boost::system::error_code& err);
		
		void ReadSize(Server::Client::ptr client);
		void ReadMessage(Server::Client::ptr client, const boost::system::error_code& error);
		void HandleMessage(Server::Client::ptr client, const boost::system::error_code& error);
		
		typedef boost::shared_array<char> wbuf_ptr;
		
		void WriteProto(Server::Client::ptr client, protocol::message &msg);
		void WriteFinish(Server::Client:: ptr client, const boost::system::error_code& error, wbuf_ptr wb);
		void WriteAll(protocol::message &msg);
		
		void StartGame(void);
		void BadMove(Server::Client::ptr client);
		void SendOK(Server::Client::ptr client);
		
		void NextTurn(void);
		void SpawnPowers(void);
};

#endif /* !OR_NETWORK_HPP */
