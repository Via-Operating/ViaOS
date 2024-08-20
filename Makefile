PREFIX = /usr/local
INCLUDEDIR = $(PREFIX)/include/via
DESTDIR ?= # If not set, default to empty, meaning no extra staging directory

all:
	mkdir -p build
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.
	nasm -f elf32 init/boot.asm -o build/boot.o
	nasm -f elf32 init/gdt.asm -o build/gdt.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c init/main.c -o build/main.o
	ld -m elf_i386 -T linker.ld -nostdlib -o build/mxos.bin build/gdt.o build/boot.o build/main.o

clean:
	rm build/main.o build/boot.o build/gdt.o build/mxos.bin
	rm -rf build

iso:
	grub-file --is-x86-multiboot build/mxos.bin
	mkdir -p build/isodir/boot/grub/
	cp build/mxos.bin build/isodir/boot/
	cp grub.cfg build/isodir/boot/grub/
	grub-mkrescue -o build/via.iso build/isodir/