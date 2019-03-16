/*
 * mqtt_interface.cpp
 *
 *  Created on: 28 Sep 2018
 *      Author: christo
 */

#include "mqtt_interface.h"
#include "caboodle/utils.h"


#define TRACE(_x, ...) INFO_TRACE("MQTT", _x, ##__VA_ARGS__)

MQTT_Interface *MQTT_Interface::__instance = 0;

MQTT_Interface::MQTT_Interface(MQTTNetwork* network, MQTT::Client<MQTTNetwork, Countdown> *client) : mMQTTNetwork(network), mClient(client), work(osPriorityLow, OS_STACK_SIZE, NULL, "mqttInterface")
{
    mHostname = 0;
    mPort = 0;

    connectedCallback = 0;

    work.start(callback(run, this));
    state = MQTT_UNKNOWN;
}

MQTT_Interface::~MQTT_Interface()
{
    if(__instance->mHostname != 0)
        free(__instance->mHostname);
    __instance->mHostname = 0;
}

void MQTT_Interface::init(MQTTNetwork* network, MQTT::Client<MQTTNetwork, Countdown> *client)
{
    if(!__instance)
        __instance = new MQTT_Interface(network, client);
}

void MQTT_Interface::connect(char *ipAddr, uint16_t port)
{
	if(__instance->state == MQTT_CONNECTED)
	{
	    TRACE(YELLOW("Allready connected\n"));
		return;
	}

	__instance->state = MQTT_CONNECT;
	if(__instance->mHostname != 0)
	    free(__instance->mHostname);

	int len = strlen(ipAddr);
	__instance->mHostname = (char*)malloc(len+1);
	memset(__instance->mHostname, 0x00, len+1);
	memcpy(__instance->mHostname, ipAddr, len);
	__instance->mPort = port;
}


bool MQTT_Interface::isConnected()
{
    if(__instance->state == MQTT_CONNECTED)
        return true;

    return false;
}

void MQTT_Interface::setConnectedCallback(void (*callback)(void))
{
    __instance->connectedCallback = callback;
}

int MQTT_Interface::publish(const char *topic, uint8_t *buf, int len, bool debug)
{
    if(__instance->state < MQTT_CONNECTED)
        return -1;

    if(!__instance->mClient->isConnected())
    {
        TRACE(RED("Publish Fail\n"));
        __instance->state = MQTT_FAIL;
        return -1;
    }

	MQTT::Message message;

	message.qos = MQTT::QOS0;
	message.retained = false;
	message.dup = false;
	message.payload = (void*)buf;
	message.payloadlen = len;

	int rc = __instance->mClient->publish(topic, message);

	if(debug)
	{
	    TRACE("Publish @ %s : ", topic);
	    if(rc)
	    {
	        printf(RED("FAIL\n"));
	        __instance->state = MQTT_FAIL;
	        return rc;
	    }else
	        printf(GREEN("OK\n"));

	    diag_dump_buf(message.payload, message.payloadlen);
	}
	return rc;
}

int MQTT_Interface::subscribe(const char *topic, void (*messageHandler)(MQTT::MessageData &data))
{
    if(__instance->state < MQTT_CONNECTED)
        return -1;

    TRACE("Subscribe : %s : ", topic);

    int rc = __instance->mClient->subscribe(topic, MQTT::QOS2, messageHandler);
    if (rc != 0)
    {
        printf(RED("FAIL\n"));
        __instance->state = MQTT_FAIL;
    }
    else
        printf(GREEN("OK\n"));

    return rc;
}

void MQTT_Interface::run(MQTT_Interface *instance)
{
	printf("mqttInterface: 0x%X\n", (int)Thread::gettid());
	while(__instance)
	{
		wait(1);
		switch(__instance->state)
		{
		case MQTT_UNKNOWN:
		{
		}break;
		case MQTT_FAIL:
		{
		    TRACE("Connect Fail\n");
		    __instance->state = MQTT_CONNECT;
		    wait(5);
		}break;
		case MQTT_CONNECT:
		{
		    TRACE("Network: %s:%d\n", __instance->mHostname, __instance->mPort);

			int rc = __instance->mMQTTNetwork->connect(__instance->mHostname, __instance->mPort);
		    if(!rc)
		    {
		        TRACE(GREEN("CONNECT OK\n"));

		        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
		        data.MQTTVersion = 3;
		        data.clientID.cstring = (char*)"mbed-sample";
		        //      data.username.cstring = "testuser";
		        //      data.password.cstring = "testpassword";

		        TRACE("CLIENT : %s\n", data.clientID.cstring);

		        rc = __instance->mClient->connect(data);
		        if(rc)
		        {
		            TRACE(RED("Client Connect FAIL\n"));
		            __instance->state = MQTT_FAIL;
		            break;
		        }

		        wait(1);
		        TRACE(GREEN("Client Connect OK\n"));
		        __instance->state = MQTT_CONNECTED;

		        if(__instance->connectedCallback != 0)
		            __instance->connectedCallback();
		    }
		    else
		    {
		        TRACE(RED("CONNECT FAIL : %d\n"), (int)rc);
		        __instance->state = MQTT_FAIL;
		    }
		}break;
		case MQTT_CONNECTED:
		{
            __instance->mClient->yield(30000);
		}break;
		}
	}
}
