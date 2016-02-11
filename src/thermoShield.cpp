// ******************************************************************
//
// Implementation of Arduino's setup and loop
//
// Date: 2015-11-05
// Author: Mikhail Soloviev
//
// ******************************************************************
#include "SD.h"
#include "button.h"
#include "adcChannel.h"
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <RTClib.h>
#include "storage.h"
#include "actuator.h"


// CONSTANTS
static const uint8_t BUTTON_PIN = 2;
static const uint8_t ALARM_LED_PIN = 3;
static const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Global data

Button b(BUTTON_PIN);

AdcChannel ADCs[8];

LiquidCrystal_I2C	lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the I2C bus address for an unmodified backpack

RTC_DS1307 rtc;

Actuator Actuators[8];

Storage Store;
int CurrentIndex = -1;
unsigned long notTooOftenCounter = 0;

bool ForceADCReadout = false;

int freeRam ();


void setup()
{
	// debugging channel

	Serial1.begin(57600);
	while (!Serial1)
	{
	  delay(100);
	}

	Serial1.print( "RAM at setup " );
	Serial1.println( freeRam() );

	// ALRAM
	pinMode(ALARM_LED_PIN, OUTPUT);
	digitalWrite( ALARM_LED_PIN, LOW );

	// must "begin" the button to get proper pin assignment/muxing
	b.begin();

	// initializing ADC channels. The channels 0...7 are assigned to the
	// pins A0...A7. Note that the display and the config.txt files, as well as
	// the shield's silk layer use enumeration 1 to 8

	ADCs[0] = AdcChannel(A0);
	ADCs[1] = AdcChannel(A1);
	ADCs[2] = AdcChannel(A2);
	ADCs[3] = AdcChannel(A3);
	ADCs[4] = AdcChannel(A4);
	ADCs[5] = AdcChannel(A5);
	ADCs[6] = AdcChannel(A6);
	ADCs[7] = AdcChannel(A7);

	// initializing actuators. The Id is 0 to 7, a bit number in
	// actuator byte of the ADC channel. The mapping to the pin is
	// pin = A8 + Id

	Actuators[0] = Actuator(0, true);
	Actuators[1] = Actuator(1, true);
	Actuators[2] = Actuator(2, true);
	Actuators[3] = Actuator(3, true);
	Actuators[4] = Actuator(4, true);
	Actuators[5] = Actuator(5, true);
	Actuators[6] = Actuator(6, true);
	Actuators[7] = Actuator(7, false);		// the last actuator is not connected through ULN2003 but directly

	// The Real Time Clock

	rtc.begin();			// returns bool, but is never false

	Serial1.print( "RAM after rtc.begin " );
	Serial1.println( freeRam() );

	if(rtc.isrunning())
		Serial1.println("RTC is running");
	else
	{
		Serial1.println("RTC is NOT running");
		rtc.adjust(DateTime(__DATE__, __TIME__));		// setup the current date and time initially
	}

    DateTime now = rtc.now();

    Serial1.print(now.year(), DEC);
    Serial1.print('/');
    Serial1.print(now.month(), DEC);
    Serial1.print('/');
    Serial1.print(now.day(), DEC);
    Serial1.print(" (");
    Serial1.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial1.print(") ");
    Serial1.print(now.hour(), DEC);
    Serial1.print(':');
    Serial1.print(now.minute(), DEC);
    Serial1.print(':');
    Serial1.print(now.second(), DEC);
    Serial1.println();

	// activate LCD module
	lcd.begin (16,2); // for 16 x 2 LCD module
	lcd.setBacklightPin(3,POSITIVE);
	lcd.setBacklight(HIGH);

	Serial1.print( "RAM after lcd.begin " );
	Serial1.println( freeRam() );


	if(!Store.begin())
	{
		Serial1.println("Error initializing the storage");
		digitalWrite( ALARM_LED_PIN, HIGH );
	}
	Serial1.print( "RAM after Storage.begin " );
	Serial1.println( freeRam() );


	// the below makes sure the 1st active item is displayed upon start up. Otherwise
	// the item 0 is displayed, even if inactive
	Store.mIndex = CHANNEL_COUNT-1;
	Store.Advance();
}


// The loop function is called in an endless loop
void loop()
{
	switch(b.getState())
	{
	case Button::pressed:		// if press and hold is allowed, the pressed state should not be used
		break;
	case Button::released:
		Serial1.println("Released condition");
		b.resetState();

		Store.Advance();

		break;
	case Button::press_hold:
		Serial1.println("Press and hold condition");

		ForceADCReadout = true;

		switch( Store.getItemState(Store.mIndex) )
		{
		case Item::normal:
			Serial1.println("normal->forced_off");
			Store.setItemState(Store.mIndex, Item::forced_off);
			break;
		case Item::forced_off:
			Serial1.println("forced_off->forced_on");
			Store.setItemState(Store.mIndex, Item::forced_on);
			break;
		case Item::forced_on:
			Serial1.println("forced_on->normal");
			Store.setItemState(Store.mIndex, Item::normal);
			break;
		}
		b.resetState();
		break;
	default:;

	}

	// ADC sampling

	for(int i = 0; i < CHANNEL_COUNT; i++ )
	{
		AdcChannel * ch = &ADCs[i];
		if( ch->isActive() && ( ForceADCReadout || ch->isDue() ))
		{
			Store.temperatureReading(i, ch->getTemperature());
		}
	}
	ForceADCReadout = false;

	// update LCD display if necessary

	if( Store.isAnyActiveChannel() == true )
	{
		if(CurrentIndex != Store.mIndex || Store.getDirty(Store.mIndex))
		{
			CurrentIndex = Store.mIndex;
			lcd.home (); // set cursor to 0,0

			char buf[32];
			float t = Store.getTemperature(Store.mIndex);
			int intPart = t;
			unsigned int fractPart = abs((t - (float)intPart)*10.0);

			sprintf( buf, "CH%d %d.%01dC %s", CurrentIndex + 1, intPart, fractPart, Store.getIsOn(Store.mIndex) ? "ON     " : "OFF    ");
			Store.setDirty(Store.mIndex, false);
			lcd.print( buf );
			lcd.setCursor (0,1);        // go to start of 2nd line

			if( Store.getItemState(Store.mIndex) == Item::forced_off || Store.getItemState(Store.mIndex) == Item::forced_on )
			{
				lcd.print( "FORCED " );
			}
			else
			{
				lcd.print( Store.getLow(Store.mIndex) );
				lcd.print("..");
				lcd.print( Store.getHigh(Store.mIndex) );
				lcd.print("C ");
			}

			// list controlled actuators
			lcd.print("A:");

			for( int i = 0; i < CHANNEL_COUNT; i++ )
			{
				if( Store.getActuators(Store.mIndex) & (1 << i) )
				{
					lcd.print((char)(i + '1'));
				}
			}
			lcd.print("      ");
		}
	}
	else
	{
		lcd.home (); // set cursor to 0,0
		lcd.print("ALL CHANNELS");
		lcd.setCursor (0,1);        // go to start of 2nd line
		lcd.print("INACTIVE");
	}


	// log the data if time comes. Protect against wrap around

	unsigned long ms = millis();
	if( ms < notTooOftenCounter || notTooOftenCounter + 60000 < ms )
	{
		notTooOftenCounter = millis();

		DateTime now = rtc.now();
		if( !Store.LogIfDue( now ) )
		{
			Serial1.println("No logging. The write attempt failed. Is the SD inserted? Insert and reset!");
			digitalWrite( ALARM_LED_PIN, HIGH );
		}
	}

}
