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
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

#include "TCPSocket.h"
#include "ESP8266Interface.h"
#include "EEP24xx16.h"

#include "nvm_config.h"

#include "jsmn.h"
#include "rtc_alarm.h"


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

MQTT::Client<MQTTNetwork, Countdown> *mClient = 0;
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

static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
            strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

void parseRequestJsonObj(jsmntok_t *tokens, int tokenCount, char *msg)
{
    if (jsoneq(msg, &tokens[2], "thresholds") == 0)
    {
        sThresholds_t thresholds;
        NvmConfig::getThresholds(&thresholds);

        char buffer[64];
        memset(buffer, 0, 64);
        snprintf(buffer, 64,
                "{"
                "\"tempLow\":%d,"
                "\"tempHigh\":%d,"
                "\"humidLow\":%d,"
                "\"humidHigh\":%d"
                "}", thresholds.tempLow,
                thresholds.tempHigh,
                thresholds.humidLow,
                thresholds.humidHigh);

        printInfo("JSON out");
        printf("%s\n" ,  buffer);

        MQTT::Message message;
        message.qos = MQTT::QOS0;
        message.retained = false;
        message.dup = false;
        message.payload = buffer;
        message.payloadlen = strlen(buffer);

        printInfo("MQTT Publish");
        if(mClient->publish("mbed", message))
            printf(RED("FAIL\n"));
        else
            printf(GREEN("OK\n"));
    }
}


void parseTempThreshJsonObj(jsmntok_t *tokens, int tokenCount, char *msg)
{
    sThresholds_t thresholds;
    NvmConfig::getThresholds(&thresholds);

    char value[16];
    memset(value, 0, 16);

    /* Loop over all keys of the root object */
    for (int i = 1; i < tokenCount; i++)
    {
        if (jsoneq(msg, &tokens[i], "tempLow") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            thresholds.tempLow = atoi(value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "tempHigh") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            thresholds.tempHigh = atoi(value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "humidLow") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            thresholds.humidLow = atoi(value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "humidHigh") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            thresholds.humidHigh = atoi(value);
        }
    }
    NvmConfig::setThresholds(&thresholds);
}

void parseTimeJsonObj(jsmntok_t *tokens, int tokenCount, char *msg)
{
    struct tm t;

    char value[16];
    memset(value, 0, 16);

    /* Loop over all keys of the root object */
    for (int i = 1; i < tokenCount; i++)
    {
        if (jsoneq(msg, &tokens[i], "year") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            t.tm_year = atoi(value) - 1900;
            printf("year  %s\n", value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "month") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            t.tm_mon = atoi(value) - 1;
            printf("month %s\n", value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "day") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            t.tm_mday = atoi(value);
            printf("day %s\n", value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "hour") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            t.tm_hour = atoi(value);
            printf("hour %s\n", value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "minute") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            t.tm_min = atoi(value);
            printf("minute %s\n", value);

            i++;
        } else if (jsoneq(msg, &tokens[i], "second") == 0)
        {
            int len = tokens[i+1].end-tokens[i+1].start;
            memcpy(value, msg + tokens[i+1].start, len);
            value[len] = 0;

            t.tm_sec = atoi(value);
            printf("minute %s\n", value);

            i++;
        }
    }

    set_time(mktime(&t));


}

