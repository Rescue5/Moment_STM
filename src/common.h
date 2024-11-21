#pragma once

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "tusb.h" // tinyusb
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"

#ifndef NDEBUG
# define pr_dbg(t, ...) printf(t, ##__VA_ARGS__)
#else
# define pr_dbg(t, ...) do {} while (0)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof((x)[0]))
# endif

////////////////////////////////////////////////////////////////////////////////

enum {
	__tlmIdxTs = 0,
    __tlmIdxLoad1,
    __tlmIdxLoad2,
    __tlmIdxLoad3,
	__tlmIdxTemp1,
	__tlmIdxTemp2,
	__tlmIdxTemp3,
	__tlmIdxBrake,
	__tlmIdxMotorI,
	__tlmIdxMotorU,
	__tlmIdxMotorP,
	__tlmIdxMotorRPM,
	__tlmIdxMotorThrottle,
	__tlmIdxGyroX,
	__tlmIdxGyroY,
	__tlmIdxGyroZ,
	__tlmIdxMAX
};

#define TLM_ID_V		0x1000 // version
#define TLM_ID_N(n)		(TLM_ID_V + __tlmIdx##n)
#define telemetry_set(n, val)   telemetry()->set(__tlmIdx##n, val)

extern void board_init(void);
extern void rx_bytes(uint8_t *ptr, size_t len);
extern volatile uint32_t uptimeMillis;
extern uint32_t GetMicros(void);
