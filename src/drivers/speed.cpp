#include "common.h"

#include "drivers.hpp"

namespace {

class driver : public Runnable, public MspDriver, public MspCallback, public MspIrqHandler {
	volatile uint32_t _rpm_t, _rpm_ts, _last_ts;

	long do_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOC_CLK_ENABLE();

		gpio.Pin = GPIO_PIN_5;
		gpio.Mode = GPIO_MODE_IT_RISING;
		gpio.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOA, &gpio);

		HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

		return HAL_OK;
	}

	static inline constexpr uint32_t ss2ms(uint32_t n) {
		return n * 1'000;
	}

	static inline constexpr uint32_t ms2us(uint32_t n) {
		return n * 1'000;
	}

public:

	long init(void) {
		return do_init();
	}

	long mspInit(Handle *) {
		return HAL_OK;
	}

	void mspCallback(void *p, Handle *h) {
		if (p == &HAL_GPIO_EXTI_Callback && h->Pin == GPIO_PIN_5) {
			uint32_t t = GetMicros();
			// FIXME: better debouncing & some averaging?
			if ((t - _last_ts) < 500) {
				_rpm_t = 0, _last_ts = t;
			} else {
				_rpm_t = t - _last_ts, _last_ts = t;
			}
		}
	}

	void mspIrqHandler(IRQn_Type irq) {
		if (irq == EXTI9_5_IRQn) {
			HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_5);
		}
	}

	void run(uint32_t millis) {
		uint32_t rpm = 0, rpm_t = _rpm_t; // grab rpm_t
		// NOTE: support 600-60000 RPM range with 1-100ms period times
		if (rpm_t >= ms2us(1) && rpm_t <= ms2us(100)) {
			rpm = ms2us(ss2ms(60)) / (1 + rpm_t);
		}
		telemetry_set(MotorRPM, rpm);
		Runnable::arm(HAL_GetTick() + 10);
	}
};


} // namespace

Driver *speed(void) {
	static driver drv;
	return &drv; // singleton
}
