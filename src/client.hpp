#ifndef OR_CLIENT_HPP
#define OR_CLIENT_HPP

#include <stdint.h>
#include <boost/asio.hpp>
#include <string>
#include <boost/shared_ptr.hpp>

#include "octradius.hpp"
#include "octradius.pb.h"

class Client {
	public:
		Client(std::string host, uint16_t port, std::string name);
		void DoStuff(void);
		
	private:
		typedef boost::shared_ptr<std::string> wbuf_ptr;
		
		boost::asio::io_service io_service;
		boost::asio::ip::tcp::socket socket;
		
		uint32_t msgsize;
		std::string msgbuf;
		
		void WriteProto(const protocol::message &msg);
		void WriteFinish(const boost::system::error_code& error, wbuf_ptr wb);
		
		void ReadSize(void);
		void ReadMessage(const boost::system::error_code& error);
		void ReadFinish(const boost::system::error_code& error);
};

#endif /* !OR_CLIENT_HPP */
