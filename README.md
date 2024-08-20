# ViaOS
An x86 Operating System built on the VD Kernel (VDK) by Via.

# Complile
> cd ViaOS

``` make ```

# Run
Copy mxos.bin kernel from build/ to your working directory and:

``` qemu-system-i386 -kernel mxos.bin ```
## OR
do ``` make iso ``` and then copy via.iso and either:

- Flash it on a USB and boot.
- use qemu cdrom.

# Look
![damn](https://github.com/user-attachments/assets/83f99a85-4afa-490e-a6bf-b3c633285403)
