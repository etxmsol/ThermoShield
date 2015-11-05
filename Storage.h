/*
 * Storage.h
 *
 *  Created on: 22 okt 2015
 *      Author: Mikhail
 */

#ifndef STORAGE_H_
#define STORAGE_H_

#include "RTClib.h"

const int CHANNEL_COUNT = 8;

struct Item
{
	float Temperature;
	int mLow;
	int mHigh;
	bool mIsDirty;			// the new value has not been displayed
	uint8_t mActuators;		// bit 0 - Actuator 0, bit 7 - actuator 7
};

class Storage
{
public:
	Storage();
	virtual ~Storage() {};

	bool begin();
	void Advance();		// advance the mIndex, will be displayed
	void temperatureReading(uint8_t item, float t);


	bool isLogDue( DateTime );

	int mIndex;			// currently selected item (for display)

public:
	Item mItems[8];

private:
	void parseln(const char*);

	bool mSDInserted;
	long mLastLog;
};



#endif /* STORAGE_H_ */
