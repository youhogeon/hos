all: kernal32.bin

kernal32.bin: entrypoint.s
	nasm -o kernal32.bin $<

clean:
	rm -f kernal32.bin