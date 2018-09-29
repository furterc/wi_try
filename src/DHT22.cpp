/*
 * DHT22.cpp
 *
 *  Created on: 29 Sep 2018
 *      Author: christo
 */

#include <DHT22.h>

DHT22::DHT22(PinName pinName) : mPinName(pinName)
{
	mTemperature = 0;
	mHumidity = 0;
}

DHT22::~DHT22() {
	// TODO Auto-generated destructor stub
}

bool DHT22::sample()
{
	DigitalInOut DHT22(mPinName);
	uint8_t dht22_dat[5];
	DHT22.output();
	DHT22.write(0);
	wait_ms(18);
	DHT22.write(1);
	DHT22.input();
	wait_us(40);
	wait_us(80);
	uint8_t result=0;
	for (int i=0; i<5; i++) {
		result=0;
		for (int j=0; j<8; j++) {
			while (DHT22);
			while (!DHT22);
			wait_us(50);
			uint8_t p;
			p=DHT22;
			p=p <<(7-j);
			result=result|p;
		}
		dht22_dat[i] = result;
	}
	int dht22_check_sum;
	dht22_check_sum=dht22_dat[0]+dht22_dat[1]+dht22_dat[2]+dht22_dat[3];
	dht22_check_sum= dht22_check_sum%256;
	if (dht22_check_sum==dht22_dat[4])
	{
		mHumidity = dht22_dat[0] << 8;
		mHumidity |= dht22_dat[1];
		mTemperature = dht22_dat[2] << 8;
		mTemperature |= dht22_dat[3];
		return true;
	}
	return false;
}

void DHT22::getValues(uint16_t &temperature, uint16_t &humidity)
{
	temperature = mTemperature;
	humidity = mHumidity;
}
