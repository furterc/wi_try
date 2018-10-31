/*
 * mqtt_interface.cpp
 *
 *  Created on: 28 Sep 2018
 *      Author: christo
 */

#include "mqtt_interface.h"
#include "caboodle/utils.h"


MQTT_Interface *MQTT_Interface::__instance = 0;

MQTT_Interface::MQTT_Interface(MQTTNetwork* network, MQTT::Client<MQTTNetwork, Countdown> *client) : mMQTTNetwork(network), mClient(client), work(osPriorityLow, OS_STACK_SIZE, NULL, "mqttInterface")
{
    mHostname = 0;
    mPort = 0;

    connectedCallback = 0;

    work.start(callback(run, this));
    state = MQTT_IDLE;
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
	if(__instance->state != MQTT_IDLE)
	{
		printf(YELLOW("MQTT Busy\n"));
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
	return __instance->mClient->isConnected();
}

void MQTT_Interface::setConnectedCallback(void (*callback)(void))
{
    __instance->connectedCallback = callback;
}

int MQTT_Interface::publish(const char *topic, uint8_t *buf, int len, bool debug)
{
    if(!__instance->mClient->isConnected())
    {
        printInfo("MQTT Publish: ");
        printf(RED("FAIL\n"));
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
	    printInfo("MQTT Publish: ");
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
      printInfo("MQTT Sub Topic");
      printf("%s : ", topic);

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
			__instance->state = MQTT_IDLE;
		}break;
		case MQTT_CONNECT:
		{
			printInfo("NETWORK");
			printf("%s:%d\n", __instance->mHostname, __instance->mPort);

		    printInfo("NETWORK");
			int rc = __instance->mMQTTNetwork->connect(__instance->mHostname, __instance->mPort);
		    if(!rc)
		    {
		        printf(GREEN("CONNECT OK\n"));

		        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
		        data.MQTTVersion = 3;
		        data.clientID.cstring = (char*)"mbed-sample";
		        //      data.username.cstring = "testuser";
		        //      data.password.cstring = "testpassword";

		        printInfo("MQTT CLIENT");
		        printf("%s\n", data.clientID.cstring);

		        printInfo("MQTT CLIENT");
		        rc = __instance->mClient->connect(data);
		        if(rc)
		        {
		            printf(RED("CONNECT FAIL : %d\n"), (int)rc);
		            __instance->state = MQTT_FAIL;
		            break;
		        }
		        else
		        {
		            printf(GREEN("CONNECT OK\n"));
		            if(__instance->connectedCallback != 0)
		                __instance->connectedCallback();
		        }

		        __instance->state = MQTT_IDLE;
		    }
		    else
		    {
		        printf(RED("CONNECT FAIL : %d\n"), (int)rc);
		        __instance->state = MQTT_FAIL;
		    }
		}break;
		case MQTT_CONNECTED:
		{

		}break;
		case MQTT_FAIL:
		{
		    printInfo("MQTT");
		    printf(RED("Failed to connect\n"));
		    wait(5);
		    __instance->state = MQTT_CONNECT;
		}break;
		case MQTT_IDLE:
		{
			__instance->mClient->yield(30000);
//			if(!__instance->mClient->isConnected())
//			    __instance->state = MQTT_CONNECT;

		}break;
		default:
			break;
		}
//		__instance->handleByte(__instance->mSerial->getc());
	}
}
