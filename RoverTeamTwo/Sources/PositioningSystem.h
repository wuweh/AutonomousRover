#ifndef _POSITIONING_SYSTEM_H_
#define _POSITIONING_SYSTEM_H_


#include "Rover.h"

void InitializePositioningSystem( void );

inches_t GetDistanceToBeacon( beaconId_t beaconId );

#endif
