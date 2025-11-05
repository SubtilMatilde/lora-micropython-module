# lora-micropython-module

## How to build 
Clone MicroPython and ESP-IDF
```bash
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git
git clone --recursive https://github.com/micropython/micropython.git
```
Clone this repo in a folder named "modules"
The "modules" folder and MicroPython have to be in same location


### In the same console:
```bash
cd esp-idf
./install.sh esp32 #Once
source export.sh from espidf # Every session
```
Now go to the esp32 port repository
```
cd micropython/ports/esp32 folder
idf.py build
make USER_C_MODULES=../../../../modules/micropython.cmake
idf.py build -DUSER_C_MODULES=../../../../modules/micropython.cmake
idf.py flash
```