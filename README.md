# lora-micropython-module


## How to build 

you have to be in the folder micropython/ports/esp32
and you need to source install.sh from espidf
```
idf.py build
idf.py build -DUSER_C_MODULES=<MODULE_DIR_PATH>  (idf.py build -DUSER_C_MODULES=../../../modules)
idf.py flash
```