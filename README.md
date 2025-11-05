# lora-micropython-module

## Cloning and project structure
Clone MicroPython and ESP-IDF
```bash
git clone -b v5.5.1 --recursive https://github.com/espressif/esp-idf.git
git clone --recursive https://github.com/micropython/micropython.git
```
Create a folder named modules next to the micropython folder and clone this repo into it.
You should have this structure
```
project/
├─ micropython/
└─ modules/
   ├─ micropython.cmake
   ├─ README.md
   └─ lora-micropython-module/
      ├─ lora.c
      ├─ micropython.cmake
      ├─ micropython.mk
      ├─ README.md
      ├─ sx1276radiodriver.c
      └─ sx1276radiodriver.h
```
## How to build 
In the same console:
```bash
cd esp-idf
./install.sh esp32 #Once
source export.sh # Every session
```
Now go to the esp32 port repository
```
cd micropython/ports/esp32 folder
idf.py build
make USER_C_MODULES=../../../../modules/micropython.cmake
idf.py build -DUSER_C_MODULES=../../../../modules/micropython.cmake
idf.py flash
```