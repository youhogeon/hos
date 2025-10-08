NASM32 = nasm
GCC32 = gcc -c -m32 -ffreestanding
LD32 = ld -melf_i386 -T ../ldscripts/elf_i386.x -nostdlib -e _start -Ttext 0x10200
OBJCOPY32 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

BUILD_DIR = build

all: prepare $(BUILD_DIR)/kernel32.bin

prepare:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/kernel32.bin: $(BUILD_DIR)/entrypoint.bin $(BUILD_DIR)/kernel32.elf.bin
	cat $^ > $@
	truncate -s 4K $@

$(BUILD_DIR)/entrypoint.bin: entrypoint.s
	$(NASM32) -o $@ $<

$(BUILD_DIR)/kernel32.elf.bin: $(BUILD_DIR)/dependency.dep $(BUILD_DIR)/kernel32.elf
	$(OBJCOPY32) $(BUILD_DIR)/kernel32.elf $@


################################################################
# C
################################################################

CENTRYPOINTOBJECTFILE = $(BUILD_DIR)/main.o
CSOURCEFILES = $(wildcard ./*.c)
ASSEMBLYSOURCEFILES = $(wildcard ./*.asm)

# C -> o
$(BUILD_DIR)/%.o: ./%.c
	$(GCC32) -c $< -o $@

# ASM -> o
$(BUILD_DIR)/%.o: ./%.asm
	$(NASM32) -f elf32 -o $@ $<

# o 리스트 생성
COBJECTFILES = $(patsubst ./%.c,$(BUILD_DIR)/%.o,$(CSOURCEFILES))
ASSEMBLYOBJECTFILES = $(patsubst ./%.asm,$(BUILD_DIR)/%.o,$(ASSEMBLYSOURCEFILES))

# dependency
$(BUILD_DIR)/dependency.dep: $(CSOURCEFILES)
	$(GCC32) -MM $(CSOURCEFILES) | sed 's@\([^ ]*\.o\)@$(BUILD_DIR)/\1@g' > $@

# 링크
$(BUILD_DIR)/kernel32.elf: $(CENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD32) -o $@ $^

ifeq ($(BUILD_DIR)/dependency.dep, $(wildcard $(BUILD_DIR)/dependency.dep))
include $(BUILD_DIR)/dependency.dep
endif