#include "drivers.hpp"

#include "common.h"

//
// port    pin          initial state   | HR4988  | inverted |
// ------+-------------+----------------+---------+----------|
// GPIOB | GPIO_PIN_1  | GPIO_PIN_RESET | DIR     |          |
// GPIOB | GPIO_PIN_9  | GPIO_PIN_SET   | MS3     | yes      |
// GPIOC | GPIO_PIN_5  | GPIO_PIN_RESET | STEP    |          |
// GPIOC | GPIO_PIN_13 | GPIO_PIN_SET   | \SLEEP  |          |
// GPIOE | GPIO_PIN_0  | GPIO_PIN_SET   | MS1     | yes      |
// GPIOE | GPIO_PIN_1  | GPIO_PIN_SET   | MS2     | yes      |
// GPIOE | GPIO_PIN_5  | GPIO_PIN_RESET | \ENABLE |          |
// GPIOE | GPIO_PIN_6  | GPIO_PIN_SET   | \RST    |          |
//

namespace {

class driver : public MotorBrake {
	int _pos = 0, _targetPos = 0;

	void setDir(int dir) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
	}

	void setStepMode(int mode) {
		GPIO_PinState ms1 = (mode & 0b001) ? GPIO_PIN_RESET : GPIO_PIN_SET;
		GPIO_PinState ms2 = (mode & 0b010) ? GPIO_PIN_RESET : GPIO_PIN_SET;
		GPIO_PinState ms3 = (mode & 0b100) ? GPIO_PIN_RESET : GPIO_PIN_SET;

		__disable_irq();
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, ms1);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, ms2);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, ms3);
		__enable_irq();
	}

	void step(void) {
		int dir = _targetPos > _pos ? +1 : -1;

		__disable_irq();
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_SET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
		__enable_irq();

		if (_pos > INT_MIN && _pos < INT_MAX) {
			_pos += dir;
		}

		telemetry_set(Brake, _pos);
	}

	void moveTo(int target) {
		_targetPos = target;
		setDir(!!(_targetPos > _pos));
	}

	long do_init(void) {
		GPIO_InitTypeDef gpio = { 0 };

		__HAL_RCC_GPIOB_CLK_ENABLE();
		__HAL_RCC_GPIOC_CLK_ENABLE();
		__HAL_RCC_GPIOE_CLK_ENABLE();

		gpio.Pin = GPIO_PIN_1 | GPIO_PIN_9; // PB1, PB9
		gpio.Pull = GPIO_NOPULL;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOB, &gpio);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, GPIO_PIN_SET); // inv

		gpio.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_5 | GPIO_PIN_6; // PE0, PE1, PE5, PE6
		gpio.Pull = GPIO_NOPULL;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOE, &gpio);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET); // inv
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET); // inv
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_5, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_6, GPIO_PIN_SET);

		gpio.Pin = GPIO_PIN_5 | GPIO_PIN_13; // PC5, PC13
		gpio.Pull = GPIO_NOPULL;
		gpio.Mode = GPIO_MODE_OUTPUT_PP;
		HAL_GPIO_Init(GPIOC, &gpio);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_5, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

		return HAL_OK;
	}

public:
	long init(void) {
		return do_init();
	}

	long mspInit(Handle *h) {
		return HAL_OK;
	}

	void run(uint32_t millis) {
		constexpr int sps = 1000; // steps per second
		if (moving()) step();
		Runnable::arm(HAL_GetTick() + (1000 / sps));
	}

	void tare(void) {
		_pos = _targetPos = 0;
		telemetry_set(Brake, _pos);
	}

	bool moving(void) {
		return _pos != _targetPos;
	}

	void control(int cmd, int arg) {
		if (cmd == 0) {
			moveTo(0); // return back
		} else if (cmd == 1) {
			switch (arg) {
			case 0:
				moveTo(_pos); // pause
				break;
			case INT_MIN:
				moveTo(INT_MIN);
				break;
			case INT_MAX:
				moveTo(INT_MAX);
				break;
			default:
				moveTo(_targetPos + arg);
				break;
			}
		}
	}

};

} // namespace

MotorBrake *motorBrake(void) {
	static driver drv;
	return &drv; // singleton
}
