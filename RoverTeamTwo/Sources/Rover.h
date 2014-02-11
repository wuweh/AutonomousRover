#ifndef _ROVER_H_
#define _ROVER_H_

#include "MC9S12C128.h"
#include <stdtypes.h>


/*** TYPE DECLARATIONS ***/

typedef Byte boolean_t;
typedef sWord degree_t;
typedef Word pulseCount_t;
typedef Byte direction_t;
typedef Byte registerValue_t;

// Time  
typedef Word milliseconds_t;
typedef LWord microseconds_t;
typedef Word timerCount_t;

// Distance
typedef Word inches_t; 
typedef Byte feet_t; 


/*** ROVER IO CONTROL ***/

extern const direction_t FORWARD_MOTION;
extern const direction_t REVERSE_MOTION;
extern const direction_t STOP_MOTION;

extern const microseconds_t WAIT_FOR_ROVER_TO_ACTUALLY_STOP_DELAY;

#define MOTOR_DRIVE_IO PORTA
#define MOTOR_DRIVE_LEFT_IN_0 PORTA_BIT0
#define MOTOR_DRIVE_LEFT_IN_1 PORTA_BIT1
#define MOTOR_DRIVE_RIGHT_IN_0 PORTA_BIT2
#define MOTOR_DRIVE_RIGHT_IN_1 PORTA_BIT3
#define MOTOR_DRIVE_LEFT_ENABLE PORTA_BIT4
#define MOTOR_DRIVE_RIGHT_ENABLE PORTA_BIT5
 
#define OBJECT_DETECTION_PIN PTT_PTT0
#define OBJECT_DETECTION_DDR DDRT_DDRT0


/*** CONSTANTS ***/

extern const boolean_t False;
extern const boolean_t True;

extern const LWord CLOCK_SPEED_HZ;
extern const Word SPEED_OF_SOUND_INCH_PER_SEC;
extern const timerCount_t CLOCK_CYCLES_PER_INCH;
extern const Byte INCHES_PER_FOOT;

extern const Byte TIMER_COUNTER_PRESCALE;

extern const Byte PING_FREQUENCY;


/*** GLOBAL VARIABLE ***/

extern degree_t pingAngle;


/*** FLAGS ***/

extern boolean_t RoverInMotionFlag;


/*** USEFUL FUNCTIONS ***/

void delay( microseconds_t time );


#endif

// I don't like the CPU hogging delay function in delay() FIX IT

/*** INITIALIZATION FUNCTIONS ***/

void initializeTimers();


