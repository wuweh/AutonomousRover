#include <hidef.h>
#include "derivative.h"
#include <libdefs.h>

#include "Rover.h"
#include "MotorControlSystem.h"
#include "NavigationSystem.h"
#include "ObstacleAvoidanceSystem.h"
#include "PositioningSystem.h"


#include "Compass.h"
#include "I2C.h"

static degree_t bearing[ 5 ];

void main( void )
{  
   Byte i;
  /* InitializePositioningSystem();
   for ( ; ; )
   {        
      GetDistanceToBeacon( 2 );
      Delay( 1000 );
   }*/
   
   waitForAndDetectReceivedSonarPulse();
   TurnOnErrorLight();
   for(;;); 
      
   InitializeTimers();
   InitializeMotorControlSystem();
   //CalibrateCompass();

   MoveForward( 10000 );
   for (;;);
}
