#include <stdio.h>
#include <console.h>
#include <Serial.h>

#include "utils.h"

#include "mbed.h"

#define logMessage printf

#define MQTTCLIENT_QOS2 1

#include "easy-connect.h"

#include "mqtt_interface.h"
#include "DHT22.h"

/* Console */
Serial pc(SERIAL_TX, SERIAL_RX, 115200);
const char *SWdatetime  =__DATE__ " " __TIME__;


NetworkInterface* network = 0;
bool networkAvailable = false;
int arrivedcount = 0;

MQTTNetwork *mqttNetwork = 0;

MQTT::Client<MQTTNetwork, Countdown> *client = 0;
float version = 0.6;
const char* topic = "mbed";

DHT22 *dht22 = 0;


void messageArrived(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    logMessage("Message arrived: qos %d, retained %d, dup %d, packetid %d\r\n", message.qos, message.retained, message.dup, message.id);
    logMessage("Payload %.*s\r\n", message.payloadlen, (char*)message.payload);
    ++arrivedcount;
}

void scanWiFi(int argc,char *argv[])
{
//	MQTT_Interface::send();
}

void wifiConnect(int argc,char *argv[])
{
	MQTT_Interface::connect();
}

HAL_StatusTypeDef sample_dht22(uint16_t &temp, uint16_t &humid)
{
	if(dht22->sample())
	{
		dht22->getValues(temp, humid);
		return HAL_OK;
	}

	printf("sample dht22 fail\n");
	return HAL_ERROR;
}

void sensorSample(int argc,char *argv[])
{
	uint16_t temp = 0;
	uint16_t humid = 0;
	sample_dht22(temp, humid);
	printf("temperature: %d\n", temp);
	printf("humidity: %d\n", humid);

}

const Console::cmd_list_t mainCommands[] =
{
      {"MAIN"    ,0,0,0},
      {"ws",     "",                   "Scan for WiFi Devices", scanWiFi},
	  {"mc",     "",                   "MQTT Connect", wifiConnect},
	  {"ss",     "",                   "Sample Sensors", sensorSample},
      {0,0,0,0}
};

Console::cmd_list_t *Console::mCmdTable[] =
{
        (cmd_list_t*)shellCommands,
		(cmd_list_t*)mainCommands,
        0
};


int main()
{
	printf("\n\nWiTry Demo\n");
	printf("Version: 0x%08X\n", MBED_CONF_APP_VERSION);
	printf("Build  : %s\n\n", SWdatetime);

	dht22 = new DHT22(A0);

	Console::init(&pc, "wi_try");

	network = easy_connect(true);
	if (!network) {
		printf(RED("Network Not Connected\n"));
	}
	else
	{
		MQTT_Interface::init(network);
		MQTT_Interface::connect();
		printf(GREEN("Network Connected\n"));
	}

    while (1)
	{
    	wait(10);
    	uint16_t temp = 0;
    	uint16_t humid = 0;
    	sample_dht22(temp, humid);
    	printf("temperature: %d\n", temp);
    	printf("humidity: %d\n", humid);
    	if(MQTT_Interface::isConnected())
    	{
    		uint8_t buf[4];
    		buf[0] |= (temp >> 8) & 0xFF;
    		buf[1] |= temp & 0xFF;
    		buf[2] |= (humid >> 8) & 0xFF;
    		buf[3] |= humid & 0xFF;
//    		printf("temp: %")
    		MQTT_Interface::send(buf, 4);
    	}
	}

	return 1;
}
