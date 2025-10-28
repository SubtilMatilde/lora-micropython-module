#ifndef SX1276_RADIO_DRIVER_H
#define SX1276_RADIO_DRIVER_H

#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <stdint.h>
#include <stdbool.h>

// Pin configuration
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 27
#define SPI_SCK_PIN   5
#define SPI_CS_PIN   17
#define RST_PIN      18
#define DIO0_PIN     23

// Register addresses
#define REG_00_FIFO              0x00
#define REG_01_OP_MODE           0x01
#define REG_06_FRF_MSB           0x06
#define REG_07_FRF_MID           0x07
#define REG_08_FRF_LSB           0x08
#define REG_09_PA_CONFIG         0x09
#define REG_0D_FIFO_ADDR_PTR     0x0D
#define REG_0E_FIFO_TX_BASE_ADDR 0x0E
#define REG_0F_FIFO_RX_BASE_ADDR 0x0F
#define REG_12_IRQ_FLAGS         0x12
#define REG_1D_MODEM_CONFIG1     0x1D
#define REG_1E_MODEM_CONFIG2     0x1E
#define REG_20_PREAMBLE_MSB      0x20
#define REG_21_PREAMBLE_LSB      0x21
#define REG_26_MODEM_CONFIG3     0x26
#define REG_40_DIO_MAPPING1      0x40
#define REG_42_VERSION           0x42
#define REG_4D_PA_DAC            0x4D


// Modes
#define MODE_LONG_RANGE    0x80
#define MODE_SLEEP         0x00
#define MODE_STDBY         0x01
#define MODE_TX            0x03
#define MODE_RX_CONTINUOUS 0x05
#define MODE_RX_SINGLE     0x06

// DIO Mapping -> DIO0 only
// RXDONE = 00
//TXDONE = 01
//CADDONE = 10
// IRQ flags
#define IRQ_TX_DONE      0x08
#define IRQ_RX_DONE      0x40
#define IRQ_CAD_DONE     0x04
#define IRQ_CAD_DETECTED 0x01

// Power Amplifier
#define PA_SELECT         0x80
#define PA_DAC_HIGH_POWER 0x07
#define PA_DAC_DEFAULT    0x04
#define MAX_POWER         0x70

// Bandwidth
#define BW_7K8   0x00
#define BW_10K4  0x10
#define BW_15K6  0x20   //0b00100000
#define BW_20K8  0x30   //0b00110000
#define BW_31K25 0x40   //0b01000000
#define BW_41K7  0x50   //0b01010000
#define BW_62K5  0x60   //0b01100000
#define BW_125K  0x70   //0b01110000
#define BW_250K  0x80   //0b10000000
#define BW_500K  0x90   //0b10010000

// Coding Rate
#define CR_45 0x02      //0b00000010
#define CR_46 0x04      //0b00000100
#define CR_47 0x06      //0b00000110
#define CR_48 0x08      //0b00001000

// Header
#define EXPLICIT_HEADER 0x00  //0b00000000
#define IMPLICIT_HEADER 0x01  //0b00000001

// SF
#define SF_6  0x60    //0b01100000
#define SF_7  0x70    //0b01110000
#define SF_8  0x80    //0b10000000
#define SF_9  0x90    //0b10010000
#define SF_10 0xA0    //0b10100000
#define SF_11 0xB0    //0b10110000
#define SF_12 0xC0    //0b11000000

//CRC ON-OFF
#define CRC_DISABLE 0x00 //0b00000000
#define CRC_ENABLE  0x04 //0b00000100

//Low Data Rate Optimize
#define LDR_OPTIMIZE_DISABLE 0x00
#define LDR_OPTIMIZE_ENABLE  0x04

// Others
#define FIFO_MAX_SIZE 256

#define EU_FREQ_UPPER_BOUND 0x33DB2580 //870MHz
#define EU_FREQ_LOWER_BOUND 0x337055c0 //863MHz
#define EU_868_FREQ         0x33bca100 //868MHz
#define EU_868_1            0x33be27a0 //868.1MHz
#define EU_868_3            0x33c134e0 //868.3MHz
#define EU_868_5            0x33c44220 //868.5MHz

#define MIN_PREAMBLE 0x0006
#define MAX_PREAMBLE 0xFFFF //65535


// Useful Struct to hold info
typedef struct {
    uint32_t frequency;
    uint16_t preamble;
    uint8_t spreading_factor;
    uint8_t bandwidth;
    uint8_t coding_rate;
    uint8_t crc;
    uint8_t  power;
    uint8_t pa_select;
} sx1276_state_t;

// Functions' Prototypes
esp_err_t sx1276_init();
void sx1276_reset();
uint8_t sx1276_read_register(uint8_t register_address);
void sx1276_write_register(uint8_t register_address, uint8_t value);
void sx1276_write_fifo(uint8_t* data, size_t length);
void sx1276_read_fifo(uint8_t* data, size_t length);
void set_frequency(uint32_t freq_Hz);
uint32_t get_frequency();
uint32_t get_chip_frequency();
void set_preamble(uint16_t preamble);
unsigned int get_preamble();
unsigned int get_chip_preamble();
#endif