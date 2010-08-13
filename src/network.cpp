#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <iostream>

#include "network.hpp"
#include "octradius.pb.h"

Server::Server(uint16_t port) : acceptor(io_service) {
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	
	acceptor.open(endpoint.protocol());
	acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();
	
	StartAccept();
}

void Server::StartAccept(void) {
	Server::Client::ptr client(new Server::Client(io_service));
	
	acceptor.async_accept(client->socket, boost::bind(&Server::HandleAccept, this, client, boost::asio::placeholders::error));
}

void Server::HandleAccept(Server::Client::ptr client, const boost::system::error_code& err) {
	if(err) {
		throw std::runtime_error("Accept error: " + err.message());
	}
	
	clients.insert(client);
	ReadSize(client);
	
	StartAccept();
}

#define MAX_MSGSIZE 2048 /* This will annoy mike */

void Server::ReadSize(Server::Client::ptr client) {
	async_read(client->socket, boost::asio::buffer(&client->msgsize, sizeof(uint32_t)), boost::bind(&Server::ReadMessage, this, client, boost::asio::placeholders::error));
}

void Server::ReadMessage(Server::Client::ptr client, const boost::system::error_code& error) {
	if(error) {
		std::cerr << "Read error: " + error.message() << std::endl;
		clients.erase(client);
		
		return;
	}
	
	client->msgsize = ntohl(client->msgsize);
	
	if(client->msgsize > MAX_MSGSIZE) {
		std::cerr << "Oversized message" << std::endl;
		clients.erase(client);
		
		return;
	}
	
	client->msgbuf.reserve(client->msgsize);
	
	/* This is probably undefined... */
	
	async_read(client->socket, boost::asio::buffer((char*)client->msgbuf.data(), client->msgsize), boost::bind(&Server::HandleMessage, this, client, boost::asio::placeholders::error));
}

void Server::HandleMessage(Server::Client::ptr client, const boost::system::error_code& error) {
	if(error) {
		std::cerr << "Read error: " + error.message() << std::endl;
		clients.erase(client);
		
		return;
	}
	
	protocol::message msg;
	if(!msg.ParseFromString(client->msgbuf)) {
		std::cerr << "Invalid protobuf recieved?" << std::endl;
		clients.erase(client);
		
		return;
	}
	
	/* Do stuff */
	
	ReadSize(client);
}
