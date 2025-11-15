#pragma once

#include <string>
#include <chrono>
#include <iostream>

#include "common/string.h"

#define COLOR_RESET		"\033[0m"
#define COLOR_GREY		"\033[90m"
#define COLOR_CYAN		"\033[36m"
#define COLOR_GREEN		"\033[32m"
#define COLOR_YELLOW	"\033[33m"
#define COLOR_RED		"\033[31m"
#define COLOR_MAGENTA	"\033[35m"

namespace lmc::logger {
	enum class Level {
		Info,
		Warn,
		Error,
		Debug
	};

	static const char* strlevel(Level level) {
		switch (level) {
		case Level::Warn:   return COLOR_YELLOW "[WARN] ";
		case Level::Error:  return COLOR_RED "[ERROR] ";
		case Level::Debug:  return COLOR_MAGENTA "[DEBUG] ";
		default:            return COLOR_CYAN "[INFO] ";
		}
	}

	inline std::string timestamp() {
		using namespace std::chrono;

		auto now = system_clock::now();
		auto t = system_clock::to_time_t(now);

		tm tm;
		localtime_s(&tm, &t);

		std::ostringstream oss;
		oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
		return oss.str();
	}

	template <typename T>
	void printc(const T& value) {
		if constexpr (std::is_invocable_v<T, std::ostream&>)
			value(std::cout);
		else if constexpr (std::is_same_v<T, const char*>)
			std::cout << COLOR_GREEN << value << COLOR_RESET;
		else if constexpr (std::is_same_v<T, lmc::string>)
			std::cout << COLOR_GREEN << std::string_view{ value.data(), value.length() } << COLOR_RESET;
		else if constexpr (std::is_same_v<T, std::string_view> || std::is_same_v<T, std::string>)
			std::cout << COLOR_GREEN << value << COLOR_RESET;
		else if constexpr (std::is_arithmetic_v<T>) {
			std::ios_base::fmtflags flags = std::cout.flags();

			std::ostringstream oss;
			oss.setf(flags, std::ios::basefield);
			oss << value;

			if ((flags & std::ios::basefield) == std::ios::hex)
				std::cout << COLOR_YELLOW << "0x" << oss.str() << COLOR_RESET;
			else
				std::cout << COLOR_YELLOW << oss.str() << COLOR_RESET;
		} else
			std::cout << value << COLOR_RESET;
	}

	template <typename ...Args>
	void printv(Args&&... args) {
		(printc(std::forward<Args>(args)), ...);
	}

	template <typename ...Args>
	void log(Level level, Args&&... args) {
		std::cout << COLOR_GREY << "[" << timestamp() << "] " << strlevel(level) << COLOR_RESET;

		printv(std::forward<Args>(args)...);
		std::cout << "\n";
	}
}
