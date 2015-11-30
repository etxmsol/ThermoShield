/*******************************************************************************
 ******************************* Copyright 2015 ********************************
 *******************************************************************************
 *
 * Total storage class
 *
 * Created on: 		2015-10-25
 * Modified on:
 * Author:			Mikhail Soloviev
 *
 *******************************************************************************
 */

#include <SD.h>
#include <EEPROM.h>
#include <avr/pgmspace.h>
#include "storage.h"
#include <string.h>
#include "actuator.h"
#include "adcChannel.h"

const char configFileHeader[] PROGMEM = {
"******************************************************************************\r\n\
* This is the configuration file config.txt. All the lines, not\r\n\
* starting with \"CH\" are ignored by the parser. The parser is not very\r\n\
* intelligent. So be careful with the format.\r\n\
*\r\n\
* The following format applies:\r\n\
*  CH<x> <TempLow> <TempHigh> [A:<y1> [y2 [y3 ...]]]\r\n\
* \r\n\
*  where <x>        is the value 1 to 8, corresponding to ADC channels\r\n\
*        <TempLow>  is the lower temperature limit in C, e.g. 20 \r\n\
*        <TempHigh> is the higher temperature limit in C, e.g. 22\r\n\
*        <y>        is the actuator that is affected by this ADC\r\n\
* \r\n\
* Example: ADC T-4 controls OUT 4, 5 and 6\r\n\
*  CH4 L:ON 20 22 A:4 5 6 \r\n\
* \r\n\
* Example: ADC T-3 is logging only\r\n\
*  CH3 L:ON\r\n\
* \r\n\
* Example: ADC T-5 is inactive. It will be skipped in the LCD\r\n\
*  CH5 L:OFF\r\n\
* \r\n\
* Notice, the CHx line removed from the file would mean default configuration\r\n\
* for this channel, and not that it is inactive\r\n\
*\r\n\
* If you screwed up the configuration, just remove this file. The default\r\n\
* configuration will be created when the SD card is inserted and reset is done.\r\n\
*******************************************************************************\r\n\r\n"};

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
static const byte MAGIC_EEPROM_BYTE2 = 0x02;	// revision

Storage::Storage()
{
	mIndex = 0;
	mSDInserted = false;
	mLastLog = 0;
	mIsAnyActiveChannel = false;
	mCheckPointsTotal = 0;

	Item defaultItem;
	defaultItem.Temperature = 0;
	defaultItem.mHigh = 22;
	defaultItem.mLow = 20;
	defaultItem.mIsDirty = true;
	defaultItem.mIsOn = false;
	defaultItem.mItemState = Item::normal;
	defaultItem.mIsLogging = true;
	defaultItem.mCheckPointsActive = 0;


	for( int i = 0; i < CHANNEL_COUNT; i++ )
	{
		defaultItem.mActuators = 1 << i;
		mItems[i] = defaultItem;
	}
}



