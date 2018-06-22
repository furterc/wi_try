#include <stdio.h>
#include <console.h>
#include <Serial.h>

#include "utils.h"

#include "mbed.h"

#include "ESP8266Interface.h"
ESP8266Interface wifi(D10, D3);

/* Console */
Serial pc(SERIAL_TX, SERIAL_RX, 115200);
const char *SWdatetime  =__DATE__ " " __TIME__;

Console::cmd_list_t *Console::mCmdTable[] =
{
        (cmd_list_t*)shellCommands,
        0
};


const char *sec2str(nsapi_security_t sec)
{
    switch (sec) {
        case NSAPI_SECURITY_NONE:
            return "None";
        case NSAPI_SECURITY_WEP:
            return "WEP";
        case NSAPI_SECURITY_WPA:
            return "WPA";
        case NSAPI_SECURITY_WPA2:
            return "WPA2";
        case NSAPI_SECURITY_WPA_WPA2:
            return "WPA/WPA2";
        case NSAPI_SECURITY_UNKNOWN:
        default:
            return "Unknown";
    }
}

void scan_demo(WiFiInterface *wifi)
{
    WiFiAccessPoint *ap;

    printf("Scan:\r\n");

    int count = wifi->scan(NULL,0);

    /* Limit number of network arbitrary to 15 */
    count = count < 15 ? count : 15;

    ap = new WiFiAccessPoint[count];
    count = wifi->scan(ap, count);
    for (int i = 0; i < count; i++)
    {
        printf("Network: %s secured: %s BSSID: %hhX:%hhX:%hhX:%hhx:%hhx:%hhx RSSI: %hhd Ch: %hhd\r\n", ap[i].get_ssid(),
               sec2str(ap[i].get_security()), ap[i].get_bssid()[0], ap[i].get_bssid()[1], ap[i].get_bssid()[2],
               ap[i].get_bssid()[3], ap[i].get_bssid()[4], ap[i].get_bssid()[5], ap[i].get_rssi(), ap[i].get_channel());
    }
    printf("%d networks available.\r\n", count);

    delete[] ap;
}

void http_demo(NetworkInterface *net)
{
    TCPSocket socket;

    printf("Sending HTTP request to www.arm.com...\r\n");

    // Open a socket on the network interface, and create a TCP connection to www.arm.com
    socket.open(net);
    socket.connect("10.0.0.174", 1880);

    // Send a simple http request
    char sbuffer[] = "GET 10.0.0.174:1880/all HTTP/1.1\r\n\r\n";
    int scount = socket.send(sbuffer, sizeof sbuffer);
    printf("sent %d [%.*s]\r\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);

    // Recieve a simple http response and print out the response line
    char rbuffer[64];
    printf("a\n");
    int rcount = socket.recv(rbuffer, sizeof rbuffer);
    printf("b\n");
    printf("recv %d [%.*s]\r\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);
    // Close the socket to return its memory and bring down the network interface
    socket.close();
}

int main()
{
	printf("\n\nWiTry Demo\n");
	printf("Version: 0x%08X\n", MBED_CONF_APP_VERSION);
	printf("Build  : %s\n", SWdatetime);

	// Scan for available access points
//	    scan_demo(&wifi);

	Console::init(&pc, "wi_try");

    printf("\r\nConnecting...\r\n");
    int ret = wifi.connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
    if (ret != 0) {
        printf("\r\nConnection error\r\n");
        return -1;
    }

    printf("my ip     : %s\n", wifi.get_ip_address());
    printf("my gateway: %s\n", wifi.get_gateway());

    printf("Success\r\n\r\n");

    http_demo(&wifi);

	while (1)
	{
		wait(0.1);
	}

	return 1;
}
