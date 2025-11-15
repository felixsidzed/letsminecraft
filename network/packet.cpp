#include "packet.h"

#include <format>
#include <ws2tcpip.h>

uint32_t readVarInt(const uint8_t* data, uint32_t& offset) {
	uint32_t result = 0;
	uint32_t shift = 0;

	uint8_t byte = 0;
	do {
		byte = *(data + offset++);
		result |= (byte & 0x7F) << shift;
		shift += 7;
	} while (byte & 0x80);

	return result;
}

static void writeVarInt(uint8_t* buf, uint32_t value, uint32_t& offset) {
	do {
		uint8_t byte = value & 0x7F;
		value >>= 7;
		if (value != 0)
			byte |= 0x80;
		buf[offset++] = byte;
	} while (value != 0);
}

template <typename T>
static T read(const uint8_t* buf, uint32_t& offset) {
	T value = 0;
	for (size_t i = 0; i < sizeof(T); ++i) {
		value <<= 8;
		value |= buf[offset++];
	}
	return value;
}

template <typename T>
static void write(uint8_t* buf, T value, uint32_t& offset) {
	for (int i = sizeof(T) - 1; i >= 0; i--) {
		buf[++offset] = (uint8_t)(value & 0xFF);
		value >>= 8;
	}
	offset += sizeof(T);
}

static uint32_t varIntLength(uint32_t value) {
	if (value < 0x80) return 1;
	else if (value < 0x4000) return 2;
	else if (value < 0x200000) return 3;
	else if (value < 0x10000000) return 4;
	else return 5;
}

static uint32_t parseIPV4(const char* s, size_t size) {
	char tmp[16];
	size = size < 15 ? size : 15;

	tmp[size] = '\0';

	uint32_t result;
	if (!inet_pton(AF_INET, tmp, &result))
		return 0;
	return ntohl(result);
}


namespace lmc::net {
	std::unique_ptr<Packet> Packet::decode(const uint8_t* raw) {
		uint32_t offset = 0;
		uint32_t length = readVarInt(raw, offset);
		net::PacketID id = (net::PacketID)readVarInt(raw, offset);

		switch (id) {
		case PacketID::Handshake: {
			auto packet = std::make_unique<HandshakePacket>();
			packet->length = length;
			packet->id = id;
			packet->raw = std::make_unique<uint8_t[]>(length);
			memcpy_s(packet->raw.get(), length, raw, length);

			packet->ProtocolVersion = readVarInt(raw, offset);
			uint32_t ServerAddressLength = readVarInt(raw, offset);
			packet->ServerAddress = parseIPV4((char*)(packet->raw.get() + offset), ServerAddressLength);
			offset += ServerAddressLength;
			packet->ServerPort = read<uint16_t>(raw, offset);
			packet->NextState = (net::ProtocolState)readVarInt(raw, offset);

			return packet;
		}

		default:
			auto packet = std::make_unique<Packet>();
			packet->length = length;
			packet->id = id;
			packet->raw = std::make_unique<uint8_t[]>(length);
			memcpy_s(packet->raw.get(), length, raw, length);
			return packet;
		}
	}

	StatusPacket::StatusPacket(
		const char* VersionName, uint16_t ProtocolVersion,
		uint16_t MaxPlayers, uint16_t OnlinePlayers,
		const char* Description,
		bool EnforcesSecureChat
	)
		: ProtocolVersion(ProtocolVersion), MaxPlayers(MaxPlayers), OnlinePlayers(OnlinePlayers), Description(Description), EnforcesSecureChat(EnforcesSecureChat)
	{
		VersionName++; // Skip 1 (Minecraft 2 isn't releasing any time soon)
		memcpy_s(this->VersionName, 7, VersionName, strlen(VersionName));
	}

	std::pair<uint8_t*, uint32_t> StatusPacket::encode() {
		const std::string& json = std::format(
			R"({{"version":{{"name":"1{}","protocol":{}}},"players":{{"max":{},"online":{}}},"description":{{"text":"{}"}},"enforcesSecureChat":{}}})",
			(char*)VersionName,
			ProtocolVersion,
			MaxPlayers, OnlinePlayers,
			Description,
			EnforcesSecureChat ? "true" : "false"
		);

		size_t total = json.size() + varIntLength((uint32_t)json.size()) + 1;
		size_t actualTotal = total + varIntLength(total);
		raw.reset(new uint8_t[actualTotal]);

		length = (uint32_t)total;

		uint32_t offset = 0;
		writeVarInt(raw.get(), (uint32_t)total, offset);
		writeVarInt(raw.get(), (uint32_t)id, offset);
		writeVarInt(raw.get(), (uint32_t)json.size(), offset);
		memcpy_s(raw.get() + offset, actualTotal - offset, json.data(), json.size());

		return { raw.get(), actualTotal };
	}
}