// Storage::begin ***************************************************
// ******************************************************************
// initialize the storage and parse the config
//
bool Storage::begin()
{
	bool isSD = false;
	bool cfgFileExists = false;

	byte mb1 = EEPROM.read( 0 );		// version
	byte mb2 = EEPROM.read( 1 );		// revision

	char s[32];
	sprintf( s, "EEPROM Magic Byte 1 = %02X", mb1 );
	Serial1.println( s );
	sprintf( s, "EEPROM Magic Byte 2 = %02X", mb2 );
	Serial1.println( s );

	bool isValidConfigEEPROM = (mb1 == MAGIC_EEPROM_BYTE1 && mb2 == MAGIC_EEPROM_BYTE2);

	if( isValidConfigEEPROM )
	{
		Serial1.println( "EEPROM contains valid configuration" );
	}
	else
	{
		Serial1.println( "EEPROM does not contain valid configuration" );
	}


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
		isSD = true;			// the SD card is at least inserted

		Serial1.println( "Populating Items from SD-card" );

		// open the file. note that only one file can be open at a time,
		// so you have to close this one before opening another.

		// check if the file exists.

		cfgFileExists = SD.exists("config.txt");

		if( cfgFileExists )
		{
			Serial1.println( "config.txt found" );

			cfgFile = SD.open("config.txt");

			if ( cfgFile && cfgFile.available() )
			{
				Serial1.println("config.txt:");

				char buf[256];
				char * bufPtr = buf;

				// read from the file until there's nothing else in it:
				do
				{
					int c = cfgFile.read();

					if(c == '\r')
						continue;

					if(c == '\n' || !cfgFile.available() /*EOF*/)
					{
						if( !cfgFile.available() )
						{
							*bufPtr++ = c;
						}

						*bufPtr = 0;
						bufPtr = buf;

						if( memcmp( buf, "CH", 2 ) )		// ignore all lines not starting with "CH"
							continue;

						if( parseln( buf ) == false )
						{
							return false;
						}
					}
					else
					{
						*bufPtr++ = c;
					}

				} while ( cfgFile.available() );

				// close the file:
				cfgFile.close();
			}
			else
			{
				// if the file didn't open, print an error. Someone probably
				// removed the SD card. It is an error situation. Raise an alarm
				Serial1.println("error opening config.txt for reading");
				return false;
			}
		}
		else
		{
			Serial1.println( "config.txt does not exist" );
		}
	}

	// if reading SD configuration fails, the second alternative will be populating Items from EEPROM.
	// The item state is only stored in EEPROM. So read it in for valid EEPROM configurations

	if( isValidConfigEEPROM )
	{
		if( isSD == false )
			Serial1.println( "Populating Items from EEPROM" );

		byte ptr = 2;				// skip the first two bytes (version, revision)

		for( byte i = 0; i < CHANNEL_COUNT; i++ )
		{
			if( isSD == false )
			{
				mItems[i].mLow = (int)EEPROM.read( ptr );
				mItems[i].mHigh = (int)EEPROM.read( ptr + 1 );
				mItems[i].mActuators = EEPROM.read( ptr + 2 );
				mItems[i].mIsLogging = EEPROM.read( ptr + 3 );
			}
			mItems[i].mItemState = static_cast<Item::ItemState_t>(EEPROM.read( ptr + 4 ));

			ptr += EEPROM_ITEM_SZ;				// move to the next record
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

			Serial1.println("config.txt: creating default configuration");

			// insert the header

			int len = strlen_P( configFileHeader );
			for (int k = 0; k < len; k++)
			{
				cfgFile.print( static_cast<char>(pgm_read_byte_near( configFileHeader + k ) ) );
			}

			for( int i = 0; i < CHANNEL_COUNT; i++ )
			{
				char buf[256];
				sprintf(buf, "CH%d L:%s %d %d A:", i+1, mItems[i].mIsLogging ? "ON" : "OFF", mItems[i].mLow, mItems[i].mHigh );
				cfgFile.print(buf);

				for( int k = 0; k < CHANNEL_COUNT; k++ )
				{
					if( mItems[i].mActuators & (1 << k) )
					{
						cfgFile.print( ' ' );
						cfgFile.print( (char)(k + '1') );
					}
				}
				cfgFile.println();
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
		EEPROM.write( ptr++, mItems[i].mIsLogging );

		if( !isValidConfigEEPROM )
			EEPROM.write( ptr, mItems[i].mItemState );
		ptr++;

		sprintf( b, "mItems[%d].mLow=%d", i, mItems[i].mLow);
		Serial1.println( b );
		sprintf( b, "mItems[%d].mHigh=%d", i, mItems[i].mHigh);
		Serial1.println( b );
		sprintf( b, "mItems[%d].mActuators=0x%X", i, mItems[i].mActuators);
		Serial1.println( b );
		sprintf( b, "mItems[%d].mIsLogging=%s", i, mItems[i].mIsLogging ? "ON" : "OFF");
		Serial1.println( b );
		sprintf( b, "mItems[%d].mItemState=%d", i, mItems[i].mItemState );
		Serial1.println( b );

		// activate the corresponding ADC

		if ( mItems[i].mActuators || mItems[i].mIsLogging )
		{
			mIsAnyActiveChannel = true;
			ADCs[i].activate();
		}
	}
	return true;
}


// Storage::parseln *************************************************
// ******************************************************************
//
bool Storage::parseln( const char * line )
{
	char lBuf[256];
	strcpy(lBuf, line);

	int low, high;
	uint8_t acts = 0;		// bit mask of the controlled actuators

	char * ch = strstr(lBuf, "CH");

	if(!ch)
	{
		return true;
	}

	int chId = *(ch+2)-'1';

	if(chId < 0 || chId > 7)
	{
		Serial1.println("Channel ID out of range (1...8)");
		return false;
	}

	Serial1.print("CH");
	Serial1.print(chId+1);		// towards the user all the channels are counted 1 to 8

	// look for L:ON|L:OFF

	if( strstr(line, "L:ON") )
		mItems[chId].mIsLogging = true;
	else
	{
		if( strstr(line, "L:OFF") )
			mItems[chId].mIsLogging = false;
		else
		{
			Serial1.println( "Missing logging switch" );
			return false;
		}
	}

	strcpy( lBuf, strstr(line, "L:") );
	const char * pch = strtok( lBuf, " " );
	pch = strtok (NULL," ");

	if(!pch || !*pch)
	{
		mItems[chId].mActuators = 0;		// the default Item maps to an actuator, remove this mapping

		if( mItems[chId].mIsLogging )
			Serial1.println(" logging only");
		else
			Serial1.println(" inactive channel");

		return true;
	}

	Serial1.print(" mLow=");
	low = atoi(pch);
	Serial1.print(low);

	if(!(pch = strtok (NULL," ")))
	{
		Serial1.println(" Malformed line");
		return false;
	}

	Serial1.print(" mHigh=");
	high = atoi(pch);
	Serial1.print(high);

	mItems[chId].mLow = low;
	mItems[chId].mHigh = high;

	if(!(pch = strtok (NULL," ")))
	{
		mItems[chId].mActuators = 0;
		return true;
	}

	strcpy(lBuf, line);
	char * actuators = strstr( lBuf, "A:" );

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
				Serial1.print(aInt);
				Serial1.println( " <== Actuator value out of range (1..8)" );
				return false;
			}
		}
		Serial1.println();

		mItems[chId].mActuators = acts;
	}
	return true;
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
	mCheckPointsTotal++;

	for(int i = 0; i < CHANNEL_COUNT; i++ )
	{
		if( mItems[i].mIsOn )
			mItems[i].mCheckPointsActive++;
	}

	if( mLastLog + LOGGING_INTERVAL < dt.secondstime() )
	{
		mLastLog = dt.secondstime();

		for(int i = 0; i < CHANNEL_COUNT; i++ )
		{
			if( mItems[i].mIsLogging )
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
					sprintf(buf, "%d%02d%02d %02d00  %d  %d%%", dt.year(), dt.month(), dt.day(), dt.hour(),
							(int)(mItems[i].Temperature),
							(int)((float)(mItems[i].mCheckPointsActive)/(float)mCheckPointsTotal * 100));
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

		mCheckPointsTotal = 0;

		for(int i = 0; i < CHANNEL_COUNT; i++ )
		{
			mItems[i].mCheckPointsActive = 0;
		}

		return true;
	}
	return true;
}


// Storage::setItemState ********************************************
// ******************************************************************
// this is a special setter. It not only updates the mItem but
// updates the value in EEPROM
//
void Storage::setItemState(int index, Item::ItemState_t itemState)
{
	mItems[index].mItemState = itemState;

	// find the relevant item in EEPROM and update only this byte

	EEPROM.write( 2 + index * EEPROM_ITEM_SZ + 4, itemState);
}
