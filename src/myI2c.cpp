/*
  si2c.c - Software I2C library for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the esp8266 core for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
  Modified January 2017 by Bjorn Hammarberg (bjoham@esp8266.com) - i2c slave support
*/
#include "arduino.h"
#include "myI2c.h"

extern "C" {

static unsigned int preferred_si2c_clock = 100000;


static unsigned char twi_dcount = 18;
static unsigned char twi_sda, twi_scl;
static uint32_t twi_clockStretchLimit;
static unsigned char twi_addr = 0;

#define SDA_LOW()   (GPES = (1 << twi_sda)) //Enable SDA (becomes output and since GPO is 0 for the pin, it will pull the line low)
#define SDA_HIGH()  (GPEC = (1 << twi_sda)) //Disable SDA (becomes input and since it has pullup it will go high)
#define SDA_READ()  ((GPI & (1 << twi_sda)) != 0)
#define SCL_LOW()   (GPES = (1 << twi_scl))
#define SCL_HIGH()  (GPEC = (1 << twi_scl))
#define SCL_READ()  ((GPI & (1 << twi_scl)) != 0)

#ifndef FCPU80
#define FCPU80 80000000L
#endif

#if F_CPU == FCPU80
#define TWI_CLOCK_STRETCH_MULTIPLIER 3
#else
#define TWI_CLOCK_STRETCH_MULTIPLIER 6
#endif

void my_twi_setClock(unsigned int freq){
  preferred_si2c_clock = freq;
#if F_CPU == FCPU80
  if(freq <= 50000) twi_dcount = 38;//about 50KHz
  else if(freq <= 100000) twi_dcount = 19;//about 100KHz
  else if(freq <= 200000) twi_dcount = 8;//about 200KHz
  else if(freq <= 300000) twi_dcount = 3;//about 300KHz
  else if(freq <= 400000) twi_dcount = 1;//about 400KHz
  else twi_dcount = 1;//about 400KHz
#else
  if(freq <= 50000) twi_dcount = 64;//about 50KHz
  else if(freq <= 100000) twi_dcount = 32;//about 100KHz
  else if(freq <= 200000) twi_dcount = 14;//about 200KHz
  else if(freq <= 300000) twi_dcount = 8;//about 300KHz
  else if(freq <= 400000) twi_dcount = 5;//about 400KHz
  else if(freq <= 500000) twi_dcount = 3;//about 500KHz
  else if(freq <= 600000) twi_dcount = 2;//about 600KHz
  else twi_dcount = 1;//about 700KHz
#endif
}

void my_twi_setClockStretchLimit(uint32_t limit){
  twi_clockStretchLimit = limit * TWI_CLOCK_STRETCH_MULTIPLIER;
}

void my_twi_init(unsigned char sda, unsigned char scl)
{
  twi_sda = sda;
  twi_scl = scl;
  pinMode(twi_sda, INPUT_PULLUP);
  pinMode(twi_scl, INPUT_PULLUP);
  my_twi_setClock(preferred_si2c_clock);
  my_twi_setClockStretchLimit(230/2); // default value is 230 uS
}

void my_twi_setAddress(uint8_t address)
{
  // set twi slave address (skip over R/W bit)
  twi_addr = address << 1;
}

static void ICACHE_RAM_ATTR twi_delay(unsigned char v){
  unsigned int i;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
  unsigned int reg;
  for (i = 0; i < v; i++) {
    reg = GPI;
  }
  (void)reg;
#pragma GCC diagnostic pop
}

static bool ICACHE_RAM_ATTR twi_write_start(void) {
  SCL_HIGH();
  SDA_HIGH();
  if (SDA_READ() == 0) {
    return false;
  }
  twi_delay(twi_dcount);
  SDA_LOW();
  twi_delay(twi_dcount);
  return true;
}

static bool ICACHE_RAM_ATTR twi_write_stop(void){
  uint32_t i = 0;
  SCL_LOW();
  SDA_LOW();
  twi_delay(twi_dcount);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit); // Clock stretching
  twi_delay(twi_dcount);
  SDA_HIGH();
  twi_delay(twi_dcount);
  return true;
}

static bool ICACHE_RAM_ATTR twi_write_bit(bool bit) {
  uint32_t i = 0;
  SCL_LOW();
  if (bit) SDA_HIGH();
  else SDA_LOW();
  twi_delay(twi_dcount+1);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);// Clock stretching
  twi_delay(twi_dcount);
  return true;
}

static bool ICACHE_RAM_ATTR twi_read_bit(void) {
  uint32_t i = 0;
  SCL_LOW();
  SDA_HIGH();
  twi_delay(twi_dcount+2);
  SCL_HIGH();
  while (SCL_READ() == 0 && (i++) < twi_clockStretchLimit);// Clock stretching
  bool bit = SDA_READ();
  twi_delay(twi_dcount);
  return bit;
}

static bool ICACHE_RAM_ATTR twi_write_byte(unsigned char byte) {
  unsigned char bit;
  for (bit = 0; bit < 8; bit++) {
    twi_write_bit(byte & 0x80);
    byte <<= 1;
  }
  return !twi_read_bit();//NACK/ACK
}

static unsigned char ICACHE_RAM_ATTR twi_read_byte(bool nack) {
  unsigned char byte = 0;
  unsigned char bit;
  for (bit = 0; bit < 8; bit++) byte = (byte << 1) | twi_read_bit();
  twi_write_bit(nack);
  return byte;
}

unsigned char ICACHE_RAM_ATTR my_twi_writeTo(unsigned char address, unsigned char * buf, unsigned int len, unsigned char sendStop){
  unsigned int i;
  if(!twi_write_start()) return 4;//line busy
  if(!twi_write_byte(((address << 1) | 0) & 0xFF)) {
    if (sendStop) twi_write_stop();
    return 2; //received NACK on transmit of address
  }
  for(i=0; i<len; i++) {
    if(!twi_write_byte(buf[i])) {
      if (sendStop) twi_write_stop();
      return 3;//received NACK on transmit of data
    }
  }
  if(sendStop) twi_write_stop();
  i = 0;
  while(SDA_READ() == 0 && (i++) < 10){
    SCL_LOW();
    twi_delay(twi_dcount);
    SCL_HIGH();
    unsigned int t=0; while(SCL_READ()==0 && (t++)<twi_clockStretchLimit); // twi_clockStretchLimit
    twi_delay(twi_dcount);
  }
  return 0;
}

unsigned char ICACHE_RAM_ATTR my_twi_readFrom(unsigned char address, unsigned char* buf, unsigned int len, unsigned char sendStop){
  unsigned int i;
  if(!twi_write_start()) return 4;//line busy
  if(!twi_write_byte(((address << 1) | 1) & 0xFF)) {
    if (sendStop) twi_write_stop();
    return 2;//received NACK on transmit of address
  }
  for(i=0; i<(len-1); i++) buf[i] = twi_read_byte(false);
  buf[len-1] = twi_read_byte(true);
  if(sendStop) twi_write_stop();
  i = 0;
  while(SDA_READ() == 0 && (i++) < 10){
    SCL_LOW();
    twi_delay(twi_dcount);
    SCL_HIGH();
    unsigned int t=0; while(SCL_READ()==0 && (t++)<twi_clockStretchLimit); // twi_clockStretchLimit
    twi_delay(twi_dcount);
  }
  return 0;
}

}
