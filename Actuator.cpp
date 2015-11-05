/*
 * Actuator.cpp
 *
 *  Created on: 22 okt 2015
 *      Author: Mikhail
 */

#include "Actuator.h"

Actuator::Actuator(uint8_t id)
{
	mId = id;
	mChannels = 0;
	pinMode(62+mId, OUTPUT);
}


void Actuator::activate(uint8_t adcChannel)
{
	mChannels |= 1 << adcChannel;
	digitalWrite(62+mId, LOW);
}

void Actuator::deactivate(uint8_t adcChannel)
{
	mChannels &= ~(1 << adcChannel);

	if(!mChannels)
	{
		digitalWrite(62+mId, HIGH);
	}
}
