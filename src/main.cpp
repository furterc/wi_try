#include "mbed.h"

#include "utils.h"
#include <console.h>

#include "nvm_config.h"

#include "ESP8266Interface.h"
#include "EEP24xx16.h"
#include "mqtt_interface.h"
#include "jsmn.h"

#include "LCDPCF8574.h"
#include "LCDController.h"

#include "DHT22.h"
#include "Relay.h"
#include "InlineFan.h"
#include "InlineController.h"
#include "Json.h"

/* Build Date */
const char *SWdatetime  =__DATE__ " " __TIME__;

/* Tick */
Timer tickTimer;

/* Console */
Serial pc(SERIAL_TX, SERIAL_RX, 115200);

/* EEPROM */
EEP24xx16 *eeprom = 0;

/* WiFi */
ESP8266Interface wifi(D8, D2);
TCPSocket socket;

NetworkInterface* network = 0;
bool networkAvailable = false;
int arrivedcount = 0;

enum eWifiStates{
    WIFI_STATUS_CONNECTING,
    WIFI_STATUS_CONNECTED,
    WIFI_STATUS_DISCONNECTED,
    WIFI_STATUS_WAIT
};
eWifiStates wifiState = WIFI_STATUS_CONNECTING;

/* LCD */
LCDPCF8574 *lcd = 0;
LCDController *lcdController = 0;

/* LED */
DigitalOut led(D9);

/* Temp & Humid */
DHT22 *dht22 = 0;

/* Light Sensor */
DigitalIn lightSense(D4);

/* Relays */
Relay inlinePower(A1, RELAY_NC);
Relay inlineHighSpeed(A2, RELAY_NO);
Relay rl3(A3, RELAY_NO);
Relay rl4(A4, RELAY_NO);

Relay *relays[4] = {&inlinePower, &inlineHighSpeed, &rl3, &rl4};

InlineFan fan(&inlinePower, &inlineHighSpeed);
InlineController inlineController(&fan);

/* TREND Defines */
#define TREND_DUTY_MS   30000 //ms
int trendTick = 0;

/* ----------------------- FUNCTIONS ----------------------- */
void inlineFan(int argc,char *argv[])
{
    if(argc == 1)
    {
        INFO_TRACE("INLINE", "");
        switch(fan.getSpeed())
        {
        case INLINE_OFF:
            printf("OFF\n");
            break;
        case INLINE_LOW_SPEED:
            printf("LOW SPEED\n");
            break;
        case INLINE_HIGH_SPEED:
            printf("HIGH SPEED\n");
            break;
        }
        return;
    }

    if(argc == 2)
    {
        if(!strcmp("off", argv[1]))
        {
            fan.setSpeed(INLINE_OFF);
            return;
        }
        if(!strcmp("low", argv[1]))
        {
            fan.setSpeed(INLINE_LOW_SPEED);
            return;
        }
        if(!strcmp("high", argv[1]))
        {
            fan.setSpeed(INLINE_HIGH_SPEED);
            return;
        }
    }

    printf("Invalid fan speed\n");
    printf("Try: off/low/high\n");
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
	  {"ss",            "",                   "Sample Sensors", sensorSample},
	  {"rl",            "",                   "Relay Debug", relayDebug},
	  {"il",            "",                   "Inline Fan", inlineFan},
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
//    INFO_TRACE("RSSI", "%d\n", wifi.get_rssi());
}

void sendThresholds()
{
    sTempThresholds_t thresholds;
    NvmConfig::getThresholds(&thresholds);

    printf("send Thresholds\n");
    char buffer[128];
    memset(buffer, 0, 128);
    snprintf(buffer, 128,
            "{\"th\":{"
            "\"i_ov\":%d,"
            "\"il_off\":%d,"
            "\"il_on\":%d,"
            "\"ih_off\":%d,"
            "\"ih_on\":%d,"
            "\"a_t\":%d,"
            "\"a_h\":%d"
            "}}", thresholds.inlineOverride, thresholds.inlineLowOff, thresholds.inlineLowOn, \
                thresholds.inlineHighOff, thresholds.inlineHighOn, \
                thresholds.tempAlarm, thresholds.humidAlarm);

    MQTT_Interface::publish("up", (uint8_t*)buffer, strlen(buffer), 1);
}

