#include "esp_log.h"
#include "fonts.h"
#include "ssd1306.hpp"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
//#include <cstdio>
#include <string>
#include <sstream>
#include <iostream>

#include <stdlib.h>
#include "freertos/queue.h"
#include <stdio.h>
#include "c_timeutils.h"


#define GPIO_BUTTON_IN_1	23
#define GPIO_BUTTON_IN_2	18
#define GPIO_BUTTON_IN_3	12
#define GPIO_BUTTON_IN_4	21

#define GPIO_BUTTON_I2C_SCL 19
#define GPIO_BUTTON_I2C_SDA 22

#define GPIO_INPUT_PIN_SEL 	((1<<GPIO_BUTTON_IN_1) | (1<<GPIO_BUTTON_IN_2) | (1<<GPIO_BUTTON_IN_3) | (1<<GPIO_BUTTON_IN_4))
#define GPIO_OUTPUT_PIN_SEL  ((1<<GPIO_BUTTON_I2C_SCL) | (1<<GPIO_BUTTON_I2C_SDA))

#define ESP_INTR_FLAG_DEFAULT 0


using namespace std;
static const gpio_num_t gpioButton1 = GPIO_NUM_23;
static const gpio_num_t gpioButton2 = GPIO_NUM_18;
static const gpio_num_t gpioButton3 = GPIO_NUM_12;
static const gpio_num_t gpioButton4 = GPIO_NUM_21;

static char button_tag[] = "Button";
static QueueHandle_t q1;
OLED oled = OLED(GPIO_NUM_19, GPIO_NUM_22, SSD1306_128x64);

static void showOnDisplay(const char* text){
	ostringstream os;
			os.str("");
			os << text << endl;
			oled.clear();
			oled.select_font(1);
			oled.draw_string(10, 10, os.str(), WHITE, BLACK);
			oled.refresh(true);
}

static void handler(void *args) {
	uint32_t gpio_num = (uint32_t) args;
	if (gpio_num == GPIO_BUTTON_IN_1){
		ESP_LOGI(button_tag, "button 1 pressed");
		showOnDisplay("1");
	} else if (gpio_num == GPIO_BUTTON_IN_2){
		ESP_LOGI(button_tag, "button 2 pressed");
		showOnDisplay("2");
	} else if (gpio_num == GPIO_BUTTON_IN_3){
		ESP_LOGI(button_tag, "button 3 pressed");
		showOnDisplay("3");
	} else if (gpio_num == GPIO_BUTTON_IN_4){
		ESP_LOGI(button_tag, "button 4 pressed");
		showOnDisplay("4");
	} else {
		ESP_LOGI(button_tag, "button ? pressed");
		showOnDisplay("button ? pressed");
	}

	xQueueSendToBackFromISR(q1, &gpio_num, NULL);
}


void buttonsHandler(void *ignore){
	struct timeval lastPress;
	ESP_LOGD(button_tag, ">> buttonsHandler started");
	gettimeofday(&lastPress, NULL);
	gpio_num_t gpio;
	q1 = xQueueCreate(10, sizeof(gpio_num_t));
	gpio_config_t gpioConfig =
	{
		.pin_bit_mask = GPIO_INPUT_PIN_SEL,
		.mode = GPIO_MODE_INPUT,
		.pull_up_en = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_ENABLE,
		.intr_type = GPIO_INTR_POSEDGE,
	};
	gpio_config(&gpioConfig);
	gpio_install_isr_service(0);

	gpio_isr_handler_add(gpioButton1, handler, (void*) GPIO_BUTTON_IN_1);
	gpio_isr_handler_add(gpioButton2, handler, (void*) GPIO_BUTTON_IN_2);
	gpio_isr_handler_add(gpioButton3, handler, (void*) GPIO_BUTTON_IN_3);
	gpio_isr_handler_add(gpioButton4, handler, (void*) GPIO_BUTTON_IN_4);
	while(1) {
		xQueueReceive(q1, &gpio, portMAX_DELAY);
		struct timeval now;
		gettimeofday(&now, NULL);
		if (timeval_durationBeforeNow(&lastPress) > 100)
		{
			ESP_LOGI(button_tag, "Registered a click");
		}
		lastPress = now;
	}
	vTaskDelete(NULL);
}


void myTask(void *pvParameters) {
	adc1_config_width(ADC_WIDTH_12Bit);
	adc1_config_channel_atten(ADC1_CHANNEL_4, ADC_ATTEN_0db);

	ostringstream os;
	uint16_t graph[128];
	memset(graph, 0, sizeof(graph));
}

static xQueueHandle gpio_evt_queue = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg)
	{
	    uint32_t gpio_num = (uint32_t) arg;
	    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
	}


#ifdef __cplusplus
extern "C" {
#endif

void app_main(){
	if (oled.init()) {
		ESP_LOGI("OLED", "oled inited");
		ESP_LOGI("TASK","creating tasks");
		xTaskCreate(&buttonsHandler, "buttonsHandler", 2048, NULL, 5, NULL);
		ESP_LOGI("TASK","after task 1");
	} else {
		ESP_LOGE("OLED", "oled init failed");
	}
}

#ifdef __cplusplus
}
#endif
