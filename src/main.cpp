#include <stdio.h>
#include <console.h>
#include <Serial.h>
#include <string.h>

#include "utils.h"

#include "mbed.h"

#define logMessage printf

#define MQTTCLIENT_QOS2 1

#include "mqtt_interface.h"
#include "DHT22.h"
#include "picojson.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

#include "TCPSocket.h"
#include "ESP8266Interface.h"
#include "EEP24xx16.h"

#include "nvm_config.h"


ESP8266Interface wifi(D8, D2);
TCPSocket socket;

EEP24xx16 *eeprom = 0;


/* Console */
Serial pc(SERIAL_TX, SERIAL_RX, 115200);
const char *SWdatetime  =__DATE__ " " __TIME__;


NetworkInterface* network = 0;
bool networkAvailable = false;
int arrivedcount = 0;

//MQTTNetwork *mqttNetwork = 0;

MQTT::Client<MQTTNetwork, Countdown> *client = 0;
float version = 0.6;
const char* topic = "mbed";

DHT22 *dht22 = 0;

void scanWiFi(int argc,char *argv[])
{
//	picojson::object v;
//	picojson::object inner;
//	string val = "tt";
//
//	v["aa"] = picojson::value(val);
//	v["bb"] = picojson::value(1.66);
//	inner["test"] =  picojson::value(true);
//	inner["integer"] =  picojson::value(1.0);
//	v["inner"] =  picojson::value(inner);
//
//	string str = picojson::value(v).serialize();
//	printf("serialized content = %s\r\n" ,  str.c_str());
}

void wifiConnect(int argc,char *argv[])
{

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
	printf("temperature: %d'c\n", temp);
	printf("humidity   : %d\n", humid);
}

const Console::cmd_list_t mainCommands[] =
{
      {"MAIN"    ,0,0,0},
      {"ws",            "",                   "Scan for WiFi Devices", scanWiFi},
	  {"mc",            "",                   "MQTT Connect", wifiConnect},
	  {"ss",            "",                   "Sample Sensors", sensorSample},
      {0,0,0,0}
};

const Console::cmd_list_t configureCommands[] =
{
        {"Configure"    ,0,0,0},
        {"wid",       "",  "Set the WiFi SSID", NvmConfig::wifiSSIDConfig},
        {"wpw",       "",  "Set the WiFi Password", NvmConfig::wifiPasswordConfig},
        {"mip",       "",  "Set the MQTT IP", NvmConfig::mqttIPConfig},
        {"mp",        "",  "Set the MQTT Port", NvmConfig::mqttPortConfig},
        {0,0,0,0}
};

Console::cmd_list_t *Console::mCmdTable[] =
{
        (cmd_list_t*)shellCommands,
		(cmd_list_t*)mainCommands,
		(cmd_list_t*)configureCommands,
        0
};

