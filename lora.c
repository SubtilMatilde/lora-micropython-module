#include "py/builtin.h"
#include "py/runtime.h"

// info()
static mp_obj_t py_lora_info(void) {
    return MP_OBJ_NEW_SMALL_INT(43);
}
MP_DEFINE_CONST_FUN_OBJ_0(lora_info_obj, py_lora_info);

static const mp_rom_map_elem_t mp_module_lora_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lora) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&lora_info_obj) },
};
static MP_DEFINE_CONST_DICT(mp_module_lora_globals, mp_module_lora_globals_table);

const mp_obj_module_t mp_module_lora = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_module_lora_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lora, mp_module_lora);
