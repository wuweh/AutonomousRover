#include "I2C.h"
#include "Rover.h"
#include "Compass.h"

static const Byte SettingsCRA = 0x7C; // 0x10 default
static const Byte SettingsCRB = 0x00; // 0x20 default
static const Byte SettingsMode = 0x00; // 0x00 default

static const Byte AddressCRA = 0x00;
static const Byte AddressCRB = 0x01;
static const Byte AddressMode = 0x02;

static const Byte SlaveWrite = 0x3C;
static const Byte SlaveRead = 0x3D;

static const sWord xShiftValue = 189;
static const sWord yShiftValue = 175;
static const sWord yScaleValueNumerator = 18;
static const sWord yScaleValueDenominator = 100;

static const boolean_t NACK = 1;

static const Byte MaxNumberOfFailedAttempts = 10;
static const degree_t MaxTolerableDifferenceBetweenTwoConsecutiveCompassReadings = 2;

static fullCompassRegister_t rawXData;
static fullCompassRegister_t rawYData;
static fullCompassRegister_t rawZData;


boolean_t writeByteToCompass( boolean_t sendStart, boolean_t sendStop, Byte data )
{
   Byte numberOfFailedAttempts = 0;
   while ( I2CWriteByte( sendStart, sendStop, data ) == 1 ) // I2CWriteByte returned nack = 1
   {  
      numberOfFailedAttempts++;
      if ( numberOfFailedAttempts == MaxNumberOfFailedAttempts )
      {
         TurnOnErrorLight();
         return 1; // nack = 1
      }
   } 
   return 0; // nack = 0
}

boolean_t InitializeCompass()
{
   if ( writeByteToCompass( 1, 0, SlaveWrite ) == NACK ) return False;
   if ( writeByteToCompass( 0, 0, AddressCRA ) == NACK ) return False;
   if ( writeByteToCompass( 0, 1, SettingsCRA ) == NACK ) return False;
   
   if ( writeByteToCompass( 1, 0, SlaveWrite ) == NACK ) return False;
   if ( writeByteToCompass( 0, 0, AddressCRB ) == NACK ) return False;
   if ( writeByteToCompass( 0, 1, SettingsCRB ) == NACK ) return False;
   
   if ( writeByteToCompass( 1, 0, SlaveWrite ) == NACK ) return False;
   if ( writeByteToCompass( 0, 0, AddressMode ) == NACK ) return False;
   if ( writeByteToCompass( 0, 1, SettingsMode ) == NACK ) return False;
   
   return True;
}

Byte readByteFromCompass( boolean_t sendStop )
{
   return I2CReadByte( 0, sendStop ); // send nack = 0 as the response byte
}

// returns 1 on success and 0 on failure
boolean_t GetDataFromCompass()
{
   if ( writeByteToCompass( 1, 0, SlaveRead ) == NACK ) return False;
   
   rawXData.bytes.upper = readByteFromCompass( 0 );
   rawXData.bytes.lower = readByteFromCompass( 0 );
   rawZData.bytes.upper = readByteFromCompass( 0 );
   rawZData.bytes.lower = readByteFromCompass( 0 );
   rawYData.bytes.upper = readByteFromCompass( 0 );
   rawYData.bytes.lower = readByteFromCompass( 1 );
   
   // Make sure register pointer is at first register by writing nothing to it
   if ( writeByteToCompass( 1, 0, SlaveWrite ) == NACK ) return False;
   if ( writeByteToCompass( 0, 1, 0x03 ) == NACK ) return False;
   
   return True;
}

// returns calculated bearing compared to magnetic north
degree_t GetASingleCompassReading()
{
   degree_t angle;
   sWord balancedX, balancedY;
   
   GetDataFromCompass();
   balancedX = rawXData.value + xShiftValue;
   balancedY = rawYData.value + yShiftValue;
   balancedY = balancedY + ( balancedY * yScaleValueNumerator ) / yScaleValueDenominator;
   
   angle = arcTangent( balancedY, balancedX );
   
   if ( angle < 0 )
   {
      angle += 360;  
   }
   return angle;
}

// takes two back to back measurements and returns angle on success or -1 on failure.
degree_t GetAnAccurateCompassReading()
{
   degree_t reading1, reading2, difference1And2;
   reading1 = GetASingleCompassReading();
   reading2 = GetASingleCompassReading();
   
   difference1And2 = reading1 - reading2;
   
   //absolute value
   difference1And2 = difference1And2 > 0 ? difference1And2 : -difference1And2;
   
   if ( difference1And2 <= MaxTolerableDifferenceBetweenTwoConsecutiveCompassReadings ||
        difference1And2 >= 360 - MaxTolerableDifferenceBetweenTwoConsecutiveCompassReadings ) // for reading1 = 358 and reading2 = 0
   {
      //return the more current measurement
      return reading2;
   }
   return -1;
}