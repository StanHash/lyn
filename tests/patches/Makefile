CC := arm-none-eabi-gcc

CFLAGS := -O2 -Wall -Wextra -Wno-unused -ffreestanding \
          -mthumb -mthumb-interwork -mcpu=arm7tdmi -mabi=apcs-gnu -mfloat-abi=softfp -mlong-calls

C_SOURCES := $(wildcard *.c)
C_OBJECTS := $(C_SOURCES:%.c=%.o)

S_SOURCES := $(wildcard *.s)
S_OBJECTS := $(S_SOURCES:%.s=%.o)

all: $(C_OBJECTS) $(S_OBJECTS)

.PHONY: all

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

%.o: %.s
	$(CC) -c $< -o $@ $(CFLAGS)
