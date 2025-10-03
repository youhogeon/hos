all: dummy_os.bin

dummy_os.bin: dummy_os.asm
	nasm -o dummy_os.bin dummy_os.asm

clean:
	rm -f dummy_os.bin