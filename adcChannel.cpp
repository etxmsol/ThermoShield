/*
 * adcChannel.cpp

 *
 *  Created on: 21 okt 2015
 *      Author: Mikhail
 */
#include "Arduino.h"
#include "adcChannel.h"

AdcChannel::AdcChannel() : mAnalogPin(0), lastSampledTime(0), mIsActive(false) {}
AdcChannel::AdcChannel(int analogPin) : mAnalogPin(analogPin), lastSampledTime(0), mIsActive(false) {}

void AdcChannel::activate()
{
	mIsActive = true;
	pinMode(mAnalogPin, INPUT);
}

bool AdcChannel::isDue()
{
	return (millis() - lastSampledTime) >= samplePeriod;
}

float AdcChannel::getTemperature()
{
	long Vout = analogRead(mAnalogPin);	// voltage
	lastSampledTime = millis();

	// calculate thermistor resistance
	float ratio = (float)1/((float)1023/(float)Vout-(float)1);
	float Rth = 100000.0 * ratio;

	float temp = 1.0/(1.0/298.15 + 1.0/B*log(Rth/R0))-273.15;

	//float temp = B/log(Rth/(R0*exp(-B/298.15)))-273.15;
	//THERM=100k*exp( 3950/(temp+273) - 3950/(25+273) )
	return temp;
}