int getJsonObjectData(Json *json, int objectIndex, char *buffer, int bufferSize)
{
    const char *valueStart  = json->tokenAddress(objectIndex);
    int valueLength = json->tokenLength(objectIndex);

    if(valueLength > bufferSize)
        return -1;

    strncpy(buffer, valueStart, valueLength);
    buffer[valueLength] = 0;

    return 0;
}

void setThresholds(Json *json)
{
    int thresholdIndex = json->findKeyIndexIn("t", 0);
    if(thresholdIndex > 0)
    {
        int thresholdChildIndex = json->findChildIndexOf( thresholdIndex, -1 );
        if ( thresholdChildIndex > 0 )
        {
            char childData[128] = {0};
            getJsonObjectData(json, thresholdChildIndex, childData, 128);

            sTempThresholds_t thresholds;
            NvmConfig::getThresholds(&thresholds);

            Json thJson(childData, strlen(childData));
            char parentsChildData[16] = {0};

            int parentIdx = thJson.findKeyIndexIn("i_ov", 0);
            if(parentIdx > 0)
            {
                int speedChildIndex = json->findChildIndexOf(parentIdx, -1);
                if(speedChildIndex > 0)
                {
                    getJsonObjectData(&thJson, speedChildIndex, parentsChildData, 16);
                    thresholds.inlineOverride = atoi(parentsChildData);
                }
            }

            parentIdx = thJson.findKeyIndexIn("il_off", 0);
            if(parentIdx > 0)
            {
                int speedChildIndex = json->findChildIndexOf(parentIdx, -1);
                if(speedChildIndex > 0)
                {
                    getJsonObjectData(&thJson, speedChildIndex, parentsChildData, 16);
                    thresholds.inlineLowOff= atoi(parentsChildData);
                }
            }

            parentIdx = thJson.findKeyIndexIn("il_on", 0);
            if(parentIdx > 0)
            {
                int speedChildIndex = json->findChildIndexOf(parentIdx, -1);
                if(speedChildIndex > 0)
                {
                    getJsonObjectData(&thJson, speedChildIndex, parentsChildData, 16);
                    thresholds.inlineLowOn= atoi(parentsChildData);
                }

            }

            parentIdx = thJson.findKeyIndexIn("ih_off", 0);
            if(parentIdx > 0)
            {
                int speedChildIndex = json->findChildIndexOf(parentIdx, -1);
                if(speedChildIndex > 0)
                {
                    getJsonObjectData(&thJson, speedChildIndex, parentsChildData, 16);
                    thresholds.inlineHighOff= atoi(parentsChildData);
                }
            }

            parentIdx = thJson.findKeyIndexIn("ih_on", 0);
            if(parentIdx > 0)
            {
                int speedChildIndex = json->findChildIndexOf(parentIdx, -1);
                if(speedChildIndex > 0)
                {
                    getJsonObjectData(&thJson, speedChildIndex, parentsChildData, 16);
                    thresholds.inlineHighOn= atoi(parentsChildData);
                }
            }

            parentIdx = thJson.findKeyIndexIn("a_t", 0);
            if(parentIdx > 0)
            {
                int speedChildIndex = json->findChildIndexOf(parentIdx, -1);
                if(speedChildIndex > 0)
                {
                    getJsonObjectData(&thJson, speedChildIndex, parentsChildData, 16);
                    thresholds.tempAlarm = atoi(parentsChildData);
                }
            }

            parentIdx = thJson.findKeyIndexIn("a_h", 0);
            if(parentIdx > 0)
            {
                int speedChildIndex = json->findChildIndexOf(parentIdx, -1);
                if(speedChildIndex > 0)
                {
                    getJsonObjectData(&thJson, speedChildIndex, parentsChildData, 16);
                    thresholds.humidAlarm = atoi(parentsChildData);
                }

            }

            //update thresholds
            NvmConfig::setThresholds(&thresholds);
        }
    }
}

void setTime(Json *json)
{
    int timeIndex = json->findKeyIndexIn("t", 0);
    if(timeIndex > 0)
    {
        int timeChildIndex = json->findChildIndexOf( timeIndex, -1 );
        if ( timeChildIndex > 0 )
        {
            char childData[16] = {0};
            getJsonObjectData(json, timeChildIndex, childData, 16);

            char *pEnd;
            time_t timestamp = strtoull(childData, &pEnd, 10);
            timestamp += 7200;  // add 2 hours for CAT

            struct tm epoch_time;
            memcpy(&epoch_time, localtime(&timestamp), sizeof (struct tm));
            set_time(mktime(&epoch_time));
        }
    }
}

