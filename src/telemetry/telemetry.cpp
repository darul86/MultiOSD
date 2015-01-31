/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "telemetry.h"
#include "../config.h"
#include <avr/pgmspace.h>

#ifdef TELEMETRY_MODULES_UAVTALK
#	include "uavtalk/uavtalk.h"
#endif
#ifdef TELEMETRY_MODULES_ADC_BATTERY
#	include "adc_battery/adc_battery.h"
#endif
#ifdef TELEMETRY_MODULES_I2C_BARO
#	include "i2c_baro/i2c_baro.h"
#endif
#include "../lib/timer/timer.h"
#include <math.h>

namespace telemetry
{

namespace status
{

	uint8_t connection = CONNECTION_STATE_DISCONNECTED;
	uint16_t flight_time = 0;
	uint8_t flight_mode = FLIGHT_MODE_MANUAL;
	bool armed = false;
	uint8_t rssi = 0;

}  // namespace status

namespace attitude
{

	float roll = 0;
	float pitch = 0;
	float yaw = 0;

}  // namespace attitude

namespace input
{

	bool connected = false;
	int16_t throttle = 0;
	int16_t roll = 0;
	int16_t pitch = 0;
	int16_t yaw = 0;
	int16_t collective = 0;
	int16_t thrust = 0;
	uint8_t flight_mode_switch = 0;
	uint16_t channels [INPUT_CHANNELS];

}  // namespace input

namespace gps
{

	float latitude = 0.0;
	float longitude = 0.0;
	float altitude = 0.0;
	float speed = 0.0;
	float heading = 0.0;
	int8_t sattelites = 0;
	uint8_t state = GPS_STATE_NO_FIX;
	float climb = 0.0;

}  // namespace gps

namespace barometer
{

	float altitude = 0.0;

}  // namespace barometer

namespace stable
{

	float climb = 0.0;
	float altitude = 0.0;
	float ground_speed = 0.0;
	float air_speed = 0.0;

	static uint32_t _alt_update_time = 0;

	void update_alt_climb (float _alt)
	{
		uint32_t ticks = timer::ticks ();
		climb = (_alt - altitude) / (ticks - _alt_update_time) * 1000;
		altitude = _alt;
		_alt_update_time = ticks;
	}

}  // namespace stable

namespace battery
{

	float voltage = 0.0;
	float current = 0.0;
	float consumed = 0;

}  // namespace battery

namespace messages
{

	bool battery_low = false;
	bool rssi_low = false;

}  // namespace messages

namespace home
{

	uint8_t state = HOME_STATE_NO_FIX;

	float distance = 0;
	uint8_t direction = HOME_DIR_00R;

	float longitude;
	float latitude;
	float altitude = 0;

//	static uint16_t _fix_timeout = 3000;
//	static uint32_t _fix_timestop = 0;

	void update ()
	{
		//uint32_t ticks = timer::ticks ();
		if (state != HOME_STATE_FIXED)
			switch (gps::state)
			{
				case GPS_STATE_NO_FIX:
					state = HOME_STATE_NO_FIX;
					break;
				case GPS_STATE_FIXING:
				case GPS_STATE_2D:
					if (state == HOME_STATE_NO_FIX) state = HOME_STATE_FIXING;
					break;
				case GPS_STATE_3D:
					state = HOME_STATE_FIXED;
					longitude = gps::longitude;
					latitude = gps::latitude;
					altitude = gps::altitude;
					break;
			}
		if (state != HOME_STATE_FIXED) return;

	    float rads = fabs (latitude) * 0.0174532925;
	    double scale_down = cos (rads);
	    double scale_up = 1.0 / cos (rads);

	    // distance
	    float dstlat = fabs (latitude - gps::latitude) * 111319.5;
	    float dstlon = fabs (longitude - gps::longitude) * 111319.5 * scale_down;
	    distance = sqrt (dstlat * dstlat + dstlon * dstlon);

	    // DIR to Home
	    dstlon = (longitude - gps::longitude); 				// x offset
	    dstlat = (latitude - gps::latitude) * scale_up; 	// y offset
	    int16_t bearing = 90 + atan2 (dstlat, -dstlon) * 57.295775; // absolute home direction
	    if (bearing < 0) bearing += 360;	// normalization
	    bearing -= 180;						// absolute return direction
	    if (bearing < 0) bearing += 360;	// normalization
	    bearing -= gps::heading;			// relative home direction
	    if (bearing < 0) bearing += 360;	// normalization
	    direction = round ((float) (bearing / 360.0f) * 16.0f) + 1;
	    if (direction > 15) direction = 0;
	}

}  // namespace home

void init ()
{
#ifdef TELEMETRY_MODULES_UAVTALK
	uavtalk::init ();
#endif
#ifdef TELEMETRY_MODULES_ADC_BATTERY
	adc_battery::init ();
#endif
#ifdef TELEMETRY_MODULES_I2C_BARO
	i2c_baro::reset ();
#endif
}

bool update ()
{
	bool res = false;
#ifdef TELEMETRY_MODULES_UAVTALK
	res |= uavtalk::update ();
#endif
#ifdef TELEMETRY_MODULES_ADC_BATTERY
	res |= adc_battery::update ();
#endif
#ifdef TELEMETRY_MODULES_I2C_BARO
	res |= i2c_baro::update ();
#endif
	return res;
}

void fprintf_build (FILE *stream, char delimeter)
{
#ifdef TELEMETRY_MODULES_UAVTALK
	fprintf_P (stream, PSTR ("UAVTalk%c"), delimeter);
#endif
#ifdef TELEMETRY_MODULES_ADC_BATTERY
	fprintf_P (stream, PSTR ("ADCBatt%c"), delimeter);
#endif
#ifdef TELEMETRY_MODULES_I2C_BARO
	fprintf_P (stream, PSTR ("I2CBaro%c"), delimeter);
#endif
}

namespace settings
{

void reset ()
{
#ifdef TELEMETRY_MODULES_UAVTALK
	uavtalk::settings::reset ();
#endif
#ifdef TELEMETRY_MODULES_ADC_BATTERY
	adc_battery::settings::reset ();
#endif
#ifdef TELEMETRY_MODULES_I2C_BARO
	i2c_baro::settings::reset ();
#endif
}

}  // namespace settings

}  // namespace telemetry