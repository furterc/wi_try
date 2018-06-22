#include <stdio.h>
#include <console.h>
#include <Serial.h>

#include "utils.h"

#include "mbed.h"

#include "esp_wifi.h"
#include "ESP8266Interface.h"
ESP8266Interface wifi(D10, D3);
ESPWiFi *espWiFi;

/* Console */
Serial pc(SERIAL_TX, SERIAL_RX, 115200);
const char *SWdatetime  =__DATE__ " " __TIME__;

void scanWiFi(int argc,char *argv[])
{
	espWiFi->scanDevices(&wifi);
}

const Console::cmd_list_t mainCommands[] =
{
      {"MAIN"    ,0,0,0},
      {"ws",     "",                   "Scan for WiFi Devices", scanWiFi},
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

	Console::init(&pc, "wi_try");

	// Scan for available access points
	espWiFi = new ESPWiFi(&wifi);
	if(espWiFi->connect() == 1)
	{

	}


    while (1)
	{
		wait(0.1);
	}

	return 1;
}
