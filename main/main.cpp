#include <stdio.h>
#include <tm1638.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const auto PIN_CLK = static_cast<gpio_num_t>(4);
const auto PIN_DIO = static_cast<gpio_num_t>(5);
const auto PIN_STB = static_cast<gpio_num_t>(6);

const char *TAG = "TM1638Tester";
extern "C"
void app_main(void){
    tm1638 leds(1, PIN_CLK, PIN_DIO, PIN_STB);
    int offset = 0;

    leds.clear();
    leds.enable();

    while(1){
        ESP_LOGI(TAG, "Setting segments, offset=%d", offset);
        //leds.brightness(offset);
        for(int i = 0; i < 8; i++){
            leds.set((uint8_t)i, 0xFF);
            //(uint8_t)((i + offset) % 7));
        }
        offset = (offset + 1) % 8;

        auto keys = leds.get();
        ESP_LOGI(TAG, "Read keys, value=%x", keys);

        vTaskDelay(100);
    }
};
