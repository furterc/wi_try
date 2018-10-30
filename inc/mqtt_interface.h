/*
 * mqtt_interface.h
 *
 *  Created on: 28 Sep 2018
 *      Author: christo
 */

#ifndef SRC_MQTT_INTERFACE_H_
#define SRC_MQTT_INTERFACE_H_

#include "mbed.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTT/MQTTClient.h"

#include "mbed-os/rtos/Thread.h"

enum eMQTTInterface_state {
	MQTT_UNKNOWN,
	MQTT_CONNECT,
	MQTT_CONNECTED,
	MQTT_FAIL,
	MQTT_IDLE,
};

class MQTT_Interface {
	static MQTT_Interface *__instance;

	MQTT_Interface(MQTTNetwork* network, MQTT::Client<MQTTNetwork, Countdown> *client);

	eMQTTInterface_state state;

	MQTTNetwork *mMQTTNetwork;
	MQTT::Client<MQTTNetwork, Countdown> *mClient;

	char *mHostname;
	uint16_t mPort;

	void (*connectedCallback)(void);

	rtos::Thread work;
public:
	virtual ~MQTT_Interface();


    static void init(MQTTNetwork* network, MQTT::Client<MQTTNetwork, Countdown> *client);
    static void connect(char *ipAddr, uint16_t port);
    static bool isConnected();
    static void setConnectedCallback(void (*callback)());

    static int publish(const char *topic, uint8_t *buf, int len, bool debug = false);
    static int subscribe(const char *topic, void (*messageHandler)(MQTT::MessageData &data));

    static void run(MQTT_Interface *instance);

};

#endif /* SRC_MQTT_INTERFACE_H_ */
