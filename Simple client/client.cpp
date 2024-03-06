#include <iostream>
#include <net1++.h>

class CustomClient : public net::client_interface<net::message_types> {
public:
	void pingServer() {
		net::message<net::message_types> msg;
		msg.getHeader().id = net::message_types::ServerPing;

		// Caution with systemclock because of platform dependence
		std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

		msg << timeNow;
		this->send(msg);
	}

	void messageAll() {
		net::message<net::message_types> msg;
		msg.getHeader().id = net::message_types::ServerAll;
		this->send(msg);
	}
};

int main() {
	CustomClient c;
	c.connect("127.0.0.1", 60000);
	

	// WIN32/-64 only!
	bool key[3] = { false, false, false };
	bool old_key[3] = { false, false, false };

	bool bQuit = false;
	while (!bQuit) {
		//if (GetForegroundWindow() == GetConsoleWindow()) {
			key[0] = GetAsyncKeyState('1') & 0x8000;
			key[1] = GetAsyncKeyState('2') & 0x8000;
			key[2] = GetAsyncKeyState('3') & 0x8000;
		//}

		if (key[0] && !old_key[0]) c.pingServer();
		if (key[1] && !old_key[1]) c.messageAll();
		if (key[2] && !old_key[2]) bQuit = true;

		for (int i = 0; i < 3; i++) old_key[i] = key[i];

		if (c.isConnected()) {
			if (!c.incoming().empty()) {
				auto msg = c.incoming().pop_front().getMsg();

				switch (msg.getHeader().id) {
					// Server accepted conenction request				
					case net::message_types::ServerAccept: {
						std::cout << "Server Accepted Connection\n";
					}
					break;
					// Server has responded to a ping request
					case net::message_types::ServerPing: {
						std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
						std::chrono::system_clock::time_point timeThen;
						msg >> timeThen;
						std::cout << "Ping: " << std::chrono::duration<double>(timeNow - timeThen).count() << "\n";
					}
					break;
					// Server sends message to all clients
					case net::message_types::ServerAll: {
						uint32_t clientID;
						msg >> clientID;
						std::cout << "Hello from [" << clientID << "]\n";
					}
					break;
				}
			}
		}
		else {
			std::cout << "Server Down\n";
			bQuit = true;
		}

	}
	return 0;
}