#include <Arduino.h>
#include <string>
#include <arduino_homekit_server.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include "wifi_info.h"
#include "index.h"

#define SIMPLE_INFO(fmt, ...) printf_P(PSTR(fmt "\n"), ##__VA_ARGS__);

uint32_t next_heap_millis = 0;

AsyncWebServer server(80);

extern "C" void led_update_v(int);
extern "C" int get_light_bri();

void setup()
{
	Serial.begin(9600);
	Serial.setRxBufferSize(32);
	Serial.setDebugOutput(false);

	pinMode(BUILTIN_LED, OUTPUT);
	wifi_connect();

	SIMPLE_INFO("");
	SIMPLE_INFO("SketchSize: %d", ESP.getSketchSize());
	SIMPLE_INFO("FreeSketchSpace: %d", ESP.getFreeSketchSpace());
	SIMPLE_INFO("FlashChipSize: %d", ESP.getFlashChipSize());
	SIMPLE_INFO("FlashChipRealSize: %d", ESP.getFlashChipRealSize());
	SIMPLE_INFO("FlashChipSpeed: %d", ESP.getFlashChipSpeed());
	SIMPLE_INFO("SdkVersion: %s", ESP.getSdkVersion());
	SIMPLE_INFO("FullVersion: %s", ESP.getFullVersion().c_str());
	SIMPLE_INFO("CpuFreq: %dMHz", ESP.getCpuFreqMHz());
	SIMPLE_INFO("FreeHeap: %d", ESP.getFreeHeap());
	SIMPLE_INFO("ResetInfo: %s", ESP.getResetInfo().c_str());
	SIMPLE_INFO("ResetReason: %s", ESP.getResetReason().c_str());
	INFO_HEAP();
	homekit_setup();
	INFO_HEAP();
	blink_led(200, 3);

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
		AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", lamp_html_gz, lamp_html_gz_len);
		response->addHeader("Content-Encoding", "gzip");
		request->send(response);
	});
	server.on("/led_bri", HTTP_GET, [](AsyncWebServerRequest *request) {
		int i = get_light_bri();
		char c[7];
		itoa(i,c,10);
		Serial.println(i);
		Serial.println(c);
		request->send(200, "text/plain", c);
	});
	server.on("/lamp", HTTP_GET, [](AsyncWebServerRequest *request) {
		if(request->hasArg("v")) {
			String v = request->arg("v");
			led_update_v(atoi(v.c_str()));
			request->send(200, "text/plain", "Hi! I am ESP8266 lamp. " + v);
		} else {
			request->send(200, "text/plain", "Hi! I am ESP8266 lamp.");
		}
	});
	server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
		homekit_storage_reset();
		request->send(200, "text/plain", "homekit_storage_reset ...");
	});
	AsyncElegantOTA.begin(&server);    // Start ElegantOTA
	server.begin();
	Serial.println("HTTP server started");
}

void loop()
{
	homekit_loop();
}

void blink_led(int interval, int count)
{
	for (int i = 0; i < count; i++)
	{
		builtinledSetStatus(true);
		delay(interval);
		builtinledSetStatus(false);
		delay(interval);
	}
}

void builtinledSetStatus(bool on)
{
	digitalWrite(BUILTIN_LED, on ? LOW : HIGH);
}

//==============================
// Homekit setup and loop
//==============================

extern "C" homekit_server_config_t config;
extern "C" homekit_characteristic_t name;
extern "C" void led_toggle();
extern "C" void accessory_init();

void homekit_setup()
{
	accessory_init();
	uint8_t mac[WL_MAC_ADDR_LENGTH];
	WiFi.macAddress(mac);
	int name_len = snprintf(NULL, 0, "%s_%02X%02X%02X",
							name.value.string_value, mac[3], mac[4], mac[5]);
	char *name_value = (char *)malloc(name_len + 1);
	snprintf(name_value, name_len + 1, "%s_%02X%02X%02X",
			 name.value.string_value, mac[3], mac[4], mac[5]);
	name.value = HOMEKIT_STRING_CPP(name_value);

	arduino_homekit_setup(&config);
}

void homekit_loop()
{
	arduino_homekit_loop();
	uint32_t time = millis();
	if (time > next_heap_millis) {
		INFO("heap: %d, sockets: %d", ESP.getFreeHeap(), arduino_homekit_connected_clients_count());
		next_heap_millis = time + 5000;
	}
}
