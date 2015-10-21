/*
 * button.h
 *
 *  Created on: 19 okt 2015
 *      Author: Mikhail
 */

#ifndef BUTTON_H_
#define BUTTON_H_

class Button
{
public:
	enum State {
		idle,
		pressed,
		released,
		press_hold,
		double_press
	};

public:
	Button(int pin);
	virtual ~Button() {};

	State getState();
	void resetState()
	{
		mState = idle;
		lastPressAndHoldTime = 0;
	}

private:
	State mState;		// the state upon release or timeout
	int mPin;
	int mButtonState;	// the physical state, currently pressed or depressed
	long lastDebounceTime;  // the last time the output pin was toggled
	long lastPressAndHoldTime;
	int lastButtonState;

	static const long debounceDelay = 50;
	static const long pressAndHoldDelay = 1000;

};



#endif /* BUTTON_H_ */
