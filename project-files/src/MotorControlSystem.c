#include "MC9S12C128.h"     
#include <hidef.h>

#include "Rover.h"
#include "MotorControlSystem.h"
#include "NavigationSystem.h"
#include "Compass.h"

/*** STATIC CONSTANTS ***/

static const Byte MaxPower = 0xFF;

static const Byte InitialRightTreadPower = 0xFC;  // 0xE1 90% ( out of 255 )
static const Byte InitialLeftTreadPower = 0xFF;   // 0xE1 90% ( out of 255 )
                                                                 
static const milliseconds_t WaitForRoverToActuallyStopDelay = 200;

static const pulseCount_t PulsesPerInch = 11;
static const pulseCount_t PulsesPerFoot = 137;
static const pulseCount_t PulsesPerFiveFeet = 690;
static const pulseCount_t PulsesPerTwentyFiveFeet = 3400;

static const pulseCount_t PulsesPerDegreeNumerator = 100;
static const pulseCount_t PulsesPerDegreeDenominator = 72;

static const Byte LEFT_TREAD = 0;
static const Byte RIGHT_TREAD = 1;

/*** STATIC VARIABLES ***/

// SETS SPEED OF ROVER
#define DESIRED_ENCODER_PERIOD_IN_MICROSECONDS 10000  // 10000 is microseconds should be average speed
static timerCount_t DesiredEncoderPeriod = DESIRED_ENCODER_PERIOD_IN_MICROSECONDS * 2 / TIMER_COUNTER_PRESCALE; 

static pulseCount_t initialPACNTValue;

static Byte LeftTreadPower = 0;
static Byte RightTreadPower = 0;


/*** Flags ***/

static boolean_t RoverInMotionFlag = 0;

/*** Constant Definitions ***/

const direction_t FORWARD_MOTION = 0x0;
const direction_t REVERSE_MOTION = 0x1;
const direction_t STOP_MOTION = 0x2;
const direction_t ROTATE_MOTION = 0x3;


/*** Functions ***/

void InitializeMotorControlSystem()
{
   InitializeCompass();
   MOTOR_DRIVE_LEFT_IN_0_DDR = OUTPUT;
   MOTOR_DRIVE_LEFT_IN_1_DDR = OUTPUT;
   MOTOR_DRIVE_RIGHT_IN_0_DDR = OUTPUT;
   MOTOR_DRIVE_RIGHT_IN_1_DDR = OUTPUT;
	
   PWMPOL_PPOL2 = 1;
	PWMPOL_PPOL3 = 1;
	
	PWMCLK_PCLK2 = 0;
	PWMCLK_PCLK3 = 0;
	
	PWMPRCLK_PCKB = 0x0;
	
	PWMCAE_CAE2 = 0;
	PWMCAE_CAE3 = 0;
	
	PWMCTL_CON23 = 0;

   SetLeftTreadDrivePower( InitialLeftTreadPower );
	SetRightTreadDrivePower( InitialRightTreadPower );
	
	StopMotion();
}

