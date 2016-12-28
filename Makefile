.PHONY: all believe clean
.SECONDARY: $(OBJ)

ifeq ($(TARGET),)
    $(info Inspecting compiler to use...)
    ifneq ($(shell which arm-elf-gcc),)
        TARGET=arm-elf
    else
        ifneq ($(shell which arm-linux-gcc),)
            TARGET=arm-linux
        else
            $(error No compiler found. Check $$PATH or provide a $$TARGET)
        endif
    endif
endif

CC := $(TARGET)-gcc
LD := $(TARGET)-ld

ELF2HP ?= elf2hp
HP2APT ?= ./hp2aplet

SRC ?= crt0.c display.c
OBJ ?= $(SRC:%.c=%.o)
INC ?= "$(HPGCC)\include"
LIB ?= "$(HPGCC)\lib"


CFLAGS ?= -std=c99 -Wall -Os -I$(INC) -L$(LIB) -fomit-frame-pointer \
	-mtune=arm920t -mcpu=arm920t -mlittle-endian -msoft-float

crt0.o: CFLAGS += -msingle-pic-base -fpic -mpic-register=r10

LDFLAGS := -L$(LIB) -T MMUld.script -lhplib -lgcc


all: believe hp39dir.000

believe: neko_video.apt

clean:
	rm *.o *.elf *.hp

%.apt: %.hp
	$(HP2APT) $< $@ "Neko Video"

%.hp: %.elf
	$(ELF2HP) $< $@

%.elf: $(OBJ)
	$(LD) $(OBJ) $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
