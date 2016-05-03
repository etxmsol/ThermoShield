/*******************************************************************************
 ******************************* Copyright 2015 ********************************
 *******************************************************************************
 *
 * Implementation of the ADC channel
 *
 * Created on: 		2015-11-05
 * Modified on:
 * Author:			Mikhail Soloviev
 *
 *******************************************************************************
 */
#include "Arduino.h"
#include "adcChannel.h"

AdcChannel::AdcChannel() : mAnalogPin(0), lastSampledTime(0), mIsActive(false)
{
	memset( mSampleWindow, 0, sizeof(float) * SAMPLE_WINDOW );
	mCurrentSampleIndex = 0;
	mIsSampleWindowFull = false;
}

AdcChannel::AdcChannel(int analogPin) : mAnalogPin(analogPin), lastSampledTime(0), mIsActive(false)
{
	memset( mSampleWindow, 0, sizeof(float) * SAMPLE_WINDOW );
	mCurrentSampleIndex = 0;
	mIsSampleWindowFull = false;
}



void AdcChannel::activate()
{
	mIsActive = true;
	pinMode(mAnalogPin, INPUT);
}



bool AdcChannel::isDue()
{
	unsigned long ms = millis();

	// provide for the wrap around of millis()

	return !lastSampledTime || ms < lastSampledTime || (ms - lastSampledTime) >= samplePeriod;
}



float AdcChannel::getTemperature()
{
	long accumulator = 0;

	for( int i = 0; i < 5; i++ )
	{
		int v = analogRead(mAnalogPin);	// voltage;
		v = v == 0 ? 1 : v;

		accumulator += v;
	}
	long Vout = accumulator / 5;

	lastSampledTime = millis();

	// calculate thermistor resistance
	float ratio = (float)1/((float)1023/(float)Vout-(float)1);
	float Rth = (float)R0 * ratio;

	float temp = 1.0/(1.0/298.15 + 1.0/B*log(Rth/R0))-273.15;

	return temp;

	//float temp = B/log(Rth/(R0*exp(-B/298.15)))-273.15;
	//THERM=100k*exp( 3950/(temp+273) - 3950/(25+273) )

	// the temp is an instantaneous sample, but the getTemperature
	// returns the value averaged over the last SAMPLE_WINDOW samples

	if( mCurrentSampleIndex == SAMPLE_WINDOW )
	{
		mIsSampleWindowFull = true;
		mCurrentSampleIndex =  0;
	}

	mSampleWindow[mCurrentSampleIndex++] = temp;

	float sampleSum = 0;
	int sampleCnt = mIsSampleWindowFull ? SAMPLE_WINDOW : mCurrentSampleIndex;

	for( int i = 0; i < sampleCnt; i++ )
	{
		sampleSum += mSampleWindow[i];
	}
	return sampleSum/(float)sampleCnt;
}
