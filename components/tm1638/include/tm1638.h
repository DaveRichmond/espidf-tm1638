#pragma once

#include <freertos/FreeRTOS.h>
#include "esp_check.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#include <vector>

class tm1638 {
    private:
        enum CMD_TYPE {
            CMD_DATA = 0x40,
            CMD_CTRL = 0x80,
            CMD_ADDR = 0xC0,
        };

        enum DATA {
            DATA_WRITE = 0x00,
            DATA_READ_KEYS = 0x02,
        };
        enum DATA_ADDRESSING {
            DATA_ADDR_AUTOINC = 0x00,
            DATA_ADDR_FIXED = 0x04,
        };
          
        enum CTRL {
            PULSE_1_16 = 0x00,
            PULSE_2_16 = 0x01,
            PULSE_4_16 = 0x02,
            PULSE_10_16 = 0x03,
            PULSE_11_16 = 0x04,
            PULSE_12_16 = 0x05,
            PULSE_13_16 = 0x06,
            PULSE_14_16 = 0x07,
            DISPLAY_OFF = 0x00,
            DISPLAY_ON = 0x08,
        };

        static constexpr uint8_t control_command(CTRL c){
            return ((uint8_t)CMD_CTRL) | ((uint8_t)c);
        }
        static constexpr uint8_t data_command(DATA op, DATA_ADDRESSING addr){
            return ((uint8_t)CMD_DATA) | ((uint8_t)op) | ((uint8_t)addr);
        }
        static constexpr uint8_t mem_command(uint8_t addr){
            addr = addr & 0x0F; // clear upper 4 bits, only the lower 4 are the pointer
            return ((uint8_t)CMD_ADDR) | addr;
        }

        int spi_bus;
        spi_device_handle_t dev;

        constexpr spi_host_device_t get_spi(int n);

        void write(uint8_t c, std::vector<uint8_t>data, size_t len);
        void write(uint8_t c);
    
        std::vector<uint8_t> read(uint8_t c, size_t len);
        gpio_num_t clk, dio, stb;

    public:
        tm1638(int spi_bus, gpio_num_t clk, gpio_num_t dio, gpio_num_t stb);
        void enable();
        void disable();
        void brightness(int b);
        void clear();
        void set(uint8_t segment, uint8_t value);
        uint16_t get(void);
        constexpr uint8_t map(char c);
};