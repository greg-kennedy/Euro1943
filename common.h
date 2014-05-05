/* EURO1943 - GREG KENNEDY
	http://greg-kennedy.com */
	
/* common.h - Declarations common to both Client and Server go here.
	 This file should be consistent between both (per version), and if something
	 differs, it probably won't work right. */

#ifndef COMMON_H_
#define COMMON_H_

#define VERSION "1.2"

#define DEFAULT_OS_PORT 5009
#define DEFAULT_OS_HOST "greg-kennedy.com"
#define DEFAULT_PORT 5010

#define UPDATE_FREQ 200
#define CONTROL_FREQ 85

#define MAP_X 100
#define MAP_Y 100

#define OBJ_HUMAN 0
#define OBJ_VEHICLE 1
#define OBJ_PROJECTILE 2
#define OBJ_PARTICLE 3

#define STATUS_DEAD 0
#define STATUS_HUMAN 1
#define STATUS_VEHICLE 2
#define STATUS_HQ 3

#endif
