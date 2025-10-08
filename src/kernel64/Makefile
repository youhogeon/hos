NASM64 = nasm -f elf64
GCC64 = gcc -c -m64 -ffreestanding
LD64 = ld -melf_x86_64 -T ../ldscripts/elf_x86_64.x -nostdlib -e _start -Ttext 0x200000
OBJCOPY64 = objcopy -j .text -j .data -j .rodata -j .bss -S -O binary

BUILD_DIR = build

all: prepare $(BUILD_DIR)/kernel64.bin

prepare:
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR)/kernel64.bin: $(BUILD_DIR)/kernel64.elf.bin
	cat $^ > $@

$(BUILD_DIR)/kernel64.elf.bin: $(BUILD_DIR)/dependency.dep $(BUILD_DIR)/kernel64.elf
	$(OBJCOPY64) $(BUILD_DIR)/kernel64.elf $@


################################################################
# C
################################################################
ENTRYPOINTSOURCEFILE = entrypoint.s
ENTRYPOINTOBJECTFILE = $(BUILD_DIR)/entrypoint.o
CENTRYPOINTOBJECTFILE = $(BUILD_DIR)/_start.o
CSOURCEFILES = $(wildcard ./*.c ./*/*.c)
ASSEMBLYSOURCEFILES = $(wildcard ./*.asm ./*/*.asm)

# C -> o
$(BUILD_DIR)/%.o: ./%.c
	mkdir -p $(dir $@)
	$(GCC64) -c $< -o $@

# ASM -> o
$(BUILD_DIR)/%.o: ./%.asm
	mkdir -p $(dir $@)
	$(NASM64) -o $@ $<

$(ENTRYPOINTOBJECTFILE): $(ENTRYPOINTSOURCEFILE)
	$(NASM64) -o $@ $<

# o 리스트 생성
COBJECTFILES = $(patsubst ./%.c,$(BUILD_DIR)/%.o,$(CSOURCEFILES))
ASSEMBLYOBJECTFILES = $(patsubst ./%.asm,$(BUILD_DIR)/%.o,$(ASSEMBLYSOURCEFILES))

# dependency
$(BUILD_DIR)/dependency.dep: $(CSOURCEFILES)
	$(GCC64) -MM $(CSOURCEFILES) | sed 's@\([^ ]*\.o\)@$(BUILD_DIR)/\1@g' > $@

# 링크
$(BUILD_DIR)/kernel64.elf: $(ENTRYPOINTOBJECTFILE) $(CENTRYPOINTOBJECTFILE) $(COBJECTFILES) $(ASSEMBLYOBJECTFILES)
	$(LD64) -o $@ $^

ifeq ($(BUILD_DIR)/dependency.dep, $(wildcard $(BUILD_DIR)/dependency.dep))
include $(BUILD_DIR)/dependency.dep
endif