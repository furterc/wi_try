#include <stdio.h>
#include <console.h>
#include <Serial.h>
#include <string.h>

#include "utils.h"

#include "mbed.h"

#include "linked_list.h"

#define logMessage printf

#define MQTTCLIENT_QOS2 1

#include "DHT22.h"
#include "MQTTNetwork.h"
#include "MQTTmbed.h"
#include "MQTTClient.h"

#include "ESP8266Interface.h"
#include "EEP24xx16.h"

#include "nvm_config.h"

#include "jsmn.h"
#include "rtc_alarm.h"

#include "mqtt_interface.h"

#include "LCDPCF8574.h"
#include "LCDController.h"
#include "Relay.h"

Relay rl1(A1);
Relay rl2(A2);
Relay rl3(A3);
Relay rl4(A4);

Relay *relays[4] = {&rl1, &rl2, &rl3, &rl4};

DigitalOut led(D9);
DigitalIn lightSense(D4);

ESP8266Interface wifi(D8, D2);
TCPSocket socket;

EEP24xx16 *eeprom = 0;

/* Console */
Serial pc(SERIAL_TX, SERIAL_RX, 115200);

UARTSerial rs485uart(PA_11, PA_12, 115200);
const char *SWdatetime  =__DATE__ " " __TIME__;

NetworkInterface* network = 0;
bool networkAvailable = false;
int arrivedcount = 0;

//MQTTNetwork *mqttNetwork = 0;

MQTT::Client<MQTTNetwork, Countdown> *mClient = 0;
float version = 0.6;
const char* topic = "mbed";

DHT22 *dht22 = 0;

LCDPCF8574 *lcd = 0;
LCDController *lcdController = 0;

void scanWiFi(int argc,char *argv[])
{

}

