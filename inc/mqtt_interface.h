/*
 * mqtt_interface.h
 *
 *  Created on: 28 Sep 2018
 *      Author: christo
 */

#ifndef SRC_MQTT_INTERFACE_H_
#define SRC_MQTT_INTERFACE_H_

#include "mbed.h"
#include "easy-connect.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

#include "mbed-os/rtos/Thread.h"

enum eMQTTInterface_state {
	MQTT_UNKNOWN,
	MQTT_CONNECT,
	MQTT_CONNECTED,
	MQTT_SEND,
	MQTT_IDLE,
};

class MQTT_Interface {
	static MQTT_Interface *__instance;

	MQTT_Interface(NetworkInterface* network);

	eMQTTInterface_state state;
	MQTTNetwork mMQTTNetwork;
	MQTT::Client<MQTTNetwork, Countdown> client;

	rtos::Thread work;
public:
	virtual ~MQTT_Interface();


    static void init(NetworkInterface* network);
    static void connect();
    static bool isConnected();
    static void send(uint8_t *buf, int len);

    static void messageArrived(MQTT::MessageData& md);

    static void run(MQTT_Interface *instance);

};

#endif /* SRC_MQTT_INTERFACE_H_ */
