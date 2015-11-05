/*
 * Actuator.h
 *
 *  Created on: 22 okt 2015
 *      Author: Mikhail
 */

#ifndef ACTUATOR_H_
#define ACTUATOR_H_

#include <Arduino.h>

 class Actuator
 {
 public:
	 Actuator() : mId(0), mChannels(0) {};
	 Actuator(uint8_t id);
	 virtual ~Actuator(){};

	 void activate(uint8_t adcChannel);		// adcChannel is a bit number CH
	 void deactivate(uint8_t adcChannel);

 private:
	 uint8_t mId;			// 0..7 (pins A8...A15)
	 uint8_t mChannels;	//
 };


#endif /* ACTUATOR_H_ */
