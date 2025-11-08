#include "encrypt.h"

void encrypt_payload(uint8_t *data, uint8_t data_length, unsigned int frame_counter, uint8_t direction, uint8_t *dev_addr, uint8_t *AppSkey) {
    uint8_t i = 0x00;
    uint8_t j;
    uint8_t number_of_blocks = 0x00;
    uint8_t incomplete_block_size = 0x00;

    uint8_t block_A[16];

    // Calculate number of blocks
    number_of_blocks = data_length / 16;
    incomplete_block_size = data_length % 16;
    if (incomplete_block_size != 0) {
        number_of_blocks++;
    }

    for (i = 1; i <= number_of_blocks; i++) {
        block_A[0] = 0x01;
        block_A[1] = 0x00;
        block_A[2] = 0x00;
        block_A[3] = 0x00;
        block_A[4] = 0x00;

        block_A[5] = direction;

        block_A[6] = dev_addr[3];
        block_A[7] = dev_addr[2];
        block_A[8] = dev_addr[1];
        block_A[9] = dev_addr[0];

        block_A[10] = (frame_counter & 0x00FF);
        block_A[11] = ((frame_counter >> 8) & 0x00FF);

        block_A[12] = 0x00; // Frame counter upper Bytes
        block_A[13] = 0x00;

        block_A[14] = 0x00;

        block_A[15] = i;

        // Calculate S
        AES_encrypt(block_A, AppSkey); // original

        // Check for last block
        if (i != number_of_blocks) {
            for (j = 0; j < 16; j++) {
                *data = *data ^ block_A[j];
                data++;
            }
        } else {
            if (incomplete_block_size == 0) {
                incomplete_block_size = 16;
            }
            for (j = 0; j < incomplete_block_size; j++) {
                *data = *data ^ block_A[j];
                data++;
            }
        }
    }
}

