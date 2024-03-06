#include <iostream>
#include <net1++.h>

class CustomServer : public net::server_interface<net::message_types> {
public:
	CustomServer(uint16_t port)
		: net::server_interface<net::message_types>(port)
	{}

protected:
	bool onClientConnect(std::shared_ptr<net::connection<net::message_types>> client) override {
		net::message<net::message_types> msg;
		msg.getHeader().id = net::message_types::ServerAccept;
		client->send(msg);
		return true;
	}

	void onClientDisconnect(std::shared_ptr<net::connection<net::message_types>> client) override {
		std::cout << "Removing Client [" << client->getID() << "]\n";
	}

	void onMessage(std::shared_ptr<net::connection<net::message_types>> client, net::message<net::message_types>& msg) override {
		switch (msg.getHeader().id) {
			case net::message_types::ServerAll: {
				std::cout << "[" << client->getID() << "]: Message All\n";
				net::message<net::message_types> msg;
				msg.getHeader().id = net::message_types::ServerAll;
				msg << client->getID();
				this->messageAllClients(msg, client);
			}
			break;
			case net::message_types::ServerPing: {
				std::cout << "[" << client->getID() << "]: Server Ping\n";
				client->send(msg);
			}
			break;
		}
	}
};

int main() {
	CustomServer server(60000);
	server.start();

	while (1)
		server.update(-1, true);

	return 0;
}
