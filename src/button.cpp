/*
 * Button.cpp
 *
 *  Created on: 19 okt 2015
 *      Author: Mikhail
 */

#include "Arduino.h"
#include "button.h"

Button::Button(int pin) : mState(idle), mPin(pin), mButtonState(LOW), lastDebounceTime(0), lastPressAndHoldTime(0),
						  lastButtonState(LOW)
{
	pinMode(mPin, INPUT);
}

Button::State Button::getState()
{
	// if the button entered press_hold state, it shall remain in that
	// until the state is reset by the owner. Until that the pin readings are
	// not important

	if(lastPressAndHoldTime && ((millis() - lastPressAndHoldTime) > pressAndHoldDelay))
	{
		mState = press_hold;
		return mState;
	}

	int reading = digitalRead(mPin);

	if (reading != lastButtonState)
	{
		// reset the debouncing timer
		lastDebounceTime = millis();
	}

	if (lastDebounceTime && ((millis() - lastDebounceTime) > debounceDelay))
	{
		lastDebounceTime = 0;

		// whatever the reading is at, it's been there for longer
		// than the debounce delay, so take it as the actual current state:

		// if the button state has changed:
		if (reading != mButtonState)
		{
			mButtonState = reading;

			if(reading == HIGH)
			{
				mState = pressed;
				lastPressAndHoldTime = millis();
			}
			else if (reading == LOW)
			{
				if(mState == pressed)
				{
					mState = released;
				}
				lastPressAndHoldTime = 0;
			}
		}
	}


	// save the reading.  Next time through the loop,
	// it'll be the lastButtonState:
	lastButtonState = reading;
	return mState;
}



