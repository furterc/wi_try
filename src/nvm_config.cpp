/*
 * NvmConfig.cpp
 *
 *  Created on: 07 Oct 2018
 *      Author: christo
 */

#include "nvm_config.h"
#include "caboodle.git/include/caboodle/utils.h"

NvmConfig *NvmConfig::__instance = 0;


NvmConfig::NvmConfig(EEP24xx16 *eeprom) : mEeprom(eeprom)
{
    printf("EEPROM Block Size : 0x%04X\n", mEeprom->getBlockSize());
}

NvmConfig::~NvmConfig()
{

}

void NvmConfig::init(EEP24xx16 *eeprom)
{
    if(__instance == 0)
        __instance = new NvmConfig(eeprom);
}

int NvmConfig::setWifiCredentials(sNetworkCredentials_t *credentials)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->write(NVM_OFFSET_WIFICONFIG, credentials, sizeof(sNetworkCredentials_t));

    if(len == sizeof(sNetworkCredentials_t))
        return 0;

    return 1;
}

int NvmConfig::getWifiCredentials(sNetworkCredentials_t *credentials)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->read(NVM_OFFSET_WIFICONFIG, credentials, sizeof(sNetworkCredentials_t));

    if(len == sizeof(sNetworkCredentials_t))
        return 0;

    return 1;
}


int NvmConfig::setThresholds(sThresholds_t *thresholds)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->write(NVM_OFFSET_THRESHOLDS, thresholds, sizeof(sThresholds_t));

    if(len == sizeof(sThresholds_t))
        return 0;

    return 1;
}

int NvmConfig::getThresholds(sThresholds_t *thresholds)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->read(NVM_OFFSET_THRESHOLDS, thresholds, sizeof(sThresholds_t));

    if(len == sizeof(sThresholds_t))
        return 0;

    return 1;
}

int NvmConfig::setAlarms(sAlarmTimes_t *alarms)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->write(NVM_OFFSET_ALARMS, alarms, sizeof(sAlarmTimes_t));

    if(len == sizeof(sAlarmTimes_t))
        return 0;

    return 1;
}

int NvmConfig::getAlarms(sAlarmTimes_t *alarms)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->read(NVM_OFFSET_ALARMS, alarms, sizeof(sAlarmTimes_t));

    if(len == sizeof(sAlarmTimes_t))
        return 0;

    return 1;
}

void NvmConfig::wifiSSIDConfig(int argc,char *argv[])
{
    sNetworkCredentials_t cred;
    if(NvmConfig::getWifiCredentials(&cred) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf("Wifi SSID = %s\n", cred.wifi_ssid);
        return;
    }

    if(argc != 2)
    {
        printf(YELLOW("wid <ssid>\n"));
        return;
    }

    char *ssid = argv[1];

    int len = strlen(ssid);
    if(len > 31)
    {
        printf(RED("Maximum SSID length 32 characters\n"));
        return;
    }

    memset(cred.wifi_ssid, 0x00, sizeof(cred.wifi_ssid));
    memcpy(cred.wifi_ssid, ssid, len);

    NvmConfig::setWifiCredentials(&cred);
}

void NvmConfig::wifiPasswordConfig(int argc,char *argv[])
{
    sNetworkCredentials_t cred;
    if(NvmConfig::getWifiCredentials(&cred) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf("WiFi Password = ");
        for(uint8_t k = 0; k < strlen(cred.wifi_pw); k++)
        {
            printf("*");
        }
        printf("\n");
        return;
    }

    if(argc != 2)
    {
        printf(YELLOW("wpw <password>\n"));
        return;
    }

    char *password = argv[1];

    int len = strlen(password);
    if(len > 31)
    {
        printf(RED("Maximum password length 32 characters\n"));
        return;
    }

    memset(cred.wifi_pw, 0x00, sizeof(cred.wifi_pw));
    memcpy(cred.wifi_pw, password, len);

    NvmConfig::setWifiCredentials(&cred);
}

bool NvmConfig::checkIp(char* ip)
{
    if(strlen(ip) > 16)
        return false;

    char tempIp[16];
    memcpy(tempIp, ip, strlen(ip));

    char *p = strchr(tempIp, '.');
    if(!p)
        return false;

    *p = 0;
    p++;

    char *digit = tempIp;
    int i = atoi(digit);
    if((i < 0) || (i > 255))
        return false;

    uint8_t dotCount = 3;

    while(p && dotCount)
    {
        digit = p;
        p = strchr(digit, '.');

        if(p)
        {
            *p = 0;
            p++;
        }

        int i = atoi(digit);
        if((i < 0) || (i > 255))
            break;

        dotCount--;
    }

    if(dotCount)
        return false;

    return true;
}

