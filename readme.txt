          PDOS - Public Domain Operating System
          -------------------------------------

Version 0.9X, released YYYY-MM-DD
Written by Paul Edwards, mutazilah@gmail.com
Released to the public domain
You may use this entire package for any purpose whatsoever
without restriction, as discussed here:
http://creativecommons.org/publicdomain/zero/1.0/
That includes, but is not limited to, closed-source,
proprietary commercial products. It is hoped that a
company (possibly an American company) will convert
PDOS into a commercial product that they sell to
compete with the Microsoft/IBM monopolies.


INTRODUCTION
------------

PDOS currently comes in quite a variety of flavors:


Mainframe:

1. z/PDOS - runs on an IBM mainframe (z/Arch), nominally
in AMODE 64, but (via the use of page tables) effectively
AMODE 32, giving a normal 32-bit environment for MVS
programs that would otherwise run as AMODE 31.

2. PDOS/390 - runs on an IBM mainframe (S/390 architecture).
Predominantly designed to be compatible with a subset of the
31-bit MVS interface, so it can run programs as AMODE 31,
RMODE ANY. Also intended to support Posix interface.

3. PDOS/370 - runs on an IBM mainframe (S/370 architecture).
Predominantly designed to be compatible with a subset of the
24-bit MVS interface, so that it can run programs as
RMODE 24, AMODE 24.

4. PDOS/380 - runs on Hercules/380 (S/380 architecture).
Predominantly designed to be compatible with a subset of the
MVS/380 interface, so that it can run programs as
AMODE 32 (not just 31), RMODE ANY.


PC:

1. PDOS/86 (aka PDOS16) - long-term goal is to be compatible with
MSDOS, and thus supports a subset of the 16-bit MSDOS API.

2. PDOS/386 (aka PDOS32) - designed to be a 32-bit version of MSDOS,
so API is similar to PDOS/86, except most values take a
32-bit integer that would otherwise normally be 16-bit. This
interface is not meant to be directly used, and is subject
to change. You should be using the Win32 API instead.

3. PD-Windows - similiar to how Windows 95 was built on top
of MSDOS, PD-Windows is designed to support Win32
applications, but built on top of 32-bit PDOS/386 instead of
16-bit MSDOS.

4. PDOS/x64 - runs under UEFI on an x86_64 processor.


Smartphone:

1. PDOS-generic runs on 32-bit ARM



For a version 1.0 to be created requires the following
enhancements:

1. PD-Windows should support the binutils compile chain
2. PDOS/380 and Hercules/380 should use the "separate
memory" proposal where Hercules/380 manages the ATL
memory.


MAINFRAME PURPOSE
-----------------

To provide an alternative platform to run MVS applications, that
can be expanded on as required. Also to experiment with different
techniques. Also in some ways it helps to simply test MVS
applications, as they get executed in a different environment.



PC PURPOSE
----------

The long term goal is to be a rival to both
Microsoft Windows (commercial) and React OS
(copyrighted freeware). It is hoped that this public
domain code can be picked up by some commercial
enterprises to produce commercial-quality
alternatives to the above. It is also hoped that
government research departments will take the
lead in updating the public domain base.


The package more accurately contains multiple,
mostly independent things:

1. BOS application programming interface, for
standardized access to the BIOS.

2. POS application programming interface, for
standardized access to MSDOS interrupts.

3. PDPCLIB, a C runtime library, for multiple
environments, e.g. MVS, MSDOS and any POS-compliant
operating system (e.g. MSDOS and PDOS are both
POS-compliant).

4. PDOS/86, an operating system which can be
considered a clone of MSDOS.

5. PDOS/386, a 32-bit operating system, which
can't be considered a clone of any existing
operating system (that I know of, anyway).
It shares similarities with MSDOS, except it
is 32-bit.  It shares similarities with DOS
extenders, except the API is different, and
it doesn't require an OS for support.  It shares
similarities with Unix (the executable format),
except it doesn't support the Posix API.  So all
in all, it's a new operating system which thus
necessitates all applications to be recompiled,
although not necessarily rewritten. Note that
PDOS/386 is pure 32-bit.  It doesn't have any
16-bit code in it, although it does call
16-bit BIOS functions by switching to real mode.

6. PD-Windows, a 32-bit operating system built
on top of PDOS/386, that is designed to run a
subset of 32-bit Windows applications so you
don't need to recompile. It is hoped that
Windows device drivers will also work on
PD-Windows one day, and that PD-Windows will
no longer be dependent on the 16-bit BIOS.
Note that PD-Windows and PDOS/386 ship as a
single product.



PC INSTALLING - TRADITIONAL
---------------------------

To get the 16-bit version on floppy disk, you need
to get hold of a program called "rawrite" from the
internet and then follow the instructions (from
rawrite) to get pdos16.img onto a 1.44 MB floppy disk.
pdos32.img will give you the 32-bit version. Both of
these files can be found in the "src" directory.

