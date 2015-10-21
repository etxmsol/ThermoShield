/*
 * adcChannel.cpp

 *
 *  Created on: 21 okt 2015
 *      Author: Mikhail
 */
#include "Arduino.h"
#include "adcChannel.h"

AdcChannel::AdcChannel(int analogPin) : mAnalogPin(analogPin), lastSampledTime(0)
{

}

bool AdcChannel::isDue()
{
	return (millis() - lastSampledTime) >= samplePeriod;
}

long AdcChannel::getTemperature()
{
	long Vout = analogRead(mAnalogPin);	// voltage
	Vout = Vin * 1000 * Vout / 1024;
	lastSampledTime = millis();

	// calculate thermistor resistance
	float ratio = (float)1/((float)5000/(float)Vout-(float)1);
	long Rth = 100000 * ratio;

	//THERM=100k*exp( 3950/(temp+273) - 3950/(25+273) )
	return Rth;
}
