#include "common.h"

#include "device.hpp"

#include "drivers.hpp"

static inline int issign(int c) {
	return c == '-' || c == '+';
}

static int atoi(const char **p) {
	int v = 0, inv = 0;

	while (**p && !issign((int)**p) && !isdigit((int)**p)) ++(*p);

	if (issign(**p)) {
		inv = (**p == '-'), (*p)++;
	}

	while (isdigit((int)**p)) {
		v = v * 10 + (**p - '0'), (*p)++;
	}

	return inv ? -v : v;
}

////////////////////////////////////////////////////////////////////////////////

namespace {

class device : public Device {
	enum Connection {
		Unplugged,
		Plugged,
	};

	uint32_t _lastCommandAt = 0;

	bool connectionAlive(void) {
		return _lastCommandAt && ((_lastCommandAt + 500) > HAL_GetTick());
	}

	Connection _connection = Connection::Unplugged;

	void connectionSwitch(Connection what) {
		if (what == _connection) {
			return; // no change
		}

		switch (what) {
		case Connection::Plugged:
			led()->setMode(100);
			chiller()->start(20);
			break;
		case Connection::Unplugged:
			led()->setMode(500);
			chiller()->stop(10 * 1000);
			motorBrake()->moveBack();
			motor()->stop();
			break;
		}

		_connection = what;
	}

public:
	device() {}

	long init(void) {
		return driverManager()->init();
	}

	void tick(uint32_t millis) {
		driverManager()->tick(millis);
		if (connectionAlive()) {
			connectionSwitch(Connection::Plugged);
		} else {
			connectionSwitch(Connection::Unplugged);
		}
	}

	void dispatch(const char *cmd) {
		const char *p = cmd + 1;

		if (cmd[0] != '/') {
			return; // not a command prefix
		} else if (!strcmp(p, "id")) {
			tinyUSB()->write("DronMotors");
		} else if (!strcmp(p, "ping")) {
			tinyUSB()->write("PONG");
		} else {
			bool resOk = true;
#define is_action(cmd, action)	(!strncmp(cmd, action, sizeof(action) - 1))
			if (is_action(p, "tare")) {
				hx711()->tare();
				motorBrake()->tare();
			} else if (is_action(p, "brake")) {
				motorBrake()->control(atoi(&p), atoi(&p));
			} else if (is_action(p, "sample")) {
				telemetry()->updateInterval(atoi(&p));
			} else if (is_action(p, "chiller")) {
				chiller()->control(atoi(&p), atoi(&p));
			} else if (is_action(p, "throttle")) {
				motor()->throttle(atoi(&p));
			} else {
				resOk = false;
			}
# undef is_action
			tinyUSB()->write(resOk ? "OK" : "ERROR"); // ack
		}

		_lastCommandAt = HAL_GetTick();
	}

	static void plug(device *dev) {
		dev->connectionSwitch(Connection::Plugged);
	}

	static void unplug(device *dev) {
		dev->connectionSwitch(Connection::Unplugged);
	}
};

} // namespace

//
// FIXME: USB disconnect detection is not working, see
//        https://github.com/hathach/tinyusb/issues/2872
//

#if 0
void tud_mount_cb(void) {
	pr_dbg("tud_mount_cb\n\r");
}

void tud_umount_cb(void) {
	pr_dbg("tud_umount_cb\n\r");
}

void tud_resume_cb(void) {
	pr_dbg("tud_resume_cb\n\r");
}

void tud_suspend_cb(bool f) {
	pr_dbg("tud_suspend_cb\n\r");
}

void tud_cdc_line_state_cb(uint8_t itf, bool dtr, bool rts) {
	pr_dbg("tud_cdc_line_state_cb\n\r");
}
#endif

////////////////////////////////////////////////////////////////////////////////

Device *theDevice(void) {
	static device dev;
	return &dev;
}
