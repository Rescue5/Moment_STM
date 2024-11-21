#include "common.h"

#include "drivers.hpp"

#include <algorithm>

namespace {
	class driver : public Telemetry {
		uint32_t _interval = 10;

		driver() {}
		driver(const driver&) {}

#define telemetry_item(n)			\
	[ __tlmIdx##n * 2 ] = TLM_ID_N(n),	\
	[ __tlmIdx##n * 2 + 1] = 0

		uint32_t _telemetry[ __tlmIdxMAX * 2 ] = {
			telemetry_item(Ts),
			telemetry_item(Load1),
            telemetry_item(Load2),
            telemetry_item(Load3),
			telemetry_item(Temp1),
			telemetry_item(Temp2),
			telemetry_item(Temp3),
			telemetry_item(Brake),
			telemetry_item(MotorI),
			telemetry_item(MotorU),
			telemetry_item(MotorP),
			telemetry_item(MotorRPM),
			telemetry_item(MotorThrottle),
			telemetry_item(GyroX),
			telemetry_item(GyroY),
			telemetry_item(GyroZ),
		};

# undef telemetry_item

		void update(int id, uint32_t val) {
			if (id < __tlmIdxMAX) _telemetry[ id * 2 + 1] = val;
		}

	public:
		long init(void) {
			return HAL_OK;
		}

		void run(uint32_t millis) {
			telemetry_set(Ts, millis);
			tinyUSB()->write((const void *)_telemetry, sizeof(_telemetry));
			Runnable::arm(HAL_GetTick() + _interval);
		}

		void updateInterval(int v) {
			_interval = std::clamp(v, 1, 1000);
		}

		static driver *instance(void) {
			static driver drv;
			return &drv;
		}
	};
}

Telemetry *telemetry(void) {
	return driver::instance();
}
