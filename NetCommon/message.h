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

#ifndef _NETWORK_MESSAGE_
#define _NETWORK_MESSAGE_

#include "net_common.h"

BEGIN_NET_NS

/// <summary>
/// Message Header s sent at the start of all messages.
/// The template allows us to use a user defined enum class.
/// </summary>
/// <typeparam name="T"> = User defined enum-class </typeparam>
template <typename T>
struct message_header {
	T id{};
	uint32_t size = 0;
};

/// <summary>
/// Message Body contains a header and a std::vector, containing raw bytes
/// of infomation. This way the message can be variable length, but the size
/// in the header must be updated.
/// </summary>
/// <typeparam name="T"> = User defined enum-class </typeparam>
template <typename T>
class message {
public:

	/// <summary>
	/// Returns size of entire message packet
	/// </summary>
	/// <returns>Size in bytes</returns>
	size_t size() const {
		return this->body.size() + sizeof(message_header<T>);
	}

	inline message_header<T>& getHeader() { return this->header; }
	inline std::vector<uint8_t>& getBody() { return this->body; }
	inline message_header<T> getHeader() const { return this->header; }
	inline std::vector<uint8_t> getBody() const { return this->body; }

private:
	message_header<T> header{};
	std::vector<uint8_t> body;
public:
	/// <summary>
	/// Override for std::cout compatibility.
	/// Produces simple message description
	/// </summary>
	/// <param name="os"></param>
	/// <param name="msg"></param>
	/// <returns>Output stream</returns>
	friend std::ostream& operator << (std::ostream& os, const message<T>& msg) {
		os << "ID:" << int(msg.getHeader().id) << " Size:" << msg.getHeader().size;
		return os;
	}

	/// <summary>
	/// Pushes data into the message buffer.
	/// </summary>
	/// <typeparam name="DT"></typeparam>
	/// <param name="msg"></param>
	/// <param name="data"></param>
	/// <returns>New message object</returns>
	template <typename DT>
	friend message<T>& operator << (message<T>& msg, const DT& data) {
		static_assert(std::is_standard_layout<DT>::value, "Type is too complex to use!");

		size_t i = msg.getBody().size();
		msg.body.resize(msg.getBody().size() + sizeof(DT));
		std::memcpy(msg.getBody().data() + i, &data, sizeof(DT));
		msg.header.size = msg.size();

		return msg;
	}

	/// <summary>
	/// Extracts data from the message buffer.
	/// </summary>
	/// <typeparam name="DT"></typeparam>
	/// <param name="msg"></param>
	/// <param name="data"></param>
	/// <returns>New message object</returns>
	template <typename DT>
	friend message<T>& operator >> (message<T>& msg, DT& data) {
		static_assert(std::is_standard_layout<DT>::value, "Type is too complex to use!");

		size_t i = msg.getBody().size() - sizeof(DT);
		std::memcpy(&data, msg.getBody().data() + i, sizeof(DT));
		msg.body.resize(i);
		msg.getHeader().size = msg.size();

		return msg;
	}
};

// Forward declare the conenction because we use it in this file.
template <typename T>
class connection;

/// <summary>
/// An "owned" message is identical to a regular message, but it is associated with
/// a connection. On a server, the owner would be the client that sent the message, 
// on a client the owner would be the server.
/// </summary>
/// <typeparam name="T"> = User defined enum-class </typeparam>
template <typename T>
class owned_message {
public:
	inline message<T>& getMsg() { return this->msg; }
	inline ref<connection<T>> getRemote() const { return this->remote; }
public:
	owned_message() = default;
	owned_message(const message<T>& msg, ref<connection<T>> remote = nullptr) 
		: msg(msg), remote(remote)
	{}
private:
	message<T> msg;
	ref<connection<T>> remote = nullptr;
public:
	/// <summary>
	/// Override for std::cout compatibility.
	/// Produces simple message description
	/// </summary>
	/// <param name="os"></param>
	/// <param name="msg"></param>
	/// <returns>Output stream</returns>
	friend std::ostream& operator << (std::ostream& os, const owned_message<T>& msg) {
		os << msg.getMsg();
		return os;
	}
};

/// <summary>
/// Standard message types enum.
/// Is valid for certain cases otherwise the user needs to define a custom enum-class type.
/// </summary>
enum class message_types : uint32_t {
	ServerAccept,
	ServerDeny,
	ServerPing,
	ServerAll,
	ServerMessage
};

END_NET_NS

#endif