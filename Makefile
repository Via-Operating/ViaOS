# horrendous makefile, god please forgive me.

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
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c drivers/independent/disk/ata/IDE.c -o build/ata.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c drivers/independent/graphics/vga/vga.c -o build/vga.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c include/shell/shell32/desktop/window.c -o build/window.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c include/shell/shell32/accessories/pmm.c -o build/pmm.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c include/shell/shell32/accessories/notepad.c -o build/notepad.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c include/shell/shell32/desktop/taskbar.c -o build/taskbar.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c include/shell/shell32/accessories/viaver.c -o build/ver.o
	gcc -m32 -std=gnu99 -ffreestanding -Wall -Wextra -c include/shell/shell32/accessories/paint.c -o build/paint.o
	ld -m elf_i386 -T linker.ld -nostdlib -o build/mxos.bin build/gdt.o build/boot.o build/ata.o build/vga.o build/window.o build/pmm.o build/notepad.o build/ver.o build/taskbar.o build/paint.o build/main.o

clean:
	rm build/main.o build/boot.o build/gdt.o build/mxos.bin build/ata.o build/vga.o build/window.o build/pmm.o build/notepad.o build/taskbar.o build/ver.o build/paint.o
	rm -rf build

iso:
	grub-file --is-x86-multiboot build/mxos.bin
	mkdir -p build/isodir/boot/grub/
	cp build/mxos.bin build/isodir/boot/
	cp grub.cfg build/isodir/boot/grub/
	grub-mkrescue -o build/via.iso build/isodir/