all: disk.iso

_bootloader:
	make -C src/bootloader

_kernal32:
	make -C src/kernal32

disk.img: _bootloader _kernal32
	cat src/bootloader/bootloader.bin src/kernal32/dummy_os.bin > build/disk.img

disk.iso: disk.img
	truncate -s 1474560 build/disk.img
	mkisofs -o build/disk.iso -b build/disk.img .


clean:
	make -C src/bootloader clean
	make -C src/kernal32 clean
	rm -f build/*