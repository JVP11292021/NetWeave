#ifndef _NETWORK_SERVER_
#define _NETWORK_SERVER_

#include "net_common.h"
#include "tsqueue.h"
#include "message.h"
#include "connection.h"

BEGIN_NET_NS

template <typename T>
class server_interface {
public:
	server_interface(uint16_t port) 
		: asioAcceptor(context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
	{}

	virtual ~server_interface() {
		this->stop();
	}

public:

	/// <summary>
	/// Starts the server.
	/// </summary>
	/// <returns>bool</returns>
	bool start() {
		try {
			this->waitForClientConnection();

			this->threadContext = std::thread([this]() { context.run(); });
		}
		catch (std::exception& e) {
			std::cerr << "[SERVER] Exception: " << e.what() << "\n";
			return false;
		}

		std::cout << "[SERVER] Started!\n";
		return true;
	}

	/// <summary>
	/// Stops the server.
	/// </summary>
	void stop() {
		this->context.stop();
		if (this->threadContext.joinable()) this->threadContext.join();
		std::cout << "[SERVER] Stopped!\n";
	}

	/// <summary>
	/// ASYNC - Instruct asio to wait for conenction.
	/// </summary>
	void waitForClientConnection() {
		this->asioAcceptor.async_accept(
			[this](std::error_code ec, asio::ip::tcp::socket socket) {
				if (!ec) {
					std::cout << "[SERVER] New Connection: " << socket.remote_endpoint() << "\n";

					ref<connection<T>> newconn =
						std::make_shared<connection<T>>(
								connection<T>::owner::server,
								context,
								std::move(socket),
								qMessagesIn
							);

					if (onClientConnect(newconn)) {
						deqConnections.push_back(std::move(newconn));
						deqConnections.back()->connectToClient(this, cIDCounter++);
						std::cout << '[' << deqConnections.back()->getID() << "] Connection Approved\n";
					}
					else
						std::cout << "[SERVER] Connection Denied!\n";
				}
				else
					std::cout << "[SERVER] New Connection Error: " << ec.message() << "\n";

				this->waitForClientConnection();
			}
		);
	}

	/// <summary>
	/// Send message to a specific client.
	/// </summary>
	/// <param name="client"></param>
	/// <param name="msg"></param>
	void messageClient(ref<connection<T>> client, const message<T>& msg) {
		if (client && client->isConnected()) {
			client->send(msg);
		}
		else {
			this->onClientDisconnect(client);
			client.reset();
			this->deqConnections.erase(
					std::remove(deqConnections.begin(), deqConnections.end(), client), 
					this->deqConnections.end()
				);

		}
	}

	/// <summary>
	/// Send a message to all of the clients.
	/// </summary>
	/// <param name="msg"></param>
	/// <param name="client"></param>
	void messageAllClients(const message<T>& msg, ref<connection<T>> ignoreClient = nullptr) {
		int8_t invalidClientExists = 0;

		for (auto& client : this->deqConnections) {
			if (client && client->isConnected()) {
				if (client != ignoreClient)
					client->send(msg);
			}
			else {
				this->onClientDisconnect(client);
				client.reset();
				invalidClientExists = 1;
			}
		}

		if (invalidClientExists)
			this->deqConnections.erase(
				std::remove(deqConnections.begin(), deqConnections.end(), nullptr),
				this->deqConnections.end()
			);
	}

	/// <summary>
	/// Updates the server input with incomming message packets in the global thread safe queue.
	/// </summary>
	/// <param name="maxMessages"></param>
	void update(size_t maxMessages = -1, int8_t wait = 0) {
		if (wait) this->qMessagesIn.wait();

		size_t messageCounter = 0;
		while (messageCounter < maxMessages && !this->qMessagesIn.empty()) {
			auto msg = this->qMessagesIn.pop_front();
			this->onMessage(msg.getRemote(), msg.getMsg());
			messageCounter++;
		}
	}

protected:
	/// <summary>
	/// Called when client connects, you can redo the connection by returning false
	/// </summary>
	/// <param name="client"></param>
	/// <returns>bool</returns>
	virtual bool onClientConnect(ref<connection<T>> client) { return false; }

	/// <summary>
	/// Is called when client disconnects from the server.
	/// </summary>
	/// <param name="client"></param>
	virtual void onClientDisconnect(ref<connection<T>> client) {}

	/// <summary>
	/// Called when a message arrives.
	/// </summary>
	/// <param name="client"></param>
	/// <param name="msg"></param>
	virtual void onMessage(ref<connection<T>> client, message<T>& msg) {}
public:
	/// <summary>
	/// Called when a client is validated.
	/// </summary>
	/// <param name="client"></param>
	virtual void onClientValidated(ref<connection<T>> client) {}

protected:
	tsqueue<owned_message<T>> qMessagesIn;							// Thread safe Queue for incoming message packets.
		
	std::deque<ref<connection<T>>> deqConnections;					// Container of active validated connections

	asio::io_context context;										// asio context handles the data transfer ...
	std::thread threadContext;										// ... but also needs athread of it's own to execute commands

	asio::ip::tcp::acceptor asioAcceptor;

	uint32_t cIDCounter = 10000;									// Clients will be identified in the system via ID codes
};

END_NET_NS

#endif