SRC_BOOT = src/bootloader
SRC_K32 = src/kernel32

BUILD_DIR = build

all: prepare $(BUILD_DIR)/disk.iso

disk.img: prepare $(BUILD_DIR)/disk.img

prepare:
	mkdir -p build

$(SRC_BOOT)/bootloader.bin:
	make -C $(SRC_BOOT)

$(SRC_K32)/build/kernel32.bin:
	make -C $(SRC_K32)

$(BUILD_DIR)/disk.img: $(SRC_BOOT)/bootloader.bin $(SRC_K32)/build/kernel32.bin
	cat $^ > $@

$(BUILD_DIR)/disk.iso: $(BUILD_DIR)/disk.img
	truncate -s 1474560 $<
	mkisofs -o $@ -b $< .

clean:
	make -C $(SRC_BOOT) clean
	make -C $(SRC_K32) clean
	rm -rf $(BUILD_DIR)