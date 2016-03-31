// =============================================================================
// Description: LiveGrapher wire protocol
// Author: FRC Team 3512, Spartatroniks
// =============================================================================

#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <string>

/* See README.md in the root directory of this project for protocol
 * documentation.
 */

struct[[gnu::packed]] HostPacket{
    uint8_t ID;
};

constexpr uint8_t k_hostConnectPacket = 0b00 << 6;
constexpr uint8_t k_hostDisconnectPacket = 0b01 << 6;
constexpr uint8_t k_hostListPacket = 0b10 << 6;

struct[[gnu::packed]] ClientDataPacket{
    uint8_t ID;
    uint64_t x;
    float y;
};

struct ClientListPacket {
    uint8_t ID;
    uint8_t length;
    std::string name;
    uint8_t eof;
};

constexpr uint8_t k_clientDataPacket = 0b00 << 6;
constexpr uint8_t k_clientListPacket = 0b01 << 6;

#endif // PROTOCOL_HPP
