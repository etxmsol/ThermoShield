//
//    FILE: AD5165.H
//  AUTHOR: Mikhail Soloviev
//    DATE: 04-june-2016
// VERSION: 0.1.00
// PURPOSE: SPI AD5165 library for Arduino
//     URL: 
//
// HISTORY:
// 

#ifndef _AD5165_DPOT_H
#define _AD5165_DPOT_H

#include "Arduino.h"

#define AD5165_LIB_VERSION "0.1.00"

class AD5165
{
public:
	AD5165();

	void resistance(uint8_t);

private:

	uint8_t m_value;
};

#endif
