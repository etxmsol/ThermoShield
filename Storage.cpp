/*
 * Storage.cpp
 *
 *  Created on: 22 okt 2015
 *      Author: Mikhail
 */

#include <SD.h>
#include "Storage.h"
#include <string.h>
#include "Actuator.h"
#include "adcChannel.h"

// change this to match your SD shield or module;
// Arduino Ethernet shield: pin 4
// Adafruit SD shields and modules: pin 10
// Sparkfun SD shield: pin 8
const int chipSelect = 10;

File cfgFile;

extern Actuator Actuators[8];
extern AdcChannel ADCs[8];

static const int LOGGING_INTERVAL = 3600;	// seconds

Storage::Storage()
{
	mIndex = 0;
	mSDInserted = false;
	mLastLog = 0;

	Item defaultItem;
	defaultItem.Temperature = 0;
	defaultItem.mHigh = 27;
	defaultItem.mLow = 24;
	defaultItem.mIsDirty = true;

	for(int i=0; i<8; i++)
	{
		defaultItem.mActuators = 1 << i;
		mItems[i] = defaultItem;
	}
}

bool Storage::begin()
{
	  Serial1.println("\nInitializing SD card...");
	  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
	  // Note that even if it's not used as the CS pin, the hardware SS pin
	  // (10 on most Arduino boards, 53 on the Mega) must be left as an output
	  // or the SD library functions will not work.
	  pinMode(SS, OUTPUT);


	  if (!SD.begin(chipSelect, 11, 12, 13)) {
	    Serial1.println("SD card not found. Default configuration will be used");
	  }

	  // open the file. note that only one file can be open at a time,
	  // so you have to close this one before opening another.

	  // check if the file exists. Create default settings if not
	  bool cfgFileExists = SD.exists("config.txt");

	  cfgFile = SD.open("config.txt", FILE_WRITE);

	  if (cfgFile)
	  {
		  if(!cfgFileExists)
		  {
			  Serial1.println("config.txt: creating default configuration");
			  for(int i=0; i<8; i++)
			  {
				  char buf[256];
				  sprintf(buf, "CH%d %d %d A:%d", i+1, 20, 22, i+1);
				  cfgFile.println(buf);
			  }

			// close the file:
			cfgFile.close();
			Serial1.println("done.");
		  }
	  } else {
	    // if the file didn't open, print an error:
	    Serial1.println("error opening config.txt for writing");
	  }


	  // re-open the file for reading:
	  cfgFile = SD.open("config.txt");
	  if (cfgFile) {
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
	  } else {
	  	// if the file didn't open, print an error:
	    Serial1.println("error opening config.txt for reading");

	    // the logging shield must be off, but we want the actuators to work anyway.
	    // We need to activate all the actuators (which otherwise is done in the
	    // parsing of the config file)
	    ADCs[0].activate();
	    ADCs[1].activate();
	    ADCs[2].activate();
	    ADCs[3].activate();
	    ADCs[4].activate();
	    ADCs[5].activate();
	    ADCs[6].activate();
	    ADCs[7].activate();
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

	// activate the corresponding ADC

	ADCs[chId].activate();

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
			acts |= 1<<(aInt-1);
			Serial1.print(aInt);
			Serial1.print(" ");
			a = strtok(NULL, " ");
		}
		Serial1.println();

		mItems[chId].mActuators = acts;
	}

}

void Storage::temperatureReading(uint8_t item, float t)
{
	mItems[item].Temperature = t;
	mItems[item].mIsDirty = true;

	// actuate
	for(uint8_t actuatorId = 0; actuatorId<8; actuatorId++)
	{
		if(mItems[item].mActuators & (1 << actuatorId))
		{
			if(t <= mItems[item].mLow)
			{
				Actuators[actuatorId].activate(item);
			}
			else
			{
				if(t >= mItems[item].mHigh)
					Actuators[actuatorId].deactivate(item);
			}
		}
	}
}

void Storage::Advance()
{
	mIndex = mIndex + 1 == 8 ? 0 : mIndex + 1;
}

bool Storage::isLogDue( DateTime dt )
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
					f.println(buf);
					Serial1.println( buf );

					f.close();
				} else {
				// if the file didn't open, print an error:
					Serial1.print("error opening ");
					Serial1.print( fileName );
					Serial1.println(" for writing");
				}

			}
		}
		return true;
	}
	return false;
}
