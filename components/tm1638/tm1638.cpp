#include <stdio.h>
#include "tm1638.h"

#include <freertos/task.h>

#include <esp_log.h>

static const char *TAG = "TM1638";

constexpr spi_host_device_t tm1638::get_spi(int n){
    switch(n){
        case 0: return SPI1_HOST;
        case 1: return SPI2_HOST;
        case 2: return SPI3_HOST;
        default: return SPI_HOST_MAX;
    }
}


tm1638::tm1638(int spi_bus, gpio_num_t clk, gpio_num_t dio, gpio_num_t stb)
    : spi_bus(spi_bus), clk(clk), dio(dio), stb(stb){

    /* FIXME: Do we need to initialise the GPIOs first?
    ESP_LOGI(TAG, "Initialising GPIO");
    gpio_config_t conf = {};
    conf.intr_type = GPIO_INTR_DISABLE;
    conf.mode = GPIO_MODE_OUTPUT;
    conf.pull_down_en = (gpio_pulldown_t)0;
    conf.pull_up_en = (gpio_pullup_t)0;
    conf.pin_bit_mask = (1ULL << this->clk) | (1ULL << this->dio) | (1ULL << this->stb);
    ESP_ERROR_CHECK(gpio_config(&conf));
    */

    ESP_LOGI(TAG, "Initialising SPI Bus %d", this->spi_bus);
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = this->dio;
    buscfg.sclk_io_num = this->clk;
    buscfg.miso_io_num = -1;
    buscfg.max_transfer_sz = 32; // should be larger than any transaction
    ESP_ERROR_CHECK(spi_bus_initialize(get_spi(this->spi_bus), &buscfg, SPI_DMA_DISABLED));

    // According to https://github.com/MartyMacGyver/TM1638-demos-and-examples
    // the closest SPI mode is 3, and maximum 500KHz
    ESP_LOGI(TAG, "Adding device to SPI bus");
    spi_device_interface_config_t devcfg = {};
    devcfg.spics_io_num = this->stb;
    devcfg.command_bits = 8;
    devcfg.mode = 3;
    devcfg.flags = SPI_DEVICE_HALFDUPLEX | SPI_DEVICE_3WIRE; // both read and write go on the same data line
    devcfg.clock_speed_hz = 500 * 1000;
    devcfg.queue_size = 4; // allow a queue of 4?
    ESP_ERROR_CHECK(spi_bus_add_device(get_spi(this->spi_bus), &devcfg, &this->dev));
 
    write(data_command(DATA_WRITE, DATA_ADDR_AUTOINC)); // initialise our preferred data mode
}

void tm1638::write(uint8_t c, std::vector<uint8_t> data, size_t len){
    ESP_LOGI(TAG, "Write cmd(%02x), len(%d)", c, len);

    spi_transaction_t transaction = {};
    transaction.cmd = (uint8_t) c;
    transaction.length = len * 8; // bits!!!
    transaction.tx_buffer = (uint8_t *)data.data();

    ESP_ERROR_CHECK(spi_device_transmit(this->dev, &transaction));
    vTaskDelay(100);
}
void tm1638::write(uint8_t c){
    write(c, {}, 0);
}

std::vector<uint8_t> tm1638::read(uint8_t c, size_t len){
    ESP_LOGI(TAG, "Read cmd(%02x), len(%d)", c, len);
    std::vector<uint8_t> data(len);

    spi_transaction_t transaction = {};
    transaction.cmd = c;
    transaction.rxlength = len * 8;
    transaction.rx_buffer = (uint8_t *)data.data();

    ESP_ERROR_CHECK(spi_device_transmit(this->dev, &transaction));

    return data;
}

void tm1638::clear(){
    std::vector<uint8_t> buf(8);

    write(mem_command(0), buf, 8);
}

void tm1638::set(uint8_t segment, uint8_t value){
    ESP_LOGD(TAG, "Set command segment=%d, v=%x", segment, value);
    std::vector<uint8_t> buf = { value };
    write(data_command(DATA_WRITE, DATA_ADDR_FIXED));
    write(mem_command(segment), buf, buf.size());
}
uint16_t tm1638::get(){
    auto buf = read(data_command(DATA_READ_KEYS, DATA_ADDR_AUTOINC), 4);
    uint16_t keys = 0;

    // there's some weird mapping going on, we'll need to do more than this
    //for(int i = 0; i < buf.size(); i++){
    //    ESP_LOGI(TAG, "Keys %d -> %x", i, buf[i]);
    //    keys |= (buf[i] & 0x0F) << (i * 4);
    //}
    
    // row 1
    if(buf[0] & 0x20) keys |= 1 << 0;
    if(buf[0] & 0x02) keys |= 1 << 1;
    if(buf[1] & 0x20) keys |= 1 << 2;
    if(buf[1] & 0x02) keys |= 1 << 3;

    // row 2
    if(buf[2] & 0x20) keys |= 1 << 4;
    if(buf[2] & 0x02) keys |= 1 << 5;
    if(buf[3] & 0x20) keys |= 1 << 6;
    if(buf[3] & 0x02) keys |= 1 << 7;

    // row 3
    // 8 seems broken on this board
    if(buf[0] & 0x04) keys |= 1 << 9;
    if(buf[1] & 0x40) keys |= 1 << 10;
    if(buf[1] & 0x04) keys |= 1 << 11;

    // row 4
    if(buf[2] & 0x40) keys |= 1 << 12;
    if(buf[2] & 0x04) keys |= 1 << 13;
    if(buf[3] & 0x40) keys |= 1 << 14;
    if(buf[3] & 0x04) keys |= 1 << 15;

    return keys;
}

void tm1638::brightness(int b){
    ESP_LOGI(TAG, "Set brightness %d", b);
    CTRL c;
    switch(b){
        case 1: c = PULSE_1_16; break;
        case 2:
        case 3: 
                c = PULSE_2_16; break;
        case 4: 
        case 5: 
        case 6:
        case 7:
        case 8:
        case 9:
                c = PULSE_4_16; break;
        case 10: c = PULSE_10_16; break;
        case 11: c = PULSE_11_16; break;
        case 12: c = PULSE_12_16; break;
        case 13: c = PULSE_13_16; break;
        case 14: 
        case 15: 
                c = PULSE_14_16; break;
        default:
                c = DISPLAY_OFF;
    }

    if(c != DISPLAY_OFF){
        write(control_command(DISPLAY_ON) | control_command(c));
    } else {
        write(control_command(DISPLAY_OFF));
    }
}
void tm1638::enable(void){
    ESP_LOGI(TAG, "Enabling display");
    write(control_command(DISPLAY_ON) | control_command(PULSE_14_16));
}
void tm1638::disable(void){
    ESP_LOGI(TAG, "Disabling display");
    write(control_command(DISPLAY_OFF));
}

constexpr uint8_t map(char c){
    switch(c){
        case '0': return 0x3F;
        case '1': return 0x06;
        case '2': return 0x5B;
        case '3': return 0x4F;
        case '4': return 0x66;
        case '5': return 0x6D;
        case '6': return 0x7D;
        case '7': return 0x07;
        case '8': return 0x7F;
        case '9': return 0x6F;
    }
}

