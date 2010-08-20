#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "network.hpp"
#include "octradius.pb.h"
#include "powers.hpp"

Server::Server(uint16_t port, Scenario &s, uint players)
	: acceptor(io_service), scenario(s), req_players(players), turn(clients.end()), pspawn_turns(1), pspawn_num(1) {
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	
	CopyTiles(tiles, scenario.tiles);
	
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
		if(err.value() == boost::asio::error::operation_aborted) {
			return;
		}
		
		throw std::runtime_error("Accept error: " + err.message());
	}
	
	clients.insert(client);
	ReadSize(client);
	
	StartAccept();
}

void Server::ReadSize(Server::Client::ptr client) {
	async_read(client->socket, boost::asio::buffer(&client->msgsize, sizeof(uint32_t)), boost::bind(&Server::ReadMessage, this, client, boost::asio::placeholders::error));
}

void Server::ReadMessage(Server::Client::ptr client, const boost::system::error_code& error) {
	if(error) {
		QuitClient(client, "Read error: " + error.message());
		return;
	}
	
	client->msgsize = ntohl(client->msgsize);
	
	if(client->msgsize > MAX_MSGSIZE) {
		QuitClient(client, "Oversized message");
		return;
	}
	
	client->msgbuf.resize(client->msgsize);
	async_read(client->socket, boost::asio::buffer(&(client->msgbuf[0]), client->msgsize), boost::bind(&Server::HandleMessage, this, client, boost::asio::placeholders::error));
}

void Server::HandleMessage(Server::Client::ptr client, const boost::system::error_code& error) {
	if(error) {
		QuitClient(client, "Read error: " + error.message());
		return;
	}
	
	std::string msgstring(client->msgbuf.begin(), client->msgbuf.end());
	
	protocol::message msg;
	if(!msg.ParseFromString(msgstring)) {
		QuitClient(client, "Invalid message recieved");
		return;
	}
	
	if(msg.msg() == protocol::INIT) {
		int c;
		bool match = true;
		
		for(c = 0; c < SPECTATE && match;) {
			std::set<Server::Client::ptr>::iterator i = clients.begin();
			match = false;
			
			for(; i != clients.end(); i++) {
				if((*i)->colour != SPECTATE && (*i)->colour == (PlayerColour)c) {
					match = true;
					c++;
					
					break;
				}
			}
		}
		
		if(c == SPECTATE) {
			QuitClient(client, "No colours available");
			return;
		}
		
		if(msg.player_name().empty()) {
			QuitClient(client, "No player name supplied");
			return;
		}
		
		client->playername = msg.player_name();
		client->colour = (PlayerColour)c;
		
		uint n_clients = 0;
		
		for(std::set<Server::Client::ptr>::iterator ci = clients.begin(); ci != clients.end(); ci++) {
			if((*ci)->colour != SPECTATE) {
				n_clients++;
			}
		}
		
		if(n_clients == req_players) {
			StartGame();
		}
	}
	
	if(msg.msg() == protocol::MOVE) {
		if(msg.pawns_size() != 1) {
			goto END;
		}
		
		const protocol::pawn &p_pawn = msg.pawns(0);
		
		Pawn *pawn = FindPawn(tiles, p_pawn.col(), p_pawn.row());
		Tile *tile = FindTile(tiles, p_pawn.new_col(), p_pawn.new_row());
		
		if(!pawn || !tile || pawn->colour != client->colour || *turn != client) {
			goto END;
		}
		
		bool hp = tile->has_power;
		
		if(pawn->Move(tile)) {
			WriteAll(msg);
			
			if(hp) {
				protocol::message msg;
				msg.set_msg(protocol::UPDATE);
				
				msg.add_pawns();
				pawn->CopyToProto(msg.mutable_pawns(0), true);
				
				WriteProto(client, msg);
			}
			
			NextTurn();
		}else{
			BadMove(client);
		}
	}
	
	if(msg.msg() == protocol::USE && msg.pawns_size() == 1) {
		Tile *tile = FindTile(tiles, msg.pawns(0).col(), msg.pawns(0).row());
		
		if(!tile || !tile->pawn || !tile->pawn->UsePower(msg.pawns(0).use_power())) {
			BadMove(client);
		}else{
			WriteAll(msg);
			
			if(tile->pawn && tile->pawn->powers.empty()) {
				protocol::message update;
				update.set_msg(protocol::UPDATE);
				
				update.add_pawns();
				tile->pawn->CopyToProto(update.mutable_pawns(0), false);
				
				WriteAll(update);
			}
			
			SendOK(client);
		}
	}
	
	END:
	ReadSize(client);
}

void Server::WriteProto(Server::Client::ptr client, protocol::message &msg, void (Server::*callback)(Server::Client::ptr, const boost::system::error_code&, wbuf_ptr)) {
	std::string pb;
	msg.SerializeToString(&pb);
	
	uint32_t psize = htonl(pb.size());
	wbuf_ptr wb(new char[pb.size()+sizeof(psize)]);
	
	memcpy(wb.get(), &psize, sizeof(psize));
	memcpy(wb.get()+sizeof(psize), pb.data(), pb.size());
	
	async_write(client->socket, boost::asio::buffer(wb.get(), pb.size()+sizeof(psize)), boost::bind(callback, this, client, boost::asio::placeholders::error, wb));
}

