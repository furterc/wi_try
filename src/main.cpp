#include <stdio.h>
#include <console.h>
#include <Serial.h>
#include <string.h>

#include "utils.h"

#include "mbed.h"

#define logMessage printf

#define MQTTCLIENT_QOS2 1

#include "easy-connect.h"

#include "mqtt_interface.h"
#include "DHT22.h"
#include "picojson.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

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
	picojson::object v;
	    picojson::object inner;
	    string val = "tt";

	    v["aa"] = picojson::value(val);
	    v["bb"] = picojson::value(1.66);
	    inner["test"] =  picojson::value(true);
	    inner["integer"] =  picojson::value(1.0);
	    v["inner"] =  picojson::value(inner);

	    string str = picojson::value(v).serialize();
	    printf("serialized content = %s\r\n" ,  str.c_str());
}

void wifiConnect(int argc,char *argv[])
{
//	MQTT_Interface::connect();
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

		printf(GREEN("Network Connected\n"));
	}

	MQTTNetwork mqttNetwork(network);
	MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);
	MQTT_Interface::init(&mqttNetwork, &client);
	MQTT_Interface::connect();
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
    		picojson::object v;
    		picojson::object inner;
    		v["temp"] = picojson::value((double)(temp));
    		v["humid"] = picojson::value((double)humid);

    		string str = picojson::value(v).serialize();
    		printf("serialized content = %s\r\n" ,  str.c_str());

    		MQTT_Interface::send((uint8_t *)str.c_str(), strlen(str.c_str()));
    	}
	}

	return 1;
}
