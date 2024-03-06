#ifndef _NETWORK_CLIENT_
#define _NETWORK_CLIENT_

#include "net_common.h"
#include "message.h"
#include "tsqueue.h"
#include "connection.h"

BEGIN_NET_NS 

template <typename T>
class client_interface {
public:
	client_interface() : socket(context) {}
	virtual ~client_interface() { this->disconnect(); }
public:
	/// <summary>
	/// Connect to the server with hostname/ip-address and port number.
	/// </summary>
	/// <param name="host"></param>
	/// <param name="port"></param>
	/// <returns></returns>
	bool connect(const std::string& host, const uint16_t port) {
		try {
			asio::ip::tcp::resolver resolver(this->context);
			asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));

			this->conn = std::make_unique<connection<T>>(
				connection<T>::owner::client,
				this->context,
				asio::ip::tcp::socket(this->context),
				this->qMessagesIn); 

			this->conn->connectToServer(endpoints);
			this->threadContext = std::thread([this]() { context.run(); });
		}
		catch (std::exception& e) {
			std::cerr << "[Client Exception] " << e.what() << "\n";
			return false;
		}

		return true;
	}

	/// <summary>
	/// Disconnect from the server.
	/// </summary>
	void disconnect() {
		if (this->isConnected())
			this->conn->disconnect();

		this->context.stop();
		if (this->threadContext.joinable()) threadContext.join();

		this->conn.release();
	}

	/// <summary>
	/// Checks if client is truly connected to the server.
	/// </summary>
	/// <returns></returns>
	bool isConnected() {
		if (this->conn)
			return this->conn->isConnected();
		return false;
	}

	/// <summary>
	/// Sends a message object through the connection.
	/// </summary>
	/// <param name="msg"></param>
	/// <returns></returns>
	void send(const message<T>& msg) {
		if (this->isConnected())
			this->conn->send(msg);
	}

public:
	inline tsqueue<owned_message<T>>& incoming() {
		return this->qMessagesIn;
	}

protected:
	asio::io_context context;					// asio context handles the data transfer ...
	std::thread threadContext;					// asio context also needs athread of it's own to execute commands
	asio::ip::tcp::socket socket;				// This is the hardware socket connected to the server
	scope<connection<T>> conn;		// The client has a single instance of a "connection" object, which handles data transfer
private:
	tsqueue<owned_message<T>> qMessagesIn;		// This is the thread safe queue of incoming messages from the server
};

END_NET_NS

#endif