void Server::WriteFinish(Server::Client:: ptr client, const boost::system::error_code& error, wbuf_ptr wb) {
	if(error) {
		std::cerr << "Write error: " << error.message() << std::endl;
		CloseClient(client);
		return;
	}
}

void Server::StartGame(void) {
	protocol::message begin;
	Tile::List::iterator i = tiles.begin();
	std::set<Server::Client::ptr>::iterator c;
	
	begin.set_msg(protocol::BEGIN);
	begin.set_grid_cols(scenario.cols);
	begin.set_grid_rows(scenario.rows);
	
	for(; i != tiles.end(); i++) {
		begin.add_tiles();
		(*i)->CopyToProto(begin.mutable_tiles(begin.tiles_size()-1));
		
		if((*i)->pawn) {
			int cm = 0;
			
			for(c = clients.begin(); c != clients.end(); c++) {
				if((*i)->pawn->colour == (*c)->colour) {
					cm = 1;
					break;
				}
			}
			
			if(cm) {
				begin.add_pawns();
				(*i)->pawn->CopyToProto(begin.mutable_pawns(begin.pawns_size()-1), false);
			}else{
				delete (*i)->pawn;
				(*i)->pawn = NULL;
			}
		}
	}
	
	for(c = clients.begin(); c != clients.end(); c++) {
		int i = begin.players_size();
		
		begin.add_players();
		begin.mutable_players(i)->set_name((*c)->playername);
		begin.mutable_players(i)->set_colour((protocol::colour)(*c)->colour);
		
		std::cout << "Added colour " << (*c)->colour << std::endl;
	}
	
	for(c = clients.begin(); c != clients.end(); c++) {
		begin.set_colour((protocol::colour)(*c)->colour);
		WriteProto(*c, begin);
	}
	
	acceptor.cancel();
	acceptor.close();
	
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

void Server::QuitClient(Server::Client::ptr client, const std::string &msg) {
	std::cout << "Forcing client quit: " << msg << std::endl;
	
	protocol::message pmsg;
	pmsg.set_msg(protocol::QUIT);
	pmsg.set_quit_msg(msg);
	
	WriteProto(client, pmsg, &Server::QuitFinish);
}

void Server::QuitFinish(Server::Client::ptr client, const boost::system::error_code& error, wbuf_ptr wb) {
	if(error && error.value() != boost::asio::error::operation_aborted) {
		std::cerr << "Error writing QUIT message: " << error.message() << std::endl;
	}
	
	CloseClient(client);
}

void Server::CloseClient(Server::Client::ptr client) {
	clients.erase(client);
}

void Server::NextTurn(void) {
	std::set<Server::Client::ptr>::iterator last = turn;
	
	while(1) {
		if(turn == clients.end()) {
			turn = clients.begin();
		}else{
			if(++turn == clients.end()) {
				continue;
			}
		}
		
		if(turn == last) {
			std::cout << "It's a draw!" << std::endl;
			exit(0);
		}
		
		if((*turn)->colour != SPECTATE) {
			int match = 0;
			
			for(Tile::List::iterator t = tiles.begin(); t != tiles.end(); t++) {
				if((*t)->pawn && (*t)->pawn->colour == (*turn)->colour) {
					match = 1;
					break;
				}
			}
			
			if(match) {
				break;
			}
		}
	}
	
	if(turn == last) {
		std::cout << "Team " << (*turn)->colour << " won!" << std::endl;
		exit(0);
	}
	
	if(--pspawn_turns == 0) {
		SpawnPowers();
	}
	
	protocol::message tmsg;
	tmsg.set_msg(protocol::TURN);
	tmsg.set_colour((protocol::colour)(*turn)->colour);
	
	WriteAll(tmsg);
}

void Server::SpawnPowers(void) {
	Tile::List ctiles;
	Tile::List::iterator t = tiles.begin();
	
	for(; t != tiles.end(); t++) {
		if(!(*t)->pawn && !(*t)->has_power) {
			ctiles.push_back(*t);
		}
	}
	
	Tile::List stiles = RandomTiles(ctiles, pspawn_num, true);
	
	protocol::message msg;
	msg.set_msg(protocol::UPDATE);
	
	for(t = stiles.begin(); t != stiles.end(); t++) {
		(*t)->power = Powers::RandomPower();
		(*t)->has_power = true;
		
		msg.add_tiles();
		(*t)->CopyToProto(msg.mutable_tiles(msg.tiles_size()-1));
	}
	
	pspawn_turns = (rand() % 6)+1;
	pspawn_num = (rand() % 4)+1;
	
	WriteAll(msg);
}

void Server::SendOK(Server::Client::ptr client) {
	protocol::message msg;
	msg.set_msg(protocol::OK);
	WriteProto(client, msg);
}
