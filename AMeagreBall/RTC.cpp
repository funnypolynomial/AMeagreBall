#include <Arduino.h>
#define USE_SOFTWIRE
#ifdef USE_SOFTWIRE
#include <SoftWire.h>
#else
#include <SoftwareI2C.h>
#endif


#include "Boing.h"
#include "Pins.h"
#include "RTC.h"

// real time clock
RTC rtc;

#ifdef RTC_I2C_ADDRESS
#ifdef USE_SOFTWIRE
SoftWire softWire(PIN_RTC_SDA, PIN_RTC_SCL);
char swTxBuffer[4];
char swRxBuffer[4];
#else
SoftwareI2C softWire;
#endif
#endif

RTC::RTC():
  m_Hour24(12),
  m_Minute(0),
  m_Second(0),
  m_DayOfWeek(1),
  m_DayOfMonth(1),
  m_Month(1),
  m_Year(12)
{
}

#ifdef RTC_I2C_ADDRESS
void RTC::Setup()
{
#ifdef USE_SOFTWIRE
  softWire.setTxBuffer(swTxBuffer, sizeof(swTxBuffer));
  softWire.setRxBuffer(swRxBuffer, sizeof(swRxBuffer));
  softWire.begin();
#else  
  softWire.begin(PIN_RTC_SDA, PIN_RTC_SCL);
#endif  
  
  ReadTime(true);
}

void RTC::ReadTime(bool Full)
{
  if (Full)
  {
    // from register 0
    softWire.beginTransmission(RTC_I2C_ADDRESS);
    softWire.write((byte)0x00);
    softWire.endTransmission();

    softWire.requestFrom(RTC_I2C_ADDRESS, 7);

    m_Second = BCD2Dec(softWire.read());
  }
  else
  {
    // just the minutes and hours
    // from register 1
    softWire.beginTransmission(RTC_I2C_ADDRESS);
    softWire.write((byte)0x01);
    softWire.endTransmission();

    softWire.requestFrom(RTC_I2C_ADDRESS, 2);
  }
  m_Minute = BCD2Dec(softWire.read());
  byte Register2 = softWire.read();
  if (Register2 & 0x40)  // 12/24 hr
  {
    // 12 hr mode, bit 6=PM
    m_Hour24 = BCD2Dec(Register2 & 0x3F);
    if (Register2 & 0x20)
    {
      m_Hour24 += 12;
      if (m_Hour24 > 23) m_Hour24 = 0;
    }
  }
  else
  {
    // 24 hour mode
    m_Hour24 = BCD2Dec(Register2 & 0x3F);
  }

  if (Full)
  {
    m_DayOfWeek  = BCD2Dec(softWire.read());
    m_DayOfMonth = BCD2Dec(softWire.read());
    m_Month      = BCD2Dec(softWire.read());
    m_Year       = BCD2Dec(softWire.read());
  }
  if (m_Minute >= 60)
  {
     m_Hour24 = 12;
     m_Minute = m_Second = 0;
     m_DayOfWeek = m_DayOfMonth = m_Month = m_Year = 1;     
  }
}

byte RTC::ReadSecond()
{
  // from register 01
  softWire.beginTransmission(RTC_I2C_ADDRESS);
  softWire.write(0x00);
  softWire.endTransmission();

  softWire.requestFrom(RTC_I2C_ADDRESS, 1);

  m_Second = BCD2Dec(softWire.read());
  return m_Second;
}

byte RTC::ReadMinute()
{
  // from register 01
  softWire.beginTransmission(RTC_I2C_ADDRESS);
  softWire.write(0x01);
  softWire.endTransmission();

  softWire.requestFrom(RTC_I2C_ADDRESS, 1);

  return BCD2Dec(softWire.read());
}

void RTC::WriteTime()
{
  softWire.beginTransmission(RTC_I2C_ADDRESS);
  softWire.write((byte)0x00);
  softWire.write(Dec2BCD(m_Second));
  softWire.write(Dec2BCD(m_Minute));
  softWire.write(Dec2BCD(m_Hour24));  // 24 hr mode
  softWire.write(Dec2BCD(m_DayOfWeek));
  softWire.write(Dec2BCD(m_DayOfMonth));
  softWire.write(Dec2BCD(m_Month));
  softWire.write(Dec2BCD(m_Year));
  softWire.endTransmission();
}

unsigned long RTC::Entropy()
{
  unsigned long seed = millis() + micros();
  seed += m_Second;
  seed += m_Minute;
  seed += m_Hour24;
  seed += m_DayOfWeek;
  seed += m_DayOfMonth;
  seed += m_Month;
  seed += m_Year;
  return seed;
}

#else
void RTC::Setup()
{
}

void RTC::ReadTime(bool )
{
}

byte RTC::ReadSecond()
{
  return m_Second;
}

byte RTC::ReadMinute()
{
  return m_Minute;
}

void RTC::WriteTime()
{
}

unsigned long RTC::Entropy()
{
  unsigned long seed = millis() + micros();
  return seed;
}

#endif

bool RTC::CheckPeriod(unsigned long& Timer, unsigned long PeriodMS)
{
  // has the timer interval gone off?
  unsigned long Now = millis();
  if (Now - Timer > PeriodMS)
  {
    Timer = Now;
    return true;
  }
  return false;
}

byte RTC::BCD2Dec(byte BCD)
{
  return (BCD / 16 * 10) + (BCD & 0x0F);
}

byte RTC::Dec2BCD(byte Dec)
{
  return (Dec / 10 * 16) + (Dec % 10);
}
