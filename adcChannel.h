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
	AdcChannel(int analogPin);
	virtual  ~AdcChannel() {};

	bool isDue();
	long getTemperature();

private:
	int mAnalogPin;
	long lastSampledTime;

	static const long samplePeriod = 10000;
	static const int Vin = 5;
};



#endif /* ADCCHANNEL_H_ */