To install on hard disk, you need MSDOS (I tested
with version 5.0) to format a FAT-16 partition,
then edit the doinst.bat file, choose location of
executables you wish to use (use full path including
drive letter!), plus the drive you want to install
it on.  PDOS will happily run from a logical
partition.  You can't use Win98 or Freedos to do
this work, because the "sys" command produces an
incompatible boot sector. Although there is a
workaround for Freedos:

compb (after editing to switch to wasmr and tlink)
compbu
tcc sectread.c bos.c
tcc sectwrit.c bos.c
get xychop.c from OZPD at http://pdos.sourceforge.net
format d: /u
doinst
newboot (or newboota for floppies)
and to get a new MBR for a hard disk, go:
newmbr (after checking disk number)



PC INSTALLING - MODERN
----------------------

You can obtain a hard disk image in the form pdos.vhd.
You can mount this drive under Windows and copy extra
files onto it. Eject it, burn it to USB stick using
Win32 Disk Imager.



PC DEVELOPMENT ENVIRONMENT
--------------------------

I use the following bits of software.

MSDOS 5.0

Freedos

Turbo C++ 1.01 (for 16-bit C compiler)
https://web.archive.org/web/20060114221019/http://community.borland.com/article/images/21751/tcpp101.zip

Open Watcom 1.6 (for 16-bit wasmr assembler)
http://www.openwatcom.org

EMX 0.9d (which includes GCC 2.8.1) (for 32-bit C compiler and assembler):
http://hobbes.nmsu.edu/cgi-bin/h-browse?sh=1&dir=//pub/os2/dev/emx/v0.9d

Bochs 2.4.5
http://bochs.sourceforge.net/

Qemu 3.0.0
https://www.qemu.org/

I have a 900 MB C drive (FAT-16) under Bochs on which I do
development, and then install the newly-created executables
onto the D drive, then reboot using a slightly different Bochs
configuration to test PDOS.

raread/rawrite (unknown origin)

info-zip zip is available at:
ftp://ftp.info-zip.org/pub/infozip/win32/zip300xn.zip
https://web.archive.org/web/20230115084431/ftp://ftp.info-zip.org/pub/infozip/win32/zip300xn.zip

info-zip unzip, unknown origin

GCC 3.2.3 to do cross-compile from Windows:
https://sourceforge.net/projects/pdos/files/pdwdev/pdwdev-rel1.zip/download
refer to gccdos.txt for documentation

binutils 2.14a to do cross-assembly etc. from Windows:
https://sourceforge.net/projects/pdos/files/pdwdev/pdwdev-rel1.zip/download

dmake with -B option allows space instead of tabs in makefiles
https://github.com/mohawk2/dmake/releases
I use 4_12_2_2

other version of gcc to build Windows pdptest (makefile.w32)

jwasm to build winsupa.asm in pdptest (makefile.w32).
https://sourceforge.net/projects/jwasm/files/JWasm%20Windows%20binary/JWasm194cb.zip/download

Not used:
MASM 6.15:
http://msdn2.microsoft.com/en-us/vstudio/aa718349.aspx
https://web.archive.org/web/20070202015826/http://download.microsoft.com/download/vb60ent/update/6/w9x2kxp/en-us/vcpp5.exe
(go "vcpp5 /c" to extract and choose to install in any scratch
directory, then copy ml.exe and ml.err into your PATH,
and save MasmRef.doc as a reference)

Not used:
64-bit MASM from Visual Studio 2005:
https://www.microsoft.com/en-us/download/details.aspx?id=14477
6.1.6000.16384.10.WindowsSDK_Vista_Feb2007Update_rtm.DVD.Rel.iso
WinSDKCompiler-WinSDKCompiler_X64_VC8CppCrossCompiler-x86.0.cab
ren ml64_exe.DFB19EFA_5C47_43EC_95E0_F97ACB8E5B40 ml64.exe

HX DOS extender:
https://www.japheth.de/HX.html



PC RECOMPILING - TRADITIONAL
----------------------------

If you have the above software, you can just type "build"
and it will rebuild the two .img files used in the shipment,
except for kernel32.dll.
Note that you need to edit build.bat and put the proper
paths in before that will actually work.

The step by step guide is below...


To compile 16-bit version using Turbo C++ 1.01:

Go to pdpclib and type "compile"
Go to src and type "comp1" then "comp2" then "comp3".
Then "compw16"

To compile 32-bit version using EMX 0.9d and Turbo C++ 1.01:

Go to pdpclib and type "compp"
Go to src and type "comp4" then "comp5" then "comp6".
Then "compw32"

Run comp0 and then doinst (after editing it!!!)
to install either 16 or 32-bit version.
NOTE! Installing PDOS in this manner will use
the MSDOS boot sector rather than PDOS's one.
The MSDOS boot sector is potentially copyrighted
(but the funny thing is that if it is, even a
blank formatted disk is copyright!).  Their boot
sector is better than PDOS's, but it's up to you
whether you use it or not.