void StabilizeTreads()
{
   boolean_t isFirstMeasurement[ 2 ];
   Byte index;
   sByte adjustment8Bit;
   
   sWord error, errorAttenuation[ 2 ], adjustment, integralError[ 2 ], integralErrorAttenuation[ 2 ];
   timerCount_t currentPulsePeriod, currentRisingEdge, lastRisingEdge[ 2 ];
  
   timerCount_t standardPulsePeriod = 1000;
  
   errorAttenuation[ LEFT_TREAD ] = 0x007F;
   errorAttenuation[ RIGHT_TREAD ] = 0x7ffF;
   
   integralErrorAttenuation[ LEFT_TREAD ] = 0x04FF;
   integralErrorAttenuation[ RIGHT_TREAD ] = 0x03FF;

   integralError[ LEFT_TREAD ] = 0;
   integralError[ RIGHT_TREAD ] = 0;

   isFirstMeasurement[ 0 ] = TRUE;
   isFirstMeasurement[ 1 ] = TRUE;
   
   isFirstMeasurement[ LEFT_TREAD ] = TRUE;
   isFirstMeasurement[ RIGHT_TREAD ] = TRUE;

   // initialize timers
   
   TIOS_IOS2 = 0; // input capture
   TIOS_IOS3 = 0; // input capture
           
   TIE_C2I = 0; // disable interrupts
   TIE_C3I = 0; // disable interrupts
      
   TFLG1_C2F = 1; // clear the flags
   TFLG1_C3F = 1; // clear the flags
      
   TCTL4_EDG2x = 0x01; // capture rising edge
   TCTL4_EDG3x = 0x01; // capture rising edge 
  
   while( RoverInMotionFlag )
   {
         if ( TFLG1_C2F )
         {
            currentRisingEdge = TC2;
            index = LEFT_TREAD;
            TFLG1_C2F = 1;       // clear the flag
            TCTL4_EDG2x = 0x01;  // capture rising edge
         } 
         else if ( TFLG1_C3F )   // then TFGL1_C3F is set
         {
            currentRisingEdge = TC3;
            index = RIGHT_TREAD;
            TFLG1_C3F = 1;       // clear the flag
            TCTL4_EDG3x = 0x01;  // capture rising edge
         }
         else
         {
            continue;
         }
         
         if ( isFirstMeasurement[ index ] )
         {
            lastRisingEdge[ index ] = currentRisingEdge;
            isFirstMeasurement[ index ] = FALSE;
            continue;  
         }
         
         if ( index == RIGHT_TREAD )
         {
            standardPulsePeriod = currentRisingEdge - lastRisingEdge[ index ];
            lastRisingEdge[ index ] = currentRisingEdge;
            continue;
         }
         
         currentPulsePeriod = currentRisingEdge - lastRisingEdge[ index ];
         error = ( sWord ) currentPulsePeriod - ( sWord ) standardPulsePeriod;
         integralError[ index ] += error;
         
         lastRisingEdge[ index ] = currentRisingEdge;

         adjustment = error / errorAttenuation[ index ] + integralError[ index ] / integralErrorAttenuation[ index ];
         adjustment8Bit = ConvertSignedWordToSignedByte( adjustment );

         AdjustTreadDrivePower( index, adjustment8Bit );
      }
}

void DisableTreadStabilization()
{
}

static sByte ConvertSignedWordToSignedByte( sWord adjustment16 )
{
   sByte adjustment8 = adjustment16;
   adjustment8 = adjustment16 > 127 ? 127 : adjustment8;
   adjustment8 = adjustment16 < -128 ? -128 : adjustment8;
   return adjustment8;
}

static void AdjustTreadDrivePower( Byte treadId, sByte adjustment )
{
   Byte newPWMDTY, oldPWMDTY;
   oldPWMDTY = treadId == LEFT_TREAD ? LeftTreadPower : RightTreadPower;
   newPWMDTY = oldPWMDTY + adjustment;
   if ( adjustment > 0 && newPWMDTY < oldPWMDTY ) newPWMDTY = MaxPower;
   if ( treadId == LEFT_TREAD ) SetLeftTreadDrivePower( newPWMDTY );
   else SetRightTreadDrivePower( newPWMDTY );
}

static void SetLeftTreadDrivePower( Byte power )
{
	MOTOR_DRIVE_LEFT_DUTY = power;
	MOTOR_DRIVE_LEFT_PERIOD = 0xFF;
   LeftTreadPower = MOTOR_DRIVE_LEFT_DUTY;
}

static void SetRightTreadDrivePower( Byte power )
{               
	MOTOR_DRIVE_RIGHT_DUTY = power;
	MOTOR_DRIVE_RIGHT_PERIOD = 0xFF;           
   RightTreadPower = MOTOR_DRIVE_RIGHT_DUTY;
}


