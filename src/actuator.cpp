/*******************************************************************************
 ******************************* Copyright 2015 ********************************
 *******************************************************************************
 *
 * Actuator class
 *
 * Created on: 		2015-11-06
 * Modified on:
 * Author:			Mikhail Soloviev
 *
 *******************************************************************************
 */

#include "actuator.h"

Actuator::Actuator(uint8_t id, bool isActiveHigh)
{
	mId = id;
	mChannels = 0;
	mActiveHigh = isActiveHigh;
	pinMode(A8+mId, OUTPUT);
	digitalWrite(A8+mId, mActiveHigh ? LOW : HIGH);
}


void Actuator::activate(uint8_t adcChannel)
{
	mChannels |= 1 << adcChannel;
	digitalWrite(A8+mId, mActiveHigh ? HIGH : LOW);
}


void Actuator::deactivate(uint8_t adcChannel)
{
	mChannels &= ~(1 << adcChannel);

	if(!mChannels)
	{
		digitalWrite(A8+mId, mActiveHigh ? LOW : HIGH);
	}
}
