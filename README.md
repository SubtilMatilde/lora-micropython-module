# lora-micropython-module
This repository was created for my master's thesis.
It implements LoRa and LoRaWAN (ABP only) for a generic ESP32 board and RFM95 transceiver.
The encryption is inspired by Adafruit's TinyLora @ [Link Text](https://github.com/adafruit/TinyLoRa)
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
├─ esp-idf/
├─ micropython/
└─ modules/
   ├─ micropython.cmake
   ├─ README.md
   └─ lora-micropython-module/
      ├─ encrypt.c
      ├─ encrypt.h
      ├─ lorawan.c
      ├─ lorawan.h
      ├─ micropython.cmake
      ├─ micropython.mk   
      ├─ modlora.c
      ├─ README.md
      ├─ sx1276radiodriver.c
      └─ sx1276radiodriver.h
```

In the modules, add a micropython.cmake file and copy paste this
```
include(${CMAKE_CURRENT_LIST_DIR}/lora-micropython-module/micropython.cmake)
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

## Usage
### LoRaPHY sender
```python
from lora import *
from time import sleep
# default f = 868.1MHz, SF = 7, BW = 125kHz
l = LoRa(mode=LoRa.LORA)
l.frequency(868000000)
l.sf(8)
l.bw(LoRa.BW_250k)
while True:
    l.send(b'hello')
    sleep(5)
```

### LoRaPHY receiver
```python
from lora import *
from time import sleep
l = LoRa(mode=LoRa.LORA)
while True:
    v = l.recv()
    if v is not None:
        print(v)
```

### ABP
```python
from lora import *
import ubinascii

l = LoRa(mode=LoRa.LORAWAN, region=LoRa.EU868)

# Set your own device address and session keys please
dev_addr = ubinascii.unhexlify('260BDEA8')
nwk_skey = ubinascii.unhexlify('35EEAB94F077775CC20874D0462E89CC')
app_skey = ubinascii.unhexlify('11FE4C6CBAB7D2851FCD09BD9ED9E266')

l.authenticate(dev_addr=dev_addr, nwk_skey=nwk_skey, app_skey=app_skey)

l.send('hello')
```