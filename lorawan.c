#include "lorawan.h"

uint16_t frame_counter = 0;

void set_datarate(datarates_t datarate){
    uint8_t sf;
    uint8_t bw;
    uint8_t ldro;
    switch (datarate){
        case DR0:
            sf = SF_12;
            bw = BW_125K;
            ldro = 0x0C;
        break;
        case DR1:
            sf = SF_11;
            bw = BW_125K;
            ldro = 0x0C;
        break;
        case DR2:
            sf = SF_10;
            bw = BW_125K;
            ldro = 0x04;
        break;
        case DR3:
            sf = SF_9;
            bw = BW_125K;
            ldro = 0x04;
        break;
        case DR4:
            sf = SF_8;
            bw = BW_125K;
            ldro = 0x04;
        break;
        case DR5:
            sf = SF_7;
            bw = BW_125K;
            ldro = 0x04;
        break;
        case DR6:
            sf = SF_7;
            bw = BW_250K;
            ldro = 0x04;
        break;
        default:
            sf = 0x74;
            bw = 0x72;
            ldro = 0x04;
        break;
        
    }
    set_sf(sf);
    set_bw(bw);
}

esp_err_t lorawan_send(uint8_t *payload, size_t payload_length, auth_t auth, uint8_t frame_port){

    uint8_t direction = UPLINK;

    uint8_t lorawan_data[64];
    uint8_t packet_length;
    uint8_t mic[4];

    uint8_t frame_control = 0x00;

    uint8_t tmp_data[payload_length];

    memcpy(tmp_data, payload, payload_length);

    encrypt_payload(tmp_data, payload_length, frame_counter, direction, auth.dev_addr, auth.app_skey);


    lorawan_data[0] = MAC_HEADER;
    lorawan_data[1] = auth.dev_addr[3];
    lorawan_data[2] = auth.dev_addr[2];
    lorawan_data[3] = auth.dev_addr[1];
    lorawan_data[4] = auth.dev_addr[0];
    lorawan_data[5] = frame_counter;
    lorawan_data[6] = (frame_control & 0x00FF);
    lorawan_data[7] = ((frame_control >> 8) & 0x00FF);
    lorawan_data[8] = frame_port;

    


    packet_length = 9;
    memcpy(lorawan_data+packet_length, tmp_data, payload_length);

    // Add data Lenth to package length
    packet_length += payload_length;

    // Calculate MIC
    calculate_mic(lorawan_data, mic, packet_length, frame_counter, direction, auth.dev_addr, auth.nwk_skey);

    // Load MIC in package
    memcpy(lorawan_data+packet_length, mic, 4);

    // Add MIC length to RFM package length
    packet_length += 4;
    frame_counter++;

    set_datarate(DR5);
    set_sync_word(SYNC_WORD_PUBLIC);

    return sx1276_tx(lorawan_data, packet_length);
}