void calculate_mic(uint8_t *data, uint8_t *final_MIC, uint8_t data_length, unsigned int frame_counter, uint8_t direction, uint8_t *dev_addr, uint8_t *NwkSkey) {
    uint8_t i;
    uint8_t block_B[16];

    uint8_t key_K1[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t key_K2[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    // uint8_t Data_Copy[16];

    uint8_t old_data[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t new_data[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

    uint8_t number_of_blocks = 0x00;
    uint8_t incomplete_block_size = 0x00;
    uint8_t block_counter = 0x01;

    // Create block_B
    block_B[0] = 0x49;
    block_B[1] = 0x00;
    block_B[2] = 0x00;
    block_B[3] = 0x00;
    block_B[4] = 0x00;

    block_B[5] = direction;

    block_B[6] = dev_addr[3];
    block_B[7] = dev_addr[2];
    block_B[8] = dev_addr[1];
    block_B[9] = dev_addr[0];

    block_B[10] = (frame_counter & 0x00FF);
    block_B[11] = ((frame_counter >> 8) & 0x00FF);

    block_B[12] = 0x00; // Frame counter upper bytes
    block_B[13] = 0x00;

    block_B[14] = 0x00;
    block_B[15] = data_length;

    // Calculate number of Blocks and blocksize of last block
    number_of_blocks = data_length / 16;
    incomplete_block_size = data_length % 16;

    if (incomplete_block_size != 0) {
        number_of_blocks++;
    }

    generate_keys(key_K1, key_K2, NwkSkey);

    // Perform Calculation on Block B0

    // Perform AES encryption
    AES_encrypt(block_B, NwkSkey);

    // Copy block_B to old_data
    for (i = 0; i < 16; i++) {
        old_data[i] = block_B[i];
    }

    // Perform full calculating until n-1 messsage blocks
    while (block_counter < number_of_blocks) {
        // Copy data into array
        for (i = 0; i < 16; i++) {
            new_data[i] = *data;
            data++;
        }

        // Perform xor with old data
        _xor(new_data, old_data);

        // Perform AES encryption
        AES_encrypt(new_data, NwkSkey);

        // Copy new_data to old_data
        for (i = 0; i < 16; i++) {
            old_data[i] = new_data[i];
        }

        // Raise Block counter
        block_counter++;
    }

    // Perform calculation on last block
    // Check if Datalength is a multiple of 16
    if (incomplete_block_size == 0) {
        // Copy last data into array
        for (i = 0; i < 16; i++) {
            new_data[i] = *data;
            data++;
        }

        // Perform xor with Key 1
        _xor(new_data, key_K1);

        // Perform xor with old data
        _xor(new_data, old_data);

        // Perform last AES routine
        // read NwkSkey from PROGMEM
        AES_encrypt(new_data, NwkSkey);
    } else {
        // Copy the remaining data and fill the rest
        for (i = 0; i < 16; i++) {
            if (i < incomplete_block_size) {
                new_data[i] = *data;
                data++;
            }
                if (i == incomplete_block_size) {
                new_data[i] = 0x80;
            }
                if (i > incomplete_block_size) {
                new_data[i] = 0x00;
            }
        }

        // Perform xor with Key 2
        _xor(new_data, key_K2);

        // Perform xor with Old data
        _xor(new_data, old_data);

        // Perform last AES routine
        AES_encrypt(new_data, NwkSkey);
    }

    final_MIC[0] = new_data[0];
    final_MIC[1] = new_data[1];
    final_MIC[2] = new_data[2];
    final_MIC[3] = new_data[3];
}

void generate_keys(uint8_t *K1, uint8_t *K2, uint8_t *NwkSkey) {
    uint8_t i;
    uint8_t MSB_Key;

    // Encrypt the zeros in K1 with the NwkSkey
    AES_encrypt(K1, NwkSkey);

    // Create K1
    // Check if MSB is 1
    if ((K1[0] & 0x80) == 0x80) {
        MSB_Key = 1;
    } else {
        MSB_Key = 0;
    }

    // Shift K1 one bit left
    shift_left(K1);

    // if MSB was 1
    if (MSB_Key == 1) {
        K1[15] = K1[15] ^ 0x87;
    }

    // Copy K1 to K2
    for (i = 0; i < 16; i++) {
        K2[i] = K1[i];
    }

    // Check if MSB is 1
    if ((K2[0] & 0x80) == 0x80) {
        MSB_Key = 1;
    } else {
        MSB_Key = 0;
    }

    // Shift K2 one bit left
    shift_left(K2);

    // Check if MSB was 1
    if (MSB_Key == 1) {
        K2[15] = K2[15] ^ 0x87;
    }
}

void shift_left(uint8_t *data) {
    uint8_t i;
    uint8_t overflow = 0;
    // uint8_t High_Byte, Low_Byte;

    for (i = 0; i < 16; i++) {
        // Check for overflow on next byte except for the last byte
        if (i < 15) {
            // Check if upper bit is one
            if ((data[i + 1] & 0x80) == 0x80) {
            overflow = 1;
            } else {
            overflow = 0;
            }
        } else {
            overflow = 0;
        }

        // Shift one left
        data[i] = (data[i] << 1) + overflow;
    }
}

void _xor(uint8_t *new_data, uint8_t *old_data) {
    uint8_t i;

    for (i = 0; i < 16; i++) {
        new_data[i] = new_data[i] ^ old_data[i];
    }
}

void AES_encrypt(uint8_t *data, uint8_t *key) {
    uint8_t row, column, round = 0;
    uint8_t round_key[16];
    uint8_t state[4][4];

    //  Copy input to state arry
    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            state[row][column] = data[row + (column << 2)];
        }
    }

    //  Copy key to round key
    memcpy(&round_key[0], &key[0], 16);

    //  Add round key
    AES_add_round_key(round_key, state);

    //  Perform 9 full rounds with mixed columns
    for (round = 1; round < 10; round++) {
        //  Perform Byte substitution with S table
        for (column = 0; column < 4; column++) {
            for (row = 0; row < 4; row++) {
                state[row][column] = AES_sub_byte(state[row][column]);
            }
        }

        //  Perform Row Shift
        AES_shift_rows(state);

        //  Mix columns
        AES_mix_columns(state);

        //  Calculate new round key
        AES_calculate_round_key(round, round_key);

        //  Add the round key to the Round_key
        AES_add_round_key(round_key, state);
    }

    //  Perform Byte substitution with S table whitout mix columns
    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            state[row][column] = AES_sub_byte(state[row][column]);
        }
    }

    //  Shift rows
    AES_shift_rows(state);

    //  Calculate new round key
    AES_calculate_round_key(round, round_key);

    //  Add round key
    AES_add_round_key(round_key, state);

    //  Copy the state into the data array
    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            data[row + (column << 2)] = state[row][column];
        }
    }
}

