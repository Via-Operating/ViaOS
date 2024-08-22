# ViaOS
An x86 Operating System built on the Via Dynamic Kernel (VDK) by The Via Operating Project. Via is designed for an open-sourced, free non-profit operating system designed for developers and casual use.

## How to compile
### Linux
``` sudo apt install build-essential gcc g++ ``` - So that the prograams will actually work (No need to run this command if you have this)

``` git clone https://github.com/Via-Operating/ViaOS/ ``` - Clone ViaOS repo (No need to run this command if you have this)

``` cd ViaOS ``` - Change the directory to the ViaOS folder

```./include/shell/all-commands.sh ``` - Include all commands (Optional)

``` sudo make && sudo make iso ``` - Make and build iso

### Windows
Use WSL and follow the steps for linux.

### MacOS
Pretty much same as linux, use homebrew.

### ViaOS
It doesn't have a toolchain or GNU-Binutils. Yet.

## Run
```qemu-system-i386 -drive file=via.iso,media=disk,format=raw```
