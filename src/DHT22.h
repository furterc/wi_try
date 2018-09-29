/*
 * DHT22.h
 *
 *  Created on: 29 Sep 2018
 *      Author: christo
 */

#ifndef SRC_DHT22_H_
#define SRC_DHT22_H_

#include "mbed.h"

class DHT22 {
	PinName mPinName;
	uint16_t mTemperature;
	uint16_t mHumidity;
public:
	DHT22(PinName);
	virtual ~DHT22();

	bool sample();
	void getValues(uint16_t &, uint16_t &);
};

#endif /* SRC_DHT22_H_ */
