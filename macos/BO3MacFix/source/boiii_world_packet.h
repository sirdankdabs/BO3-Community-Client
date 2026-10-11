#ifndef BOIII_WORLD_PACKET_H
#define BOIII_WORLD_PACKET_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
// Header + empty acknowledgements occupy 41 bytes in the observed protocol.
// Require payload beyond that header; reliable gamestates are excluded.
static inline bool boiii_world_packet(const uint8_t *bytes, size_t length) {
    return bytes && length > 64 && bytes[1] == 0xe0 && bytes[18] == 0x00;
}
#endif
