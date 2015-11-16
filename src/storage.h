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
	typedef enum ItemState {
		normal,
		forced_off,
		forced_on
	} ItemState_t;

	float Temperature;
	int mLow;
	int mHigh;
	bool mIsDirty;			//!< the new value has not been displayed
	uint8_t mActuators;		//!< bit 0 - Actuator 0, bit 7 - actuator 7
	bool mIsOn;				//!< checks if the trigger conditions are satisfied
	ItemState_t mItemState;
};

class Storage
{
public:
	Storage();
	virtual ~Storage() {};

	/*!
	 * @brief      Storage initialization.
	 *
	 * The purpose of this function is to load and parse the configuration from the
	 * config.txt on the SD card. If the SD is not inserted, it uses the configuration
	 * stored in the EEPROM if available. If no valid configuration is found in EEPROM,
	 * it just takes the items as is (default).
	 *
	 * once the configuration is selected, it tries to store it both on SD and EEPROM.
	 *
	 * if the SD is inserted but no config.txt exists, the default config.txt is created
	 *
	 * @return     Returns false on parsing errors only
	 */
	bool begin();

	void Advance();		// advance the mIndex, will be displayed
	void temperatureReading(uint8_t item, float t);


	bool LogIfDue( DateTime );
	bool isAnyActiveChannel() { return mIsAnyActiveChannel; }	//!< returns true if there is at least one active channel

	int mIndex;			//!< currently selected item (for display)

public:
	Item mItems[8];

private:
	bool parseln(const char*);

	bool mSDInserted;
	long mLastLog;

	bool mIsAnyActiveChannel;
};



#endif /* STORAGE_H_ */
