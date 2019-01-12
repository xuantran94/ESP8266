PROGRAM=SmartHome
#ESPPORT ?= /dev/tty.SLAB_USBtoUART
#ESPPORT ?= /dev/tty.wchusbserial1410
ESPPORT ?= /dev/tty.SLAB_USBtoUART
#ESPPORT ?= /dev/tty.usbserial-A9M9DV3R
ESPBAUD ?= 921600
#ESPBAUD ?= 115200
FLASH_MODE ?= dout
EXTRA_COMPONENTS= /../../00_Lib/esp-open-rtos/extras/http_client_ota /../../00_Lib/esp-open-rtos/extras/mbedtls /../../00_Lib/esp-open-rtos/extras/paho_mqtt_c /../../00_Lib/esp-open-rtos/extras/rboot-ota /../../00_Lib/esp-open-rtos/extras/dhcpserver
#extras/paho_mqtt_c 

PROGRAM_SRC_DIR =. ./Lib/Json ./BSW/GPIO ./ASW/Network ./ASW/MQTT


include /Users/tranxuan/Documents/Workspace/00_Lib/esp-open-rtos/common.mk 

