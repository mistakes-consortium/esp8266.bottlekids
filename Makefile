TARGET_OUT:=image.elf
FW_FILE_1:=$(TARGET_OUT)-0x00000.bin
FW_FILE_2:=$(TARGET_OUT)-0x40000.bin

all : clean build

build : $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)

OBJS:=driver/uart.o\
	user/user_main.o

SRCS:=driver/uart.c \
	user/user_main.c \
	
FTDI:=/dev/ttyUSB0

# begin section for using esp open sdk
# this references a symlink the user should create to a standalone-built esp-open-sdk
ESP_OPEN_SDK:=esp-open-sdk
SDK:=$(ESP_OPEN_SDK)/sdk
GCC_FOLDER:=$(ESP_OPEN_SDK)/xtensa-lx106-elf
ESPTOOL_PY:=$(ESP_OPEN_SDK)/esptool/esptool.py
# end section for using esp open sdk

# begin section for using sdk 0.9.5_b1 installed based on https://github.com/esp8266/esp8266-wiki/wiki/Toolchain
#GCC_FOLDER:=/opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf
#ESPTOOL_PY:=/opt/Espressif/esptool-py/esptool.py
#SDK:=/opt/Espressif/esp_iot_sdk_v0.9.5_b1
# end section for manual install

# derived locations
XTGCCLIB:=$(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/4.8.2/libgcc.a
PREFIX:=$(GCC_FOLDER)/bin/xtensa-lx106-elf-
CC:=$(PREFIX)gcc
XTLIB:=$(SDK)/lib

CFLAGS:=-mlongcalls -I$(SDK)/include -Imyclib -Iinclude -Idriver -Iuser -Os -I$(SDK)/include/

LDFLAGS_CORE:=\
	-nostdlib \
	-Wl,--relax -Wl,--gc-sections \
	-L$(XTLIB) \
	-L$(XTGCCLIB) \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libssl.a \
	$(SDK)/lib/libupgrade.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/liblwip.a \
	$(SDK)/lib/libwpa.a \
	$(SDK)/lib/libnet80211.a \
	$(SDK)/lib/libphy.a \
	$(SDK)/lib/libmain.a \
	$(SDK)/lib/libpp.a \
	$(XTGCCLIB) \
	-T $(SDK)/ld/eagle.app.v6.ld

LINKFLAGS:= \
	$(LDFLAGS_CORE) \
	-B$(XTLIB)

$(TARGET_OUT) : $(SRCS)
	$(PREFIX)gcc $(CFLAGS) $^  -flto $(LINKFLAGS) -o $@


$(FW_FILE_1): $(TARGET_OUT)
	PATH=$(PATH):$(GCC_FOLDER)/bin $(ESPTOOL_PY) elf2image $(TARGET_OUT)

$(FW_FILE_2): $(TARGET_OUT)
	PATH=$(PATH):$(GCC_FOLDER)/bin $(ESPTOOL_PY) elf2image $(TARGET_OUT)

burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(FTDI) write_flash 0x00000 $(FW_FILE_1) 0x40000 $(FW_FILE_2))||(true)

term :
	screen $(FTDI) 115200

config :
	rm user/user_config.h
	ln -s user_config.$(CONFIG).h user/user_config.h

test :
	rm -f tests/testsuite
	gcc tests/testsuite.c -o tests/testsuite
	tests/testsuite

clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


