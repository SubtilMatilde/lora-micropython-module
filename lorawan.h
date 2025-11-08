#ifndef LORAWAN_H
#define LORAWAN_H

#include <stdint.h>
#include "sx1276radiodriver.h"
#include "encrypt.h"


#define UPLINK 0x00
#define MAC_HEADER 0x40

typedef enum datarates{
    DR0,
    DR1,
    DR2,
    DR3,
    DR4,
    DR5,
    DR6,
}datarates_t;

typedef struct auth{
    uint8_t dev_addr[4];
    uint8_t nwk_skey[16];
    uint8_t app_skey[16];
}auth_t;

esp_err_t lorawan_send(uint8_t *payload, size_t payload_length, auth_t auth, uint8_t frame_port);


#endif // LORAWAN_H