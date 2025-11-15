#pragma once

#include <thread>
#include <winsock2.h>

#include "common/mutex.h"
#include "common/vector.h"
#include "common/string.h"

#include "network/state.h"

namespace lmc {
	class Server {
		class ClientInfo {
		public:
			net::ProtocolState state;

			SOCKET socket;

			ClientInfo(net::ProtocolState state, SOCKET socket) : state(state), socket(socket) {}
			~ClientInfo() {
				closesocket(socket);
			}

			ClientInfo(ClientInfo&&) noexcept = default;
			ClientInfo& operator=(ClientInfo&&) noexcept = default;
		};

		SOCKET socket;

		uint16_t maxPlayers;

		lmc::mutex clientsMtx;
		lmc::vector<ClientInfo> clients;

	public:
		static constexpr uint32_t ProtocolVersion = 773;

		lmc::string description;

		Server(uint16_t maxPlayers);
		~Server();

		void listen(uint16_t port);

	private:
		void serve(ClientInfo* client);
	};
}
