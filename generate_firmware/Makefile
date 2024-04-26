.PHONY : generate_firmware

TARGET=WINDOWS

ifeq ($(TARGET), WINDOWS)
WINOPT = -mwin32 
EXE =.exe
else
WINOPT = 
EXE =
endif

generate_firmware: main.c cobl_crc.c
	#$(CC) $(WINOPT) -I. -o $@ $+
	$(CC)  -g -I. -o $@ $+
	./$@$(EXE) --help

all: generate_firmware

help:
	@echo "make generate_firmware"
	@echo "Target: $(TARGET)"

