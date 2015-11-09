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



Button * b = NULL;		// create the object in setup, so that all button initialization
						// is done in the constructor, not in setup() body
AdcChannel ADCs[8];

LiquidCrystal_I2C	lcd(0x27,2,1,0,4,5,6,7); // 0x27 is the I2C bus address for an unmodified backpack
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

RTC_DS1307 rtc;

Actuator Actuators[8];

Storage Store;
int CurrentIndex = -1;
unsigned long notTooOftenCounter = 0;

static const uint8_t ALARM_LED_PIN = 3;

//The setup function is called once at startup of the sketch
void setup()
{
	pinMode(ALARM_LED_PIN, OUTPUT);
	digitalWrite( ALARM_LED_PIN, LOW );

	b = new Button(2);

	ADCs[0] = AdcChannel(A0);
	ADCs[1] = AdcChannel(A1);
	ADCs[2] = AdcChannel(A2);
	ADCs[3] = AdcChannel(A3);
	ADCs[4] = AdcChannel(A4);
	ADCs[5] = AdcChannel(A5);
	ADCs[6] = AdcChannel(A6);
	ADCs[7] = AdcChannel(A7);

	Actuators[0] = Actuator(0);
	Actuators[1] = Actuator(1);
	Actuators[2] = Actuator(2);
	Actuators[3] = Actuator(3);
	Actuators[4] = Actuator(4);
	Actuators[5] = Actuator(5);
	Actuators[6] = Actuator(6);
	Actuators[7] = Actuator(7);

	Serial1.begin(9600);
	while (!Serial1)
	{
	  delay(100);
	}

	if (! rtc.begin())
	{
		Serial1.println("Couldn't find RTC");
	}
	else
	{
		Serial1.println("RTC detected");
	}

	Serial1.println("Checking if RTC is running");
	if(rtc.isrunning())
		Serial1.println("RTC is running");
	else
	{
		Serial1.println("RTC is NOT running");
		rtc.adjust(DateTime(__DATE__, __TIME__));
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

	if(!Store.begin())
	{
		Serial1.println("Error initializing the storage");
	}

}


// The loop function is called in an endless loop
void loop()
{
	switch(b->getState())
	{
	case Button::pressed:		// if press and hold is allowed, the pressed state should not be used
		break;
	case Button::released:
		Serial1.println("Released condition");
		b->resetState();

		Store.Advance();

		break;
	case Button::press_hold:
		Serial1.println("Press and hold condition");

		switch( Store.mItems[Store.mIndex].mItemState )
		{
		case Item::normal:
			Store.mItems[Store.mIndex].mItemState = Item::forced_off;
			break;
		case Item::forced_off:
			Store.mItems[Store.mIndex].mItemState = Item::forced_on;
			break;
		case Item::forced_on:
			Store.mItems[Store.mIndex].mItemState = Item::normal;
			break;
		}
		b->resetState();
		break;
	default:;

	}

	// ADC sampling

	for(int i = 0; i < CHANNEL_COUNT; i++ )
	{
		AdcChannel * ch = &ADCs[i];
		if( ch->isActive() && ch->isDue())
		{
			Store.temperatureReading(i, ch->getTemperature());
		}
	}

	// update LCD display if necessary

	if( Store.isAnyActiveChannel() == true )
	{
		if(CurrentIndex != Store.mIndex || Store.mItems[Store.mIndex].mIsDirty)
		{
			CurrentIndex = Store.mIndex;
			lcd.home (); // set cursor to 0,0
			lcd.print("CH");
			lcd.print(CurrentIndex + 1);
			lcd.print(" ");
			lcd.print( Store.mItems[Store.mIndex].Temperature );
			lcd.print("C ");
			lcd.print( Store.mItems[Store.mIndex].mIsOn ? "ON     " : "OFF    " );
			Store.mItems[Store.mIndex].mIsDirty = false;
			lcd.setCursor (0,1);        // go to start of 2nd line

			if( Store.mItems[Store.mIndex].mItemState == Item::forced_off )
			{
				lcd.print( "FORCE OFF" );
			}
			else
			{
				if( Store.mItems[Store.mIndex].mItemState == Item::forced_on )
				{
					lcd.print( "FORCE ON " );
				}
				else
				{
					lcd.print( Store.mItems[Store.mIndex].mLow );
					lcd.print("..");
					lcd.print( Store.mItems[Store.mIndex].mHigh );
					lcd.print("C ");
				}
			}

			// list controlled actuators
			lcd.print("A:");

			for( int i = 0; i < CHANNEL_COUNT; i++ )
			{
				if( Store.mItems[Store.mIndex].mActuators & (1 << i) )
				{
					lcd.print((char)(i + '1'));
				}
			}
			lcd.print("     ");
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
	if( ms < notTooOftenCounter || notTooOftenCounter + 10000 < ms )
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
