#include <stdint.h>
#include <boost/asio.hpp>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/bind.hpp>
#include <stdexcept>

#include "octradius.hpp"
#include "octradius.pb.h"
#include "client.hpp"

Client::Client(std::string host, uint16_t port, std::string name) : socket(io_service) {
	boost::asio::ip::tcp::endpoint ep;
	boost::asio::ip::address ip;
	
	ip.from_string(host);
	ep.address(ip);
	ep.port(port);
	socket.connect(ep);
	
	protocol::message msg;
	msg.set_msg(protocol::INIT);
	msg.set_player_name(name);
	
	WriteProto(msg);
}

void Client::WriteProto(const protocol::message &msg) {
	wbuf_ptr wb(new std::string);
	
	msg.SerializeToString(wb.get());
	async_write(socket, boost::asio::buffer(wb->data(), wb->size()), boost::bind(&Client::WriteFinish, this, boost::asio::placeholders::error, wb));
}

void Client::WriteFinish(const boost::system::error_code& error, wbuf_ptr wb) {
	if(error) {
		throw std::runtime_error("Write error: " + error.message());
	}
}

void Client::DoStuff(void) {
	io_service.poll();
	
	/* Handle the UI */
}

void Client::ReadSize(void) {
	async_read(socket, boost::asio::buffer(&msgsize, sizeof(uint32_t)), boost::bind(&Client::ReadMessage, this, boost::asio::placeholders::error));
}

void Client::ReadMessage(const boost::system::error_code& error) {
	if(error) {
		throw std::runtime_error("Read error: " + error.message());
	}
	
	msgsize = ntohl(msgsize);
	
	if(msgsize > MAX_MSGSIZE) {
		throw std::runtime_error("Recieved oversized message from server");
	}
	
	msgbuf.reserve(msgsize);
	
	/* This is probably undefined... */
	
	async_read(socket, boost::asio::buffer((char*)msgbuf.data(), msgsize), boost::bind(&Client::ReadFinish, this, boost::asio::placeholders::error));
}

void Client::ReadFinish(const boost::system::error_code& error) {
	if(error) {
		throw std::runtime_error("Read error: " + error.message());
	}
	
	protocol::message msg;
	if(!msg.ParseFromString(msgbuf)) {
		throw std::runtime_error("Invalid protobuf recieved?");
	}
	
	ReadSize();
}
