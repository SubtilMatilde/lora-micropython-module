#include "sx1276radiodriver.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hal/spi_types.h"

#define TAG "SX1276"

static spi_device_handle_t spi_handle;

struct spi_transaction_t spi;

const float FXOSC = 32000000.0;

const float FSTEP = (FXOSC / 524288);



static sx1276_state_t sx1276_state ={
    EU_868_FREQ,
    0x0008,
    SF_7,
    BW_125K,
    CR_45,
    CRC_ENABLE,
    14,
    PA_SELECT
};


//SX1276 functions
esp_err_t sx1276_init(){
    esp_log_level_set(TAG, ESP_LOG_INFO);
    // SPI config
    spi_bus_config_t buscfg = {
        .miso_io_num = SPI_MISO_PIN,
        .mosi_io_num = SPI_MOSI_PIN,
        .sclk_io_num = SPI_SCK_PIN,
        .quadwp_io_num = -1, //NOT USED
        .quadhd_io_num = -1, //NOT USED
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,   //CPOL = 0 & CPHA = 0  for SX1276
        .spics_io_num = SPI_CS_PIN,
        .queue_size = 1, // à verifier
    };
    esp_err_t ret_value;
     
    ret_value = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret_value != ESP_OK && ret_value != ESP_ERR_INVALID_STATE) return ret_value;

    ret_value = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret_value != ESP_OK) return ret_value;


    gpio_reset_pin(RST_PIN);
    gpio_set_direction(RST_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(DIO0_PIN);
    gpio_set_direction(DIO0_PIN, GPIO_MODE_INPUT);

    sx1276_reset();

    sx1276_write_register(REG_01_OP_MODE, MODE_SLEEP | MODE_LONG_RANGE);
    sx1276_write_register(REG_01_OP_MODE, MODE_STDBY | MODE_LONG_RANGE);

    //Set FIFO addresses for both tx and rx at 0
    sx1276_write_register(REG_0E_FIFO_TX_BASE_ADDR, 0x00);
    sx1276_write_register(REG_0F_FIFO_RX_BASE_ADDR, 0x00);

    //Default parameters : BW = 125K, CR = 4/5, SF = 7, CRC ENABLED, EXPLICIT HEADER
    // Also optimization for symbols longer than 16ms ??? needed ???
    sx1276_write_register(REG_1D_MODEM_CONFIG1, BW_125K | CR_45 | EXPLICIT_HEADER);
    sx1276_write_register(REG_1E_MODEM_CONFIG2, SF_7 | CRC_ENABLE);
    sx1276_write_register(REG_26_MODEM_CONFIG3, LDR_OPTIMIZE_ENABLE);

    //Preamble 8, it can go from 6 to 65535, so total 6+4 to 65535+4
    set_preamble(0x0008);
    
    //Frequency
    set_frequency(EU_868_FREQ);

    //TX Power
    //set_tx_power(14);
    //To Delete
    sx1276_write_register(REG_4D_PA_DAC, PA_DAC_HIGH_POWER | PA_SELECT); 
    sx1276_write_register(REG_09_PA_CONFIG, 0x8F);

    sx1276_write_register(REG_01_OP_MODE, MODE_SLEEP | MODE_LONG_RANGE);

    ESP_LOGI(TAG, "SPI initialized !");
    return ESP_OK;

}

void sx1276_reset(){
    ESP_LOGI(TAG, "Reset SX1276 ...");
    gpio_set_level(RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "SX1276 reset done !");
    uint8_t reg_version = sx1276_read_register(0x42);
    ESP_LOGI(TAG, "SX1276 reset done ! Version = %d", reg_version);
}

void sx1276_write_register(uint8_t register_address, uint8_t value) {
    uint8_t tx[2] = {(uint8_t)(register_address | 0x80), value};
    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx,
        .flags = 0
    };
    spi_device_polling_transmit(spi_handle, &t);
}

uint8_t sx1276_read_register(uint8_t register_address) {
    uint8_t tx[2] = {(uint8_t)(register_address & 0x7F), 0x00};
    uint8_t rx[2] = {0};
    spi_transaction_t t = {
        .length = 8 * 2, //2 bytes 
        .tx_buffer = tx,
        .rx_buffer = rx,
        .flags = 0
    };
    spi_device_polling_transmit(spi_handle, &t);
    return rx[1];
}

