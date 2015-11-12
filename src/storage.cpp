/*
 * Storage.cpp
 *
 *  Created on: 22 okt 2015
 *      Author: Mikhail
 */

#include <SD.h>
#include <EEPROM.h>
#include "storage.h"
#include <string.h>
#include "actuator.h"
#include "adcChannel.h"

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int chipSelect = 10;

File cfgFile;

extern Actuator Actuators[8];
extern AdcChannel ADCs[8];

static const int LOGGING_INTERVAL = 3600;		// seconds

static const byte MAGIC_EEPROM_BYTE1 = 0x01;	// version
static const byte MAGIC_EEPROM_BYTE2 = 0x00;	// revision

Storage::Storage()
{
	mIndex = 0;
	mSDInserted = false;
	mLastLog = 0;
	mIsAnyActiveChannel = false;

	Item defaultItem;
	defaultItem.Temperature = 0;
	defaultItem.mHigh = 22;
	defaultItem.mLow = 20;
	defaultItem.mIsDirty = true;
	defaultItem.mIsOn = false;
	defaultItem.mItemState = Item::normal;

	for( int i = 0; i < CHANNEL_COUNT; i++ )
	{
		defaultItem.mActuators = 1 << i;
		mItems[i] = defaultItem;
	}
}



bool Storage::begin()
{
	bool isSD = false;
	bool cfgFileExists = false;


	Serial1.println("\nInitializing SD card...");

	// On the Ethernet Shield, CS is pin 4. It's set as an output by default.
	// Note that even if it's not used as the CS pin, the hardware SS pin
	// (10 on most Arduino boards, 53 on the Mega) must be left as an output
	// or the SD library functions will not work.
	pinMode(SS, OUTPUT);


	if ( !SD.begin(chipSelect, 11, 12, 13) )
	{
		Serial1.println("SD card not found. EEPROM configuration will be used");
	}
	else
	{
		// open the file. note that only one file can be open at a time,
		// so you have to close this one before opening another.

		// check if the file exists.

		cfgFileExists = SD.exists("config.txt");

		if( cfgFileExists )
		{
			cfgFile = SD.open("config.txt");

			if (cfgFile)
			{
				Serial1.println("config.txt:");

				char buf[256];
				char * bufPtr = buf;

				// read from the file until there's nothing else in it:
				while (cfgFile.available())
				{
					int c = cfgFile.read();
					//Serial1.write(c);
					*bufPtr++ = c;
					if(c == '\n')
					{
						*bufPtr = 0;
						parseln(buf);
						bufPtr = buf;
					}
				}
				// close the file:
				cfgFile.close();
				isSD = true;			// the SD card is at least inserted
			}
			else
			{
				// if the file didn't open, print an error:
				Serial1.println("error opening config.txt for reading");
			}
		}
	}

	// if reading SD configuration fails, the second alternative will be the configuration
	// stored in the EEPROM.

	if( isSD == false || cfgFileExists == false )
	{
		byte ptr = 0;
		byte mb1 = EEPROM.read( ptr++ );
		byte mb2 = EEPROM.read( ptr++ );

		char s[32];
		sprintf( s, "EEPROM Magic Byte 1 = %02X", mb1 );
		Serial1.println( s );
		sprintf( s, "EEPROM Magic Byte 2 = %02X", mb2 );
		Serial1.println( s );

		if( mb1 == MAGIC_EEPROM_BYTE1 && mb2 == MAGIC_EEPROM_BYTE2 )
		{
			Serial1.println( "EEPROM contains valid configuration" );

			for( byte i = 0; i < CHANNEL_COUNT; i++ )
			{
				mItems[i].mLow = (int)EEPROM.read( ptr++ );
				mItems[i].mHigh = (int)EEPROM.read( ptr++ );
				mItems[i].mActuators = EEPROM.read( ptr++ );
			}
		}
		else
		{
			Serial1.println( "EEPROM does not contain valid configuration" );
		}
	}

	// now write down the whatever configuration to EEPROM and to SD card (if possible)

	cfgFile = SD.open("config.txt", FILE_WRITE);

	if (cfgFile)
	{
		// if the SD card is in, but there is no config file, create it

		if( isSD == true && !cfgFileExists )
		{
			// if the SD card is in but the config file is not found, the default
			// file will be generated and stored on the SD card (and in EEPROM)

			Serial1.println("config.txt: creating configuration");

			for( int i = 0; i < CHANNEL_COUNT; i++ )
			{
				char buf[256];
				sprintf(buf, "CH%d %d %d A:%d", i+1, mItems[i].mLow, mItems[i].mHigh, mItems[i].mActuators);
				cfgFile.println(buf);
			}

			// close the file:
			cfgFile.close();
			Serial1.println("done.");
		}
	}
	else
	{
		// if the file didn't open, print an error:
		Serial1.println("error opening config.txt for writing");
	}

	// store to EEPROM

	byte ptr = 0;
	EEPROM.write( ptr++, MAGIC_EEPROM_BYTE1 );
	EEPROM.write( ptr++, MAGIC_EEPROM_BYTE2 );

	char b[128];

	for( byte i = 0; i < CHANNEL_COUNT; i++ )
	{
		EEPROM.write( ptr++, mItems[i].mLow);
		EEPROM.write( ptr++, mItems[i].mHigh );
		EEPROM.write( ptr++, mItems[i].mActuators );

		sprintf( b, "mItems[%d].mLow=%d", i, mItems[i].mLow);
		Serial1.println( b );
		sprintf( b, "mItems[%d].mHigh=%d", i, mItems[i].mHigh);
		Serial1.println( b );
		sprintf( b, "mItems[%d].mActuators=0x%X", i, mItems[i].mActuators);
		Serial1.println( b );

		// activate the corresponding ADC

		if ( mItems[i].mActuators )
		{
			mIsAnyActiveChannel = true;
			ADCs[i].activate();
		}
	}
	return true;
}



