/*
 * esp_wifi.cpp
 *
 *  Created on: 22 Jun 2018
 *      Author: christo
 */

#include <esp_wifi.h>

#define TRACE(_x, ...) INFO_TRACE("WiFi ", _x, ##__VA_ARGS__)

ESPWiFi::ESPWiFi(WiFiInterface *net) : mWiFi(net)
{
	mConnected = false;
	// TODO Auto-generated constructor stub

}

ESPWiFi::~ESPWiFi() {
	// TODO Auto-generated destructor stub
}
//
//void http_demo(NetworkInterface *net)
//{
//    TCPSocket socket;
//
//    printf("Sending HTTP request to www.arm.com...\r\n");
//
//    // Open a socket on the network interface, and create a TCP connection to www.arm.com
//    socket.open(net);
//    socket.connect("10.0.0.174", 1880);
//
//    // Send a simple http request
//    char sbuffer[] = "GET /test HTTP/1.1\r\nHost: 10.0.0.174:1880\r\n\r\n";
//
//    int scount = socket.send(sbuffer, sizeof sbuffer);
//    printf("sent %d [%.*s]\r\n", scount, strstr(sbuffer, "\r\n")-sbuffer, sbuffer);
//
//    // Recieve a simple http response and print out the response line
//    char rbuffer[64];
//    printf("a\n");
//    int rcount = socket.recv(rbuffer, sizeof rbuffer);
//    printf("b\n");
//    printf("recv %d [%.*s]\r\n", rcount, strstr(rbuffer, "\r\n")-rbuffer, rbuffer);
//    // Close the socket to return its memory and bring down the network interface
//    socket.close();
//}

const char *ESPWiFi::sec2str(nsapi_security_t sec)
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

int ESPWiFi::connect()
{
	TRACE("Connecting\n");
	int ret = mWiFi->connect(MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2);
	if (ret != 0)
	{
		TRACE(RED("Failed\n"));
		mConnected = false;
		return -1;
	}

	TRACE(GREEN("Success\n"));
	TRACE("IP %s\n", mWiFi->get_ip_address());
	mConnected = true;
	return 1;
}

void ESPWiFi::scanDevices(WiFiInterface *wifi)
{
	if(!mConnected)
		return;


    WiFiAccessPoint *ap;

    printf("ESP WiFi Scan:\r\n");

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

