all: bootloader.bin

bootloader.bin: bootloader.asm
	nasm -o bootloader.bin bootloader.asm
 
clean:
	rm -f bootloader.bin