#pragma once

#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

struct Handle {
	void *Instance;
	union {
		uint16_t Pin;
	};
};

class Runnable {
	uint32_t _deadline = 1;
public:
	void arm(uint32_t deadline = 1) {
		_deadline = deadline;
	}

	void disarm(void) {
		_deadline = 0;
	}

	void tick(uint32_t millis) {
		if (_deadline && millis > _deadline) {
			disarm();
			run(millis);
		}
	}

protected:
	virtual void run(uint32_t millis) = 0;
};

class Driver {
public:
	virtual void tare(void) {
		// default = nothing
	}

	virtual long init(void) {
		return HAL_OK;
	}
};

class MspDriver : public Driver {
public:
	virtual long mspInit(Handle *h) = 0;
};

class MspCallback {
public:
	virtual void mspCallback(void *p, Handle *) = 0;
};

class MspIrqHandler {
public:
	virtual void mspIrqHandler(IRQn_Type) = 0;
};

class DriverManager : public Driver {
public:
	virtual void tick(uint32_t) = 0;
	virtual long msp_init(Handle *) = 0;
	virtual void msp_callback(void *, Handle *) = 0;
	virtual void msp_irq_handler(IRQn_Type) = 0;
};

extern DriverManager *driverManager(void);

extern Driver *hx711();
extern Driver *ina236();
extern Driver *mpu6050();
extern Driver *max6675();
extern Driver *speed();

////////////////////////////////////////////////////////////////////////////////

class Serial : public MspDriver {
public:
	virtual void putchar(int) = 0;
};

extern Serial *serial(void);

////////////////////////////////////////////////////////////////////////////////

class Led : public Runnable, public MspDriver {
public:
	virtual void setMode(unsigned int) = 0;
};

extern Led *led(void);

////////////////////////////////////////////////////////////////////////////////

class Motor : public Runnable, public MspDriver {
public:
	void stop(void) {
		throttle(0);
	}

	virtual void throttle(int) = 0;
};

extern Motor *motor(void);

////////////////////////////////////////////////////////////////////////////////

class MotorBrake : public Runnable, public MspDriver {
public:
	void stop(void) {
		control(1, 0);
	}

	void move(int arg) {
		control(1, arg);
	}

	void moveBack(void) {
		control(0, 0);
	}

	virtual void tare(void) = 0;
	virtual bool moving(void) = 0;
	virtual void control(int, int = 0) = 0;
};

extern MotorBrake *motorBrake(void);

////////////////////////////////////////////////////////////////////////////////

class Beeper : public Runnable, public MspDriver {
public:
	virtual void disco(void) = 0;
};

extern Beeper *beeper(void);

////////////////////////////////////////////////////////////////////////////////

class Chiller : public Runnable, public MspDriver {
public:
	void stop(int deadline) {
		control(0, deadline);
	}

	void start(int throttle) {
		control(1, throttle);
	}

	virtual void control(int cmd, int arg) = 0;
};

extern Chiller *chiller(void);

////////////////////////////////////////////////////////////////////////////////

class TinyUSB : public Runnable, public MspDriver {
public:
	virtual void write(const void *, size_t) = 0;
	virtual void write(const char *) = 0;
};

extern TinyUSB *tinyUSB(void);

////////////////////////////////////////////////////////////////////////////////

class Telemetry : public Runnable, public Driver {
public:
	template <typename T>
	void set(int id, T val) {
		update(id, (uint32_t)val);
	}
	virtual void updateInterval(int v) = 0;

protected:
	virtual void update(int id, uint32_t val) = 0;
};

extern Telemetry *telemetry();
