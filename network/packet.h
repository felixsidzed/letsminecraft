#pragma once

#include "common/string.h"
#include "common/vector.h"

#include "state.h"

namespace lmc::net {
	using bytebuf = lmc::vector<uint8_t>;

	enum class PacketID {
		// Login in
		LoginStart = 0x00, LoginEncryption = 0x01, LoginSuccess = 0x02,

		// Miscellaneous
		Handshake = 0x00, Status = 0x00, Ping = 0x01
	};

	class Packet {
	public:
		// WARNING: This is not always set for clientbound packets
		uint32_t length = 0;
		PacketID id = PacketID::Handshake;

		std::unique_ptr<uint8_t[]> raw = nullptr;

		virtual std::pair<uint8_t*, uint32_t> encode() {
			throw std::runtime_error("This packet cannot be encoded");
		}

		static std::unique_ptr<Packet> decode(const uint8_t* raw);
	};

	class HandshakePacket : public Packet {
	public:
		uint32_t ProtocolVersion = 0;
		uint32_t ServerAddress = 0;
		ProtocolState NextState = ProtocolState::Handshaking;
		uint16_t ServerPort = 0;
	};

	class StatusPacket : public Packet {
	public:
		char VersionName[7];
		bool EnforcesSecureChat;
		uint32_t ProtocolVersion;
		uint16_t MaxPlayers;
		uint16_t OnlinePlayers;
		const char* Description;

		std::pair<uint8_t*, uint32_t> encode() override;

		StatusPacket(
			const char* VersionName, uint16_t ProtocolVersion,
			uint16_t MaxPlayers, uint16_t OnlinePlayers,
			const char* Description,
			bool EnforcesSecureChat
		);
	};
}
