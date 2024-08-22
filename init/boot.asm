; constants for multiboot header
MBALIGN     equ  1<<0
MEMINFO     equ  1<<1
FLAGS       equ  MBALIGN | MEMINFO
MAGIC       equ  0x1BADB002
CHECKSUM    equ -(MAGIC + FLAGS)

; set multiboot section
section .multiboot
    align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .data
    align 4096

; initial stack
section .initial_stack, nobits
    align 4

stack_bottom:
    ; 1 MB of uninitialized data for stack
    resb 104856
stack_top:

; kernel entry, main text section
section .text

; fuck the bootloader
%include "init/gdt.asm"

    global _start
    global load_gdt
    global load_idt
    global keyboard_handler
    global ata_handler
    global ioport_in
    global ioport_out
    global enable_interrupts
    global disable_interrupts

	extern kmain
	extern handle_keyboard_interrupt
	extern ide_irq

; load Global Descriptor Table
load_gdt:
	lgdt [gdt_descriptor] ; gdt.asm
	ret

load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	ret

enable_interrupts:
	sti
	ret

disable_interrupts:
	cli
	ret

keyboard_handler:
	pushad ; push ads to the stack lmfao
	cld
	call handle_keyboard_interrupt
	popad
	iret ; if this doesnt work, use iretd instead

ata_handler:
	pushad ; push ads to the stack lmfao
	cld
	call ide_irq
	popad
	iret ; if this doesnt work, use iretd instead

ioport_in:
	mov edx, [esp + 4] ; PORT_TO_READ
	in al, dx
	ret

ioport_out:
	mov edx, [esp + 4] ; PORT_TO_WRITE
	mov eax, [esp + 8] ; VALUE_TO_WRITE
	out dx, al
	ret

; define _start, aligned by linker.ld script
; BE PREPARED TO SEE THE WORST CODE EVER
; YALL DONT UNDERSTAND WHEN I WRITE ASM MY CODE QUALITY GOES DOWN THE DRAIN :sob:
_start:
	; thanks M.Petch and Stephen Grice
	lgdt [gdt_descriptor]
	jmp CODE_SEG:.setcs
	.setcs:
	mov ax, DATA_SEG
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov esp, stack_top
	cli
    mov esp, stack_top
    push ebx
    call kmain
loop:
    jmp loop
