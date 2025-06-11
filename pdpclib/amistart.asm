#######################################################################
#                                                                     #
#  This program written by Paul Edwards.                              #
#  Released to the public domain                                      #
#                                                                     #
#######################################################################
#######################################################################
#                                                                     #
#  amistart - startup code for AmigaOS.                               #
#                                                                     #
#  This uses the Motorola syntax                                      #
#                                                                     #
#  It puts the standard AmigaOS registers containing the command      #
#  length (d0) and command buffer (a0) as well as the AmigaPDOS       #
#  extension register (only visible if running AmigaPDOS) d7,         #
#  containing an alternative SysBase to use (only if d0 is greater    #
#  than or equal to 2 GiB, and in which case, 2 GiB should be         #
#  subtracted from d0 before use) on the stack.                       #
#                                                                     #
#  All this manipulation is left for the C startup code to handle.    #
#                                                                     #
#######################################################################

        .text
        .globl ___amistart
___amistart:
        move.l  sp,savedSP

# Don't use movem to push the arguments because the order
# is not what you code yourself, but other rules.
        move.l  d7,-(sp)
        move.l  a0,-(sp)
        move.l  d0,-(sp)

        jsr     ___start
        lea     12(sp),sp
        rts

# This function is required by GCC but isn't used for anything
        .globl ___main
___main:
        rts

# This function receives a return code as a parameter. The stack
# then needs to be restored and the parameter placed in register d0
# prior to return to the OS.

# This is basically acting as a "poor man's longjmp". While it will
# return to the correct address, none of the registers are expected
# to be the same as what they were on entry. A normal operating
# system like AmigaDOS is probably not dependent on a user's program
# behaving nicely, but PDOS-generic is, so will get a crash on return.

        .globl ___exita
___exita:
        move.l  4(sp),d0
        move.l  savedSP,sp
        rts

# This dummy data exists to force pdld to put the data section
# before the BSS section. The intention is that the loader code
# (exeload.c) should be changed to allow the BSS to appear before
# the data, and also for the loading to be done in a way that the
# executable in memory matches the map produced by pdld. Until
# that is done, this workaround is being used, and the data and
# BSS do not match the map. And the BSS is currently not being
# cleared either, so that needs to be fixed too. And the executable
# is constrained to be 1 MB - that needs to be fixed too.
.data
.long 0xCAFECAFE

        .bss
savedSP:
        .skip 4
