
ARMGNU ?= arm-none-eabi
CFLAGS = -Wall -Wextra -O3 -g -nostdlib -nostartfiles -fno-stack-limit -ffreestanding -Wno-unused-parameter 
CPPFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti


## Important!!! asm.o must be the first object to be linked!
OB = asm.o main.o uart.o utils.o binary_assets.o postman.o
OOB = Solver.oo Memory.oo

BUILD_DIR = build
SRC_DIR = src
OBJS=$(patsubst %.o,$(BUILD_DIR)/%.o,$(OB)) $(patsubst %.oo,$(BUILD_DIR)/%.oo,$(OOB))
LIBGCC=$(shell $(ARMGNU)-gcc -print-libgcc-file-name)

all: connect4.elf kernel 

kernel: connect4.img
	cp connect4.img bin/kernel.img

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c | $(BUILD_DIR) 
	$(ARMGNU)-gcc $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.oo : $(SRC_DIR)/%.cpp | $(BUILD_DIR) 
	$(ARMGNU)-g++ $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.s | $(BUILD_DIR) 
	$(ARMGNU)-as $< -o $@

connect4.elf : $(OBJS) 
	$(ARMGNU)-ld $(OBJS) $(LIBGCC) -T memmap -o $@

connect4.img : connect4.elf 
	$(ARMGNU)-objcopy $< -O binary $@

$(BUILD_DIR): 
	mkdir $(BUILD_DIR)

.PHONY clean :
	rm -rf $(BUILD_DIR) 
	rm -f *.elf *.img bin/kernel.img