void NvmConfig::mqttIPConfig(int argc,char *argv[])
{
    sNetworkCredentials_t cred;
    if(NvmConfig::getWifiCredentials(&cred) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf("MQTT IP : %s\n", cred.mqtt_ip);
        return;
    }

    if(argc != 2)
    {
        printf(YELLOW("mip <ssid>\n"));
        return;
    }

    char *ip = argv[1];

    int len = strlen(ip);

    printf("IP: ");
    if(!checkIp(ip))
    {
        printf(RED("Invalid IP Address\n"));
        return;
    }

    memset(cred.mqtt_ip, 0x00, sizeof(cred.mqtt_ip));
    memcpy(cred.mqtt_ip, ip, len);

    NvmConfig::setWifiCredentials(&cred);
}

void NvmConfig::mqttPortConfig(int argc,char *argv[])
{
    sNetworkCredentials_t cred;
    if(NvmConfig::getWifiCredentials(&cred) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf("MQTT Port : %d\n", cred.mqtt_port);
        return;
    }

    if(argc != 2)
    {
        printf(YELLOW("mp <port>\n"));
        return;
    }

    int port = atoi(argv[1]);

    if((port < 0) || (port > 65535))
    {
        printf("MQTT Port is a 16bit number\n");
        return;
    }

    cred.mqtt_port = port;

    NvmConfig::setWifiCredentials(&cred);
}

void NvmConfig::tempThresholdConfig(int argc,char *argv[])
{
    sThresholds_t thresholds;
    if(NvmConfig::getThresholds(&thresholds) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf("Temperature Thresholds\n - low  %d\n - high %d\n", thresholds.tempLow, thresholds.tempHigh);
        return;
    }

    if(argc != 3)
    {
        printf(YELLOW("tht <low> <high>\n"));
        return;
    }

    int low = atoi(argv[1]);
    int high = atoi(argv[2]);

    if(low > high)
    {
        printf(RED("Low > High\n"));
        return;
    }

    if(low < 0 || low > 150 || high < 0 || high > 150)
    {
        printf(RED("0 < temperature < 150\n"));
        return;
    }

    thresholds.tempLow = low;
    thresholds.tempHigh = high;

    NvmConfig::setThresholds(&thresholds);
}

void NvmConfig::humidThresholdConfig(int argc,char *argv[])
{
    sThresholds_t thresholds;
    if(NvmConfig::getThresholds(&thresholds) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf("Humidity Thresholds\n - low  %d\n - high %d\n", thresholds.humidLow, thresholds.humidHigh);
        return;
    }

    if(argc != 3)
    {
        printf(YELLOW("thh <low> <high>\n"));
        return;
    }

    int low = atoi(argv[1]);
    int high = atoi(argv[2]);

    if(low > high)
    {
        printf(RED("Low > High\n"));
        return;
    }

    if(low < 0 || low > 100 || high < 0 || high > 100)
    {
        printf(RED("0 < humidity < 100\n"));
        return;
    }

    thresholds.humidLow = low;
    thresholds.humidHigh = high;

    NvmConfig::setThresholds(&thresholds);
}

void NvmConfig::lightAlarmConfig(int argc,char *argv[])
{
    sAlarmTimes_t alarms;
    if(NvmConfig::getAlarms(&alarms) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf("Light Alarm \n - on  %02d:%02d\n - off %02d:%02d\n",
                alarms.lightOn.hour, alarms.lightOn.minute,
                alarms.lightOff.hour, alarms.lightOff.minute);
        return;
    }

    if(argc != 5)
    {
        printf(YELLOW("la <onHour> <onMinute> <offHour> <offMinute>\n"));
        return;
    }

    int onHour    = atoi(argv[1]);
    int onMinute  = atoi(argv[2]);
    int offHour   = atoi(argv[3]);
    int offMinute = atoi(argv[4]);

    if(onHour < 0 || onHour > 23 || offHour < 0 || offHour > 23)
    {
        printf(RED("0 < Hour < 23\n"));
        return;
    }

    if(onMinute < 0 || onMinute > 59 || offMinute < 0 || offMinute > 59)
    {
        printf(RED("0 < Minute < 59\n"));
        return;
    }

    alarms.lightOn.hour    = onHour;
    alarms.lightOn.minute  = onMinute;
    alarms.lightOff.hour   = offHour;
    alarms.lightOff.minute = offMinute;

    NvmConfig::setAlarms(&alarms);
}

const Console::cmd_list_t configureCommands[] =
{
        {"Configure"    ,0,0,0},
        {"wid",       "",  "Set the WiFi SSID",             NvmConfig::wifiSSIDConfig},
        {"wpw",       "",  "Set the WiFi Password",         NvmConfig::wifiPasswordConfig},
        {"mip",       "",  "Set the MQTT IP",               NvmConfig::mqttIPConfig},
        {"mp",        "",  "Set the MQTT Port",             NvmConfig::mqttPortConfig},
        {"tht",       "",  "Set temperature thresholds",    NvmConfig::tempThresholdConfig},
        {"thh",       "",  "Set humidity thresholds",       NvmConfig::humidThresholdConfig},
        {"la",        "",  "Set Light Alarm",               NvmConfig::lightAlarmConfig},
        {0,0,0,0}
};
