MKFILE_PATH := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIR := $(patsubst %/,%,$(dir ${MKFILE_PATH}))
BUILD_DIR := ${CURRENT_DIR}/tmp

SRC:=RemotePowerAndSerial.ino
BIN:=tmp/${SRC}.bin

BASE:=/usr/share/arduino
USER_BASE:=$(HOME)/.arduino15
USER_LIBS:=$(HOME)/Arduino/libraries
BOARD:=esp8266:esp8266:generic:CpuFrequency=80,ResetMethod=none,CrystalFreq=26,FlashFreq=40,FlashMode=dio,FlashSize=1M64,led=2,LwIPVariant=v2mss536,Debug=Disabled,DebugLevel=None____,FlashErase=none,UploadSpeed=115200
HARDWARE:=-hardware ${BASE}/hardware -hardware ${USER_BASE}/packages 
TOOLS:=-tools ${BASE}/tools-builder -tools ${USER_BASE}/packages
LIBRARIES=-built-in-libraries ${BASE}/lib
LIBRARIES+=-libraries ${USER_LIBS}  # Where U8g2 comes from
WARNINGS:=-warnings all -logger human

ARDUINO_BUILDER_OPTS=${HARDWARE} ${TOOLS} ${LIBRARIES}
ARDUINO_BUILDER_OPTS+=-fqbn=${BOARD} ${WARNINGS}
ARDUINO_BUILDER_OPTS+=-verbose -build-path ${BUILD_DIR} 

all:
	@mkdir -p ${BUILD_DIR}
	arduino-builder -compile ${ARDUINO_BUILDER_OPTS} ${SRC} 2>&1 | tee build.log

clean:
	rm -rf ${BUILD_DIR} build.log

upload:	all
	sudo ${USER_BASE}/packages/esp8266/tools/esptool/0.4.13/esptool -vv -cd none -cb 115200 -cp /dev/ttyUSB0 -ca 0x00000 -cf ${BIN}
