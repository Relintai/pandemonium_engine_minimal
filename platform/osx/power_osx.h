#ifndef POWER_OSX_H
#define POWER_OSX_H

/*  power_osx.h                                                          */


#include "core/os/file_access.h"
#include "core/os/os.h"
#include "dir_access_osx.h"

#include <CoreFoundation/CoreFoundation.h>

class PowerOSX {
private:
	int nsecs_left;
	int percent_left;
	OS::PowerState power_state;
	void checkps(CFDictionaryRef dict, bool *have_ac, bool *have_battery, bool *charging);
	bool GetPowerInfo_MacOSX(/*PowerState * state, int *seconds, int *percent*/);
	bool UpdatePowerInfo();

public:
	PowerOSX();
	virtual ~PowerOSX();

	OS::PowerState get_power_state();
	int get_power_seconds_left();
	int get_power_percent_left();
};

#endif // POWER_OSX_H
