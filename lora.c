#include "py/builtin.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "sx1276radiodriver.h"


// Just integer constants for parameters
#define LORA_MODE_LORA 0
#define LORA_MODE_LORAWAN 1
#define LORA_REGION_EU868 0


//Global
// info()
static mp_obj_t lora_info() {
    return MP_OBJ_NEW_SMALL_INT(43);
}
MP_DEFINE_CONST_FUN_OBJ_0(lora_info_obj, lora_info);


//Class
typedef struct _lora_obj_t {
    mp_obj_base_t base;
    uint8_t* value;
    int mode;
    int region;
    //sx1276_state_t state;
} _lora_LoRa_obj_t;


//// Constructor
static mp_obj_t lora_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {    
    _lora_LoRa_obj_t *self = mp_obj_malloc(_lora_LoRa_obj_t, type);
    esp_err_t esp_err = sx1276_init();
    if(esp_err != ESP_OK) mp_raise_ValueError(MP_ERROR_TEXT("Could not initialize RFM95"));
    //self->id = mp_obj_get_int(args[0]);
    return MP_OBJ_FROM_PTR(self);
}


//// LoRa.reset()
static mp_obj_t lora_LoRa_reset(mp_obj_t self_in) {
    _lora_LoRa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    esp_err_t esp_err = sx1276_reset();
    if(esp_err != ESP_OK) mp_raise_ValueError(MP_ERROR_TEXT("Could not initialize RFM95"));
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(lora_LoRa_reset_obj, lora_LoRa_reset);


static mp_obj_t lora_LoRa_tx(mp_obj_t self_in, mp_obj_t user_data) {
    mp_buffer_info_t buffer_info;
    mp_get_buffer_raise(user_data, &buffer_info, MP_BUFFER_READ);   // Extract bytes
    uint8_t *data = buffer_info.buf;
    size_t len = buffer_info.len;
    esp_err_t esp_err = sx1276_tx(data, len);
    if(esp_err != ESP_OK) mp_raise_ValueError(MP_ERROR_TEXT("Could not transmit any data"));
    return mp_const_true;
}
MP_DEFINE_CONST_FUN_OBJ_2(lora_LoRa_tx_obj, lora_LoRa_tx);



/*
static mp_obj_t lora_LoRa_rx(mp_obj_t self_in) {
    uint8_t data[256];
    size_t len = sizeof(data);
    esp_err_t esp_err = sx1276_rx_single(data, &len);
    if(esp_err != ESP_OK) {
        mp_raise_ValueError(MP_ERROR_TEXT("Could not receive any data"));
        return mp_const_none;
    }
    return mp_obj_new_bytes(data, len);
}
MP_DEFINE_CONST_FUN_OBJ_1(lora_LoRa_rx_obj, lora_LoRa_rx);
*/

////////////TESTS, DON'T UNCOMMENT
/*
// LoRa.read_register(addr)
static mp_obj_t lora_LoRa_read_register(mp_obj_t self_in, mp_obj_t addr_in) {
    _lora_LoRa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint8_t addr = mp_obj_get_int(addr_in);
    uint8_t value = sx1276_read_register(addr);
    return mp_obj_new_int(value);
}
MP_DEFINE_CONST_FUN_OBJ_2(lora_LoRa_readregister_obj, lora_LoRa_read_register);
*/

static mp_obj_t lora_LoRa_debugfrequency(mp_obj_t self_in) {
    _lora_LoRa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    uint32_t frequency = get_chip_frequency();
    return mp_obj_new_int(frequency);
}
MP_DEFINE_CONST_FUN_OBJ_1(lora_LoRa_debugfrequency_obj, lora_LoRa_debugfrequency);

// Methods, constants and statics from object LoRa
static const mp_rom_map_elem_t lora_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_modelora), MP_ROM_INT(LORA_MODE_LORAWAN) },
    { MP_ROM_QSTR(MP_QSTR_eu868), MP_ROM_INT(LORA_REGION_EU868) },

    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&lora_LoRa_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_tx), MP_ROM_PTR(&lora_LoRa_tx_obj) },
    //{ MP_ROM_QSTR(MP_QSTR_rx), MP_ROM_PTR(&lora_LoRa_rx_obj) },
    { MP_ROM_QSTR(MP_QSTR_debugfrequency), MP_ROM_PTR(&lora_LoRa_debugfrequency_obj) },
};
static MP_DEFINE_CONST_DICT(lora_locals_dict, lora_locals_dict_table);


// LoRa object definition
MP_DEFINE_CONST_OBJ_TYPE(
    lora_type_LoRa,
    MP_QSTR_LoRa,
    MP_TYPE_FLAG_NONE,
    make_new, lora_make_new,
    //print, lora_print,
    //call, lora_call,
    //protocol, &pin_pin_p,
    locals_dict, &lora_locals_dict
);

// GLOBAL
static const mp_rom_map_elem_t lora_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lora) },
    { MP_ROM_QSTR(MP_QSTR_LoRa),    MP_ROM_PTR(&lora_type_LoRa) }, 
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&lora_info_obj) },
};
static MP_DEFINE_CONST_DICT(lora_module_globals, lora_module_globals_table);

// lora module definition
const mp_obj_module_t module_lora = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&lora_module_globals,
}

MP_REGISTER_MODULE(MP_QSTR_lora, module_lora);