void relayDebug(int argc,char *argv[])
{
    if(argc == 1)
    {
        for(uint8_t r = 0; r < 4; r++)
            printf("relay %d = %s\n", r, (relays[r]->get() ? "ON" : "OFF"));

        return;
    }

    if(argc == 3)
    {
        int relayNr = atoi(argv[1]);
        int state = atoi(argv[2]);
        if((relayNr < 0) || (relayNr > 3))
        {
            printf(YELLOW("0 < relayNr < 4\n"));
            return;
        }

        if((state < 0) || (state > 1))
        {
            printf(YELLOW("0 < state < 2\n"));
            return;
        }

        relays[relayNr]->latch(state);
        printf(GREEN("Set relay : %d : %d\n"), relayNr, state);

        return;
    }

    printf("rl - show state\n");
    printf("rl <num> <state> - set relay\n");
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
	  {"ss",            "",                   "Sample Sensors", sensorSample},
	  {"rl",            "",                   "Relay Debug", relayDebug},
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
    INFO_TRACE("MAC", "%s\n", wifi.get_mac_address());
    INFO_TRACE("IP", "%s\n", wifi.get_ip_address());
    INFO_TRACE("Netmask", "%s\n", wifi.get_netmask());
    INFO_TRACE("Gateway", "%s\n", wifi.get_gateway());
    INFO_TRACE("RSSI", "%d\n", wifi.get_rssi());
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
    printf("parse request\n");
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


        printInfo("MQTT Publish");
        if(MQTT_Interface::publish("mbed", (uint8_t *)buffer, strlen(buffer)))
            printf(RED("FAIL\n"));
        else
            printf(GREEN("OK\n"));
    } else if (jsoneq(msg, &tokens[2], "a1") == 0) //alarm request
        {
            sAlarmTimes_t alarms;
            NvmConfig::getAlarms(&alarms);

            char buffer[64];
            memset(buffer, 0, 64);
            snprintf(buffer, 64,
                    "{"
                    "\"onHour\":%d,"
                    "\"onMinute\":%d,"
                    "\"offHour\":%d,"
                    "\"offMinute\":%d"
                    "}", alarms.lightOn.hour,
                    alarms.lightOn.minute,
                    alarms.lightOff.hour,
                    alarms.lightOff.minute);

            printInfo("JSON out");
            printf("%s\n" ,  buffer);

            printInfo("MQTT Publish");
            if(MQTT_Interface::publish("mbed", (uint8_t *)buffer, strlen(buffer)))
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

void parseEpochTimeJsonObj(jsmntok_t *tokens, int tokenCount, char *msg)
{
    if (jsoneq(msg, &tokens[1], "timestamp") == 0)
    {
        char value[32] = {0};
        int len = tokens[2].end-tokens[2].start;
        memcpy(value, msg + tokens[2].start, len);
        value[len] = 0;

        char *pEnd;
        time_t timestamp = strtoull(value, &pEnd, 10);
        timestamp += 7200;  // add 2 hours for CAT

        struct tm epoch_time;
        memcpy(&epoch_time, localtime(&timestamp), sizeof (struct tm));
        set_time(mktime(&epoch_time));
    }
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
//    printf("jsmn_parse: %d\n", r);

    /* Assume the top-level element is an object */
    if (r < 1 || tokens[0].type != JSMN_OBJECT)
    {
        printf("Object expected\n");
        return;
    }

    /* Loop over all keys of the root object */
//    for (int i = 1; i < r; i++)
//    {
    if (jsoneq(msg, &tokens[1], "timestamp") == 0)
            parseEpochTimeJsonObj(tokens, r, msg);

    if (jsoneq(msg, &tokens[1], "tempLow") == 0)
        parseTempThreshJsonObj(tokens, r, msg);

    if (jsoneq(msg, &tokens[1], "request") == 0)
    {
        printf("request obj\n");
        parseRequestJsonObj(tokens, r, msg);
    }

}

static void mqttConnected(void)
{
    const char *topic = "down";
    MQTT_Interface::subscribe(topic, messageIn);

    INFO_TRACE("NETWORK", "Request Time\n");

    char buffer[64];
    memset(buffer, 0, 64);
    snprintf(buffer, 64,
            "{"
            "\"request\":\"timestamp\"}");

    MQTT_Interface::publish("up", (uint8_t*)buffer, strlen(buffer));
}

void sendTrendFrame(int temperature, int humidity, int light)
{
    char buffer[64];
    memset(buffer, 0, 64);
    snprintf(buffer, 64,
            "{\"trend\":{"
            "\"temp\":%d,"
            "\"humid\":%d,"
            "\"light\":%d"
            "}}", temperature, humidity, light);

    MQTT_Interface::publish("up", (uint8_t*)buffer, strlen(buffer));
}

int sampleLight()
{
    for(int i=0; i < 16; i++)
    {
        if(!lightSense.read())
            return 1;

        wait(0.005);
    }

    return 0;
}

int main()
{
    printf(CYAN_B("\nWiFi monitor\n\n"));

    INFO_TRACE("Version", "0x%08X\n", MBED_CONF_APP_VERSION);
    INFO_TRACE("Build", "%s\n", SWdatetime);

	dht22 = new DHT22(A0);

	Console::init(&pc, "wi_try");
	wait(1);

	I2C i2cEEP(D14, D15);
	printInfo("EEPROM");
	eeprom = new EEP24xx16(i2cEEP);

	if(!eeprom->getMemorySize())
	    printf(RED("FAIL\n"));
	else
	{
        printf(GREEN("OK\n"));
        NvmConfig::init(eeprom);
	}

    I2C i2cLCD(D3, D6);
    lcd = new LCDPCF8574(&i2cLCD, 0);
    lcdController = new LCDController(lcd);

    {
        char version[20];
        snprintf(version, 20, "ver:0x%08X", MBED_CONF_APP_VERSION);
        lcdController->logLine(version);
        snprintf(version, 20, "%s", SWdatetime);
        lcdController->logLine(version);
    }

	static sNetworkCredentials_t wifiCredentials;
	NvmConfig::getWifiCredentials(&wifiCredentials);

	INFO_TRACE("WIFI", "Connecting..\n");
	int ret = wifi.connect(wifiCredentials.wifi_ssid, wifiCredentials.wifi_pw, NSAPI_SECURITY_WPA_WPA2);
	if (ret != 0) {

	    INFO_TRACE("WIFI", RED("Connect Fail\n"));

	    lcdController->logLine((char *)"WiFi Connect Fail");
	    lcdController->logLine((char *)"Check Credentials");

        INFO_TRACE("WIFI", YELLOW_B("Check Credentials\n"));
	    while(1)
	    {
	        wait(1);
	        lcdController->logRun();
	    }
	    return -1;
	}

	INFO_TRACE("WIFI", GREEN("Connect OK\n"));
	lcdController->logLine((char *)"WiFi Connect OK");

	{
	    printWifiInfo();
	    char ip[20];
	    snprintf(ip, 20, "IP: %s", wifi.get_ip_address());
	    lcdController->logLine(ip);
	}

	MQTTNetwork mqttNetwork(&wifi);
	MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);
    MQTT_Interface::init(&mqttNetwork, &client);

    MQTT_Interface::connect(wifiCredentials.mqtt_ip, wifiCredentials.mqtt_port);
    MQTT_Interface::setConnectedCallback(mqttConnected);

    while (1)
	{
        lcdController->logRun();

        static int inCount = 1200;

        inCount++;
        wait(0.5);

    	if(inCount > 20)
    	{
    	    inCount = 0;
//    	    if(MQTT_Interface::isConnected())
    	    {
    	        uint16_t temp = 0;
    	        uint16_t humid = 0;

    	        sample_dht22(temp, humid);
    	        printInfo("Temperature");// while (arrivedcount < 1)
    	        printf("%d\n", temp);
    	        printInfo("Humidity");
    	        printf("%d\n", humid);

    	        char line[20];
    	        memset(line, 0, 20);
    	        snprintf(line, 20, "temp: %d humid %d", temp ,humid);
    	        lcdController->logLine(line);

    	        int light = sampleLight();

    	        lcdController->updateStaticValues(temp, humid, light);

    	        sendTrendFrame(temp, humid, light);

//    	        lcd->gotoxy(0, 4);
//    	        lcd->puts(line);


    	    }
    	}
	}

	return 1;
}