void printWifiInfo()
{
    printInfo("MAC");
    printf("%s\n", wifi.get_mac_address());
    printInfo("IP");
    printf("%s\n", wifi.get_ip_address());
    printInfo("Netmask");
    printf("%s\n", wifi.get_netmask());
    printInfo("Gateway");
    printf("%s\n", wifi.get_gateway());
    printInfo("RSSI");
    printf("%d\n", wifi.get_rssi());
}

    static picojson::value v;
    static char msg[128];
    const char *msgP = (const char *)msg;

    void messageIn(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("\n");
    printInfo(GREEN("MQTT MSG IN"));
    printf("qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printInfo("MSG PAYLOAD");
    printf("%.*s\n", message.payloadlen, (char*)message.payload);

//    memcpy(msg, message.payload, message.payloadlen);
//    msg[message.payloadlen] = '\0';
//
//
//    string err = picojson::parse(v, msgP, msgP + strlen(msgP));

//    void *result = err;
//
//    if(result)
//        printf("res error? %s\r\n", err.c_str());
//
//    result =

//    printf("year =%s\r\n" ,  v.get("humid").get<string>().c_str());
//    printf("month =%s\r\n" ,  v.get("month").get<string>().c_str());
//    printf("day =%s\r\n" ,  v.get("day").get<string>().c_str());
//    printf("hour =%s\r\n" ,  v.get("hour").get<string>().c_str());
//    printf("minute =%s\r\n" ,  v.get("minute").get<string>().c_str());

}

int main()
{
	printf("\n\nWiTry Demo\n");

	printInfo("Version");
	printf("0x%08X\n", MBED_CONF_APP_VERSION);

	printInfo("Build");
	printf("%s\n\n", SWdatetime);

	dht22 = new DHT22(A0);

	Console::init(&pc, "wi_try");
	wait(1);

	printf(CYAN_B("\nWiFi example\n\n"));

	printInfo("EEPROM");
	eeprom = new EEP24xx16(D14, D15);
	if(!eeprom->getMemorySize())
	    printf(RED("FAIL\n"));
	else
	{
        printf(GREEN("OK\n"));
        NvmConfig::init(eeprom);
	}

	static sNetworkCredentials_t wifiCredentials;
	NvmConfig::getWifiCredentials(&wifiCredentials);

	printInfo("WIFI");
	printf(GREEN("Connecting...\n"));
	int ret = wifi.connect(wifiCredentials.wifi_ssid, wifiCredentials.wifi_pw, NSAPI_SECURITY_WPA_WPA2);

	printInfo("WIFI Connection");
	if (ret != 0) {
	    printf(RED("FAIL\n"));
	    return -1;
	}
	printf(GREEN("OK\n"));

	printWifiInfo();

	MQTTNetwork mqttNetwork(&wifi);
	MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);

	printInfo("MQTT CONNECT");
	printf("%s:%d\n", wifiCredentials.mqtt_ip, wifiCredentials.mqtt_port);

	int rc = mqttNetwork.connect(wifiCredentials.mqtt_ip, wifiCredentials.mqtt_port);
	printInfo("MQTT CONNECT");
	if(!rc)
	{
	    printf(GREEN("OK\n"));
	}
	else
	{
	    printf(RED("FAIL\n"));
	}

	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
	data.MQTTVersion = 3;
	data.clientID.cstring = (char*)"mbed-sample";
	//      data.username.cstring = "testuser";
	//      data.password.cstring = "testpassword";
	if ((rc = client.connect(data)) != 0)
	    printf("rc from MQTT connect is %d\r\n", rc);

	const char* topic = "mbed";
	printInfo("MQTT Sub Topic");
	printf("%s : ", topic);
	if ((rc = client.subscribe(topic, MQTT::QOS2, messageIn)) != 0)
	    printf(RED("FAIL\n"));
	else
	    printf(GREEN("OK\n"));


//	MQTT_Interface::init(&mqttNetwork, &client);
//	MQTT_Interface::connect();

    MQTT::Message message;

    while (1)
	{
        static int inCount = 1200;

        inCount++;
        wait(0.5);
    	client.yield(3000);

    	if(inCount > 1200)
    	{
    	    inCount = 0;
    	    if(MQTT_Interface::isConnected())
    	    {
    	        uint16_t temp = 0;
    	        uint16_t humid = 0;
    	        sample_dht22(temp, humid);
    	        printInfo("Temperature");
    	        printf("%d\n", temp);
    	        printInfo("Humidity");
    	        printf("%d\n", humid);
    	        picojson::object v;
    	        picojson::object inner;
    	        v["temp"] = picojson::value((double)(temp));
    	        v["humid"] = picojson::value((double)humid);

    	        string str = picojson::value(v).serialize();
    	        printInfo("JSON out");
    	        printf("%s\n" ,  str.c_str());


    	        message.qos = MQTT::QOS0;
    	        message.retained = false;
    	        message.dup = false;
    	        message.payload = (void*)str.c_str();
    	        message.payloadlen = strlen(str.c_str());

    	        printInfo("MQTT Publish");
    	        if(client.publish(topic, message))
    	            printf(RED("FAIL\n"));
    	        else
    	            printf(GREEN("OK\n"));
//    	        MQTT_Interface::send((uint8_t *)str.c_str(), strlen(str.c_str()));
    	    }
    	}
	}

	return 1;
}
