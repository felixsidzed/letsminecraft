#include "server.h"

#include <format>
#include <iomanip>

#include "logger/logger.h"
#include "network/packet.h"

extern uint32_t readVarInt(const uint8_t* data, uint32_t& offset);

namespace lmc {
	using namespace logger;

	Server::Server(uint16_t maxPlayers) : maxPlayers(maxPlayers), socket(INVALID_SOCKET) {
		WSADATA wsaData;
		(void)WSAStartup(MAKEWORD(2, 2), &wsaData);
	}

	Server::~Server() {
		closesocket(socket);
		WSACleanup();
	}

	void Server::listen(uint16_t port) {
		socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		sockaddr_in addr{};
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = INADDR_ANY;
		addr.sin_port = htons(port);
		bind(socket, (sockaddr*)&addr, sizeof(addr));
		::listen(socket, SOMAXCONN);

		log(Level::Info, "Server started");

		while (true) {
			SOCKET clientSocket = accept(socket, nullptr, nullptr);
			if (clientSocket != INVALID_SOCKET) {
				lmc::lock lock(clientsMtx);

				ClientInfo* client = clients.emplace(net::ProtocolState::Handshaking, clientSocket);
				std::thread(&Server::serve, this, client).detach();
			}
		}
	}

	void Server::serve(ClientInfo* client) {
		log(Level::Info, "Client connected");

		uint32_t offset = 0;
		std::unique_ptr<net::Packet> packet;

	nextPacket:
		{
			uint8_t tmp[5];
			if (recv(client->socket, (char*)tmp, 5, MSG_PEEK) <= 0) {
				log(Level::Error, "Client disconnected");
				goto exit;
			}

			offset = 0;
			uint32_t length = readVarInt(tmp, offset);
			length += offset;
			std::unique_ptr<uint8_t[]> raw = std::make_unique<uint8_t[]>(length);

			uint32_t total = 0;
			while (total < length) {
				int recieved = recv(client->socket, (char*)raw.get() + total, length - total, 0);
				if (recieved <= 0) {
					log(Level::Error, "Client disconnected");
					goto exit;
				}
				total += recieved;
			}

			packet = net::Packet::decode(raw.get());
		}

		log(Level::Debug, "Received packet ", std::hex, (uint32_t)packet->id, std::dec, " (length: ", packet->length, ")");

		switch (client->state) {
		case net::ProtocolState::Handshaking: {
			if (packet->id != net::PacketID::Handshake)
				goto unhandledPacket;

			net::HandshakePacket* handshake = (net::HandshakePacket*)packet.get();
			if (handshake->ProtocolVersion != ProtocolVersion)
				goto exit;

			client->state = handshake->NextState;
			goto nextPacket;
		}

		case net::ProtocolState::Status: {
			switch (packet->id) {
			case net::PacketID::Status: {
				uint32_t onlinePlayers = 0;
				{
					lmc::lock lock(clientsMtx);
					for (const auto& c : clients)
						if (c.state == net::ProtocolState::Play)
							onlinePlayers++;
				}

				net::StatusPacket status("1.21.10", ProtocolVersion, maxPlayers, onlinePlayers, description.data(), false);
				auto [re, reSize] = status.encode();

				send(client->socket, (char*)re, reSize, 0);
				goto nextPacket;
			}

			case net::PacketID::Ping:
			   send(client->socket, (char*)packet->raw.get(), packet->length, 0);
			   goto exit;

			default:
				goto unhandledPacket;
			}
		}

		default:
			log(Level::Error, "Unhandled state");
			goto exit;
		}

	unhandledPacket:
		log(Level::Error, "Unhandled packet");

	exit:
		{
			lmc::lock lock(clientsMtx);
			auto idx = (uint32_t)(clients.data - client);
			clients.erase(idx);
		}
		return;
	}
}
