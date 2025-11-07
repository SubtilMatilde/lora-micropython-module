#include "sx1276radiodriver.h"



static spi_device_handle_t spi_handle;

static sx1276_state_t sx1276_state;



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
        .max_transfer_sz = 260,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1000000,
        .mode = 0,   //CPOL = 0 & CPHA = 0  for SX1276
        .spics_io_num = SPI_CS_PIN,
        .queue_size = 1, // à verifier
        .flags = 0,
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


    sx1276_state_t sx1276_state = {
        .instanciated = 0x01,
        .frequency = EU_868_FREQ,
        .preamble = 0x0008,
        .symb_timeout = 0x0010,
        .spreading_factor = SF_7,
        .bandwidth = BW_125K,
        .coding_rate = CR_45,
        .crc = CRC_ENABLE,
        .power = 14,
        .pa_select = PA_SELECT,
        .sync_word = SYNC_WORD_PRIVATE
    };

    sx1276_write_register(REG_01_OP_MODE, MODE_SLEEP | MODE_LONG_RANGE);
    sx1276_write_register(REG_01_OP_MODE, MODE_STDBY | MODE_LONG_RANGE);

    //Set FIFO addresses for both tx and rx at 0
    sx1276_write_register(REG_0E_FIFO_TX_BASE_ADDR, 0x00);
    sx1276_write_register(REG_0F_FIFO_RX_BASE_ADDR, 0x00);

    //Default parameters : BW = 125K, CR = 4/5, SF = 7, CRC ENABLED, EXPLICIT HEADER
    // Also optimization for symbols longer than 16ms ??? needed ???
    sx1276_write_register(REG_1D_MODEM_CONFIG1, BW_125K | CR_45 | EXPLICIT_HEADER);
    sx1276_write_register(REG_1E_MODEM_CONFIG2, SF_7 | CRC_ENABLE);
    sx1276_write_register(REG_26_MODEM_CONFIG3, LDR_OPTIMIZE_ENABLE | AGC_LNA);

    //Preamble 8, it can go from 6 to 65535, so total 6+4 to 65535+4
    set_preamble(0x0008);
    
    //Frequency
    set_frequency(EU_868_1);

    //Sync Word
    sx1276_write_register(REG_39_SYNC_WORD, SYNC_WORD_PRIVATE);

    //TX Power
    //set_tx_power(14);

    sx1276_write_register(REG_4D_PA_DAC, PA_HIGH_POWER | PA_SELECT); 
    sx1276_write_register(REG_09_PA_CONFIG, 0x8F);

    //Gain
    sx1276_write_register(REG_0C_LNA, LNA_GAIN_G1);

    sx1276_write_register(REG_01_OP_MODE, MODE_SLEEP | MODE_LONG_RANGE);

    ESP_LOGI(TAG, "SPI initialized !");
    return ESP_OK;

}

