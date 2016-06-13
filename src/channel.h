/*******************************************************************************
 ******************************* Copyright 2016 ********************************
 *******************************************************************************
 *
 * Total storage class
 *
 * Created on: 		2016-04-07
 * Modified on:
 * Author:			Mikhail Soloviev
 *
 *******************************************************************************
 */

#ifndef CHANNEL_H_
#define CHANNEL_H_

#define MAX_DEPTH 		8		// max number of layers (1 master + up to 7 slaves)
#define CHANNEL_COUNT 	8
#define EEPROM_ITEM_SZ 	6


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
	long mR;				//!< digital POT impedance (should match the NTC)

	bool mIsDirty;			//!< the new value has not been displayed
	uint8_t mActuators;		//!< bit 0 - Actuator 0, bit 7 - actuator 7
	bool mIsOn;				//!< checks if the trigger conditions are satisfied
	ItemState_t mItemState;
	bool mIsLogging;

	/*! this is an accumulator of activation
	 *  It is the owner that decides when to reset the accumulator. It is a counter
	 *  stepped up if the Item is found active at the check-point. The owner resets it once per logging period.
	 *  The logging period consists of an arbitrary number of check-points. The logger should use this total
	 *  count of check-points and the activity accumulator in order to calculate the active/total ratio
	 */
	long mCheckPointsActive;

	long mToggleCounter;

	float mCalibrationValue;	//!< the calibration value for the temperature for this channel, e.g. -0.5
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


//	bool LogIfDue( DateTime );
	bool isAnyActiveChannel() { return mIsAnyActiveChannel; }	//!< returns true if there is at least one active channel


	//! Item accessors

	void setTemperature(int slaveId, int index, float t);
	float getTemperature(int slaveId, int index) { return mItems[slaveId][index].Temperature; }

	void setLow(int slaveId, int index, int mLow) { mItems[slaveId][index].mLow = mLow; }
	int getLow(int slaveId, int index) { return mItems[slaveId][index].mLow;  }

	void setHigh(int slaveId, int index, int mHigh) { mItems[slaveId][index].mHigh = mHigh; }
	int getHigh(int slaveId, int index) { return mItems[slaveId][index].mHigh;  }

	void setR(int slaveId, int index, long mR) { mItems[slaveId][index].mR = mR; }
	long getR(int slaveId, int index) { return mItems[slaveId][index].mR;  }

	void setDirty(int slaveId, int index, bool isDirty) { mItems[slaveId][index].mIsDirty = isDirty; }
	bool getDirty(int slaveId, int index) { return mItems[slaveId][index].mIsDirty; }

	void setActuators(int slaveId, int index, uint8_t as) {  mItems[slaveId][index].mActuators = as; }
	uint8_t getActuators(int slaveId, int index) {  return mItems[slaveId][index].mActuators; }

	void setIsOn(int slaveId, int index, bool isOn) {  mItems[slaveId][index].mIsOn = isOn; }
	bool getIsOn(int slaveId, int index) {  return mItems[slaveId][index].mIsOn; }

	void setItemState(int slaveId, int index, Item::ItemState_t mItemState);
	Item::ItemState_t getItemState(int slaveId, int index) { return mItems[slaveId][index].mItemState; }

	void setIsLogging(int slaveId, int index, bool isLogging) {  mItems[slaveId][index].mIsLogging = isLogging; }
	bool getIsLogging(int slaveId, int index) {  return mItems[slaveId][index].mIsLogging; }


	int mIndex;			//!< currently selected item (for display)

private:
	Item mItems[MAX_DEPTH][CHANNEL_COUNT];

private:
	bool parseln(const char*);

	bool mSDInserted;
	long mLastLog;

	bool mIsAnyActiveChannel;

	long mCheckPointsTotal;
};



#endif /* CHANNEL_H_ */