void MoveForward( inches_t distance )
{ 
 	DisableInterrupts;
   InitializePulseAccumulator( DistanceToPulses( distance ) ); 
   DisableTreads();
	
	LeftTreadForward();
	RightTreadForward();
	
 	SetRoverInMotionFlag();	
 	                    
 	EnableTreads();
 	EnableInterrupts;
 	//StabilizeTreads();
}

void MoveReverse( inches_t distance )
{ 
	DisableInterrupts;
   InitializePulseAccumulator( DistanceToPulses( distance ) ); 
   DisableTreads();
	
	LeftTreadReverse();
	RightTreadReverse();
	
 	SetRoverInMotionFlag();
 	
 	EnableTreads();
 	EnableInterrupts;	
}

void CelebrateRotate( degree_t degrees )
{ 
   InitializePulseAccumulator( DegreesToPulses( degrees ) );
   
   DisableTreads();

   if ( degrees == 0 )
   {
      StopMotion();
      return;
   }
   
   DisableInterrupts;
   
   if ( degrees > 0 )
   {
      LeftTreadForward();
      RightTreadReverse();
   }
   else if ( degrees < 0 )
   {    
      LeftTreadReverse();
      RightTreadForward();
   }       
   SetRoverInMotionFlag(); 
   
   EnableTreads();
   EnableInterrupts;  
}


void Rotate( degree_t degrees )
{
   degree_t currentBearing, desiredBearing;
   currentBearing = GetAnAccurateCompassReading();
   desiredBearing = currentBearing + degrees;
   if ( desiredBearing > 359 ) desiredBearing -= 360;
   if ( desiredBearing < 0 ) desiredBearing += 360;

   while ( degrees > 180 ) degrees -= 360;
   while ( degrees < -180 ) degrees += 360;
   
   InitializePulseAccumulator( DegreesToPulses( degrees ) );
   
   DisableTreads();

   if ( degrees == 0 )
   {
      StopMotion();
      return;
   }
   
   DisableInterrupts;
   
   if ( degrees > 0 )
   {
      LeftTreadForward();
      RightTreadReverse();
   }
   else if ( degrees < 0 )
   {    
      LeftTreadReverse();
      RightTreadForward();
   }       
   SetRoverInMotionFlag(); 
   
   EnableTreads();
   EnableInterrupts;  
   
   while ( GetRoverInMotionFlag() == TRUE );
   Delay( 200 );
   currentBearing = GetAnAccurateCompassReading();
   if ( currentBearing - desiredBearing <= -2 && currentBearing - desiredBearing >= -358 ||
        currentBearing - desiredBearing >= 2 && currentBearing - desiredBearing <= 358 ) 
   {
      Rotate( desiredBearing - currentBearing );
      return;
   }       
}

void StopMotion( void )
{
	DisableInterrupts;
	
   DisableTreads();
   BrakeTreads();
   EnableTreads();
   Delay( WaitForRoverToActuallyStopDelay );
	
	// Clear the PA overflow flag and stop the PA
	// Writing a 1 to the PAOVF flag clears it but TEN in TSCR1 must be enabled.
	// Page 320 68HC12 book and 456 of the HCS12 manual.    
	// Disable pulse accumulator and PA interrupts
	PACTL_PAEN = 0;
	PAFLG_PAOVF = 1;
	
	ClearRoverInMotionFlag();
	EnableInterrupts;
}

boolean_t GetRoverInMotionFlag()
{
   return RoverInMotionFlag;
}

/* STATIC FUNCTIONS */

static void SetRoverInMotionFlag()
{
   RoverInMotionFlag = TRUE;
}

static void ClearRoverInMotionFlag()
{
   RoverInMotionFlag = FALSE;
}

static void InitializePulseAccumulator( pulseCount_t numberOfPulsesTillInterrupt )
{
   // Write the negative number of encoder pulses to PACNT and enable PAOVI to interrupt when
	initialPACNTValue = ~numberOfPulsesTillInterrupt + 1;
	PACNT = initialPACNTValue;
	
	// Initialize pulse accumulator for encoders.
	PACTL = 0x52;
}