esp_err_t sx1276_reset(){
    ESP_LOGI(TAG, "Reset SX1276 ...");
    gpio_set_level(RST_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(RST_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    ESP_LOGI(TAG, "SX1276 reset done !");
    uint8_t reg_version = sx1276_read_register(0x42);
    ESP_LOGI(TAG, "SX1276 reset done ! Version = %d", reg_version);
    return ESP_OK;
}

esp_err_t sx1276_write_register(uint8_t register_address, uint8_t value) {
    uint8_t tx[2] = {(uint8_t)(register_address | 0x80), value};
    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx,
        .flags = 0
    };
    spi_device_polling_transmit(spi_handle, &t);
    return ESP_OK;
}

uint8_t sx1276_read_register(uint8_t register_address) {
    uint8_t tx[2] = {(uint8_t)(register_address & 0x7F), 0x00};
    uint8_t rx[2] = {0};
    spi_transaction_t t = {
        .length = 8 * 2,
        .tx_buffer = tx,
        .rx_buffer = rx,
        .flags = 0,
    };
    spi_device_polling_transmit(spi_handle, &t);
    return rx[1];
}



void sx1276_write_fifo(uint8_t *data, size_t length) {

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
esp_err_t sx1276_tx(uint8_t *payload, size_t payload_length){
    sx1276_write_register(REG_01_OP_MODE, MODE_STDBY | MODE_LONG_RANGE);
    sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
    sx1276_write_register(REG_0D_FIFO_ADDR_PTR, 0x00);
    sx1276_write_register(REG_22_PAYLOAD_LENGTH, payload_length);
    sx1276_write_fifo(payload, payload_length);
    sx1276_write_register(REG_40_DIO_MAPPING1, DIO_TX_DONE);
    sx1276_write_register(REG_01_OP_MODE, MODE_TX | MODE_LONG_RANGE);
    
    uint8_t irq_flags = 0;
    uint32_t start_time = xTaskGetTickCount();

    while(1){
        irq_flags = sx1276_read_register(REG_12_IRQ_FLAGS);
        if (irq_flags & IRQ_TX_DONE){
            sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
            ESP_LOGI(TAG, "TX done (polling mode)");
            return ESP_OK;
        }
        if ((xTaskGetTickCount() - start_time) > pdMS_TO_TICKS(50000)){
            ESP_LOGE(TAG, "TX timeout");
            sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}


esp_err_t sx1276_rx_single(uint8_t *payload, size_t* payload_length){
    sx1276_write_register(REG_01_OP_MODE, MODE_STDBY | MODE_LONG_RANGE);
    sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
    sx1276_write_register(REG_0D_FIFO_ADDR_PTR, 0x00);
    sx1276_write_register(REG_40_DIO_MAPPING1, DIO_RX_DONE);
    sx1276_write_register(REG_01_OP_MODE, MODE_RX_SINGLE | MODE_LONG_RANGE);

    uint32_t start_time = xTaskGetTickCount();
    uint8_t irq_flags = 0;

    while(1){
        irq_flags = sx1276_read_register(REG_12_IRQ_FLAGS);
        if(irq_flags & IRQ_RX_DONE){

            // Check for CRC error
            if(irq_flags & IRQ_PAYLOAD_CRC_ERROR){
                sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
                ESP_LOGI(TAG, "RX CRC error");
                return ESP_ERR_INVALID_CRC;
            }

            // Read number of received bytes
            //if(payload_length > 64) payload_length = 64;
            size_t len = sx1276_read_register(REG_13_RX_NB_BYTES);
            //if(len > payload_length) len = payload_length; // avoid overflow
            *payload_length = len;

            // Set FIFO address to start of received packet
            uint8_t fifo_addr = sx1276_read_register(REG_10_FIFO_RX_CURRENT_ADDR);
            sx1276_write_register(REG_0D_FIFO_ADDR_PTR, fifo_addr);

            // Read payload from FIFO
            sx1276_read_fifo(payload, *payload_length);

            // Clear IRQ flags
            sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);

            ESP_LOGI(TAG, "RX done (polling mode), %d bytes received", len);
            return ESP_OK;
        }

        if (irq_flags & IRQ_RX_TIMEOUT) {
            sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
            ESP_LOGI(TAG, "RX symbol timeout");
            return ESP_ERR_TIMEOUT;
        }

        // Timeout
        if ((xTaskGetTickCount() - start_time) > pdMS_TO_TICKS(10000)){
            sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
            ESP_LOGE(TAG, "RX timeout");
            return ESP_ERR_TIMEOUT;
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}


/*
void sx1276_rx_continuous(uint8_t *payload, size_t* payload_length){
    sx1276_write_register(REG_01_OP_MODE, MODE_STDBY | MODE_LONG_RANGE);
    sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
    sx1276_write_register(REG_0D_FIFO_ADDR_PTR, 0x00);
    sx1276_write_register(REG_40_DIO_MAPPING1, DIO_RX_DONE);
    sx1276_write_register(REG_01_OP_MODE, MODE_RX_CONTINUOUS | MODE_LONG_RANGE);

    uint32_t start_time = xTaskGetTickCount();
    uint8_t irq_flags = 0;

    while(1){
        irq_flags = sx1276_read_register(REG_12_IRQ_FLAGS);
        if(irq_flags & IRQ_RX_DONE){

            // Check for CRC error
            if(irq_flags & IRQ_PAYLOAD_CRC_ERROR){
                sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);
            }

            // Read number of received bytes
            //if(payload_length > 64) payload_length = 64;
            size_t len = sx1276_read_register(REG_13_RX_NB_BYTES);
            //if(len > payload_length) len = payload_length; // avoid overflow
            *payload_length = len;

            // Set FIFO address to start of received packet
            uint8_t fifo_addr = sx1276_read_register(REG_10_FIFO_RX_CURRENT_ADDR);
            sx1276_write_register(REG_0D_FIFO_ADDR_PTR, fifo_addr);

            // Read payload from FIFO
            sx1276_read_fifo(payload, *payload_length);

            // Clear IRQ flags
            sx1276_write_register(REG_12_IRQ_FLAGS, 0xFF);

            //ESP_LOGI(TAG, "RX done (polling mode), %d bytes received", len);
        }
    }
}
*/


// Getters and setters
void set_frequency(uint32_t freq_Hz) {
    if (freq_Hz > EU_FREQ_UPPER_BOUND) freq_Hz = EU_FREQ_UPPER_BOUND;
    else if (freq_Hz < EU_FREQ_LOWER_BOUND) freq_Hz = EU_FREQ_LOWER_BOUND;

    uint32_t frf = (uint32_t)((double) freq_Hz / (double) FREQ_STEP);
    sx1276_write_register(REG_06_FRF_MSB, (uint8_t) ((frf >> 16) & 0xFF));
    sx1276_write_register(REG_07_FRF_MID, (uint8_t) ((frf >> 8) & 0xFF));
    sx1276_write_register(REG_08_FRF_LSB, (uint8_t) (frf & 0xFF));
    sx1276_state.frequency = freq_Hz;
}


uint32_t get_chip_frequency(){
    uint8_t msb = sx1276_read_register(REG_06_FRF_MSB);
    uint8_t mid = sx1276_read_register(REG_07_FRF_MID);
    uint8_t lsb = sx1276_read_register(REG_08_FRF_LSB);

    uint32_t frf = ((uint32_t)msb << 16) | ((uint32_t)mid << 8) | lsb;

    return frf * FREQ_STEP;
}

uint32_t get_frequency(){
    return sx1276_state.frequency;
}

void set_preamble(uint16_t preamble){
    if(preamble < 0x0006) preamble = 0x0006;
    sx1276_write_register(REG_20_PREAMBLE_MSB, (preamble >> 8) & 0xFF);
    sx1276_write_register(REG_21_PREAMBLE_LSB, preamble & 0xFF);
    uint16_t symb_timeout = preamble + 100   ;

    // limit of 10-bit => max value = 1023
    if (symb_timeout > 1023) symb_timeout = 1023;

    uint8_t modem_config_2 = sx1276_read_register(REG_1E_MODEM_CONFIG2);
    modem_config_2 = (modem_config_2 & 0xFC) | ((symb_timeout >> 8) & 0x03);
    sx1276_write_register(REG_1E_MODEM_CONFIG2, modem_config_2);
    sx1276_write_register(REG_1F_SYMB_TIMEOUT_LSB, symb_timeout & 0xFF);
    sx1276_state.preamble = preamble;
    sx1276_state.symb_timeout = symb_timeout;

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
void sx1276_config_ocp() {
    //TODO
}
*/

/*
//Table 33 Power Amplifier Mode Selection Truth Table (SX1276 doc)
void set_tx_power(int8_t power_dbm) {

    uint8_t pa_config = 0;
    uint8_t pa_dac = 0;
    uint8_t p_out = 0;
    // Select PA_BOOST for power >= 15 dBm, else -> RFO
    bool usePaBoost = (power_dbm > 14);

    if(usePaBoost){
        // PaSelect = PA_BOOST
        pa_config = PA_SELECT; 

        // Values between 2 and 20 in PA_BOOST
        if (power_dbm > 20) power_dbm = 20;
        if (power_dbm < 2)  power_dbm = 2;

        // High (+20dBm)
        if (power_dbm > 17) {
            pa_dac = PA_HIGH_POWER; 
            // Pout = 20 - (15 - OutputPower) = -5 + OutputPower
            pa_config |= (uint8_t)((power_dbm - 5) & 0x0F); 
        } else {
            // Default (max 17 dBm)
            pa_dac = PA_DEFAULT_POWER;
            // Pout = 17 - (15 - OutputPower) = 2 + OutputPower
            pa_config |= (uint8_t)((power_dbm - 2) & 0x0F); 
        }
    }
    else{
        pa_config |= (1 << 4) * PA_MAX_POWER; // MaxPower = 0b111 -> Pmax = 15 dBm
        pa_dac = PA_LOW_POWER;

        // RFO range -1 dBm → +14 dBm
        if (power_dbm > 14) power_dbm = 14;
        if (power_dbm < -1) power_dbm = -1;

        pa_config |= (uint8_t)((power_dbm + 1) & 0x0F);
    }

    sx1276_write_register(REG_4D_PA_DAC, pa_dac);
    sx1276_write_register(REG_09_PA_CONFIG, pa_config);
    sx1276_state.power = power_dbm;
}
*/


// DATA !!! check pycom doc for stat


esp_err_t sx1276_deinit(){
    if (spi_handle) {
        spi_bus_remove_device(spi_handle);
        spi_handle = NULL;
    }

    esp_err_t ret = spi_bus_free(SPI2_HOST);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to free SPI bus: %d", ret);
    }
    sx1276_reset();
    return ESP_OK;
}



