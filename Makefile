all: disk.iso

src/bootloader/bootloader.bin:
	make -C src/bootloader

src/kernal32/kernal32.bin:
	make -C src/kernal32

disk.img: src/bootloader/bootloader.bin src/kernal32/kernal32.bin
	cat $^ > build/disk.img

disk.iso: disk.img
	truncate -s 1474560 build/disk.img
	mkisofs -o build/disk.iso -b build/disk.img .

clean:
	make -C src/bootloader clean
	make -C src/kernal32 clean
	rm -f build/*