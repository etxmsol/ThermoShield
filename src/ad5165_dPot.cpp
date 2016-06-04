//
//    FILE: ad5165_dPot.cpp
//  AUTHOR: Mikhail Soloviev
//    DATE: 04-june-2016
// VERSION: 0.1.00
// PURPOSE: SPI AD5165 library for Arduino
//     URL: 
//
// HISTORY:
// 

#include "ad5165_dPot.h"

#include <SPI.h>

const int CS = 11;

AD5165::AD5165()
{
	// set the CS as an output:
	pinMode (CS, OUTPUT);
	pinMode(22, INPUT);

	// initialize SPI:
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV32);
}

void AD5165::resistance(uint8_t value)
{
	if(value != m_value)
	{
		m_value = value;

		digitalWrite(CS, HIGH);

		SPI.transfer(value);

		digitalWrite(CS, LOW);
	}
}

