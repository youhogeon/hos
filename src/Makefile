all: _bootloader disk.img

_bootloader:
	@echo
	@echo ============== Build Boot Loader ===============
	@echo

	make -C bootloader

	@echo
	@echo =============== Build Complete ===============
	@echo

disk.img: _bootloader
	@echo
	@echo =========== Disk Image Build Start ===========
	@echo
	
	cp bootloader/bootloader.bin disk.img

	@echo
	@echo ============= All Build Complete =============
	@echo

clean:
	make -C bootloader clean
	rm -f disk.img