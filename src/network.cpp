#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "network.hpp"
#include "octradius.pb.h"

Server::Server(uint16_t port, OctRadius::TileList &t, uint players) : acceptor(io_service), tiles(t), req_players(players) {
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
	
	if(msg.msg() == protocol::INIT) {
		int c, match = 1;
		
		for(c = 0; c < 4 && match; c++) {
			std::set<Server::Client::ptr>::iterator i = clients.begin();
			match = 0;
			
			for(; i != clients.end(); i++) {
				if((*i)->playername.size() && (*i)->colour == (OctRadius::Colour)c) {
					match = 1;
					break;
				}
			}
		}
		
		if(c == 4) {
			std::cerr << "No colours available" << std::endl;
			clients.erase(client);
			return;
		}
		
		if(msg.player_name().empty()) {
			std::cerr << "No player name supplied" << std::endl;
			clients.erase(client);
			return;
		}
		
		client->playername = msg.player_name();
		client->colour = (OctRadius::Colour)c;
		
		if(clients.size() == req_players) {
			StartGame();
		}
	}
	
	ReadSize(client);
}

void Server::WriteProto(Server::Client::ptr client, protocol::message &msg) {
	boost::shared_ptr<std::string> buf(new std::string);
	
	msg.SerializeToString(buf.get());
	
	async_write(client->socket, boost::asio::buffer(buf->data(), buf->size()), boost::bind(&Server::WriteFinish, this, client, boost::asio::placeholders::error));
}

void Server::WriteFinish(Server::Client:: ptr client, const boost::system::error_code& error) {
	if(error) {
		std::cerr << "Write error: " << error.message() << std::endl;
		clients.erase(client);
		return;
	}
	
	ReadSize(client);
}

void Server::StartGame(void) {
	protocol::message begin;
	OctRadius::TileList::iterator i = tiles.begin();
	
	for(; i != tiles.end(); i++) {
		int index = begin.tiles_size();
		begin.add_tiles();
		
		begin.mutable_tiles(index)->set_col((*i)->col);
		begin.mutable_tiles(index)->set_row((*i)->row);
		begin.mutable_tiles(index)->set_height((*i)->height);
		
		if((*i)->pawn) {
			index = begin.pawns_size();
			begin.add_pawns();
			
			begin.mutable_pawns(index)->set_col((*i)->col);
			begin.mutable_pawns(index)->set_row((*i)->row);
			
			begin.mutable_pawns(index)->set_colour((protocol::colour)(*i)->pawn->colour);
			begin.mutable_pawns(index)->set_range((*i)->pawn->range);
			begin.mutable_pawns(index)->set_flags((*i)->pawn->flags);
		}
	}
	
	std::set<Server::Client::ptr>::iterator c = clients.begin();
	
	for(; c != clients.end(); c++) {
		begin.set_colour((protocol::colour)(*c)->colour);
		WriteProto(*c, begin);
	}
}
