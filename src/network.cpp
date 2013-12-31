#include <stdint.h>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <stdexcept>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include "network.hpp"
#include "hexradius.pb.h"
#include "powers.hpp"
#include "gamestate.hpp"
#include "fontstuff.hpp"
#include "animator.hpp"
#include "tile_anims.hpp"

#define KING_OF_THE_HILL_LIMIT 50

typedef Tile *(GameState::*tile_coord_fn)(Tile *);
static tile_coord_fn tile_coord_fns[] = {
	&GameState::tile_left_of,
	&GameState::tile_sw_of,
	&GameState::tile_se_of,
	&GameState::tile_right_of,
	&GameState::tile_ne_of,
	&GameState::tile_nw_of,
};

Server::Server(uint16_t port, const std::string &s) :
	game_state(0), acceptor(io_service), worm_timer(io_service)
{
	map_name = s;
	game_state = new ServerGameState(*this);
	game_state->load_file("scenario/" + s);

	idcounter = 0;

	turn = clients.end();
	state = LOBBY;

	pspawn_turns = 1;
	pspawn_num = 1;

	fog_of_war = false;
	king_of_the_hill = false;

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

	delete game_state;
}

void Server::worker_main() {
	io_service.run();
}

void Server::StartAccept(void) {
	boost::shared_ptr<Server::Client> client(new Server::Client(io_service, *this));

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

Server::base_client::~base_client()
{
}

void Server::base_client::ai_think()
{
}

void Server::base_client::Write(const protocol::message &)
{
}

void Server::Client::BeginRead() {
	async_read(socket, boost::asio::buffer(&msgsize, sizeof(uint32_t)), boost::bind(&Server::Client::BeginRead2, this, boost::asio::placeholders::error, shared_from_this()));
}

void Server::Client::BeginRead2(const boost::system::error_code& error, ptr /*cptr*/) {
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

void Server::Client::FinishRead(const boost::system::error_code& error, ptr /*cptr*/) {
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

void Server::Client::Write(const protocol::message &msg) {
	Write(msg, &Client::FinishWrite);
}

void Server::Client::Write(const protocol::message &msg, write_cb callback) {
	send_queue.push(server_send_buf(msg));
	send_queue.back().callback = callback;

	if(send_queue.size() == 1) {
		async_write(socket, boost::asio::buffer(send_queue.front().buf.get(), send_queue.front().size), boost::bind(send_queue.front().callback, this, boost::asio::placeholders::error, shared_from_this()));
	}
}

void Server::base_client::WriteBasic(protocol::msgtype type) {
	protocol::message msg;
	msg.set_msg(type);

	Write(msg);
}

void Server::Client::FinishWrite(const boost::system::error_code& error, ptr /*cptr*/) {
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
	if(king_of_the_hill && game_state->hill_tiles().empty()) {
		fprintf(stderr, "No hills on this map!\n");
		return;
	}

	for(client_iterator c(clients.begin()); c != clients.end(); ++c) {
		(*c)->score = 0;
	}

	std::set<PlayerColour> available_colours = game_state->colours();
	std::set<PlayerColour> player_colours;

	BOOST_FOREACH(boost::shared_ptr<base_client> c, clients) {
		if(c->colour == SPECTATE) continue;
		player_colours.insert(c->colour);
	}

	if(player_colours.size() <= 1) {
		fprintf(stderr, "Too few players. Need at least two.\n");
		return;
	}

	if(available_colours.size() < player_colours.size()) {
		fprintf(stderr, "Too many teams for this map. This is a %u team map.\n", (unsigned int)available_colours.size());
		return;
	}

	std::map<PlayerColour, PlayerColour> colour_map;
	for(std::set<PlayerColour>::iterator i(available_colours.begin()), j(player_colours.begin());
	    j != player_colours.end();
	    ++i, ++j) {
		colour_map.insert(std::make_pair(*i, *j));
	}

	// Remove unused teams before recolouring.
	for(std::set<PlayerColour>::iterator i(available_colours.begin()); i != available_colours.end(); ++i) {
		if(colour_map.find(*i) == colour_map.end()) {
			game_state->destroy_team_pawns(*i);
		}
	}

	game_state->recolour_pawns(colour_map);

	doing_worm_stuff = false;

	protocol::message begin;
	begin.set_msg(protocol::BEGIN);
	game_state->serialize(begin);
	WriteAll(begin);

	state = GAME;

	turn = clients.begin();
	for(int i = rand() % clients.size(); i != 0; --i) {
		++turn;
	}

	NextTurn();
}

void Server::WriteAll(const protocol::message &msg, Server::base_client *exempt) {
	for(client_set::iterator i = clients.begin(); i != clients.end(); i++) {
		if((*i).get() != exempt && (*i)->colour != NOINIT) {
			(*i)->Write(msg);
		}
	}
}

void Server::base_client::Quit(const std::string &msg, bool send_to_client) {
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

	if(server.state == GAME) {
		server.game_state->destroy_team_pawns(colour);
	}

	if(&**(server.turn) == this) {
		server.NextTurn();
	}

	if(send_to_client) {
		send_quit_message(msg);
	}

	// Ehhh.
	for(client_iterator i(server.clients.begin()); i != server.clients.end(); ++i) {
		if(&**i == this) {
			server.clients.erase(i);
			break;
		}
	}

	server.CheckForGameOver();
}

void Server::base_client::send_quit_message(const std::string &/*msg*/)
{
}

void Server::Client::send_quit_message(const std::string &msg)
{
	protocol::message pmsg;
	pmsg.set_msg(protocol::QUIT);
	pmsg.set_quit_msg(msg);

	Write(pmsg, &Server::Client::FinishQuit);
}

void Server::Client::FinishQuit(const boost::system::error_code &, ptr /*cptr*/) {}

void Server::NextTurn() {
	client_set::iterator last = turn;

	black_hole_suck();

	if(king_of_the_hill) {
		std::vector<Tile *> tiles = game_state->hill_tiles();
		for(std::vector<Tile *>::iterator t(tiles.begin()); t != tiles.end(); ++t) {
			if(!(*t)->pawn) continue;
			if((*last)->colour != (*t)->pawn->colour) continue;
			(*last)->score += 1;
			protocol::message msg;
			msg.set_msg(protocol::SCORE_UPDATE);
			msg.add_players();
			msg.mutable_players(0)->set_id((*last)->id);
			msg.mutable_players(0)->set_score((*last)->score);
			WriteAll(msg);
		}
	}

	if(CheckForGameOver()) {
		return;
	}

	while(1) {
		if(turn == clients.end()) {
			turn = clients.begin();
		}else{
			if(++turn == clients.end()) {
				continue;
			}
		}

		if((*turn)->colour != SPECTATE &&
		   !game_state->player_pawns((*turn)->colour).empty()) {
				break;
		}
	}

	if(--pspawn_turns == 0) {
		SpawnPowers();
	}

	protocol::message tmsg;
	tmsg.set_msg(protocol::TURN);
	tmsg.set_player_id((*turn)->id);

	WriteAll(tmsg);

	io_service.post(boost::bind(&base_client::ai_think, *turn));
}

bool Server::CheckForGameOver() {
	if(state == LOBBY) {
		return true;
	}
	if (getenv("HR_DONT_END_GAME"))
		return false;

	int alive = 0;
	uint16_t winner_id;
	for (client_set::iterator it = clients.begin(); it != clients.end(); it++) {
		if (game_state->player_pawns((*it)->colour).size()) {
			alive++;
			winner_id = (*it)->id;
			if(king_of_the_hill && (*it)->score >= KING_OF_THE_HILL_LIMIT) {
				alive = 1;
				fprintf(stderr, "Score limit reached!\n");
				break;
			}
		}
	}

	if (alive <= 1) {
		worm_timer.cancel();
		doing_worm_stuff = false;

		// Reload the map!
		delete game_state;
		game_state = new ServerGameState(*this);
		game_state->load_file("scenario/" + map_name);

		state = LOBBY;

		protocol::message gover;
		gover.set_msg(protocol::GOVER);
		gover.set_is_draw(alive == 0);
		if (alive > 0)
			gover.set_player_id(winner_id);

		turn = clients.end();

		WriteAll(gover);

		return true;
	}
	else {
		return false;
	}
}

void Server::SpawnPowers() {
	Tile::List stiles = RandomTiles(game_state->tiles, pspawn_num, true, true, false, false);

	protocol::message msg;
	msg.set_msg(protocol::UPDATE);

	for(Tile::List::iterator t = stiles.begin(); t != stiles.end(); t++) {
		if((*t)->smashed) continue;
		(*t)->power = Powers::RandomPower(fog_of_war);
		(*t)->has_power = true;

		msg.add_tiles();
		(*t)->CopyToProto(msg.mutable_tiles(msg.tiles_size()-1));
	}

	pspawn_turns = (rand() % 6)+1;
	pspawn_num = (rand() % 4)+1;

	WriteAll(msg);
}

void Server::black_hole_suck() {
	std::set<Tile *> black_holes;
	std::set<pawn_ptr> pawns;

	// Find all pawn and all black holes.
	for(Tile::List::iterator t = game_state->tiles.begin(); t != game_state->tiles.end(); ++t) {
		if((*t)->pawn) {
			pawns.insert((*t)->pawn);
		}
		if((*t)->has_black_hole) {
			black_holes.insert(*t);
		}
	}

	// Draw pawns towards each black hole.
	// Chance is inversely proportional to the square of the euclidean distance
	// and increased by the black hole's power.
	for(std::set<Tile *>::iterator bh = black_holes.begin(); bh != black_holes.end(); ++bh) {
		float bx = (*bh)->col + (((*bh)->row % 2) * 0.5f);
		float by = (*bh)->row * 0.5f;
		for(std::set<pawn_ptr>::iterator p = pawns.begin(); p != pawns.end(); ++p) {
			if((*p)->destroyed()) {
				continue;
			}
			float px = (*p)->cur_tile->col + (((*p)->cur_tile->row % 2) * 0.5f);
			float py = (*p)->cur_tile->row * 0.5f;
			float dx = bx - px, dy = by - py;
			float distance = sqrt(dx * dx + dy * dy);
			float chance = (*bh)->black_hole_power / (distance * distance);
			if(rand() % 100 < (chance * 100)) {
				// OM NOM NOM.
				black_hole_suck_pawn(*bh, *p);
			}
		}
	}
}

// Return true if pawn can be pulled onto the tile.
static bool can_pull_on_to(Tile *tile, pawn_ptr pawn) {
	if(tile->pawn) return false;
	// Tiles can be pulled off ledges, but not up cliffs.
	if(tile->height > pawn->cur_tile->height + 1) return false;
	return true;
}

void Server::black_hole_suck_pawn(Tile *tile, pawn_ptr pawn) {
	Tile *target;

	float bx = tile->col + ((tile->row % 2) * 0.5f);
	float by = tile->row * 0.5f;
	float px = pawn->cur_tile->col + ((pawn->cur_tile->row % 2) * 0.5f);
	float py = pawn->cur_tile->row * 0.5f;
	float angle = atan2(py - by, px - bx);

	if (angle > -M_PI / 6 && angle <= M_PI / 6) // approaching from right
		target = game_state->tile_at(pawn->cur_tile->col - 1, pawn->cur_tile->row);
	else if (angle > M_PI / 6 && angle <= M_PI / 2) // from bottom right
		target = game_state->tile_at(pawn->cur_tile->col - !(pawn->cur_tile->row % 2), pawn->cur_tile->row - 1);
	else if (angle > M_PI / 2 && angle <= 5 * M_PI / 6) // from bottom left
		target = game_state->tile_at(pawn->cur_tile->col + (pawn->cur_tile->row % 2), pawn->cur_tile->row - 1);
	else if (angle < -M_PI / 6 && angle >= -M_PI / 2) // from top right
		target = game_state->tile_at(pawn->cur_tile->col - !(pawn->cur_tile->row % 2), pawn->cur_tile->row + 1);
	else if (angle < -M_PI / 2 && angle >= -5 * M_PI / 6) // from top left
		target = game_state->tile_at(pawn->cur_tile->col + (pawn->cur_tile->row % 2), pawn->cur_tile->row + 1);
	else if (angle > 5 * M_PI / 6 || angle < -5 * M_PI / 6) // from left
		target = game_state->tile_at(pawn->cur_tile->col + 1, pawn->cur_tile->row);
	else
		target = NULL;

	if (target && can_pull_on_to(target, pawn))
		game_state->move_pawn_to(pawn, target);
}

static const char *ai_names[] = {
	"IdeaFactory",
	"HAL 9000",
	"SHODAN",
	"GLaDOS",
	"SkyNet",
	"Bomb #20",
	"Wintermute",
	"Deep Thought",
	0
};

void Server::add_ai_player()
{
	boost::shared_ptr<base_client> client(new ai_client(*this));

	// Pick unused name.
	std::set<std::string> names;
	for(unsigned int i = 0; ai_names[i]; ++i) {
		names.insert(ai_names[i]);
	}
	for(client_set::iterator i = clients.begin(); i != clients.end(); i++) {
		names.erase((*i)->playername);
	}
	if(names.empty()) {
		std::cout << "Too many AIs?" << std::endl;
		return;
	}
	size_t name_id = rand() % names.size();
	std::set<std::string>::iterator name_itr(names.begin());
	while(name_id--) ++name_itr;
	client->playername = *name_itr;

	// Pick a colour, prioritize unused colours.
	std::set<PlayerColour> colours;
	for(unsigned int i = 0; i < 6; ++i) {
		colours.insert(PlayerColour(i));
	}
	for(client_set::iterator i = clients.begin(); i != clients.end(); i++) {
		colours.erase((*i)->colour);
	}
	if(colours.empty()) {
		// All used, pick a random one.
		for(unsigned int i = 0; i < 6; ++i) {
			colours.insert(PlayerColour(i));
		}
	}
	size_t colour_id = rand() % colours.size();
	std::set<PlayerColour>::iterator colour_itr(colours.begin());
	while(colour_id--) ++colour_itr;
	client->colour = *colour_itr;

	std::pair<client_set::iterator,bool> ir;

	do {
		client->id = idcounter++;
		ir = clients.insert(client);
	} while(!ir.second);

	protocol::message pjoin;

	pjoin.set_msg(protocol::PJOIN);

	pjoin.add_players();
	pjoin.mutable_players(0)->set_name(client->playername);
	pjoin.mutable_players(0)->set_colour((protocol::colour)client->colour);
	pjoin.mutable_players(0)->set_id(client->id);

	WriteAll(pjoin, client.get());
}

void Server::ai_client::ai_think()
{
	std::vector<pawn_ptr> my_pawns = server.game_state->player_pawns(colour);
	std::random_shuffle(my_pawns.begin(), my_pawns.end());

	pawn_ptr threatened_pawn;
	Tile *threat_target;
	pawn_ptr move_pawn;
	Tile *move_target;
	for(std::vector<pawn_ptr>::iterator itr(my_pawns.begin()); itr != my_pawns.end(); ++itr) {
		pawn_ptr pawn = *itr;
		assert(pawn);
		for(int i = 0; i < 6; ++i) {
			Tile *tile = (server.game_state->*(tile_coord_fns[i]))(pawn->cur_tile);
			// Should move away from pawns above on cliffs.
			if(tile && pawn->can_move(tile, server.game_state) && tile->pawn && tile->pawn->colour != pawn->colour) {
				threatened_pawn = pawn;
				threat_target = tile;
			}
			if(tile && pawn->can_move(tile, server.game_state)) {
				move_pawn = pawn;
				move_target = tile;
			}
		}
	}
	if(threatened_pawn) {
		assert(threatened_pawn && threat_target);
		protocol::message msg;
		msg.set_msg(protocol::MOVE);
		msg.add_pawns();
		msg.mutable_pawns(0)->set_col(threatened_pawn->cur_tile->col);
		msg.mutable_pawns(0)->set_row(threatened_pawn->cur_tile->row);
		msg.mutable_pawns(0)->set_new_col(threat_target->col);
		msg.mutable_pawns(0)->set_new_row(threat_target->row);
		last_was_move = true;
		server.handle_msg_game(*server.turn, msg);
	} else if(move_pawn) {
		assert(move_pawn && move_target);
		protocol::message msg;
		msg.set_msg(protocol::MOVE);
		msg.add_pawns();
		msg.mutable_pawns(0)->set_col(move_pawn->cur_tile->col);
		msg.mutable_pawns(0)->set_row(move_pawn->cur_tile->row);
		msg.mutable_pawns(0)->set_new_col(move_target->col);
		msg.mutable_pawns(0)->set_new_row(move_target->row);
		last_was_move = true;
		server.handle_msg_game(*server.turn, msg);
	} else {
		protocol::message msg;
		msg.set_msg(protocol::RESIGN);
		last_was_move = true;
		server.handle_msg_game(*server.turn, msg);
	}
}

void Server::ai_client::Write(const protocol::message &msg)
{
	if(msg.msg() == protocol::OK && last_was_move) {
		last_was_move = false;
		return;
	}
	last_was_move = false;
	if(msg.msg() == protocol::OK || msg.msg() == protocol::BADMOVE) {
		if(&**server.turn == this) {
			server.io_service.post(boost::bind(&ai_client::ai_think, this));
		}
	}
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

		ginfo.set_map_name(map_name);

		ginfo.set_fog_of_war(fog_of_war);
		ginfo.set_king_of_the_hill(king_of_the_hill);

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
	}else if(msg.msg() == protocol::CHANGE_MAP && client->id == ADMIN_ID) {
		try {
			ServerGameState *new_state = new ServerGameState(*this);
			new_state->load_file("scenario/" + msg.map_name());
			map_name = msg.map_name();
			delete game_state;
			game_state = new_state;

			for(client_set::iterator c = clients.begin(); c != clients.end(); c++)
			{
				protocol::message ginfo;

				ginfo.set_msg(protocol::GINFO);

				ginfo.set_player_id((*c)->id);

				for(client_set::iterator d = clients.begin(); d != clients.end(); d++)
				{
					if((*d)->colour == NOINIT)
					{
						continue;
					}

					int i = ginfo.players_size();

					ginfo.add_players();
					ginfo.mutable_players(i)->set_name((*d)->playername);
					ginfo.mutable_players(i)->set_colour((protocol::colour)(*d)->colour);
					ginfo.mutable_players(i)->set_id((*d)->id);
				}

				ginfo.set_map_name(map_name);

				ginfo.set_fog_of_war(fog_of_war);
				ginfo.set_king_of_the_hill(king_of_the_hill);

				(*c)->Write(ginfo);
			}
		}
		catch(std::exception &e)
		{
			std::cerr << "Failed to load map '" << msg.map_name() << "': " << e.what() << std::endl;
		}
	} else if(msg.msg() == protocol::CHANGE_SETTING && client->id == ADMIN_ID) {
		if(msg.has_fog_of_war()) {
			fog_of_war = msg.fog_of_war();
		}
		if(msg.has_king_of_the_hill()) {
			king_of_the_hill = msg.king_of_the_hill();
		}
		WriteAll(msg);
	} else if(msg.msg() == protocol::CCOLOUR && msg.players_size() == 1) {
		if(client->id == ADMIN_ID || client->id == msg.players(0).id()) {
			boost::shared_ptr<Server::base_client> c = get_client(msg.players(0).id());

			if(c) {
				c->colour = (PlayerColour)msg.players(0).colour();
				WriteAll(msg);
			}else{
				std::cout << "Invalid player ID in CCOLOUR message" << std::endl;
			}
		}
	} else if(msg.msg() == protocol::KICK && client->id == ADMIN_ID &&
		  // The admin cannot kick themselves.
		  msg.player_id() != ADMIN_ID) {
		boost::shared_ptr<Server::base_client> c = get_client(msg.player_id());
		if(c) {
			std::cout << "Kicking player " << msg.player_id() << std::endl;
			c->Quit("Kicked");
		} else {
			std::cout << "Invalid player ID in KICK message" << std::endl;
		}
	} else if(msg.msg() == protocol::ADD_AI && client->id == ADMIN_ID) {
		add_ai_player();
	}

	return true;
}

bool Server::handle_msg_game(boost::shared_ptr<Server::base_client> client, const protocol::message &msg) {
	if(doing_worm_stuff) {
		return true;
	}
	if(msg.msg() == protocol::MOVE) {
		if(msg.pawns_size() != 1) {
			client->WriteBasic(protocol::BADMOVE);
			return true;
		}

		const protocol::pawn &p_pawn = msg.pawns(0);

		pawn_ptr pawn = game_state->pawn_at(p_pawn.col(), p_pawn.row());
		Tile *tile = game_state->tile_at(p_pawn.new_col(), p_pawn.new_row());

		if(!pawn || !tile || pawn->colour != client->colour || *turn != client) {
			client->WriteBasic(protocol::BADMOVE);
			return true;
		}

		if((pawn->flags & PWR_CONFUSED) && !(pawn->flags & PWR_JUMP)) {
			int r = rand() % 6;
			switch (r) {
				case 0:
				case 1:
				case 2: {
					std::vector<Tile*> choices;
					for (int i = 0; i < 6; i++) {
						Tile* temp = (game_state->*(tile_coord_fns[i]))(tile);
						if (temp && pawn->can_move(temp, game_state))
							choices.push_back(temp);
					}
					if (choices.size() > 0)
						tile = choices[rand() % choices.size()];
					break;
				}
				case 3: {
					pawn->flags &= ~PWR_CONFUSED;
					game_state->update_pawn(pawn);
					break;
				}
			}
		}

		if(pawn->can_move(tile, game_state)) {
			game_state->move_pawn_to(pawn, tile);
			if((pawn->flags & PWR_JUMP) && !pawn->destroyed()) {
				pawn->flags &= ~PWR_JUMP;
				game_state->update_pawn(pawn);
			}
			client->WriteBasic(protocol::OK);
			if (!CheckForGameOver())
				NextTurn();
		}else{
			client->WriteBasic(protocol::BADMOVE);
		}
	}else if(msg.msg() == protocol::USE && msg.pawns_size() == 1) {
		Tile *tile = game_state->tile_at(msg.pawns(0).col(), msg.pawns(0).row());
		pawn_ptr pawn = tile ? tile->pawn : pawn_ptr();

		int power = msg.pawns(0).use_power();

		if(!pawn || !pawn->UsePower(power, game_state)) {
			client->WriteBasic(protocol::BADMOVE);
		}else{
			if(!pawn->destroyed()) {
				update_one_pawn(pawn);
			}

			if(!doing_worm_stuff) {
				client->WriteBasic(protocol::OK);
			}

			if(!CheckForGameOver()) {
				// Make sure the player still has pawns.
				if(game_state->player_pawns((*turn)->colour).empty()) {
					NextTurn();
				}
			}
		}
	}else if(msg.msg() == protocol::RESIGN) {
		if(*turn != client)
			return true;
		std::vector<pawn_ptr> pawns = game_state->all_pawns();
		for(std::vector<pawn_ptr>::iterator it = pawns.begin(); it != pawns.end(); it++) {
			if((*it)->colour == client->colour) {
				game_state->destroy_pawn(*it, Pawn::OK);
			}
		}
		if(!CheckForGameOver()) {
			NextTurn();
		}
	}

	return true;
}

boost::shared_ptr<Server::base_client> Server::get_client(uint16_t id) {
	client_iterator i = clients.begin();

	for(; i != clients.end(); i++) {
		if((*i)->id == id) {
			return *i;
		}
	}

	return boost::shared_ptr<Server::base_client>();
}

void Server::update_one_pawn(pawn_ptr pawn)
{
	protocol::message update;
	update.set_msg(protocol::UPDATE);

	update.add_pawns();
	pawn->CopyToProto(update.mutable_pawns(0), true);

	WriteAll(update);
}

void Server::update_one_tile(Tile *tile)
{
	protocol::message update;
	update.set_msg(protocol::UPDATE);

	tile->CopyToProto(update.add_tiles());

	WriteAll(update);
}

void Server::worm_tick(const boost::system::error_code &/*ec*/)
{
	assert(doing_worm_stuff);

	if(worm_range == 0) {
		doing_worm_stuff = false;
		(*turn)->WriteBasic(protocol::OK);
		return;
	}
	worm_range -= 1;

	if (worm_tile->height < 2) {
		game_state->add_animator(new TileAnimators::ElevationAnimator(
			Tile::List(1, worm_tile), worm_tile, 0, TileAnimators::RELATIVE, 1));
		game_state->set_tile_height(worm_tile, worm_tile->height + 1);
	}
	if(worm_tile->pawn && worm_tile->pawn->colour != worm_pawn->colour) {
		game_state->destroy_pawn(worm_tile->pawn, Pawn::ANT_ATTACK, worm_pawn);
		game_state->add_animator("boom", worm_tile);
	}
	game_state->update_tile(worm_tile);

	std::vector<Tile*> choices;
	for (int i = 0; i < 6; i++) {
		Tile* temp = (game_state->*(tile_coord_fns[i]))(worm_tile);
		if (temp && temp->height < 2)
			choices.push_back(temp);
	}

	if (choices.size() == 0) {
		doing_worm_stuff = false;
		(*turn)->WriteBasic(protocol::OK);
		return;
	}

	worm_tile = choices[rand() % choices.size()];

	worm_timer.expires_at(worm_timer.expires_at() + boost::posix_time::milliseconds(1000));
	worm_timer.async_wait(boost::bind(&Server::worm_tick, this, boost::asio::placeholders::error));
}
