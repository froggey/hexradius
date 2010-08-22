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
	Server::Client::ptr client(new Server::Client(io_service, *this));
	
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
	client->BeginRead();
	
	StartAccept();
}

void Server::Client::BeginRead() {
	async_read(socket, boost::asio::buffer(&msgsize, sizeof(uint32_t)), boost::bind(&Server::Client::BeginRead2, this, boost::asio::placeholders::error, shared_from_this()));
}

void Server::Client::BeginRead2(const boost::system::error_code& error, ptr cptr) {
	if(qcalled) {
		return;
	}
	
	if(error) {
		Quit("Read error: " + error.message());
		return;
	}
	
	msgsize = ntohl(msgsize);
	
	if(msgsize > MAX_MSGSIZE) {
		Quit("Oversized message");
		return;
	}
	
	msgbuf.resize(msgsize);
	async_read(socket, boost::asio::buffer(&(msgbuf[0]), msgsize), boost::bind(&Server::Client::FinishRead, this, boost::asio::placeholders::error, shared_from_this()));
}

void Server::Client::FinishRead(const boost::system::error_code& error, ptr cptr) {
	if(qcalled) {
		return;
	}
	
	if(error) {
		Quit("Read error: " + error.message());
		return;
	}
	
	std::string msgstring(msgbuf.begin(), msgbuf.end());
	
	protocol::message msg;
	if(!msg.ParseFromString(msgstring)) {
		Quit("Invalid message recieved");
		return;
	}
	
	if(server.HandleMessage(shared_from_this(), msg)) {
		BeginRead();
	}
}

bool Server::HandleMessage(Server::Client::ptr client, const protocol::message &msg) {
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
			client->Quit("No colours available");
			return false;
		}
		
		if(msg.player_name().empty()) {
			client->Quit("No player name supplied");
			return false;
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
			return true;
		}
		
		const protocol::pawn &p_pawn = msg.pawns(0);
		
		Pawn *pawn = FindPawn(tiles, p_pawn.col(), p_pawn.row());
		Tile *tile = FindTile(tiles, p_pawn.new_col(), p_pawn.new_row());
		
		if(!pawn || !tile || pawn->colour != client->colour || *turn != client) {
			return true;
		}
		
		bool hp = tile->has_power;
		
		if(pawn->Move(tile)) {
			WriteAll(msg);
			
			if(hp) {
				protocol::message msg;
				msg.set_msg(protocol::UPDATE);
				
				msg.add_pawns();
				pawn->CopyToProto(msg.mutable_pawns(0), true);
				
				client->Write(msg);
			}
			
			NextTurn();
		}else{
			client->WriteBasic(protocol::BADMOVE);
		}
	}
	
	if(msg.msg() == protocol::USE && msg.pawns_size() == 1) {
		Tile *tile = FindTile(tiles, msg.pawns(0).col(), msg.pawns(0).row());
		
		if(!tile || !tile->pawn || !tile->pawn->UsePower(msg.pawns(0).use_power(), this, NULL)) {
			client->WriteBasic(protocol::BADMOVE);
		}else{
			WriteAll(msg);
			
			if(tile->pawn && tile->pawn->powers.empty()) {
				protocol::message update;
				update.set_msg(protocol::UPDATE);
				
				update.add_pawns();
				tile->pawn->CopyToProto(update.mutable_pawns(0), false);
				
				WriteAll(update);
			}
			
			client->WriteBasic(protocol::OK);
		}
	}
	
	return true;
}

void Server::Client::Write(const protocol::message &msg, write_cb callback) {
	std::string pb;
	msg.SerializeToString(&pb);
	
	uint32_t psize = htonl(pb.size());
	wbuf_ptr wb(new char[pb.size()+sizeof(psize)]);
	
	memcpy(wb.get(), &psize, sizeof(psize));
	memcpy(wb.get()+sizeof(psize), pb.data(), pb.size());
	
	async_write(socket, boost::asio::buffer(wb.get(), pb.size()+sizeof(psize)), boost::bind(callback, this, boost::asio::placeholders::error, wb, shared_from_this()));
}

void Server::Client::WriteBasic(protocol::msgtype type) {
	protocol::message msg;
	msg.set_msg(type);
	
	Write(msg);
}

void Server::Client::FinishWrite(const boost::system::error_code& error, wbuf_ptr wb, ptr cptr) {
	if(qcalled) {
		return;
	}
	
	if(error) {
		Quit("Write error: " + error.message(), false);
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
		(*c)->Write(begin);
	}
	
	acceptor.cancel();
	acceptor.close();
	
	NextTurn();
}

void Server::DoStuff(void) {
	io_service.poll();
}

void Server::WriteAll(const protocol::message &msg, Server::Client *exempt) {
	std::set<Server::Client::ptr>::iterator i = clients.begin();
	
	for(; i != clients.end(); i++) {
		if((*i).get() != exempt) {
			(*i)->Write(msg);
		}
	}
}

void Server::Client::Quit(const std::string &msg, bool send_to_client) {
	if(qcalled) {
		return;
	}
	
	qcalled = true;
	
	if(colour != SPECTATE) {
		protocol::message qmsg;
		qmsg.set_msg(protocol::PQUIT);
		qmsg.add_players();
		qmsg.mutable_players(0)->set_name(playername);
		qmsg.mutable_players(0)->set_colour((protocol::colour)colour);
		qmsg.set_quit_msg(msg);
		
		server.WriteAll(qmsg, this);
	}
	
	DestroyTeamPawns(server.tiles, colour);
	
	if(*(server.turn) == shared_from_this()) {
		server.NextTurn();
	}
	
	if(send_to_client) {
		protocol::message pmsg;
		pmsg.set_msg(protocol::QUIT);
		pmsg.set_quit_msg(msg);
		
		Write(pmsg, &Server::Client::FinishQuit);
	}
	
	server.clients.erase(shared_from_this());
}

void Server::Client::FinishQuit(const boost::system::error_code& error, wbuf_ptr wb, ptr cptr) {}

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
