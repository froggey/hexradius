#ifndef OR_NETWORK_HPP
#define OR_NETWORK_HPP

#include <queue>
#include <stdint.h>
#include <string>
#include <set>
#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include "octradius.pb.h"
#include "octradius.hpp"

class Server {
	struct Client {
		typedef boost::shared_ptr<Server::Client> ptr;
		
		Client(boost::asio::io_service &io_service) : socket(io_service) {}
		
		boost::asio::ip::tcp::socket socket;
		
		uint32_t msgsize;
		std::string msgbuf;
		
		std::string playername;
		OctRadius::Colour colour;
	};
	
	public:
		Server(uint16_t port);
		
	private:
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::acceptor acceptor;
		
		std::set<Server::Client::ptr> clients;
		
		void StartAccept(void);
		void HandleAccept(Server::Client::ptr client, const boost::system::error_code& err);
		
		void ReadSize(Server::Client::ptr client);
		void ReadMessage(Server::Client::ptr client, const boost::system::error_code& error);
		void HandleMessage(Server::Client::ptr client, const boost::system::error_code& error);
		
		void WriteProto(Server::Client::ptr client, protocol::message &msg);
		void WriteFinish(Server::Client:: ptr client, const boost::system::error_code& error);
};

#endif /* !OR_NETWORK_HPP */
