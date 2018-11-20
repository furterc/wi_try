#include <stdio.h>
#include <console.h>
#include <Serial.h>
#include <string.h>

#include "utils.h"

#include "mbed.h"

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

#include "RS485.h"

DigitalOut led(D9);


ESP8266Interface wifi(D8, D2);
TCPSocket socket;

EEP24xx16 *eeprom = 0;

typedef struct
{
    uint8_t addr;
    uint8_t setGET;
    uint8_t digitalIn;
    uint8_t digitalOut;
    uint8_t pwm[4];
} sNode485Packet_t;

sNode485Packet_t nodes[1];
int nodeCount = 1;

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

void scanWiFi(int argc,char *argv[])
{

}

void rs485dataIn(uint8_t *data, uint8_t len)
{
    if(len != sizeof(sNode485Packet_t))
    {
        printf("rs485 - invaled len\n");
    }

    printf("rs485 - data in\n");
    diag_dump_buf(data, len);
    memcpy(&nodes[0], data, sizeof(sNode485Packet_t));
}

void dutyCycle_debug(int argc,char *argv[])
{
    uint8_t duties[4];
    memcpy(duties, nodes[0].pwm, 4);
    if(argc == 1)
    {
        for(uint8_t i = 0; i < 4; i++)
            printf("DutyCycle %d = %d\n", i+1, duties[i]);

        return;
    }

    if(argc != 3)
    {
        printf(YELLOW("dc <channel> <duty>\n"));
        return;
    }

    int channel = atoi(argv[1]);
    if(channel < 1 || channel > 4)
    {
        printf(YELLOW("0 < channel < 5\n"));
        return;
    }

    int duty = atoi(argv[2]);
    if(duty < 0 || duty > 100)
    {
        printf(YELLOW("0 < duty =< 100\n"));
        return;
    }

    nodes[0].pwm[channel-1] = duty;
    RS485::send((uint8_t *)&nodes[0], sizeof(sNode485Packet_t));
}

void test485(int argc,char *argv[])
{

    sNode485Packet_t packet;
    memset(&packet, 0x00, sizeof(sNode485Packet_t));
    packet.addr = 1;
    packet.setGET = 1;

    RS485::send((uint8_t *)&packet, sizeof(sNode485Packet_t));

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
	  {"tx",          "",                   "Test485", test485},
	  {"dc",          "",                   "Node dutyCycle", dutyCycle_debug},
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

static void ligthAlarm(uint8_t state)
{
    printf("ledState: %d\n", state);
    led = state;
}

static void mqttConnected(void)
{
    const char *topic = "mbed";
    MQTT_Interface::subscribe(topic, messageIn);
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

	DigitalOut rs485writeEnable(PB_12);
	RS485::init(&rs485uart, &rs485writeEnable);

	RS485::setReceiveCallback(rs485dataIn);

	printf(CYAN_B("\nWiFi example\n\n"));

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
    printInfo("PCF8547");

    lcd = new LCDPCF8574(&i2cLCD, 0);
    lcd->init(LCD_DISP_ON);
    printf(GREEN("Initialized\n"));

    lcd->led(0);
    lcd->gotoxy(4, 0);
    lcd->puts("hello world!");

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
    MQTT_Interface::init(&mqttNetwork, &client);

    MQTT_Interface::connect(wifiCredentials.mqtt_ip, wifiCredentials.mqtt_port);
    MQTT_Interface::setConnectedCallback(mqttConnected);

    const char *topic = "mbed";

    static sAlarmTimes_t alarms;
    NvmConfig::getAlarms(&alarms);

    RTC_Alarm testAlarm;

    testAlarm.setAlarm(&alarms.lightOn, &alarms.lightOff);
    testAlarm.setTiggerCallback(ligthAlarm);

    while (1)
	{
        testAlarm.checkAlarm();
        static int inCount = 1200;

        inCount++;
        wait(0.5);

    	if(inCount > 60)
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
    	        lcd->gotoxy(0, 4);
    	        lcd->puts(line);

    	        char buffer[64];
    	        memset(buffer, 0, 64);
    	        snprintf(buffer, 64,
    	                "{"
    	                "\"temp\":%d,"
    	                "\"humid\":%d"
    	                "}", temp, humid);

    	        MQTT_Interface::publish(topic, (uint8_t*)buffer, strlen(buffer));
    	    }
    	}
	}

	return 1;
}
