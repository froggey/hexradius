#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "network.hpp"
#include "octradius.pb.h"

Server::Server(uint16_t port, Tile::List &t, uint players) : acceptor(io_service), tiles(t), req_players(players), turn(clients.end()) {
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
				if((*i)->colour != NOINIT && (*i)->colour == (PlayerColour)c) {
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
		client->colour = (PlayerColour)c;
		
		if(clients.size() == req_players) {
			StartGame();
		}
	}
	
	if(msg.msg() == protocol::MOVE) {
		if(msg.pawns_size() != 1) {
			goto END;
		}
		
		const protocol::pawn &pawn = msg.pawns(0);
		
		Tile *tile = FindTile(tiles, pawn.col(), pawn.row());
		Tile *newtile = FindTile(tiles, pawn.new_col(), pawn.new_row());
		
		if(!tile || !newtile || tile->pawn->colour != client->colour || *turn != client) {
			goto END;
		}
		
		if(tile->pawn->Move(newtile)) {
			WriteAll(msg);
			NextTurn();
		}else{
			BadMove(client);
		}
	}
	
	END:
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
	Tile::List::iterator i = tiles.begin();
	
	for(; i != tiles.end(); i++) {
		begin.add_tiles();
		(*i)->CopyToProto(begin.mutable_tiles(begin.tiles_size()-1));
		
		if((*i)->pawn) {
			begin.add_pawns();
			(*i)->pawn->CopyToProto(begin.mutable_pawns(begin.pawns_size()-1), false);
		}
	}
	
	std::set<Server::Client::ptr>::iterator c = clients.begin();
	
	for(; c != clients.end(); c++) {
		begin.set_colour((protocol::colour)(*c)->colour);
		WriteProto(*c, begin);
	}
	
	NextTurn();
}

void Server::DoStuff(void) {
	io_service.poll();
}

void Server::BadMove(Server::Client::ptr client) {
	protocol::message msg;
	msg.set_msg(protocol::BADMOVE);
	WriteProto(client, msg);
}

void Server::WriteAll(protocol::message &msg) {
	std::set<Server::Client::ptr>::iterator i = clients.begin();
	
	for(; i != clients.end(); i++) {
		WriteProto(*i, msg);
	}
}

void Server::NextTurn(void) {
	if(turn == clients.end()) {
		turn = clients.begin();
	}else{
		do {
			if(++turn == clients.end()) {
				turn = clients.begin();
			}
		} while((*turn)->colour == NOINIT);
	}
	
	protocol::message tmsg;
	tmsg.set_msg(protocol::TURN);
	tmsg.set_colour((protocol::colour)(*turn)->colour);
	
	WriteAll(tmsg);
}
