#include "py/builtin.h"
#include "py/runtime.h"
#include "py/obj.h"
#include "sx1276radiodriver.h"


// info()
static mp_obj_t lora_info() {
    return MP_OBJ_NEW_SMALL_INT(43);
}
MP_DEFINE_CONST_FUN_OBJ_0(lora_info_obj, lora_info);


typedef struct _lora_obj_t {
    mp_obj_base_t base;
    //sx1276_state_t state;
} _lora_LoRa_obj_t;

// Constructor
static mp_obj_t lora_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {    
    _lora_LoRa_obj_t *self = mp_obj_malloc(_lora_LoRa_obj_t, type);
    esp_err_t esp_err = sx1276_init();
    if(esp_err != ESP_OK) mp_raise_ValueError(MP_ERROR_TEXT("Could not initialize RFM95"));
    //self->id = mp_obj_get_int(args[0]);
    return MP_OBJ_FROM_PTR(self);
}


// reset()
static mp_obj_t lora_LoRa_reset(mp_obj_t self_in) {
    _lora_LoRa_obj_t *self = MP_OBJ_TO_PTR(self_in);
    sx1276_reset();
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(lora_LoRa_reset_obj, lora_LoRa_reset);

static const mp_rom_map_elem_t lora_locals_dict_table[] = {
    // Instances
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&lora_LoRa_reset_obj) }
};
static MP_DEFINE_CONST_DICT(lora_locals_dict, lora_locals_dict_table);



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

static const mp_rom_map_elem_t lora_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lora) },
    { MP_ROM_QSTR(MP_QSTR_LoRa),    MP_ROM_PTR(&lora_type_LoRa) }, 
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&lora_info_obj) },
};
static MP_DEFINE_CONST_DICT(lora_module_globals, lora_module_globals_table);

const mp_obj_module_t module_lora = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&lora_module_globals,
}

MP_REGISTER_MODULE(MP_QSTR_lora, module_lora);