void Storage::parseln(const char * line)
{
	char lBuf[256];
	strcpy(lBuf, line);

	int low, high;
	uint8_t acts = 0;		// bit mask of the controlled actuators

	char * ch = strstr(lBuf, "CH");
	if(!ch)
		return;

	int chId = *(ch+2)-'1';

	if(chId < 0 || chId > 7)
	{
		Serial1.println("Channel ID out of range (1...8)");
		return;
	}

	Serial1.print("CH");
	Serial1.print(chId+1);		// towards the user all the channels are counted 1 to 8

	const char * pch = strtok (ch," ");
	pch = strtok (NULL," ");

	if(!pch || !*pch)
	{
		mItems[chId].mActuators = 0;		// the default Item maps to an actuator, remove this mapping
		Serial1.println(" inactive channel");
		return;
	}

	Serial1.print(" mLow=");
	low = atoi(pch);
	Serial1.print(low);

	if(!(pch = strtok (NULL," ")))
	{
		Serial1.println(" Malformed line");
		return;		// TODO: revert to default
	}

	Serial1.print(" mHigh=");
	high = atoi(pch);
	Serial1.print(high);

	mItems[chId].mLow = low;
	mItems[chId].mHigh = high;

	if(!(pch = strtok (NULL," ")))
	{
		Serial1.println(" No actuators!?");
		mItems[chId].mActuators = 0;
		return;
	}

	strcpy(lBuf, line);
	char * actuators = strstr(line, "A:");

	if(actuators)
	{
		strtok(actuators, " :");
		const char * a = strtok(NULL, " ");

		while(a)
		{
			Serial1.print(" A=");
			int aInt = atoi(a);

			if( aInt <= CHANNEL_COUNT && aInt > 0 )
			{
				acts |= 1 << (aInt-1);
				Serial1.print(aInt);
				Serial1.print(" ");
				a = strtok(NULL, " ");
			}
			else
			{
				Serial1.println( "Config error. Actuator value out of range (1..8)" );
			}
		}
		Serial1.println();

		mItems[chId].mActuators = acts;
	}

}


// Storage::temperatureReading **************************************
// ******************************************************************
//
void Storage::temperatureReading(uint8_t item, float t)
{
	mItems[item].Temperature = t;
	mItems[item].mIsDirty = true;

	// actuate
	for(uint8_t actuatorId = 0; actuatorId < CHANNEL_COUNT; actuatorId++)
	{
		if(mItems[item].mActuators & (1 << actuatorId))
		{
			if(mItems[item].mItemState == Item::forced_on || (mItems[item].mItemState == Item::normal && t <= mItems[item].mLow))
			{
				Actuators[actuatorId].activate(item);
				mItems[item].mIsOn =  true;
			}
			else
			{
				if(mItems[item].mItemState == Item::forced_off ||  t >= mItems[item].mHigh)
				{
					Actuators[actuatorId].deactivate(item);
					mItems[item].mIsOn =  false;
				}
			}
		}
	}
}


// Storage::Advance *************************************************
// ******************************************************************
// advances to the next active item. If all items are inactive
// keep the value of mIndex and exit the loop
//
void Storage::Advance()
{
	// a button press caused advance of the item for displaying. Skip
	// inactive channels

	if( !mIsAnyActiveChannel )
		return;

	do {
		mIndex = mIndex + 1 == CHANNEL_COUNT ? 0 : mIndex + 1;
	} while( !ADCs[mIndex].isActive() );
}


bool Storage::LogIfDue( DateTime dt )
{
	if( mLastLog + LOGGING_INTERVAL < dt.secondstime() )
	{
		mLastLog = dt.secondstime();

		for(int i = 0; i < CHANNEL_COUNT; i++ )
		{
			if( ADCs[i].isActive() )
			{
				char fileName[13];

				itoa( dt.year(), fileName, 10 );
				itoa( dt.month(), fileName+4, 10 );
				fileName[6] = 'A';
				fileName[7] = i + '1';
				fileName[8] = 0;
				strcat( fileName, ".txt" );

				Serial1.print( "opening file " );
				Serial1.println( fileName );
				File f = SD.open( fileName, FILE_WRITE);

				if (f)
				{
					char buf[256];
					sprintf(buf, "%d%02d%02d %02d00  %d", dt.year(), dt.month(), dt.day(), dt.hour(), (int)(mItems[i].Temperature));
					if( !f.println(buf) )
					{
						Serial1.println( "Failed to log. Is the SD inserted? Do not forget to reset after insertion" );
						return false;
					}
					Serial1.println( buf );

					f.close();
				} else {
				// if the file didn't open, print an error:
					Serial1.print("error opening ");
					Serial1.print( fileName );
					Serial1.println(" for writing");
					return false;
				}
			}
		}
		return true;
	}
	return true;
}
