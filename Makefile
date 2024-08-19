all:
	mkdir -p build
	nasm -f elf32 init/boot.asm -o build/boot.o
	nasm -f elf32 init/gdt.asm -o build/gdt.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c init/main.c -o build/main.o
	ld -m elf_i386 -T linker.ld -nostdlib -o build/mxos.bin build/gdt.o build/boot.o build/main.o

clean:
	rm build/main.o build/boot.o build/gdt.o build/mxos.bin