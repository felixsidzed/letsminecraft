#pragma once

namespace lmc::net {
	enum class ProtocolState {
		Handshaking,
		Status,
		Login,
		Play
	};
}
