/*******************************************************************************
 ******************************* Copyright 2015 ********************************
 *******************************************************************************
 *
 * Actuator class
 *
 * Created on: 		2015-11-06
 * Modified on:
 * Author:			Mikhail Soloviev
 *
 *******************************************************************************
 */

#ifndef ACTUATOR_H_
#define ACTUATOR_H_

#include <Arduino.h>

 /*! @brief Implements an actuator.
 *
 * This class implements control of the relay that in its turn controls the pump. Any of the
 * temperature measurement channels can activate the actuator. The actuator is released when the
 * last of the channels releases it.
 */
class Actuator
 {
 public:
	 Actuator() : mId(0), mChannels(0) {};
	 Actuator(uint8_t id);
	 virtual ~Actuator(){};

	 /*!
	  * @brief Activates the actuator (the corresponding pump)
	  *
	  * Sets the digital output low. It shorts the relay primary coil to ground
	  * causing it to trigger (the other terminal is connected to Vdd).
	  *
	  * It sets the bit of mChannels corresponding to the requesting ADC channel in
	  * order to keep track of when all the channels release the actuator.
	  *
	  * @param[in] adcChannel Which channel triggers the actuator
	  */
	 void activate(uint8_t adcChannel);

	 /*!
	  * @brief Deactivates the actuator (the corresponding pump) when the last ADC channel releases
	  *
	  * It resets the bit of mChannels corresponding to the requesting ADC channel.
	  *
	  * When the last bit is reset (mChannels == 0) sets the digital output high. It sets Vdd to both
	  * terminals of the relay primary coil causing it to open (the other terminal is connected to Vdd).
	  *
	  * @param[in] adcChannel Which channel releases the actuator
	  */
	 void deactivate(uint8_t adcChannel);

	 /*!
	  * @brief Deactivates the actuator (the corresponding pump) when the last ADC channel releases
	  *
	  * It resets the bit of mChannels corresponding to the requesting ADC channel.
	  *
	  * When the last bit is reset (mChannels == 0) sets the digital output high. It sets Vdd to both
	  * terminals of the relay primary coil causing it to open (the other terminal is connected to Vdd).
	  *
	  * @param[in] adcChannel Which channel releases the actuator
	  */
	 bool isOn() { return mChannels != 0; }

 private:

	 uint8_t mId;			//!< The id of the actual digital output. 0..7 (pins A8...A15)

	 /*!
	  * @brief The bit mask of the ADC channels controlling this actuator
	  *
	  * Depending on the configuration (config.txt), the same actuator may be driven by more than one
	  * ADC channel. This is for example the case when the same contour runs through two neighboring
	  * rooms, each having its own temperature sensor (ADC). In this case any of the rooms may activate
	  * the actuator. It is the OR rule that applies.
	  *
	  * Another example is a very-cold channel. For example at -40C outside, all the actuators may
	  * be activated regardless of the current room temperature.
	  *
	  * This is a bit mask. The corresponding ADC bit is set by activate() and reset by deactivate()
	  */
	 uint8_t mChannels;
 };


#endif /* ACTUATOR_H_ */
