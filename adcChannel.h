/*
 * adcChannel.h
 *
 *  Created on: 21 okt 2015
 *      Author: Mikhail
 */

#ifndef ADCCHANNEL_H_
#define ADCCHANNEL_H_

class AdcChannel
{
public:
	AdcChannel();
	AdcChannel(int analogPin);
	virtual  ~AdcChannel() {};

	bool isDue();
	float getTemperature();
	void activate();
	bool isActive() { return mIsActive; }

private:
	int mAnalogPin;
	long lastSampledTime;
	bool mIsActive;

	static const long samplePeriod = 1000;
	static const int Vin = 5;
	static const float B = 3950;
	static const float R0 = 100000;
};



#endif /* ADCCHANNEL_H_ */
