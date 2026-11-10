TARGET = watch
BUILD = build
CC = arm-none-eabi-gcc
OBJCOPY = arm-none-eabi-objcopy
SIZE = arm-none-eabi-size

CFLAGS = -mcpu=cortex-m3 -mthumb -Os -std=c11 -ffreestanding -fno-builtin \
	-ffunction-sections -fdata-sections -fno-common \
	-fno-asynchronous-unwind-tables -fno-unwind-tables \
	-Wall -Wextra -Wundef -Wshadow -MMD -MP \
	-Isrc

LDFLAGS = -mcpu=cortex-m3 -mthumb -nostdlib -Wl,--gc-sections \
	-Wl,-T,src/stm32f103c8.ld -Wl,-Map,$(BUILD)/$(TARGET).map

SRCS = $(wildcard src/*.c)
OBJS = $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS)) $(BUILD)/startup.o

.PHONY: all clean test assets

all: assets $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex

assets:
	python3 tools/gen_assets.py

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/startup.o: src/startup.s | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

-include $(wildcard $(BUILD)/*.d)

$(BUILD)/$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lgcc
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/$(TARGET).hex: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

test:
	mkdir -p $(BUILD)
	gcc -std=c11 -Wall -Wextra -O2 -o $(BUILD)/test_civil test/test_civil.c
	$(BUILD)/test_civil

clean:
	rm -rf $(BUILD)
