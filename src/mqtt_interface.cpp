/*
 * mqtt_interface.cpp
 *
 *  Created on: 28 Sep 2018
 *      Author: christo
 */

#include "mqtt_interface.h"
#include "caboodle/utils.h"


MQTT_Interface *MQTT_Interface::__instance = 0;



MQTT_Interface::MQTT_Interface(NetworkInterface* network) : mMQTTNetwork(network), client(mMQTTNetwork), work(osPriorityLow, OS_STACK_SIZE, NULL, "mqttInterface")
{
    work.start(callback(run, this));
    state = MQTT_IDLE;
}

MQTT_Interface::~MQTT_Interface()
{

}

void MQTT_Interface::init(NetworkInterface* network)
{
    if(!__instance)
        __instance = new MQTT_Interface(network);
}

void MQTT_Interface::connect()
{
	if(__instance->state != MQTT_IDLE)
	{
		printf(YELLOW("MQTT Busy\n"));
		return;
	}
	__instance->state = MQTT_CONNECT;
}

bool MQTT_Interface::isConnected()
{
	return __instance->client.isConnected();

}

void MQTT_Interface::send(uint8_t *buf, int len)
{
	printf("MQTT_SEND\n");

	MQTT::Message message;

	message.qos = MQTT::QOS0;
	message.retained = false;
	message.dup = false;
	message.payload = (void*)buf;
	message.payloadlen = len;

	const char* topic = "mbed";
	int rc = __instance->client.publish(topic, message);
	//	while (arrivedcount < 1)
	//		client->yield(100);

	printf("MQTT_SEND\n");
	__instance->state = MQTT_IDLE;
//	if(__instance->state != MQTT_IDLE)
//	{
//		printf(YELLOW("MQTT Busy\n"));
//		return;
//	}
//	__instance->state = MQTT_SEND;
}

void MQTT_Interface::messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    printf("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
}

void MQTT_Interface::run(MQTT_Interface *instance)
{
	printf("mqttInterface: 0x%X\n", (int)Thread::gettid());


	while(__instance)
	{
		wait(0.1);
		switch(__instance->state)
		{
		case MQTT_UNKNOWN:
		{
			__instance->state = MQTT_IDLE;
		}break;
		case MQTT_CONNECT:
		{
			const char* hostname = "10.0.0.174";
			int port = 1883;
			printf("Connecting to %s:%d\r\n", hostname, port);
			int rc = __instance->mMQTTNetwork.connect(hostname, port);
			printf("1\n");
			if (rc != 0)
				printf("rc from TCP connect is %d\r\n", rc);

			printf("2\n");
			MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
			data.MQTTVersion = 3;
			data.clientID.cstring = (char*)"mbed-sample";
			//	    data.username.cstring = "testuser";
			//	    data.password.cstring = "testpassword";
			if ((rc = __instance->client.connect(data)) != 0)
				printf("rc from MQTT connect is %d\r\n", rc);

			printf("3\n");
			const char* topic = "mbed";
			if ((rc = __instance->client.subscribe(topic, MQTT::QOS2, messageArrived)) != 0)
				printf("rc from MQTT subscribe is %d\r\n", rc);

			__instance->state = MQTT_IDLE;
		}break;
		case MQTT_CONNECTED:
		{

		}break;
		case MQTT_SEND:
		{
			printf("MQTT_SEND\n");

			MQTT::Message message;

			const char *msg = "hi";
			message.qos = MQTT::QOS0;
			message.retained = false;
			message.dup = false;
			message.payload = (void*)msg;
			message.payloadlen = 2;

			const char* topic = "mbed";
			int rc = __instance->client.publish(topic, message);
			//	while (arrivedcount < 1)
			//		client->yield(100);

			printf("MQTT_SEND\n");
			__instance->state = MQTT_IDLE;
		}break;
		case MQTT_IDLE:
		{
			__instance->client.yield(100);
		}break;
		default:
			break;
		}
//		__instance->handleByte(__instance->mSerial->getc());
	}
}
