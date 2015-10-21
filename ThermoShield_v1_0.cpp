// Do not remove the include below
#include "SD.h"
#include "ThermoShield_v1_0.h"
#include "Button.h"
#include "adcChannel.h"

Button * b = NULL;		// create the object in setup, so that all button initialization
						// is done in the constructor, not in setup() body
AdcChannel * adc0;

//The setup function is called once at startup of the sketch
void setup()
{
	b = new Button(2);
	adc0 = new AdcChannel(A0);

	Serial1.begin(9600);
	while (!Serial1)
	{
	  delay(100);
	}
}

// The loop function is called in an endless loop
void loop()
{
	long t;
	switch(b->getState())
	{
	case Button::pressed:		// if press and hold is allowed, the pressed state should not be used
		break;
	case Button::released:
		Serial1.print("Released condition\r\n");
		b->resetState();

		t = adc0->getTemperature();
		Serial1.print(t);
		Serial1.print("\r\n");

		break;
	case Button::press_hold:
		Serial1.print("Press and hold condition\r\n");
		b->resetState();
		break;
	default:;

	}

	if(adc0->isDue())
	{
		int temp = adc0->getTemperature();
		Serial1.print(temp);
		Serial1.print("\r\n");
	}
}