static pulseCount_t DistanceToPulses( inches_t distance ) 
{
   pulseCount_t inches, feet, fiveFeet, twentyFiveFeet;
   static const pulseCount_t PulsesItTakesAfterRoverStopped = 12;
   
   twentyFiveFeet = distance / ( 25 * 12 );
   distance -= twentyFiveFeet * 25 * 12;
   fiveFeet = distance / ( 5 * 12 );
   distance -= fiveFeet * 5 * 12;
   feet = distance / 12;
   distance -= feet * 12;
   inches = distance;
   twentyFiveFeet *= PulsesPerTwentyFiveFeet;
   fiveFeet *= PulsesPerFiveFeet;
   feet *= PulsesPerFoot;
   inches *= PulsesPerInch;
	return twentyFiveFeet + fiveFeet + feet + inches - PulsesItTakesAfterRoverStopped;
}

static inches_t PulsesToDistance( pulseCount_t pulses )
{
   return pulses / PulsesPerInch;   
}

static pulseCount_t DegreesToPulses( degree_t degrees) 
{  
	return ( ( pulseCount_t ) ( ( 2 * ( degrees >= 0 ) - 1 ) * degrees ) ) * PulsesPerDegreeNumerator / PulsesPerDegreeDenominator;
}

void CommenceTurnByTurnExecution()
{
   ExecuteNextTurnByTurnInstruction();
}

static boolean_t ExecuteNextTurnByTurnInstruction()
{
   turnByTurnElement_t* TurnByTurnElement;
   if ( HasNextTurnByTurnElement() )
   {
      TurnByTurnElement = GetNextTurnByTurnElement();
      switch ( TurnByTurnElement->typeOfMotion )
      {
         case FORWARD_MOTION: MoveForward( TurnByTurnElement->value ); break;
         case REVERSE_MOTION: MoveReverse( TurnByTurnElement->value ); break;
         case ROTATE_MOTION: Rotate( TurnByTurnElement->value ); break;
         default: break;
      }
      return TRUE;
   } 
   return FALSE;
}

inches_t GetDistanceIntoCurrentRoute()
{
   pulseCount_t totalPulses = PACNT - initialPACNTValue;
   return PulsesToDistance( totalPulses );     
}

interrupt VectorNumber_Vtimpaovf void MotionCompleted()
{
	StopMotion();
}


static void LeftTreadForward()
{
   MOTOR_DRIVE_LEFT_IN_0 = 1;
	MOTOR_DRIVE_LEFT_IN_1 = 0;
}

static void RightTreadForward()
{
   MOTOR_DRIVE_RIGHT_IN_0 = 0;
	MOTOR_DRIVE_RIGHT_IN_1 = 1;
}

static void LeftTreadReverse()
{
   MOTOR_DRIVE_LEFT_IN_0 = 0;
	MOTOR_DRIVE_LEFT_IN_1 = 1;
}

static void RightTreadReverse()
{
   MOTOR_DRIVE_RIGHT_IN_0 = 1;
	MOTOR_DRIVE_RIGHT_IN_1 = 0;
}

static void BrakeTreads()
{
   MOTOR_DRIVE_LEFT_IN_0 = 1;
   MOTOR_DRIVE_LEFT_IN_1 = 1;
   MOTOR_DRIVE_RIGHT_IN_0 = 1;
   MOTOR_DRIVE_RIGHT_IN_1 = 1;
}

static void DisableTreads()
{
   MOTOR_DRIVE_LEFT_ENABLE = 0;
   MOTOR_DRIVE_RIGHT_ENABLE = 0;
}

static void EnableTreads()
{
   MOTOR_DRIVE_LEFT_ENABLE = 1;
   MOTOR_DRIVE_RIGHT_ENABLE = 1;
} 