void messageIn(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("\n");
//    printInfo(GREEN("MQTT MSG IN"));
//    printf("qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    printInfo("MSG PAYLOAD");
    printf("%.*s\n", message.payloadlen, (char*)message.payload);

    jsmn_parser parser;
    jsmntok_t tokens[16];

    jsmn_init(&parser);

    char msg[128];
    memcpy(msg, message.payload, message.payloadlen);
    msg[message.payloadlen] = '\0';


    int r = jsmn_parse(&parser, msg, strlen(msg), tokens, 16);
    printf("jsmn_parse: %d\n", r);

    /* Assume the top-level element is an object */
    if (r < 1 || tokens[0].type != JSMN_OBJECT)
    {
        printf("Object expected\n");
        return;
    }

    /* Loop over all keys of the root object */
//    for (int i = 1; i < r; i++)
//    {
    if (jsoneq(msg, &tokens[1], "year") == 0)
        parseTimeJsonObj(tokens, r, msg);

    if (jsoneq(msg, &tokens[1], "tempLow") == 0)
        parseTempThreshJsonObj(tokens, r, msg);

    if (jsoneq(msg, &tokens[1], "request") == 0)
    {
        printf("request obj\n");
        parseRequestJsonObj(tokens, r, msg);
    }

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
//
//	static sNetworkCredentials_t wifiCredentials;
//	NvmConfig::getWifiCredentials(&wifiCredentials);
//
//	printInfo("WIFI");
//	printf(GREEN("Connecting...\n"));
//	int ret = wifi.connect(wifiCredentials.wifi_ssid, wifiCredentials.wifi_pw, NSAPI_SECURITY_WPA_WPA2);
//
//	printInfo("WIFI Connection");
//	if (ret != 0) {
//	    printf(RED("FAIL\n"));
//	    return -1;
//	}
//	printf(GREEN("OK\n"));
//
//	printWifiInfo();
//
//	MQTTNetwork mqttNetwork(&wifi);
//	MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);
//	mClient = &client;
//	printInfo("MQTT CONNECT");
//	printf("%s:%d\n", wifiCredentials.mqtt_ip, wifiCredentials.mqtt_port);
//
//	int rc = mqttNetwork.connect(wifiCredentials.mqtt_ip, wifiCredentials.mqtt_port);
//	printInfo("MQTT CONNECT");
//	if(!rc)
//	{
//	    printf(GREEN("OK\n"));
//	}
//	else
//	{
//	    printf(RED("FAIL\n"));
//	}
//
//	MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
//	data.MQTTVersion = 3;
//	data.clientID.cstring = (char*)"mbed-sample";
//	//      data.username.cstring = "testuser";
//	//      data.password.cstring = "testpassword";
//	if ((rc = client.connect(data)) != 0)
//	    printf("rc from MQTT connect is %d\r\n", rc);
//
//	const char* topic = "mbed";
//	printInfo("MQTT Sub Topic");
//	printf("%s : ", topic);
//	if ((rc = client.subscribe(topic, MQTT::QOS2, messageIn)) != 0)
//	    printf(RED("FAIL\n"));
//	else
//	    printf(GREEN("OK\n"));


//	MQTT_Interface::init(&mqttNetwork, &client);
//	MQTT_Interface::connect();

//    MQTT::Message message;

    sRTCAlarmObj_t alarmOn;
    alarmOn.hour = 0;
    alarmOn.minute = 1;

    sRTCAlarmObj_t alarmOff;
    alarmOff.hour = 0;
    alarmOff.minute = 3;

    RTC_Alarm testAlarm;

    testAlarm.setAlarm(&alarmOn, &alarmOff);

    while (1)
	{
        testAlarm.checkAlarm();
        static int inCount = 1200;

        inCount++;
        wait(0.5);
//    	client.yield(3000);
//
//    	if(inCount > 60)
//    	{
//    	    inCount = 0;
//    	    if(MQTT_Interface::isConnected())
//    	    {
//    	        uint16_t temp = 0;
//    	        uint16_t humid = 0;
//    	        sample_dht22(temp, humid);
//    	        printInfo("Temperature");
//    	        printf("%d\n", temp);
//    	        printInfo("Humidity");
//    	        printf("%d\n", humid);
//
//
//    	        char buffer[64];
//    	        memset(buffer, 0, 64);
//    	        snprintf(buffer, 64,
//    	                "{"
//    	                "\"temp\":%d,"
//    	                "\"humid\":%d"
//    	                "}", temp, humid);
//
//    	        printInfo("JSON out");
//    	        printf("%s\n" ,  buffer);
//
//
//    	        message.qos = MQTT::QOS0;
//    	        message.retained = false;
//    	        message.dup = false;
//    	        message.payload = buffer;
//    	        message.payloadlen = strlen(buffer);
//
//    	        printInfo("MQTT Publish");
//    	        if(client.publish(topic, message))
//    	            printf(RED("FAIL\n"));
//    	        else
//    	            printf(GREEN("OK\n"));
////    	        MQTT_Interface::send((uint8_t *).c_str(), strlen(str.c_str()));
//    	    }
//    	}
	}

	return 1;
}
