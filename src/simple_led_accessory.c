/*
 * simple_led_accessory.c
 * Define the accessory in pure C language using the Macro in characteristics.h
 *
 *  Created on: 2020-02-08
 *      Author: Mixiaoxiao (Wang Bin)
 */

#include <Arduino.h>
#include <homekit/types.h>
#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <stdio.h>
#include <port.h>

#define ACCESSORY_NAME ("ESP8266_LED3")
#define ACCESSORY_SN ("SN_01234567") // SERIAL_NUMBER
#define ACCESSORY_MANUFACTURER ("Arduino Homekit2")
#define ACCESSORY_MODEL ("ESP82663")

const int LED_PINS[] = {0, 4, 10, 14, 16};
const int LED_PINS_LEN = sizeof(LED_PINS) / sizeof(LED_PINS[0]);


int led_bri = 100;		//[0, 100]
bool led_power = true; // true or flase

homekit_value_t led_on_get()
{
	return HOMEKIT_BOOL(led_power);
}

void led_on_set(homekit_value_t value)
{
	if (value.format != homekit_format_bool)
	{
		printf("Invalid on-value format: %d\n", value.format);
		return;
	}
	led_power = value.bool_value;
	if (led_power)
	{
		if (led_bri < 1)
		{
			led_bri = 100;
		}
	}
	led_update();
}

homekit_value_t light_bri_get()
{
	return HOMEKIT_INT(led_bri);
}

void led_bri_set(homekit_value_t value)
{
	if (value.format != homekit_format_int)
	{
		return;
	}
	led_bri = value.int_value;
	led_update();
}

homekit_characteristic_t name = HOMEKIT_CHARACTERISTIC_(NAME, ACCESSORY_NAME);
homekit_characteristic_t serial_number = HOMEKIT_CHARACTERISTIC_(SERIAL_NUMBER, ACCESSORY_SN);
homekit_characteristic_t led_on = HOMEKIT_CHARACTERISTIC_(ON, false,
														  .getter = led_on_get,
														  .setter = led_on_set);

void led_update()
{
	if (led_power) {
		led_update_v(led_bri);
	} else {
		led_update_v(0);
	}
}

void led_update_v(int v)
{
	for (int i = 0; i < LED_PINS_LEN; i++)
	{
		if (i * 20 < v)
		{
			printf("序号%d ON\n", i);
			digitalWrite(LED_PINS[i], HIGH);
		}
		else
		{
			printf("序号%d OFF\n", i);
			digitalWrite(LED_PINS[i], LOW);
		}
	}
}

void led_toggle()
{
	led_on.value.bool_value = !led_on.value.bool_value;
	led_on.setter(led_on.value);
	homekit_characteristic_notify(&led_on, led_on.value);
}

void accessory_identify(homekit_value_t _value)
{
	printf("accessory identify\n");
	for (int j = 0; j < 3; j++)
	{
		led_power = true;
		led_update();
		delay(100);
		led_power = false;
		led_update();
		delay(100);
	}
}

homekit_accessory_t *accessories[] =
	{
		HOMEKIT_ACCESSORY(
				.id = 1,
				.category = homekit_accessory_category_lightbulb,
				.services = (homekit_service_t *[]){
					HOMEKIT_SERVICE(ACCESSORY_INFORMATION,
									.characteristics = (homekit_characteristic_t *[]){
										&name,
										HOMEKIT_CHARACTERISTIC(MANUFACTURER, ACCESSORY_MANUFACTURER),
										&serial_number,
										HOMEKIT_CHARACTERISTIC(MODEL, ACCESSORY_MODEL),
										HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "0.0.1"),
										HOMEKIT_CHARACTERISTIC(IDENTIFY, accessory_identify),
										NULL}),
					HOMEKIT_SERVICE(LIGHTBULB, .primary = true, .characteristics = (homekit_characteristic_t *[]){HOMEKIT_CHARACTERISTIC(NAME, "Led"), &led_on, HOMEKIT_CHARACTERISTIC(BRIGHTNESS, 100, .getter = light_bri_get, .setter = led_bri_set), NULL}), NULL}),
		NULL};

homekit_server_config_t config = {
	.accessories = accessories,
	.password = "111-11-112",
	.setupId = "ABCDE"};

void accessory_init()
{
	for (size_t i = 0; i < LED_PINS_LEN; i++)
	{
		pinMode(LED_PINS[i], OUTPUT);
	}
	led_update();
}
