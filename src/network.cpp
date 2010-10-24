#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <iostream>
#include <boost/shared_ptr.hpp>

#include "network.hpp"
#include "octradius.pb.h"
#include "powers.hpp"

Server::Server(uint16_t port, Scenario &s) : acceptor(io_service) {
	scenario = s;
	CopyTiles(tiles, scenario.tiles);
	
	idcounter = 0;
	
	turn = clients.end();
	state = LOBBY;
	
	pspawn_turns = 1;
	pspawn_num = 1;
	
	boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), port);
	
	acceptor.open(endpoint.protocol());
	acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
	acceptor.bind(endpoint);
	acceptor.listen();
	
	StartAccept();
	
	worker = boost::thread(boost::bind(&Server::worker_main, this));
}

Server::~Server() {
	io_service.stop();
	
	std::cout << "Waiting for server thread to exit..." << std::endl;
	worker.join();
	
	FreeTiles(tiles);
}

void Server::worker_main() {
	io_service.run();
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
	
	StartAccept();
	
	if(state == GAME) {
		protocol::message qmsg;
		qmsg.set_msg(protocol::QUIT);
		qmsg.set_quit_msg("Game already in progress");
		
		client->Write(qmsg, &Server::Client::FinishQuit);
		
		return;
	}
	
	std::pair<client_set::iterator,bool> ir;
	
	do {
		client->id = idcounter++;
		ir = clients.insert(client);
	} while(!ir.second);
	
	client->BeginRead();
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
	if(msg.msg() == protocol::CHAT) {
		protocol::message chat;
		chat.set_msgtext(msg.msgtext());
		chat.set_player_id(client->id);
		
		WriteAll(chat);
		
		return true;
	}
	
	if(state == LOBBY) {
		return handle_msg_lobby(client, msg);
	}else{
		return handle_msg_game(client, msg);
	}
}

void Server::Client::Write(const protocol::message &msg, write_cb callback) {
	send_queue.push(server_send_buf(msg));
	send_queue.back().callback = callback;
	
	if(send_queue.size() == 1) {
		async_write(socket, boost::asio::buffer(send_queue.front().buf.get(), send_queue.front().size), boost::bind(send_queue.front().callback, this, boost::asio::placeholders::error, shared_from_this()));
	}
}

void Server::Client::WriteBasic(protocol::msgtype type) {
	protocol::message msg;
	msg.set_msg(type);
	
	Write(msg);
}

void Server::Client::FinishWrite(const boost::system::error_code& error, ptr cptr) {
	send_queue.pop();
	
	if(qcalled) {
		return;
	}
	
	if(error) {
		Quit("Write error: " + error.message(), false);
		return;
	}
	
	if(!send_queue.empty()) {
		async_write(socket, boost::asio::buffer(send_queue.front().buf.get(), send_queue.front().size), boost::bind(send_queue.front().callback, this, boost::asio::placeholders::error, shared_from_this()));
	}
}

void Server::StartGame(void) {
	std::set<PlayerColour> colours;
	
	for(client_iterator c = clients.begin(); c != clients.end(); c++) {
		if((*c)->colour < SPECTATE) {
			colours.insert((*c)->colour);
		}
	}
	
	tiles = scenario.init_game(colours);
	
	protocol::message begin;
	begin.set_msg(protocol::BEGIN);
	WriteAll(begin);
	
	state = GAME;
	
	NextTurn();
}

void Server::WriteAll(const protocol::message &msg, Server::Client *exempt) {
	for(client_set::iterator i = clients.begin(); i != clients.end(); i++) {
		if((*i).get() != exempt && (*i)->colour != NOINIT) {
			(*i)->Write(msg);
		}
	}
}