void setInlineFan(Json *json)
{
    int speedIndex = json->findKeyIndexIn("speed", 0);
    if(speedIndex > 0)
    {
        int speedChildIndex = json->findChildIndexOf(speedIndex, -1);
        if(speedChildIndex > 0)
        {
            char childData[16] = {0};
            getJsonObjectData(json,  speedChildIndex, childData, 16);

            printf("inline set speed : %s\n", childData);

            if(!strcmp(childData, "off"))
            {
                fan.setSpeed(INLINE_OFF);
            }else if(!strcmp(childData, "low"))
            {
                fan.setSpeed(INLINE_LOW_SPEED);
            }else if(!strcmp(childData, "high"))
            {
                fan.setSpeed(INLINE_HIGH_SPEED);
            }
        }
    }
}

void handleSetJsonMsg(Json *json, int typeChildIndex)
{
    int setMessageIndex = json->findChildIndexOf( typeChildIndex, -1 );
    if ( setMessageIndex > 0 )
    {
        char setWho[16] = {0};
        getJsonObjectData(json, setMessageIndex, setWho, 16);
        INFO_TRACE("JSON", "SET : %s\n", setWho);

        if(!strcmp(setWho, "th"))
        {
            setThresholds(json);
            return;
        }

        if(!strcmp(setWho, "time"))
        {
            setTime(json);
            return;
        }

        if(!strcmp(setWho, "inline"))
        {
            setInlineFan(json);
            return;
        }
    }
}

void handleGetJsonMsg(Json *json, int typeChildIndex)
{
    int setMessageIndex = json->findChildIndexOf( typeChildIndex, -1 );
    if ( setMessageIndex > 0 )
    {
        char getWho[16] = {0};
        getJsonObjectData(json, setMessageIndex, getWho, 16);
        INFO_TRACE("JSON", "GET : %s\n", getWho);

        if(!strcmp(getWho, "thresholds"))
        {
            sendThresholds();
            return;
        }
    }
}

void messageIn(MQTT::MessageData& md)
{
    MQTT::Message &message = md.message;
    printf("\n");
//    printInfo(GREEN("MQTT MSG IN"));
//    printf("qos %d, retained %d, dup %d, packetid %d\n", message.qos, message.retained, message.dup, message.id);
    INFO_TRACE("MQTT IN", "%.*s\n", message.payloadlen, (char*)message.payload);

    char jsonString[128];
    memcpy(jsonString, message.payload, message.payloadlen);
    jsonString[message.payloadlen] = '\0';
    Json json (jsonString, strlen(jsonString));

    if(!json.isValidJson())
    {
        INFO_TRACE("MQTT_IN", YELLOW("No JSON OBJ\n"));
        return;
    }

    if(json.type(0) != JSMN_OBJECT)
    {
        INFO_TRACE("MQTT_IN", YELLOW("Invalid JSON, ROOT Element is not Object: %s\n"), jsonString);
        return;
    }

    int messageTypeIndex = json.findKeyIndexIn("set", 0);
    if(messageTypeIndex > 0)
    {
       handleSetJsonMsg(&json, messageTypeIndex);
       return;
    }

    messageTypeIndex = json.findKeyIndexIn("get", 0);
    if(messageTypeIndex > 0)
    {
       handleGetJsonMsg(&json, messageTypeIndex);
       return;
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
            "{\"request\":\"timestamp\"}");

    MQTT_Interface::publish("up", (uint8_t*)buffer, strlen(buffer));
}

