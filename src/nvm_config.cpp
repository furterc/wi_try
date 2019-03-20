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


int NvmConfig::setThresholds(sTempThresholds_t *thresholds)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->write(NVM_OFFSET_THRESHOLDS, thresholds, sizeof(sTempThresholds_t));

    if(len == sizeof(sTempThresholds_t))
        return 0;

    return 1;
}

int NvmConfig::getThresholds(sTempThresholds_t *thresholds)
{
    if(!__instance)
        return -1;

    int len = __instance->mEeprom->read(NVM_OFFSET_THRESHOLDS, thresholds, sizeof(sTempThresholds_t));

    if(len == sizeof(sTempThresholds_t))
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

void NvmConfig::inlineThresholdConfig(int argc,char *argv[])
{
    sTempThresholds_t thresholds;
    if(NvmConfig::getThresholds(&thresholds) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf(CYAN_B("Temperature Thresholds\n"));

        printf(CYAN("Inline Fan\n"));
        printf("override: %d\n", thresholds.inlineOverride);
        printf("LOW off : %d\n", thresholds.inlineLowOff);
        printf("LOW on  : %d\n", thresholds.inlineLowOn);
        printf("HIGH off: %d\n", thresholds.inlineHighOff);
        printf("HIGH on : %d\n", thresholds.inlineHighOn);

        return;
    }

    if(argc != 5)
    {
        printf(YELLOW("thi <low_on> <low_off> <high_on> <high_off>\n"));
        return;
    }

    int lowOff  = atoi(argv[1]);
    int lowOn   = atoi(argv[2]);
    int highOff = atoi(argv[3]);
    int highOn  = atoi(argv[4]);

    if(lowOff > lowOn || lowOn > highOff || highOff > highOn)
    {
        printf(YELLOW("lowOff < lowOn < highOff < highOn\n"));
        return;
    }

    if(lowOff < 0 || highOn > 150)
    {
        printf(RED("0 < temperature < 150\n"));
        return;
    }

    thresholds.inlineLowOff  = lowOff;
    thresholds.inlineLowOn   = lowOn;
    thresholds.inlineHighOff = highOff;
    thresholds.inlineHighOn  = highOn;

    printf(GREEN("Set Inline Fan\n"));
    printf("override: %d\n", thresholds.inlineOverride);
    printf("LOW off : %d\n", thresholds.inlineLowOff);
    printf("LOW on  : %d\n", thresholds.inlineLowOn);
    printf("HIGH off: %d\n", thresholds.inlineHighOff);
    printf("HIGH on : %d\n", thresholds.inlineHighOn);


    NvmConfig::setThresholds(&thresholds);
}

void NvmConfig::inlineOverrideConfig(int argc,char *argv[])
{
    sTempThresholds_t thresholds;
    if(NvmConfig::getThresholds(&thresholds) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf(CYAN("Inline Fan\n"));
        printf("Override : %d\n", thresholds.inlineOverride);

        return;
    }

    if(argc != 2)
    {
        printf(YELLOW("thio 0/1\n"));
        return;
    }

    int inlineOverride = atoi(argv[1]);

    if(inlineOverride < 0 || inlineOverride > 1)
    {
        printf(YELLOW("0 < override < 2\n"));
        return;
    }

    thresholds.inlineOverride = inlineOverride;

    printf(CYAN("Set Inline Fan\n"));
    printf("Override : %d\n", thresholds.inlineOverride);

    NvmConfig::setThresholds(&thresholds);
}

void NvmConfig::alarmThresholdConfig(int argc,char *argv[])
{
    sTempThresholds_t thresholds;
    if(NvmConfig::getThresholds(&thresholds) == -1)
    {
        printf(RED("EEPROM Not found\n"));
        return;
    }

    if(argc == 1)
    {
        printf(CYAN("Alarm Thresholds\n"));
        printf("Temperature  : %d\n", thresholds.tempAlarm);
        printf("Humidity     : %d\n", thresholds.humidAlarm);

        return;
    }

    if(argc != 3)
    {
        printf(YELLOW("tha <temperature> <humidity>\n"));
        return;
    }

    int alarmTemperature = atoi(argv[1]);
    int alarmHumidity = atoi(argv[2]);

    if(alarmTemperature < 0 || alarmTemperature > 149 || alarmHumidity < 0 || alarmHumidity > 100)
    {
        printf(YELLOW("0 < temperature < 150\n"));
        printf(YELLOW("0 < humidity    < 101\n"));
        return;
    }

    thresholds.tempAlarm = alarmTemperature;
    thresholds.humidAlarm = alarmHumidity;

    printf(GREEN("Set Alarm Thresholds\n"));
    printf("Temperature  : %d\n", thresholds.tempAlarm);
    printf("Humidity     : %d\n", thresholds.humidAlarm);

    NvmConfig::setThresholds(&thresholds);
}

const Console::cmd_list_t configureCommands[] =
{
        {"Configure"    ,0,0,0},
        {"wid",       "",  "Set the WiFi SSID",             NvmConfig::wifiSSIDConfig},
        {"wpw",       "",  "Set the WiFi Password",         NvmConfig::wifiPasswordConfig},
        {"mip",       "",  "Set the MQTT IP",               NvmConfig::mqttIPConfig},
        {"mp",        "",  "Set the MQTT Port",             NvmConfig::mqttPortConfig},
        {"thi",       "",  "Set inline fan thresholds",     NvmConfig::inlineThresholdConfig},
        {"thio",       "",  "Set inline fan override",       NvmConfig::inlineOverrideConfig},
        {"tha",       "",  "Set alarm thresholds",          NvmConfig::alarmThresholdConfig},
        {0,0,0,0}
};
