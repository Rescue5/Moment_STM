#pragma once

#include "drivers.hpp"

class Device : public Driver {
public:
	virtual void tick(uint32_t) = 0;

	// bool plugged(void) { return _connection == Connection::Plugged; }
	// bool unplugged(void) { return _connection == Connection::Unplugged; }

	virtual void dispatch(const char *) = 0;
};

extern Device *theDevice(void);
