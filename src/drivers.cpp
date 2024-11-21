#include "drivers.hpp"

#include <iterator> // std::size

namespace {

static Driver *_drivers[] = {
	led(), //serial(), //beeper(),
	hx711(), ina236(), //mpu6050(), max6675(),
    speed(), motor(), //motorBrake(), chiller()
	telemetry(), tinyUSB(),
};

class manager : public DriverManager {
	size_t _d_runnable_count = 0;
	Runnable *_d_runnable[ std::size(_drivers) ];

	size_t _d_msp_driver_count = 0;
	MspDriver *_d_msp_driver[ std::size(_drivers) ];

	size_t _d_msp_callback_count = 0;
	MspCallback *_d_msp_callback[ std::size(_drivers) ];

	size_t _d_msp_irq_handler_count = 0;
	MspIrqHandler *_d_msp_irq_handler[ std::size(_drivers) ];

public:
	manager() {}

	long init(void) {
		for (size_t i = 0; i < std::size(_drivers); i++) {
			auto d = _drivers[i];
			if (auto v = dynamic_cast<Runnable *>(d); v != nullptr) {
				_d_runnable[ _d_runnable_count++ ] = v;
			}
			if (auto v = dynamic_cast<MspDriver *>(d); v != nullptr) {
				_d_msp_driver[ _d_msp_driver_count++ ] = v;
			}
			if (auto v = dynamic_cast<MspCallback *>(d); v != nullptr) {
				_d_msp_callback[ _d_msp_callback_count++ ] = v;
			}
			if (auto v = dynamic_cast<MspIrqHandler *>(d); v != nullptr) {
				_d_msp_irq_handler[ _d_msp_irq_handler_count++ ] = v;
			}
		}

		for (size_t i = 0; i < std::size(_drivers); i++) {
			if (auto res = _drivers[i]->init(); res != HAL_OK) {
				return res;
			}
		}

		return HAL_OK;
	}

	void tick(uint32_t millis) {
		for (size_t i = 0; i < _d_runnable_count; i++) {
			_d_runnable[i]->tick(millis);
		}
	}

	long msp_init(Handle *h) {
		for (size_t i = 0; i < _d_msp_driver_count; i++) {
			if (auto res = _d_msp_driver[i]->mspInit(h); res != HAL_OK) {
				return res;
			}
		}

		return HAL_OK;
	}

	void msp_callback(void *p, Handle *h) {
		for (size_t i = 0; i < _d_msp_callback_count; i++) {
			_d_msp_callback[i]->mspCallback(p, h);
		}
	}

	void msp_irq_handler(IRQn_Type irq) {
		for (size_t i = 0; i < _d_msp_irq_handler_count; i++) {
			_d_msp_irq_handler[i]->mspIrqHandler(irq);
		}
	}
};

} // namespace

DriverManager *driverManager(void) {
	static manager drv;
	return &drv;
}
