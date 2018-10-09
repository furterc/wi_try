/*
 * NvmConfig.h
 *
 *  Created on: 07 Oct 2018
 *      Author: christo
 */

#ifndef SRC_NVM_CONFIG_H_
#define SRC_NVM_CONFIG_H_

#include "caboodle.git/include/caboodle/console.h"

#include "EEP24xx16.h"

#define NVM_OFFSET_WIFICONFIG   0x10

typedef struct {
    char wifi_ssid[32];         //0  + 32 = 32
    char wifi_pw[32];           //32 + 32 = 64
    char mqtt_ip[16];           //64 + 16 = 80
    uint16_t mqtt_port;         //80 + 2  = 82
    uint8_t placekeeper[6];
} sNetworkCredentials_t;



class NvmConfig
{
    NvmConfig(EEP24xx16 *eeprom);

    static NvmConfig *__instance;
    EEP24xx16 *mEeprom;


public:
    virtual ~NvmConfig();

    static void init(EEP24xx16 *eeprom);
    static int setWifiCredentials(sNetworkCredentials_t *credentials);
    static int getWifiCredentials(sNetworkCredentials_t *credentials);

    static bool checkIp(char* ip);

    static void wifiSSIDConfig(int argc,char *argv[]);
    static void wifiPasswordConfig(int argc,char *argv[]);
    static void mqttIPConfig(int argc,char *argv[]);
    static void mqttPortConfig(int argc,char *argv[]);
};




#endif /* SRC_NVM_CONFIG_H_ */
