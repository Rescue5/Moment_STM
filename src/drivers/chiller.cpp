#include "drivers.hpp"

#include <algorithm>

namespace {

class driver : public Chiller {
	TIM_HandleTypeDef *_h_ppm, *_h_pwm;

	const uint32_t _pulseMin = 0;
	const uint32_t _pulseMax = 400;

	uint32_t _throttle_deadline = 0;

	void setThrottle(int n) {
		n = std::clamp(n, 1, 100);
		n = _pulseMin + ((_pulseMax - _pulseMin) * n) / 100;
		__HAL_TIM_SET_COMPARE(_h_pwm, TIM_CHANNEL_1, n);
		__HAL_TIM_SET_COMPARE(_h_pwm, TIM_CHANNEL_2, n);
	}

	void enable(bool on, int arg) {
		if (on) {
			setThrottle(arg);
			HAL_TIM_PWM_Start(_h_pwm, TIM_CHANNEL_1);
			HAL_TIM_PWM_Start(_h_pwm, TIM_CHANNEL_2);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
			_throttle_deadline = 0;
		} else if (arg) {
			_throttle_deadline = HAL_GetTick() + std::clamp(arg, 1'000, 30'000);
		} else {
			setThrottle(0);
			HAL_TIM_PWM_Stop(_h_pwm, TIM_CHANNEL_1);
			HAL_TIM_PWM_Stop(_h_pwm, TIM_CHANNEL_2);
			HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
			_throttle_deadline = 0;
		}
	}

	long do_init_ppm(void) {
		return HAL_ERROR; // FIXME
	}

	long do_init_pwm(void) {
		_h_pwm->Init.Prescaler = (168-1); // FIXME: dynamic -- to get 1MHz (1us)
		_h_pwm->Init.CounterMode = TIM_COUNTERMODE_UP;
		_h_pwm->Init.Period = (400-1);
		_h_pwm->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
		_h_pwm->Init.RepetitionCounter = 0;
		_h_pwm->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
		if (HAL_TIM_PWM_Init(_h_pwm) != HAL_OK) {
			return HAL_ERROR;
		}

		TIM_ClockConfigTypeDef clockConfig = { 0 };
		clockConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
		if (HAL_TIM_ConfigClockSource(_h_pwm, &clockConfig) != HAL_OK) {
			return HAL_ERROR;
		}

		TIM_OC_InitTypeDef channelConfig = { 0 };
		channelConfig.OCMode = TIM_OCMODE_PWM1;
		channelConfig.Pulse = 0; // throttle = 0
		channelConfig.OCPolarity = TIM_OCPOLARITY_HIGH;
		channelConfig.OCNPolarity = TIM_OCNPOLARITY_HIGH;
		channelConfig.OCFastMode = TIM_OCFAST_DISABLE;
		channelConfig.OCIdleState = TIM_OCIDLESTATE_RESET;
		channelConfig.OCNIdleState = TIM_OCNIDLESTATE_RESET;
		HAL_TIM_PWM_ConfigChannel(_h_pwm, &channelConfig, TIM_CHANNEL_1);
		HAL_TIM_PWM_ConfigChannel(_h_pwm, &channelConfig, TIM_CHANNEL_2);

		return HAL_OK;
	}

	long do_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOD_CLK_ENABLE(); // PD15

		gpio.Pin = GPIO_PIN_15;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		gpio.Pull = GPIO_NOPULL;
		HAL_GPIO_Init(GPIOD, &gpio);

		do_init_pwm();
		do_init_ppm();

		enable(0, 0);

		return HAL_OK;
	}

	long do_msp_init_ppm(void) {
		return HAL_ERROR; // FIXME
	}

	long do_msp_init_pwm(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOD_CLK_ENABLE(); // PD12, PD13

		gpio.Pin = GPIO_PIN_12 | GPIO_PIN_13;
		gpio.Mode = GPIO_MODE_AF_PP;
		gpio.Pull = GPIO_NOPULL;
		gpio.Speed = GPIO_SPEED_FREQ_LOW;
		gpio.Alternate = GPIO_AF2_TIM4;
		HAL_GPIO_Init(GPIOD, &gpio);

		__HAL_RCC_TIM4_CLK_ENABLE();

		return HAL_OK;
	}

public:
	driver(TIM_HandleTypeDef *ppm, TIM_HandleTypeDef *pwm) : _h_ppm(ppm), _h_pwm(pwm) {}

	long init(void) {
		return do_init();
	}

	long mspInit(Handle *h) {
		long res = HAL_OK;
		if (h->Instance == _h_pwm->Instance) {
			res = do_msp_init_pwm();
		} else if (h->Instance == _h_ppm->Instance) {
			res = do_msp_init_ppm();
		} return res;
	}

	void control(int cmd, int arg) {
		switch (cmd) {
		case 0:
			enable(0, arg); // stop
			break;
		case 1:
			enable(1, arg ?: 100); // (re)start
			break;
		default:
			break;
		}
	}

	void run(uint32_t millis) {
		if (_throttle_deadline && (millis > _throttle_deadline)) {
			enable(0, 0);
		}
		Runnable::arm(HAL_GetTick() + 100);
	}
};

} // namespace

Chiller *chiller(void) {
	static TIM_HandleTypeDef ppm = { .Instance = TIM3 };
	static TIM_HandleTypeDef pwm = { .Instance = TIM4 };
	static driver drv(&ppm, &pwm);
	return &drv;
}
