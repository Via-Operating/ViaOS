	MXOS kernel release 1.0

These are the release notes for MXOS version 1.0.  Read them carefully,
as they tell you what this is all about, explain how to install the
kernel, and what to do if something goes wrong. 

WHAT IS MXOS?

  MXOS is a Unix clone for 386/486-based PCs written from scratch by
  Kap Petrov with assistance from a loosely-knit team of OSDEVers
  across the internet.  It aims towards simplicity and usability. 

  It is distributed under the GNU General Public License - see the
  accompanying LICENSE file for more details. 

INSTALLING the kernel:

- make sure you have all the dependencies such as NASM, QEMU, etc.

COMPILING the kernel:

 - make sure you have gcc.  It seems older gcc
   versions can have problems compiling newer versions of MXOS. If you
   upgrade your compiler, remember to get the new binutils package too
   (for as/ld/nm and company)

 - do a "make" to create a kernel image without grub.
