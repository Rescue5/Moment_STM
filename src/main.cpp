#include "common.h"

#include <algorithm>

#include "drivers.hpp"

#include "device.hpp"

////////////////////////////////////////////////////////////////////////////////

void rx_bytes(uint8_t *ptr, size_t len) {
	static char cmd[ 64 + 1 ], cmdlen = 0;

	for (size_t i = 0; i < len; i++) {
		const char received = ptr[i];

		if (!cmdlen && (received != '/'))
			continue;
		else if (received == '\r' || received == '\n') {
			theDevice()->dispatch(cmd);
			memset(cmd, 0, sizeof(cmd)), cmdlen = 0;
		} else {
			if (cmdlen >= (sizeof(cmd) - 1)) {
				memmove(cmd, cmd + 1, sizeof(cmd) - 1), cmdlen -= 1;
			}
			cmd[(int)cmdlen++] = received;
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

int main() {
	board_init();

	theDevice()->init();

	pr_dbg("** started **\n\r");

	while (1) {
		const uint32_t millis = HAL_GetTick();
		theDevice()->tick(millis);
	}

	return 0;
}