void Server::Client::Quit(const std::string &msg, bool send_to_client) {
	if(qcalled) {
		return;
	}
	
	qcalled = true;
	
	if(colour != NOINIT) {
		protocol::message qmsg;
		qmsg.set_msg(protocol::PQUIT);
		qmsg.add_players();
		qmsg.mutable_players(0)->set_name(playername);
		qmsg.mutable_players(0)->set_colour((protocol::colour)colour);
		qmsg.mutable_players(0)->set_id(id);
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

void Server::Client::FinishQuit(const boost::system::error_code& error, ptr cptr) {}

void Server::NextTurn(void) {
	client_set::iterator last = turn;
	
	while(1) {
		if(turn == clients.end()) {
			turn = clients.begin();
		}else{
			if(++turn == clients.end()) {
				continue;
			}
		}
		
		if(turn == last) {
			CopyTiles(tiles, scenario.tiles);
			state = LOBBY;
			
			turn = clients.end();
			
			protocol::message gover;
			gover.set_msg(protocol::GOVER);
			gover.set_is_draw(true);
			
			WriteAll(gover);
			
			return;
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
		CopyTiles(tiles, scenario.tiles);
		state = LOBBY;
		
		protocol::message gover;
		gover.set_msg(protocol::GOVER);
		gover.set_is_draw(false);
		gover.set_player_id((*turn)->id);
		
		turn = clients.end();
		
		WriteAll(gover);
		
		return;
	}
	
	if(--pspawn_turns == 0) {
		SpawnPowers();
	}
	
	protocol::message tmsg;
	tmsg.set_msg(protocol::TURN);
	tmsg.set_player_id((*turn)->id);
	
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

bool Server::handle_msg_lobby(Server::Client::ptr client, const protocol::message &msg) {
	if(msg.msg() == protocol::INIT) {
		int c;
		bool match = true;
		
		for(c = 0; c < SPECTATE && match;) {
			match = false;
			
			for(client_set::iterator i = clients.begin(); i != clients.end(); i++) {
				if((*i)->colour != SPECTATE && (*i)->colour == (PlayerColour)c) {
					match = true;
					c++;
					
					break;
				}
			}
		}
		
		if(msg.player_name().empty()) {
			client->Quit("No player name supplied");
			return false;
		}
		
		client->playername = msg.player_name();
		client->colour = (PlayerColour)c;
		
		protocol::message ginfo;
		
		ginfo.set_msg(protocol::GINFO);
		
		ginfo.set_player_id(client->id);
		
		for(client_set::iterator c = clients.begin(); c != clients.end(); c++) {
			if((*c)->colour == NOINIT) {
				continue;
			}
			
			int i = ginfo.players_size();
			
			ginfo.add_players();
			ginfo.mutable_players(i)->set_name((*c)->playername);
			ginfo.mutable_players(i)->set_colour((protocol::colour)(*c)->colour);
			ginfo.mutable_players(i)->set_id((*c)->id);
		}
		
		scenario.store_proto(ginfo);
		
		client->Write(ginfo);
		
		protocol::message pjoin;
		
		pjoin.set_msg(protocol::PJOIN);
		
		pjoin.add_players();
		pjoin.mutable_players(0)->set_name(client->playername);
		pjoin.mutable_players(0)->set_colour((protocol::colour)client->colour);
		pjoin.mutable_players(0)->set_id(client->id);
		
		WriteAll(pjoin, client.get());
	}else if(msg.msg() == protocol::BEGIN && client->id == ADMIN_ID) {
		StartGame();
	}else if(msg.msg() == protocol::CCOLOUR && msg.players_size() == 1) {
		if(client->id == ADMIN_ID || client->id == msg.players(0).id()) {
			Client *c = get_client(msg.players(0).id());
			
			if(c) {
				c->colour = (PlayerColour)msg.players(0).colour();
				WriteAll(msg);
			}else{
				std::cout << "Invalid player ID in CCOLOUR message" << std::endl;
			}
		}
	}
	
	return true;
}

bool Server::handle_msg_game(Server::Client::ptr client, const protocol::message &msg) {
	if(msg.msg() == protocol::MOVE) {
		if(msg.pawns_size() != 1) {
			return true;
		}
		
		const protocol::pawn &p_pawn = msg.pawns(0);
		
		pawn_ptr pawn = FindPawn(tiles, p_pawn.col(), p_pawn.row());
		Tile *tile = FindTile(tiles, p_pawn.new_col(), p_pawn.new_row());
		
		if(!pawn || !tile || pawn->colour != client->colour || *turn != client) {
			return true;
		}
		
		bool hp = tile->has_power;
		
		if(pawn->Move(tile, this, NULL)) {
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
	}else if(msg.msg() == protocol::USE && msg.pawns_size() == 1) {
		Tile *tile = FindTile(tiles, msg.pawns(0).col(), msg.pawns(0).row());
		pawn_ptr pawn = tile ? tile->pawn : pawn_ptr();
		
		int power = msg.pawns(0).use_power();
		
		power_rand_vals.clear();
		
		if(!pawn || !pawn->UsePower(power, this, NULL)) {
			client->WriteBasic(protocol::BADMOVE);
		}else{
			protocol::message smsg = msg;
			
			smsg.clear_power_rand_vals();
			
			for(std::vector<uint32_t>::iterator i = power_rand_vals.begin(); i != power_rand_vals.end(); i++) {
				smsg.add_power_rand_vals(*i);
			}
			
			WriteAll(smsg);
			
			if(!pawn->destroyed()) {
				protocol::message update;
				update.set_msg(protocol::UPDATE);
				
				update.add_pawns();
				pawn->CopyToProto(update.mutable_pawns(0), false);
				
				WriteAll(update);
			}
			
			client->WriteBasic(protocol::OK);
		}
	}
	
	return true;
}

Server::Client *Server::get_client(uint16_t id) {
	client_iterator i = clients.begin();
	
	for(; i != clients.end(); i++) {
		if((*i)->id == id) {
			return i->get();
		}
	}
	
	return NULL;
}
