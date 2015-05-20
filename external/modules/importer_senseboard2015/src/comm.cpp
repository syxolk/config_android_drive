#include <comm/comm.h>
#include <comm/protocol.h>
#include <stdio.h>

extern uint8_t Crc8(const void *vptr, unsigned int len);

void finalize_up(struct up* up) {
    up->checksum = Crc8(up, sizeof(*up) - sizeof(up->checksum));
}

void finalize_down(struct down* down) {
	down->checksum = Crc8(down, sizeof(*down) - sizeof(down->checksum) - 1);
}

bool check_up(struct up* up) {
	uint8_t  expected = Crc8(up, sizeof(*up) - sizeof(up->checksum));
    return up->checksum == expected;
}

bool check_down(struct down* down) {
	uint8_t expected = Crc8(down, sizeof(*down) - sizeof(down->checksum) - 1);
    return down->checksum == expected;
}
