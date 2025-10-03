all: _bootloader _kernal32 disk.img

_bootloader:
	@echo
	@echo ============== Build Boot Loader ===============
	@echo

	make -C bootloader

	@echo
	@echo =============== Build Complete ===============
	@echo

_kernal32:
	@echo
	@echo ============== Build Kernel 32 ================
	@echo

	make -C kernal32

	@echo
	@echo =============== Build Complete ===============
	@echo

disk.img: _bootloader _kernal32
	@echo
	@echo =========== Disk Image Build Start ===========
	@echo
	
	cat bootloader/bootloader.bin kernal32/dummy_os.bin > disk.img

	@echo
	@echo ============= All Build Complete =============
	@echo

clean:
	make -C bootloader clean
	make -C kernal32 clean
	rm -f disk.img