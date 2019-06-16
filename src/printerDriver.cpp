#include <arduino.h>
#include "printerDriver.h"
//#include <Wire.h>
#include "myI2c.h"

static ICACHE_RAM_ATTR byte MotorByte(bool enable, int dir, bool step)
{
  byte out = 0;
  out |= 8; // used to read input switches, must be set to high
  out |= (dir<0)?4:0;
  out |= (step && dir!=0)?2:0;
  out |= enable?0:1;
  return out;
}

ICACHE_RAM_ATTR void ReadSwitches( bool *pSwitchLimitX, bool *pSwitchLimitY, bool *pSwitchLimitZ)
{
  /*
  Wire.requestFrom(32, 1);
  uint8_t out = Wire.read();
  Wire.endTransmission();
  */
  uint8_t out;
  my_twi_readFrom(32, &out, 1, 1);


  *pSwitchLimitX = (out & 0x08)>0;  // short side
  *pSwitchLimitY = (out & 0x80)>0;  // long side
  *pSwitchLimitZ = true;  // up
}

ICACHE_RAM_ATTR void MoveRobot(int stepX, int stepY, int stepZ)
{
  //read limiting switches
  bool switchLimitX;
  bool switchLimitY;
  bool switchLimitZ;
  ReadSwitches( &switchLimitX, &switchLimitY, &switchLimitZ);

  // disable the motors that hit their corresponding limiting switch
  if (switchLimitY && stepY<0)
    stepY = 0;
  if (switchLimitX && stepX<0)
    stepX = 0;

  // step motors
  /*
  Wire.beginTransmission(32);
  Wire.write(MotorByte(true, stepX, false) | (MotorByte(true, stepY, false)<<4));
  Wire.write(MotorByte(true, stepX,  true) | (MotorByte(true, stepY,  true)<<4));
  Wire.endTransmission();
  */
  byte out[2];
  out[0] = MotorByte(true, stepX, false) | (MotorByte(true, stepY, false)<<4);
  out[1] = MotorByte(true, stepX,  true) | (MotorByte(true, stepY,  true)<<4);
  my_twi_writeTo(32, out, 2, 0);
}

ICACHE_RAM_ATTR bool HomeRobot()
{
  bool switchLimitX;
  bool switchLimitY;
  bool switchLimitZ;
  ReadSwitches( &switchLimitX, &switchLimitY, &switchLimitZ);
  MoveRobot(-1, -1, 0);

  return ((switchLimitX==true) && (switchLimitY==true));
}


void DisableMotors()
{
  /*
  Wire.beginTransmission(32);
  Wire.write(MotorByte(false, 0, false) | (MotorByte(false, 0, false)<<4));
  Wire.endTransmission();
  */
  byte out = MotorByte(false, 0, false) | (MotorByte(false, 0, false)<<4);
  my_twi_writeTo(32, &out, 1, 1);

}
