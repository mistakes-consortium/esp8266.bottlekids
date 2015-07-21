all : clean image.elf

FW_FILE_1:=0x00000.bin
FW_FILE_2:=0x40000.bin

TARGET_OUT:=image.elf
OBJS:=driver/uart.o\
	user/user_main.o

SRCS:=driver/uart.c \
	user/user_main.c \
	
FTDI:=/dev/ttyUSB0

GCC_FOLDER:=/opt/Espressif/crosstool-NG/builds/xtensa-lx106-elf
ESPTOOL_PY:=/opt/Espressif/esptool-py/esptool.py
FW_TOOL:=/opt/Espressif/esptool/esptool
SDK:=/opt/Espressif/esp_iot_sdk_v0.9.5_b1
XTLIB:=$(SDK)/lib
XTGCCLIB:=$(GCC_FOLDER)/lib/gcc/xtensa-lx106-elf/4.8.2/libgcc.a
PREFIX:=$(GCC_FOLDER)/bin/xtensa-lx106-elf-
CC:=$(PREFIX)gcc

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
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -bo $@ -bs .text -bs .data -bs .rodata -bc -ec

$(FW_FILE_2): $(TARGET_OUT)
	@echo "FW $@"
	$(FW_TOOL) -eo $(TARGET_OUT) -es .irom0.text $@ -ec

burn : $(FW_FILE_1) $(FW_FILE_2)
	($(ESPTOOL_PY) --port $(FTDI) write_flash 0x00000 0x00000.bin 0x40000 0x40000.bin)||(true)

term :
	screen $(FTDI) 115200

config :
	rm user/user_config.h
	ln -s user_config.$(CONFIG).h user/user_config.h

clean :
	rm -rf user/*.o driver/*.o $(TARGET_OUT) $(FW_FILE_1) $(FW_FILE_2)


