/*
 * esp_wifi.h
 *
 *  Created on: 22 Jun 2018
 *      Author: christo
 */

#ifndef INC_ESP_WIFI_H_
#define INC_ESP_WIFI_H_

#include "mbed.h"
#include "ESP8266Interface.h"
#include "caboodle/utils.h"

class ESPWiFi {
	bool mConnected;
	WiFiInterface *mWiFi;

	const char *sec2str(nsapi_security_t sec);
public:
	ESPWiFi(WiFiInterface *net);
	virtual ~ESPWiFi();

	int connect();
	void scanDevices(WiFiInterface *wifi);
};

#endif /* INC_ESP_WIFI_H_ */