void AES_add_round_key(uint8_t *round_key, uint8_t (*state)[4]) {
    uint8_t row, column;

    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            state[row][column] ^= round_key[row + (column << 2)];
        }
    }
}

uint8_t AES_sub_byte(uint8_t byte) {
    return sTable[((byte >> 4) & 0x0F)][((byte >> 0) & 0x0F)];
}

void AES_shift_rows(uint8_t (*state)[4]) {
    uint8_t buffer;

    // Store firt byte in buffer
    buffer = state[1][0];
    // Shift all bytes
    state[1][0] = state[1][1];
    state[1][1] = state[1][2];
    state[1][2] = state[1][3];
    state[1][3] = buffer;

    buffer = state[2][0];
    state[2][0] = state[2][2];
    state[2][2] = buffer;
    buffer = state[2][1];
    state[2][1] = state[2][3];
    state[2][3] = buffer;

    buffer = state[3][3];
    state[3][3] = state[3][2];
    state[3][2] = state[3][1];
    state[3][1] = state[3][0];
    state[3][0] = buffer;
}

void AES_mix_columns(uint8_t (*state)[4]) {
    uint8_t row, column;
    uint8_t a[4], b[4];

    for (column = 0; column < 4; column++) {
        for (row = 0; row < 4; row++) {
            a[row] = state[row][column];
            b[row] = (state[row][column] << 1);

            if ((state[row][column] & 0x80) == 0x80) {
                b[row] ^= 0x1B;
            }
        }

        state[0][column] = b[0] ^ a[1] ^ b[1] ^ a[2] ^ a[3];
        state[1][column] = a[0] ^ b[1] ^ a[2] ^ b[2] ^ a[3];
        state[2][column] = a[0] ^ a[1] ^ b[2] ^ a[3] ^ b[3];
        state[3][column] = a[0] ^ b[0] ^ a[1] ^ a[2] ^ b[3];
    }
} 

void AES_calculate_round_key(uint8_t round, uint8_t *round_key){
    uint8_t i, j, b, rcon;
    uint8_t tmp[4];

    // Calculate Rcon
    rcon = 0x01;
    while (round != 1) {
        b = rcon & 0x80;
        rcon = rcon << 1;

        if (b == 0x80) {
            rcon ^= 0x1b;
        }
            round--;
    }

    //  Calculate first Temp
    //  Copy laste byte from previous key and subsitute the byte, but shift the
    //  array contents around by 1.
    tmp[0] = AES_sub_byte(round_key[12 + 1]);
    tmp[1] = AES_sub_byte(round_key[12 + 2]);
    tmp[2] = AES_sub_byte(round_key[12 + 3]);
    tmp[3] = AES_sub_byte(round_key[12 + 0]);

    //  xor with Rcon
    tmp[0] ^= rcon;

    //  Calculate new key
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            round_key[j + (i << 2)] ^= tmp[j];
            tmp[j] = round_key[j + (i << 2)];
        }
    }
}