/*
 * NetWeave - C++ Networking Library
 * Copyright 2024 - Jessy van Polanen
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _NETWORK_CONNECTION_
#define _NETWORK_CONNECTION_

#include "net_common.h"
#include "tsqueue.h"
#include "message.h"

BEGIN_NET_NS

//forward declare server interface
template <typename T>
class server_interface;

template <typename T>
class connection : public std::enable_shared_from_this<connection<T>> {
public:
	enum class owner {
		server,
		client
	};
public:
	connection(
		owner parent,
		asio::io_context& asioContext,
		asio::ip::tcp::socket socket,
		tsqueue<owned_message<T>>& qIn
	) 
		: asioContext(asioContext), socket(std::move(socket)), qMessagesIn(qIn)
	{
		this->ownerType = parent;

		if (this->ownerType == owner::server) {
			this->handShakeOut = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
			this->handShakeCheck = this->scramble(this->handShakeOut);
		}
		else {
			this->handShakeIn = 0;
			this->handShakeOut = 0;
		}

	}

	virtual ~connection() { this->disconnect(); }
public:
	/// <summary>
	/// Connects to client if owner is of server type.
	/// </summary>
	/// <param name="id"></param>
	void connectToClient(net::server_interface<T>* server, uint32_t id = 0) {
		if (this->ownerType == owner::server)
			if (this->socket.is_open()) {
				this->id = id;
				writeValidation();
				readValidation(server);
			}
	}

	/// <summary>
	/// Connects to server if owner is of client type.
	/// </summary>
	/// <param name="id"></param>
	bool connectToServer(const asio::ip::tcp::resolver::results_type& endpoints) {
		if (this->ownerType == owner::client) {
			asio::async_connect(
				this->socket,
				endpoints,
				[this](std::error_code ec, asio::ip::tcp::endpoint endpoint) {
					if (!ec) {
						readValidation();
					}
				});
			return true;
		}
		return false;
	}
 
	/// <summary>
	/// Disconnects from the connection medium.
	/// </summary>
	/// <param name="id"></param>
	bool disconnect() {
		if (this->isConnected()) { asio::post(this->asioContext, [this]() { socket.close(); }); return false; }
		return true;
	}

	/// <summary>
	/// Checks if the connection is still valid.
	/// </summary>
	/// <returns></returns>
	bool isConnected() const {
		return this->socket.is_open();
	}

	/// <summary>
	/// ASYNC - Send a message, connections are one-to-one so no need to specifiy
	/// the target, for a client, the target is the server and vice versa
	/// </summary>
	/// <param name="msg"></param>
	/// <returns></returns>
	void send(const message<T>& msg) {
		asio::post(
			this->asioContext, 
			[this, msg]() {
				bool isWritingMsg = !qMessagesOut.empty();
				qMessagesOut.push_back(msg);
				if (!isWritingMsg)
					writeHeader();
			});
	}
public:
	/// <summary>
	/// The "owner" decides how some of the connections behave.
	/// </summary>
	/// <returns>owner</returns>
	inline owner getOwner() const { return this->ownerType; }

	/// <summary>
	/// Client identifiers.
	/// </summary>
	/// <returns></returns>
	inline uint32_t getID() const { return this->id; }

protected:
	asio::ip::tcp::socket socket;				// Each connection has a unique socket to a remote
	asio::io_context& asioContext;				// This context is shared with the entire asio instance - PROVIDED BY SERVER
	tsqueue<message<T>> qMessagesOut;			// This queue holds all messages to be sent to the remote side of this connection
	tsqueue<owned_message<T>>& qMessagesIn;		// This queue holds all messages that have been received from the remote side of this connection - PROVIDED BY CLIENT/SERVER
	message<T> msgTemporaryIn;
protected: // 3-Way Handshake Validation
	uint64_t handShakeOut = 0;
	uint64_t handShakeIn = 0;
	uint64_t handShakeCheck = 0;

private:
	/// <summary>
	/// ASYNC - Prime context ready to read a message header
	/// </summary>
	void readHeader() {
		asio::async_read(
			this->socket,
			asio::buffer(
				&this->msgTemporaryIn.getHeader(),
				sizeof(message_header<T>)
			), 
			[this](std::error_code ec, std::size_t length) {
				if (!ec) {
					if (msgTemporaryIn.getHeader().size > 0) {
						msgTemporaryIn.getBody().resize(msgTemporaryIn.getHeader().size);
						readBody();
					}
					else {
						addToIncomingMessageQueue();
					}
				}
				else {
					std::cout << "[" << id << "] Read Header Fail.\n";
					socket.close();
				}

			});
	}

	/// <summary>
	/// ASYNC - Prime context ready to read a message body
	/// </summary>
	void readBody() {
		asio::async_read(
			this->socket,
			asio::buffer(
				this->msgTemporaryIn.getBody().data(),
				this->msgTemporaryIn.getBody().size()
			),
			[this](std::error_code ec, std::size_t length) {
				if (!ec) {
					addToIncomingMessageQueue();
				}
				else {
					std::cout << "[" << id << "] Read Body Fail.\n";
					socket.close();
				}
			});
	}

	/// <summary>
	/// ASYNC - Prime context ready to write a message header
	/// </summary>
	void writeHeader() {
		asio::async_write(
			this->socket,
			asio::buffer(
				&this->qMessagesOut.front().getHeader(),
				sizeof(message_header<T>)
			),
			[this](std::error_code ec, std::size_t length) {
				if (!ec) {
					if (qMessagesOut.front().getBody().size() > 0) {
						writeBody();
					}
					else {
						qMessagesOut.pop_front();
						if (!qMessagesOut.empty())
							writeHeader();
					}

				}
				else {
					std::cout << "[" << id << "] Write Header Fail.\n";
					socket.close();
				}
			});
	}

	/// <summary>
	/// ASYNC - Prime context ready to write a message body
	/// </summary>
	void writeBody() {
		asio::async_write(
			this->socket,
			asio::buffer(
				this->qMessagesOut.front().getBody().data(),
				this->qMessagesOut.front().getBody().size()
			),
			[this](std::error_code ec, std::size_t length) {
				if (!ec) {
					qMessagesOut.pop_front();
					if (!qMessagesOut.empty())
						writeHeader();
				}
				else {
					std::cout << "[" << id << "] Write Body Fail.\n";
					socket.close();
				}
			});
	}

	/// <summary>
	/// ASYNC - Used by both client and server to write validation packet
	/// </summary>
	void writeValidation() {
		asio::async_write(
			this->socket,
			asio::buffer(
				&this->handShakeOut,
				sizeof(uint64_t)
			),
			[this](std::error_code ec, std::size_t length) {
				if (!ec) {
					if (ownerType == owner::client) readHeader();
				}
				else {
					socket.close();
				}
			});
	} 

	/// <summary>
	/// ASYNC - Used by both client and server to read validation packet
	/// </summary>
	void readValidation(net::server_interface<T>* server = nullptr) {
		asio::async_read(
			this->socket,
			asio::buffer(
				&this->handShakeIn,
				sizeof(uint64_t)
			),
			[this, server](std::error_code ec, std::size_t length) {
				if (!ec) {
					if (ownerType == owner::server) {
						if (handShakeIn == handShakeCheck) {
							std::cout << "Client Validated\n";
							server->onClientValidated(this->shared_from_this());

							readHeader();
						}
					}
					else {
						handShakeOut = scramble(handShakeIn);
						writeValidation();
					}
				}
				else {
					std::cout << "Client Disconnected (ReadValidation)" << std::endl;
					socket.close();
				}
			});
	}

	/// <summary>
	/// Encrypt data.
	/// </summary>
	/// <param name="nInput"></param>
	/// <returns></returns>
	uint64_t scramble(uint64_t nInput) {
		uint64_t out = nInput ^ 0xDEADBEEFC0DECAFE;
		out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0xF0F0F0F0F0F0F0) << 4;
		return out ^ 0xC0DEFACE12345678;
	}

	/// <summary>
	/// Adds messages to the incoming message queue.
	/// </summary>
	void addToIncomingMessageQueue() {
		if (this->ownerType == owner::server)
			this->qMessagesIn.push_back(owned_message<T>(this->msgTemporaryIn, this->shared_from_this()));
		else
			this->qMessagesIn.push_back(owned_message<T>(this->msgTemporaryIn));

		this->readHeader();
	}

private:
	owner ownerType = owner::server;							// The "owner" decides how some of the connections behave.
	uint32_t id = 0;											// The client ID
};

END_NET_NS

#endif