void sx1276_write_fifo(uint8_t* data, size_t length){
    if(length > 255) length = 255; // FIFO payload limit

    uint8_t fifo_array[FIFO_MAX_SIZE]; //stack better than heap
    fifo_array[0] = REG_00_FIFO | 0x80;
    memcpy(&fifo_array[1], data, length);
    spi_transaction_t t = {
        .length = (length + 1) * 8, //because of address
        .tx_buffer = fifo_array,
        .flags = 0
    };

    spi_device_polling_transmit(spi_handle, &t);
}


void sx1276_read_fifo(uint8_t *data, size_t length){
    if (length > 255) length = 255; 

    uint8_t address[] = {REG_00_FIFO & 0x7F};
    uint8_t fifo_data[FIFO_MAX_SIZE];      

    spi_transaction_t t = {
        .length = (length + 1) * 8,    
        .tx_buffer = address,  // send address first
        .rx_buffer = fifo_data, // receive data
        .flags = 0            
    };

    spi_device_polling_transmit(spi_handle, &t);

    memcpy(data, &fifo_data[1], length);
}


// TX AND RX IMPLEMENTATION OF STATE MACHINE, SEE DOC
void sx1276_tx(uint8_t *payload, size_t payload_length){
    sx1276_write_register(REG_01_OP_MODE, MODE_STDBY | MODE_LONG_RANGE);
    sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
    sx1276_write_register(REG_0D_FIFO_ADDR_PTR, 0x00);
    sx1276_write_fifo(payload, payload_length);
    sx1276_write_register(REG_40_DIO_MAPPING1, 0x40);
    sx1276_write_register(REG_01_OP_MODE, MODE_TX | MODE_LONG_RANGE);
    uint8_t irq = sx1276_read_register(REG_12_IRQ_FLAGS);
}

void sx1276_rx_single(){
    
}

void sx1276_rx_continuous(){
    
}

// Getters and setters
void set_frequency(uint32_t freq_Hz){
    if(freq_Hz > EU_FREQ_UPPER_BOUND) freq_Hz = EU_FREQ_UPPER_BOUND;
    else if(freq_Hz < EU_FREQ_LOWER_BOUND) freq_Hz = EU_FREQ_LOWER_BOUND;
    uint32_t frf = (uint32_t)((float)freq_Hz / FSTEP);
    sx1276_write_register(REG_06_FRF_MSB, (frf >> 16) & 0xFF);
    sx1276_write_register(REG_07_FRF_MID, (frf >> 8) & 0xFF);
    sx1276_write_register(REG_08_FRF_LSB, frf & 0xFF);
}

uint32_t get_chip_frequency(){
    uint8_t msb = sx1276_read_register(REG_06_FRF_MSB);
    uint8_t mid = sx1276_read_register(REG_07_FRF_MID);
    uint8_t lsb = sx1276_read_register(REG_08_FRF_LSB);
    uint32_t frf = (uint32_t)((msb << 16) | (mid << 8) | lsb);
    float freq_Hz = frf * FSTEP;
    return freq_Hz;
}

uint32_t get_frequency(){
    return sx1276_state.frequency;
}

void set_preamble(uint16_t preamble){
    if(preamble < 0x0006) preamble = 0x0006;
    sx1276_write_register(REG_20_PREAMBLE_MSB, (preamble >> 8) & 0xFF);
    sx1276_write_register(REG_21_PREAMBLE_LSB, preamble & 0xFF);
}

unsigned int get_chip_preamble(){
    uint8_t msb = sx1276_read_register(REG_20_PREAMBLE_MSB);
    uint8_t lsb = sx1276_read_register(REG_21_PREAMBLE_LSB);
    uint16_t preamble = (uint16_t)((msb << 8) | lsb);
    return preamble;
}

unsigned int get_preamble(){
    return sx1276_state.preamble;
}
/*
//Table 33 Power Amplifier Mode Selection Truth Table (SX1276 doc)
void set_tx_power(uint8_t dbm_power){
    uint8_t pa_cfg = 0x00;
    uint8_t pa_dac = PA_DAC_DEFAULT;
    if(dbm_power > 0x0E) pa_cfg = PA_SELECT; //Higher than 14 -> SELECT PA BOOST
    
    if(dbm_power > 0x14) dbm_power = 0x14; //Max power = 20
    else if(dbm_power < 0x02) dbm_power = 0x02; //Min Power
}

*/

