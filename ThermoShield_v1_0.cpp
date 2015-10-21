// Do not remove the include below
#include "SD.h"
#include "ThermoShield_v1_0.h"
#include "Button.h"

Button * b = NULL;		// create the object in setup, so that all button initialization
						// is done in the constructor, not in setup() body

//The setup function is called once at startup of the sketch
void setup()
{
	b = new Button(2);

	Serial1.begin(9600);
	while (!Serial1)
	{
	  delay(100);
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
		Serial1.print("Released condition\r\n");
		b->resetState();
		break;
	case Button::press_hold:
		Serial1.print("Press and hold condition\r\n");
		b->resetState();
		break;
	default:;

	}
}