The boot floppy (which uses PDOS's boot sector)
was created in the following manner:

format a floppy disk using MSDOS 5.0
run doinst using the 16-bit version
raread to get the floppy into a file, e.g. pdos16.img
compb (after editing to switch to wasmr and tlink)
compbu
bootupd pdos16.img pbootsec.com
rawrite pdos16.img back to floppy

To create kernel32.dll (so that you can run Win32
programs) you need to run "compk32" from a suitable
platform. You can also build the rest of PDOS/386
by using the comp4w (needs Watcom), comp5w and
comp6w (needs a cross-compiled GCC 3.2.3) and copy
the files across to the target platform.

To create the Windows version of pdptest you need to go:
pdmake -f makefile.msv

(and copy the msvcrt.dll to the target system too).



PC RECOMPILING - MODERN
-----------------------

You typically need to create a virtual hard disk (VHD)
under Windows, as MBR and FAT, and then use Freedos
under Bochs or whatever to delete the partition and
recreate it. The batch files below should be inspected
before executing them.

After obtaining the source for PDOS/386 go:

compsys (after editing to switch to the watcom version)
(to circumvent an presumed HX bug we need two steps)
pdmake -n -f makefile.wcd >temp.bat
temp
compimbr (after editing to switch to the watcom version)

XXXXX begin
cd src
compk32
cd ..\pdpclib
pdmake -f makefile.msv
pdmake -f makefile.pdw
cd ..\src
comp4w
comp5w
comp6w
XXXXX end

Transfer pload.com, pdos.exe, pcomm.exe, kernel32.dll,
msvcrt.dll (from pdpclib), sys.exe, instmbr.exe to Freedos,
format a drive (without /s, and I think you need /u),
doinst (don't worry about patchver error), instmbr.

For PDOS/86 you instead change the bit between XXXXX with:

cd pdpclib
pdmake -f makefile.wcd
cd ..\src
comp1w
comp2w
comp3w

and you don't need kernel32.dll and msvcrt.dll.



PC RECOMPILING - MODERN - MARK II
---------------------------------

The PDOS/386 distribution comes with a misc\master.bat
that contains the procedure now used to actually build.



PC RECOMPILING - INDUSTRY STANDARD TOOLS
----------------------------------------

You can build at least the basic system using standard
Visual C and MASM. Because there is a 16-bit component,
you need to use Visual C++ 1.52C or some level below to
do the 16-bit. Modern MASM still supports 16-bit OMF
output. The last version of MASM to still work under
MSDOS (6.11) also works. It is unknown how low you can
go. Anyway, since PDOS/86 is all 16-bit, here is the
procedure for building that using Microsoft tools:

cd pdpclib
pdmake -f makefile.msc
cd ..\src
pdmake -f makeio.vsc
pdmake -f makepdos.v86
pdmake -f makecomm.v86

And for PDOS/386, start with the 16-bit tools
cd pdpclib
pdmake -f makefile.msc
cd ..\src
pdmake -f makeio.vsc TARG=PDOS32

then switch to 32-bit tools (Visual Studio 2005 is known
to work)
cd pdpclib
pdmake -f makefile.vsp
cd ..\src
pdmake -f makepdos.vsc
pdmake -f makecomm.vsc



BOOTSTRAPPING
-------------

In order to recompile from source, you need an existing
C compiler capable of building an executable. This is then
used to build the "standard tools" used by PDOS. Those tools
can then be used to produce the PDOS executables, which should
be byte-exact. The only exception is pload.com (io.sys) which
needs to be built with either Open Watcom or Visual C++ (a
version capable of producing 16-bit executables), as we don't
yet have a compiler (capable of being built under PDOS/386
from source code) capable of producing 16-bit code.

There are makefiles available assuming you wish to go through
this process starting with Visual Studio 2005. Each executable
that is built should be put in some directory, e.g. c:\winpath,
that you have put in your PATH. The required source code is
all on the PDOS/386 disk, but you may obtain it from other
sources, ie Sourceforge and pdos.org. You will need to extract
the gcc source code from custom.zip in c:\misc. The procedure
(to be run under Windows) is:

cd \devel\ozpd\c
cl rm.c
copy rm.exe \winpath

cd \devel\pdos\pdmake
nmake -f makefile.vsc
copy pdmake.exe \winpath

cd \devel\gcc\gcc
pdmake -f windowsm.mak
(after modifying as per comments, I think)
copy gcc-new.exe \winpath\gccwin.exe

cd \devel\pdos\pdas
pdmake -f makefile.vsc
copy pdas.exe \winpath

cd \devel\pdos\pdld
pdmake -f makefile.vsc
copy pdld.exe \winpath

cd \devel\xar
pdmake -f makefile.vsc
copy xar.exe \winpath

cd \devel\as86
pdmake -f makefile.vsc
copy as86.exe \winpath

cd \devel\pdos\src
pdmake -f makek32.std
cd \devel\pdos\pdpclib
ren pdptest.c pdptest.bak
copy \devel\ozpd\c\rm.c pdptest.c
pdmake -f makefile.std
copy pdptest.exe \winpath\rm.exe

cd \devel\pdos\pdmake
del pdmake.exe
pdmake -f makefile.std
copy pdmake.exe \winpath

cd \devel\gcc\gcc
pdmake -f windows.mak
copy gcc-new.exe \winpath\gccwin.exe

cd \devel\pdos\pdcc
pdmake -f makefile.std
copy pdcc.exe \winpath

cd \devel\pdos\pdas
pdmake -f makefile.std
copy pdas.exe \winpath

cd \devel\pdos\pdld
pdmake -f makefile.std
copy pdld.exe \winpath

cd \devel\xar
pdmake -f makefile.std
copy xar.exe \winpath

cd \devel\as86
pdmake -f makefile.std
copy as86.exe \winpath

That actually completes the tools, but now we need
to verify everything.

cd \devel\pdos\src
pdmake -f makek32.std
cd \devel\pdos\pdpclib
pdmake -f makefile.std
fc /b \winpath\rm.exe pdptest.exe
del pdptest.c
ren pdptest.bak pdptest.c

cd \devel\pdos\pdmake
del pdmake.exe
pdmake -f makefile.std
fc /b \winpath\pdmake.exe pdmake.exe

cd \devel\gcc\gcc
pdmake -f windows.mak
fc /b \winpath\gccwin.exe gcc-new.exe

cd \devel\pdos\pdcc
pdmake -f makefile.std
fc /b \winpath\pdcc.exe pdcc.exe

cd \devel\pdos\pdas
pdmake -f makefile.std
fc /b \winpath\pdas.exe pdas.exe

cd \devel\pdos\pdld
pdmake -f makefile.std
fc /b \winpath\pdld.exe pdld.exe

cd \devel\xar
pdmake -f makefile.std
fc /b \winpath\xar.exe xar.exe

cd \devel\as86
pdmake -f makefile.std
fc /b \winpath\as86.exe as86.exe

And now that you have the tools you can compile
the individual elements of PDOS, ie:

cd \devel\pdos\src
comp4w
comp5w
comp6w




HISTORY
-------

Some years ago (circa 1988?) someone made a comment "until DOS is made
32-bit, DOS-extenders are just a kludge".  I didn't understand the
comment at the time, and I'm still not sure what's classified as a
"kludge".  A few years later (circa 1994?) I was in a job where I was
battling to try to get some DOS applications to run in 640k, moving
around device drivers, making use of the high memory area etc., all
very frustrating.  On a machine with 32 meg of memory on it!  I thought
it would make more sense to have DOS reside outside of real mode
memory, making more room for applications.  Nothing had been done about
making DOS 32-bit.  So, I decided to make my own 32-bit port of DOS.
But I don't like writing non-ISO C programs, so decided to start by
writing wrappers over all the DOS + BIOS functions.  This was just one
of many projects I was working on so nothing much happened for years.

Then I bought a cheap ($80) second-hand computer for use in the bedroom,
and didn't have the energy to install anything other than DOS on it.
And there's not much I actually wanted to do with DOS.  So I decided
to port another one of my projects (PDPCLIB) to DOS.  After that was
done, the only other thing that I could do under DOS was write my own
DOS, so I proceeded to write a 16-bit version of DOS, but nice and
clean, i.e. don't use near/far pointers.  With the intention that when
finished, all I needed to do was recompile under a 32-bit compiler and
voila!

The original point of contention was to free up real memory, but since
I am no longer in that position, the goal has instead changed to simply
producing a nice, clean 32-bit version of DOS, mainly for my own use
in writing my own ISO C programs.  But that raises the question, what
does a 32-bit version of DOS actually look like?  This is my interpretation
of that originally posed question "until DOS is made 32 bit".  I figured
that you should be able to do things like write to location 0xb8000
without anyone complaining, but now you just addressed it as location
0xb8000!  However, with the compiler that I was using (EMX 0.9b), the
a.out executable format didn't include fixups, it assumed that CS and
DS would be pointing to where the program was loaded!  Rather than try
to change this, I just decided to provide an ABSADDR macro, so that you
can go "char *p = ABSADDR(0xb8000);" and then start writing directly to
screen memory.  ABSADDR refers to an operating system provided variable
which is how many bytes need to be subtracted from an address in order
for it to be able to access that absolute memory location.

Since then I figured out how to make EMX produce an executable with
fixups, so ABSADDR was no longer required.

The next major breakthrough came when Alica Okano added support for
loading Windows PE executables, along with loading DLLs so that we
could start supporting a subset of Win32 programs.



VERSION HISTORY
---------------

On 1994-04-28 in the PUBLIC_DOMAIN Fidonet echo, the question
was asked what was required to replace MSDOS, and experimentation
began. By 1994-07-03 the POS/PDOS project was "formally" elaborated.
On 1994-07-12 focus was switched to the C runtime library (PDPCLIB)
to support it and to identify the high priority MSDOS interfaces
that would be required by PDOS. First beta of PDPCLIB was made
available on 1994-07-30.

0.00  Released 1994-12-29 consisting of just some BIOS interface
      routines. Much more work was required on PDPCLIB before
      coming back to do PDOS-type work.

0.10  Released 1997-05-18 together with PDPCLIB 0.51

0.20  Released 1997-05-24

0.30  Released 1997-09-04

0.40  Released 1997-09-13

0.50  Released 1997-12-18

0.60  Released 1997-12-28

0.70  Released 1998-01-05

0.71  Released 1998-01-11

0.82  Released 1998-09-12

0.83  Released 2001-06-02

0.85  Released 2002-07-26

0.86  Released 2007-08-23

0.90  Released 2019-10-01 now supports running some
      Win32 applications.




PC POS INTERFACE
----------------

Instead of calling int 21h directly yourself (which will currently
still work), you should instead use the routines provided in header
file "pos.h", which is the official access to the OS, similar to
OS/2's "os2.h".  For those who need to access the BIOS directly,
the routines are in "bos.h".  As with normal DOS, you're not meant
to be accessing the BIOS directly, but there's nothing to stop you
anyway.  You can access the hardware directly too, although you're
not meant to.

Here's an example of a BIOS routine, BosDiskSectorRead.

int BosDiskSectorRead(void         *buffer,
                      unsigned int  sectors,
                      unsigned int  drive,
                      unsigned int  track,
                      unsigned int  head,
                      unsigned int  sector);

This particular function is meant to be called instead of INT 13 function 2.

Now the first thing you should know is that because this is meant to be an
interface to INT 13 function 2, you should not expect to get more out of it
than INT 13 will give you.  E.g. even though the sector number above is
given as an unsigned int, the interrupt has a limit of 255 for sector
number, so that's all you will get.  The unsigned int is designed for
future expansion (i.e. where this interrupt is bypassed).

Potentially there will be multiple things that happen when you call
BosDiskSectorRead.

1. 16-bit program under 16-bit PDOS.
   It simply calls the interrupt.  This is designed for you to continue
   writing DOS programs as you always have.  Will even work on an XT.
   Of course your parameters must be far pointers, which means they are
   equally constrained to addresses in the first meg.

2. 32-bit programs under 16-bit PDOS.
   All addresses are 32-bit.  If your program is running under plain DOS,
   the addresses will be translated to far pointers and the function will
   be done.  You do not need to make sure your addresses are in the lower
   1 meg, the data will be buffered and converted accordingly.

3. 16-bit program under 32-bit PDOS.
   It calls the interrupt, which is then intercepted, a switch made
   to protected mode so that PDOS can handle it.  PDOS may or may not
   return to real mode to execute the original BIOS interrupt.

4. 32-bit program under 32-bit PDOS.
   It calls the protected mode interrupt.  PDOS may or may not return
   to real mode to get the 16-bit version called, but IF it does, it
   will translate all the 32-bit addresses, buffering them if required.



HANDS-ON USE
------------

To remain compatible with MSDOS 5's FORMAT, SYS + boot sector, the
operating system has been split up into the traditional 3 programs,
IO.SYS, MSDOS.SYS and COMMAND.COM, except they have been renamed to
PLOAD.COM, PDOS.EXE and PCOMM.EXE respectively. The mainframe
naming standard is quite similar, but is PLOAD.SYS, PDOS.SYS and
COMMAND.EXE. And since writing, on the PC MSDOS.SYS has been renamed to
PDOS.SYS and COMMAND.COM has been renamed to COMMAND.EXE, but IO.SYS
remains the same.

As far as possible, all code has been written in C, for portability
between 16 + 32 bit systems, plus portability to other CPUs.  It is
even possible to compile the 16-bit version as an executable which
can then be debugged with Turbo Debugger etc.  The goal has been
that as far as possible, all you need to do is recompile the code on
another machine and it instantly works.

PLOAD.COM is a COM program, that has startup code which assumes
the first 3 sectors are loaded.  It then completely reloads PLOAD.COM
and PDOS.EXE, using the FAT-reading logic.  It also has the logic to do
program relocation for use by PDOS.EXE.  Also in the 32 bit version
it has the switch to/from protected mode facility.

PDOS.EXE is a large model EXE program (16-bit) or an a.out program
in 32-bit.  It uses special startup code to ensure that it doesn't
attempt to access the non-existent PSP etc.

So far I have written cut-down versions of all three executables, so that
it boots (of floppy or hard disk, including logical partitions) and prints
"Welcome to pcomm" or similar, and then prompts for a command.  Recognized
commands can be found by typing "help".

You can also launch other programs. The 16-bit version is very
wasteful of memory, so you can't do a lot with it. The focus is
on the 32-bit version. Also the program can't do any as-yet
unimplemented interrupts.  However, it is sufficiently
implemented to run C90-compliant applications that have been
built using PDPCLIB.

PDOS currently (even in the 32-bit version) uses the BIOS to read from
disk and do screen I/O and keyboard input.  This should hopefully make
it compatible with your existing hardware straight out of the box.
However, it is intended that as a non-default CONFIG.SYS option, you
can get PDOS to use internal device drivers.



MAINFRAME FILES
---------------

The "s370" directory contains the main files that comprise the
mainframe operating systems.

doit.bat - used to compile and everything
pload.c - standalone loader (PLOAD.SYS)
pdos.c - standalone operating system (PDOS.SYS)
pcomm.c - command processor (COMMAND.COM)
pdosutil.c - common utilities
pdossup.asm - assembler support routines for PDOS
ploadsup.asm - assembler support routines for PLOAD
world.c - example program



PC FILES
--------

pload.c - loader for PDOS (IO.SYS)
pdos.c - PDOS, the operating system (MSDOS.SYS)
pcomm.c - command processor (COMMAND.COM).
comp0.bat - used to compile PATCHVER.EXE (used by doinst)
comp1.bat - used to compile PLOAD.COM (IO.SYS)
comp2.bat - used to compile PDOS.EXE (MSDOS.SYS)
comp3.bat - used to compile PCOMM.EXE (COMMAND.COM)
comp4.bat - used to compile 32-bit-PDOS-supporting PLOAD.COM (IO.SYS)
comp5.bat - used to compile 32-bit version of PDOS.EXE (MSDOS.SYS)
comp6.bat - used to compile 32-bit version of PCOMM.EXE (COMMAND.COM)
compb.bat - used to compile boot sector
compbu.bat - used to compile program to update boot sector
compexe.bat - compiles PDOS + PCOMM into a single executable for debugging
doinst.bat - used to rename the 3 executables to the MSDOS-style names so
             that the "sys" command can then be used to install them.
bos.* - bios functions
pos.* - dos functions
fat.* - fat access functions
pdpgoal.txt - not the license agreement



PC INTERNALS
------------

memory map:
4 gig
memory for protected mode applications (code & data)
200000
  unused for 32-bit, presumably will hold 16-bit device drivers

110000
  area of memory still addressable by 16-bit programs,
  giving an extra 64k for any application that knows
  how to do that.  Unused by PDOS itself.
100000
  used by i/o devices and bios on IBM PC, 640k limit
  being created by the a0000 starting address.
a0000
  space for application code

50000
  64k completely wasted for memory chain control
  blocks, to avoid having to drop the flat memory
  model coding in the memory management class. (16-bit only)

40000
  some unused space here, but the 16-bit MSDOS.SYS finishes
  above 30000 since we started linking in the C library

30700 (approx.)
  MSDOS.SYS data & stack (32-bit only), then below-the-line memory

20700
  MSDOS.SYS
  Note that the 32-bit a.out has a 0x20 byte header
  so the actual code starts at 10720. For 16-bit,
  the header is likely to be 0xc00 long, for a
  code start of 11300.
10700
  MSDOS.SYS psp
10600
  IO.SYS stack (also used by 16-bit MSDOS.SYS)
07DFF
  sector 0 & bpb
07C00
07BFF
  IO.SYS
007FF
  MBR + IO.SYS
00700
  MBR
00600
00400
  REAL MODE INTERRUPTS
00000

The BIOS loads sector 0 into locations 7C00 - 7DFF.
[if a hard disk, then it is actually the MBR that gets loaded
 first, relocates itself to 0600-07FF, then loads sector 0 of
 the active partition]
IO.SYS gets loaded between 0700 and 7BFF (58 sectors).
The stack is at 10600 and grows down.
MSDOS.SYS gets loaded starting at 10700.
On transition to protected mode, sp gets set to a value 0x8000
above the end of the module. When calling
other programs, the stack is changed and set to a larger amount. When
an interrupt is done, the previous stack is restored temporarily.
It is also restored when the program terminates.  When doing a real
mode interrupt, ss & sp are temporarily set to what PLOAD was using.
The interrupt vectors for protected mode are provided as part of the
PLOAD executable - a length of 800 to give 256 8-byte interrupts.
For BIOS calls that require pointers, e.g. reading from disk sectors,
a translation is done so that the data is read to a location provided
by PLOAD and then transferred up.

Protected mode switching is done as follows:
  runaout() runs an a.out executable, by doing the following:
    load file
    runprot()
  runprot() runs a protected mode program by doing the following:
    disabling interrupts
    rawprot()
    establishing protected-mode interrupts
    reenabling interrupts
  rawprot() does the actual protected mode switch and starts
    executing code, blissfully unaware of what it is.

Note that that interface is designed to look clean, but in the case
of runprot(), the code's function is actually split between the
calling real-mode program and the protected-mode program.  Earlier
on runaout was too, the relocation of the a.out executable being
done after the switch to protected mode and in the a.out code!

You would use rawprot() if you wanted to quickly execute some
protected mode code, say already in memory, and were happy for
interrupts to be disabled whilst it was being executed.

runprot() will establish interrupt handlers for long-running bits
of code.

runaout() is just one executable format that may be run.

The first bit of the pload executable must fit into 3 * 512 bytes,
as that is all the boot sector loads.  When this was last measured,
2012-11-11, there were 542 bytes spare.  3*512 bytes corresponds to
a map reading of 0700h because of the org 0100h required for COM
files. There are 4 modules - ploadst, pload, int13x and near,
which are used to load the data beyond 3*512. And in "near", it is
only the first 3 functions that are actually called (by pload).
So in tlink you can change "-x" to "-m" to get a memory map, and
you look at the publics by value and then look for the __I4M which
is the first bit of unneeded code. Subtract 0100h from this value
to find out the amount of code in use. Then subtract that value
from 3*512 to find out how much space is still available.

When adding a new interrupt, you need to change handlers.asm,
pdos.c, protintp.c, protints.s, support.asm, support.s.



FUTURE DEVELOPMENT
------------------

PDOS/386 could actually execute 16-bit MSDOS
programs by dropping down to real mode and
executing until interrupt, then returning to
protected mode to execute that function.
But we'll probably just stick to running
Win32 programs and getting rid of dropping
down to 16-bit to use the BIOS altogether.
We'll need 32-bit device drivers to do that
though. Maybe Windows device drivers can
be used.

MSGED 4.00 and maybe later versions have support
for 80386 DOS programming. Look
at porting this and redefining PDOS/386 to match
this API.

It would be good to have a boot manager that runs
under PDOS/86 so that a PDOS boot floppy can be
flashed onto BIOS and allow the user to choose
what to boot. Note that the drive names are in
the boot sector of each partition, not in the MBR.

The Linux INT 80H calls could be supported by PDOS/386.
Along with ELF modules. To implement sbrk() PDPCLIB
needs to have MEMMGR defined and implement a ReSupply
function. PDOS needs to use memmgrRealloc(). However,
Linux only supports brk() as a syscall (sys_brk,
function 45 decimal) so it is not clear how that is
supposed to work. Also the way command line parameters
are pushed onto the stack in Unix is not very nice.
We'll probably just stick to Windows.

Use INT 15 AX=E820 to see how much memory is available.
http://www.ctyme.com/intr/rb-1741.htm
This has a problem in that protintp.c is only expecting
16-bit registers to be used in real mode, but this call
uses full 32-bit registers. A possible workaround would
be to pass the full 32-bit register values of
EAX/EBX/ECX/EDX down to the real mode interrupt.

New philosophy for PDOS:
Should be a poor man's Win 98 console mode.
Imagine someone has Windows/XP installed, but
something has gone wrong, Windows has stopped booting,
and thus they need a boot floppy to go and inspect the
hard disk, and copy some important files to a floppy.

Or maybe something has gone even more wrong than that,
and they need to write a small C program to repair some
damage and hopefully resurrect their system.  Because of
space limitations, assume they have obtained PDOS, loaded
with 3rd party utilities, on a bootable CD instead.

Maybe they've even got a second hard disk, so after
booting off the CD, they go "sys d:" to install PDOS
onto that, and then change their BIOS to boot off
that disk, and hey presto, plenty of space and a
writable disk, great for recovery work.

So something that looks identical to DOS, but is in
fact 32-bit, and can handle Win-98 32-bit console
mode executables.

And 3rd party software, such as gcc, can be
optionally distributed on the disk, but if someone
needs something comprised of purely public domain
source code, that is available too, they can put
their own (proprietary) disk-repair utility on to
a PDOS disk.

Note:
1. Some utilities, such as "sys" are required.
2. microemacs for windows should work
3. Something as simple as DOS, not as complicated as Unix, is required.

The environment as described above can also be used
to develop Windows 32-bit console mode programs.


multitasking has been added to some extent

multiprocessing (using more than one CPU core)

graphical user interface

file handles should be separate for each process

liballoc should be incorporated into kernel32.dll

processes should run in user mode, ie ring 3, to
restrict instructions and memory access

gcc and other stuff from binutils should run

maybe do a 68000 and ARM port

maybe support posix.1 programming interface

look at minimizing code that is authorized to hang system.

It would be good if PD-Windows could run the
mainframe emulator Hercules so that PDOS/3x0
can be run.

binutils 2.14a to work with PDPCLIB on Windows and then PD-Windows
gccmvs - find change that broke minizip
PDOS/370 - proper space allocation for files
Hercules/380 - implement "separate memory"
Hercules/380 - allow multiple tasks to use 64-bit registers
MVS/380 - put memmgr into Hercules/380 and adjust MVS380MN
PDOS/380 - adjust for "separate memory"
PDPCLIB for ASCII - make append do a seek
PDPCLIB for MVS - support update mode
uemacs for PDOS/3X0 - bug fixes
PDOS for 80386 - use API call to determine memory map
PDOS for 80386 - read more than 1 sector at a time
PDOS/386 - native IDE driver
PDOS/386 - native ethernet driver, TCP/IP stack
PDOS/3X0 to boot from FBA device, FAT-formatted
PDOS/386 to support virtual tape
PDOS/386 to support FBA and CKD EBCDIC disks as disks or files
build EBCDIC version of gcc and binutils for 80386
PDOS/386 to support running EBCDIC executables
document REVIEW for MVS
PosOpenFile to support POS_TEXT parameter
PDOS/386 to support automatic ASCII/EBCDIC translation and CRLF
fdisk and format utilities for ASCII PDOS
boot manager
PDOS/3X0 to support Pos API
GCCMVS to support -m64
MTS version of PDPCLIB
PDOS/386 USB driver
All version of PDOS to support command redirection
PDOS/386 to support EBCDIC FAT and CKD
PDPCLIB for MVS to support dd:include/stdio.h
PDPCLIB for MVS TSO to use IKJPARS for prefix resolution
PDPCLIB for MVS TSO to convert fred.c into c(fred)
makefile for windows pe executable using Pos* functions
make pload.com load windows pe instead of a.out
fix soft tabs in uemacs
get NT version of uemacs working
get NT WriteConsoleOutput working properly

Another thing I want to do is have a FAT32- file system
that supports files bigger than 4 GiB. The chaining
mechanism already allows that, so it's just the directory
entry that can't report the new size. What I would like is
that if the file size is (4 GiB - 1 MiB) or more, then the
file size % (4 GiB - 1 MiB) is the number of bytes used in
the last cluster, unless it is 0 in which case the last
cluster is full. 1 MiB is for 256 sectors per cluster with
a sector size of 4096. If sector size ever gets bigger than
this then the 1 MiB value will get bigger too. Note that
FAT32+ is another (unimplemented I think) proposal, hence
FAT32-.

Another thing I intend to do is obsolete a large part of the
Pos* API and switch to using a subset of C90 itself.
Specifically I will have a version of PDPCLIB that instead
of doing a PosOpenFile to access the OS, it will do an
fopen, or os_fopen, or os->fopen. And similar to the O_TEXT
that was used to extend Linux, the os_fopen will take "r"
or "rb" etc as a parameter, to allow the OS to translate
line endings, or convert text files from EBCDIC to ASCII or
whatever else may be desired. So the API would be fopen,
fread, fwrite, fseek. Replacing PosOpenFile, PosReadFile,
PosWriteFile, PosMoveFilePointer.

And another thing is that PDPCLIB will be updated to handle
generic "block" mode devices like disk drives with 512 or
4096 byte sectors, or variable length blocks as seen on MVS.
There is already block mode support but it is an independent
compilation for the mainframe. I instead want to integrate it.
Currently EFI works by requiring the caller to only request
blocks that are an even multiple of the sector size instead
of having the ability to buffer that itself.

Another thing I intend to do is support Vietnamese by using
something similar to VISCII but change the control characters
to not interfere with microemacs keystrokes.

And support Japanese by using Katakana (not Kanji) and the
Moon Array keyboard from 2channel (now 5channel). And make
toupper() on certain Katakana characters map to uppercase
English Alphabet characters - and for programs to not be
case-sensitive - so that there is no need to switch to an
English keyboard in order to use English language C90 programs.
Cultural changes to make a switch from Kanji to Katakana
are out of scope. Pandering to people who refuse to use their
own perfectly fine alphabet is also out of scope.

Another plan is to have a parameter or whatever to mfemul so
that instead of accessing a boot disk (VHD), all accesses to
files are assumed to be to the host filesystem, ie an implied
leading ":", so that the mainframe manipulates EBCDIC files
on the host. Although text files could still be stored as
ASCII and be auto-converted. This would be a relatively minor
change/detection in PosOpenFile or whatever in pdos.c to pass
requests on to the pseudo-bios.

Another possible plan is to switch from the various
PosWriteFile etc MSDOS API wrappers to the DosWrite etc
wrappers found in the MSDOS 4.0 source code - which have
a much better interface than the OS/2 functions of the
same name.

I might allow "dd:input;size=123456" as a filename for an
MVS file to constrain the size of a dataset and allow fast
seeking to end of file, as size information doesn't otherwise
appear to be available from the mvssupa.asm routines (possibly
the information is in the VTOC though, but that might in turn
require privilege).

Another idea is for an fopen of COM1: to be redirected - in
pdpclib - to a TCP/IP connection. Mainly for use in PdAndro.


Be aware of these extensions for practical use:

https://github.com/antoniusmisfit/pdos-goodies
https://github.com/antoniusmisfit/Runbat



OBTAINING
---------

The latest version currently resides at
http://pdos.sourceforge.net



DEVELOPING
----------

Code changes are most welcome! But the code changes must all
be public domain. Please refer to refer.txt for more information
on what needs to be done and how to do it.



CONTACTING THE AUTHOR
---------------------

If you have any enhancements to PDOS, please send
the code changes to mutazilah@gmail.com .
Please ensure that your code changes are public
domain, if you create a new source file, include
an explicit PD notice.  It is my intention to build
up a completely public domain system for use by EVERYONE,
including commercial vendors of operating systems.
Note that there is a Discord group that the author runs,
see link at the bottom of http://pdos.org