void sendTrendFrame(int temperature, int humidity, int light)
{
    char fanSpeed[5] = {0};
    switch(fan.getSpeed())
    {
    case INLINE_OFF:
        memcpy(fanSpeed, "off", 3);
        break;
    case INLINE_LOW_SPEED:
        memcpy(fanSpeed, "low", 3);
        break;
    case INLINE_HIGH_SPEED:
        memcpy(fanSpeed, "high", 4);
        break;
    }

    char buffer[64];
    memset(buffer, 0, 64);
    snprintf(buffer, 64,
            "{\"trend\":{"
            "\"temp\":%d,"
            "\"humid\":%d,"
            "\"light\":%d,"
            "\"inline\":\"%s\""
            "}}", temperature, humidity, light, fanSpeed);

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

void runTrend()
{
    if(trendTick < tickTimer.read_ms())
    {
        uint16_t temp = 0;
        uint16_t humid = 0;
        sample_dht22(temp, humid);

        inlineController.updateTemperature((int)temp/10.0);

        int light = sampleLight();

        INFO_TRACE("TREND", "T : %0.1f C\tH : %0.1f\tL : %d\n", (temp/10.0), (humid/10.0), light);

        lcdController->updateStaticValues(temp, humid, light);

        sendTrendFrame(temp, humid, light);

        trendTick = tickTimer.read_ms() + TREND_DUTY_MS;
    }
}

void wifiConnectedCallback(MQTTNetwork* network, MQTT::Client<MQTTNetwork, Countdown> *client)
{
    MQTT_Interface::init(network, client);

    sNetworkCredentials_t wifiCredentials;
    NvmConfig::getWifiCredentials(&wifiCredentials);

    MQTT_Interface::connect(wifiCredentials.mqtt_ip, wifiCredentials.mqtt_port);
    MQTT_Interface::setConnectedCallback(mqttConnected);
}

int main()
{
    printf(CYAN_B("\nWiFi monitor\n\n"));
    INFO_TRACE("Version", "0x%08X\n", MBED_CONF_APP_VERSION);
    INFO_TRACE("Build", "%s\n", SWdatetime);

	dht22 = new DHT22(A0);

	Console::init(&pc, "wi_try");
	wait(0.2);

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


    {
        sTempThresholds_t thresholds;
        NvmConfig::getThresholds(&thresholds);
        inlineController.setThresholds(&thresholds);
    }

    MQTTNetwork mqttNetwork(&wifi);
    MQTT::Client<MQTTNetwork, Countdown> client = MQTT::Client<MQTTNetwork, Countdown>(mqttNetwork);

    tickTimer.start();
    int wifiWaitTimeout = 0;

    while (1)
    {
        lcdController->logRun();

        switch (wifiState) {
        case WIFI_STATUS_CONNECTING:
        {
            sNetworkCredentials_t wifiCredentials;
            NvmConfig::getWifiCredentials(&wifiCredentials);

            INFO_TRACE("WIFI", "Connecting..\n");

            int ret = wifi.connect(wifiCredentials.wifi_ssid, wifiCredentials.wifi_pw, NSAPI_SECURITY_WPA_WPA2);
            if (ret != 0) {

                INFO_TRACE("WIFI", RED("Connect Fail\n"));

                lcdController->logLine((char *)"WiFi Connect Fail");
                lcdController->logLine((char *)"Check Credentials");
                lcdController->logLine((char *)"Check WIFI");

                INFO_TRACE("WIFI", YELLOW_B("Check Credentials\n"));
                wifiState = WIFI_STATUS_DISCONNECTED;
                continue;
            }

            INFO_TRACE("WIFI", GREEN("Connect OK\n"));
            lcdController->logLine((char *)"WiFi Connect OK");

            {
                printWifiInfo();
                char ip[20];
                snprintf(ip, 20, "IP: %s", wifi.get_ip_address());
                lcdController->logLine(ip);
            }
            wifiConnectedCallback(&mqttNetwork, &client);
            wifiState = WIFI_STATUS_CONNECTED;
        }
        break;
        case WIFI_STATUS_CONNECTED:
        {
            if(wifi.get_connection_status() == NSAPI_STATUS_DISCONNECTED)
            {
                printf(RED("WIFI connection fail: %d\n"), wifi.get_connection_status());
                wifiState = WIFI_STATUS_DISCONNECTED;
            }

            if(MQTT_Interface::isConnected())
                runTrend();
        }
        break;
        case WIFI_STATUS_DISCONNECTED:
        {
            INFO_TRACE("WIFI", RED("Disconnected\n"));
            wifiWaitTimeout = tickTimer.read_ms() + 30000;
            wifiState = WIFI_STATUS_WAIT;
        }
        break;
        case WIFI_STATUS_WAIT:
        {
            if(wifiWaitTimeout < tickTimer.read_ms())
                    wifiState = WIFI_STATUS_CONNECTING;
        }
        break;
        default:
            break;
        }

        wait(0.5);
    }

	return 1;
}

