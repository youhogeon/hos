all: bootloader.bin

VER_OFFSET = 495
VER_BYTES  = 12

bootloader.bin: bootloader.asm
	nasm -o bootloader.bin bootloader.asm
ifdef HOS_VERSION
	printf '%-12s' "$(HOS_VERSION)" | dd of=$@ bs=1 seek=$(VER_OFFSET) count=$(VER_BYTES) conv=notrunc status=none
endif

clean:
	rm -f bootloader.bin