SRC_BOOT = src/bootloader
SRC_K32 = src/kernel32
SRC_K64 = src/kernel64

BUILD_DIR = build

VER_OFFSET = 497
VER_BYTES  = 12

all: prepare $(BUILD_DIR)/disk.iso

disk.img: prepare $(BUILD_DIR)/disk.img

prepare:
	mkdir -p build

$(SRC_BOOT)/bootloader.bin:
	make -C $(SRC_BOOT)

$(SRC_K32)/build/kernel32.bin:
	make -C $(SRC_K32)

$(SRC_K64)/build/kernel64.bin:
	make -C $(SRC_K64)

$(BUILD_DIR)/disk.img: $(SRC_BOOT)/bootloader.bin $(SRC_K32)/build/kernel32.bin $(SRC_K64)/build/kernel64.bin
	cat $^ > $@
ifdef HOS_VERSION
	printf '%s' "$(HOS_VERSION)" | dd of=$@ bs=1 seek=$(VER_OFFSET) count=$(VER_BYTES) conv=notrunc status=none
endif

$(BUILD_DIR)/disk.iso: $(BUILD_DIR)/disk.img
	truncate -s 1474560 $<
	mkisofs -o $@ -b $< .

clean:
	make -C $(SRC_BOOT) clean
	make -C $(SRC_K32) clean
	make -C $(SRC_K64) clean
	rm -rf $(BUILD_DIR)