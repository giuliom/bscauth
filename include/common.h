#ifndef COMMON_UUID_SERVER_CLIENT_HPP
#define COMMON_UUID_SERVER_CLIENT_HPP

#include <asio.hpp>

#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// Generate an RFC-4122 version 4 UUID (random) as lowercase string.
inline std::string generate_uuid_v4() {
    std::array<std::uint8_t, 16> bytes{};

    // Use random_device directly for each 32-bit chunk (simple approach).
    // For higher throughput you could seed a fast PRNG once per thread with entropy.
    std::random_device rd;
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        bytes[i] = static_cast<std::uint8_t>(rd());
    }

    // Set version (4) and variant (10xx)
    bytes[6] = (bytes[6] & 0x0F) | 0x40; // version 4
    bytes[8] = (bytes[8] & 0x3F) | 0x80; // variant 1 (RFC 4122)

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (int i = 0; i < 16; ++i) {
        oss << std::setw(2) << static_cast<int>(bytes[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) oss << '-';
    }
    return oss.str();
}

#endif // COMMON_UUID_SERVER_CLIENT_HPP