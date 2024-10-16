# l64supb.asm - support code for C programs for x64 Linux
# For code compiled with the cc64 compiler that follows the
# Microsoft Win64 calling convention
#
# This program written by Paul Edwards
# Released to the public domain

# syscall numbers can be found here:
# https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md


.code64



.intel_syntax noprefix



# This is the new entry point. We simply capture
# the current status of the stack and pass it on
# to C code (linstart.c, not start.c) figure out
# the rest. However, it seems that the MSABI
# requires some stack to be available so that the
# 4 register parameters can be saved if required.
# Which seems to be what cc64 does unconditionally.
# Which means we wipe out the first 4 arguments
# (argv[x]) that were put on the stack. argc survives
# presumably because it is in the spot where the
# return address normally is. Bottom line is we
# need to subtract 48 bytes from the stack before
# things get clobbered.

.globl ___pdpstart
___pdpstart:

mov rcx, rsp
sub rsp, 48
call _start

# we shouldn't get here, but if we do, loop
# instead of doing something random
loop1: jmp loop1





.globl __write
__write:

push rdi
push rsi
push rdx

# function code 1 = write
mov rax, 1

# handle
xor rdi, rdi
mov edi, ecx
# data pointer
mov rsi, rdx
# length
xor rdx, rdx
mov edx, r8d

syscall

pop rdx
pop rsi
pop rdi

ret





# int ___ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);

.globl __ioctl
__ioctl:

push rdi
push rsi
push rdx

# function code 16 = ioctl
mov rax, 16

# handle
xor rdi, rdi
mov edi, ecx
# cmd
mov rsi, rdx
# arg
xor rdx, rdx
mov rdx, r8

syscall

pop rdx
pop rsi
pop rdi

ret




.globl __open
__open:

push rdi
push rsi
push rdx

# function code 2 = open
mov rax, 2

# filename
mov rdi,rcx
# flag
xor rsi, rsi
mov esi, edx
# mode
xor rdx, rdx
mov edx, r8d

syscall

pop rdx
pop rsi
pop rdi

ret



.globl __read
__read:

push rdi
push rsi
push rdx

# function code 0 = read
mov rax, 0

# handle
xor rdi, rdi
mov edi, ecx
# data pointer
mov rsi, rdx
# length
# Not sure if this is 32 bits or 64 bits
# cc64 will be using a 32-bit value. And may not
# be zeroing the high 32 bits. Linux may require
# a full clean 64-bit value
xor rdx, rdx
mov edx, r8d
#mov rdx, r8

syscall

pop rdx
pop rsi
pop rdi

ret



.globl __close
__close:

push rdi

# function code 3 = close
mov rax, 3

# handle
xor rdi, rdi
mov edi, ecx

syscall

pop rdi

ret




.globl __seek
__seek:

push rdi
push rsi
push rdx

# function code 8 = lseek
mov rax, 8

# handle
xor rdi, rdi
mov edi, ecx
# offset
mov rsi, rdx
# whence
xor rdx, rdx
mov edx, r8d

syscall

pop rdx
pop rsi
pop rdi

ret



.globl __exita
__exita:

# exit/terminate
mov rax, 60

mov rdi, rcx

syscall

# Shouldn't get here
loop2: jmp loop2

ret





.globl __rename
__rename:

push rdi
push rsi

# function code 82 = rename
mov rax, 82

# old file
mov rdi, rcx
# new file
mov rsi, rdx

syscall

pop rsi
pop rdi

ret




.globl __remove
__remove:

push rdi

# function code 87 = unlink
mov rax, 87

# filename
mov rdi, rcx

syscall

pop rdi

ret




.globl __time
__time:

push rdi

# function code 201 = retrieve current time
mov rax, 201

# pointer to time_t
mov rdi, rcx

syscall

pop rdi

ret





.globl __getpid
__getpid:

# function code 39 = getpid
mov rax, 39

syscall

ret





.globl __clone
__clone:

push rbp
mov rbp, rsp
push rdi
push rsi
push rdx
push r10
push r8

# function code 56 = clone
mov rax, 56

mov rdi, rcx
mov rsi, rdx
mov rdx, r8
mov r10, r9

mov r8, 48[rbp]

syscall

pop r8
pop r10
pop rdx
pop rsi
pop rdi
pop rbp

ret




.globl __waitid
__waitid:

push rbp
mov rbp, rsp
push rdi
push rsi
push rdx
push r10
push r8

# function code 247 = waitid
mov rax, 247

mov rdi, rcx
mov rsi, rdx
mov rdx, r8
mov r10, r9

mov r8, 48[rbp]

syscall

pop r8
pop r10
pop rdx
pop rsi
pop rdi
pop rbp

ret




.globl __execve
__execve:

push rdi
push rsi
push rdx

# function code 59 = execve
mov rax, 59

# filename
mov rdi, rcx
# argv
mov rsi, rdx
# envp
mov rdx, r8

syscall

pop rdx
pop rsi
pop rdi

ret







# 7 parameters received, but only 6 used

.globl __mmap
__mmap:

push rbp
mov rbp, rsp
push rdi
push rsi
push rdx
push r10
push r8
push r9

# function code 9 = mmap
mov rax, 9

mov rdi, rcx
mov rsi, rdx
mov rdx, r8
mov r10, r9

mov r8, 48[rbp]
mov r9, 56[rbp]

syscall

pop r9
pop r8
pop r10
pop rdx
pop rsi
pop rdi
pop rbp

ret





.globl __munmap
__munmap:

push rdi
push rsi

# function code 11 = munmap
mov rax, 11

# addr
mov rdi, rcx
# len
mov rsi, rdx

syscall

pop rsi
pop rdi

ret



# mremap is 25





.globl __chdir
__chdir:

push rdi

# function code 80 = chdir
mov rax, 80

# filename (directory name)
mov rdi, rcx

syscall

pop rdi

ret





.globl __rmdir
__rmdir:

push rdi

# function code 84 = rmdir
mov rax, 84

# pathname
mov rdi, rcx

syscall

pop rdi

ret






.globl ___mkdir
___mkdir:
.globl __mkdir
__mkdir:

push rdi
push rsi

# function code 83 = mkdir
mov rax, 83

# pathname
mov rdi, rcx
# mode
mov rsi, rdx

syscall

pop rsi
pop rdi

ret





.globl __getdents
__getdents:

push rdi
push rsi
push rdx

# function code 78 = getdents
mov rax, 78

# file descriptor
xor rdi, rdi
mov edi, ecx
# dirent
mov rsi, rdx
# count
xor rdx, rdx
mov edx, r8d

syscall

pop rdx
pop rsi
pop rdi

ret






.globl __mprotect
__mprotect:

push rdi
push rsi
push rdx

# function code 10 = mprotect
mov rax, 10

# start
mov rdi, rcx
# len
mov rsi, rdx
# prot
mov rdx, r8

syscall

pop rdx
pop rsi
pop rdi

ret





# I think rcx will have env
# And it looks like we only need to save rbx, rcx, rdx, r8, r9, rsp
# plus return address
# And first location is reserved for return value
# (second is for return address)
.globl __setj
__setj:
mov [rcx + 8*2], rbx
mov [rcx + 8*3], rcx
mov [rcx + 8*4], rdx
mov [rcx + 8*5], r8
mov [rcx + 8*6], r9
mov [rcx + 8*7], rsp
mov [rcx + 8*8], rsi
mov [rcx + 8*9], rdi
mov [rcx + 8*10], r10
mov [rcx + 8*11], r11
mov [rcx + 8*12], r12
mov [rcx + 8*13], r13
mov [rcx + 8*14], r14
mov [rcx + 8*15], r15
mov [rcx + 8*16], rbp
mov rax, [rsp]
mov [rcx + 8*1], rax
# we only return int (eax), but since we're not preserving
# rax anyway (maybe we should?), may as well clear it.
xor rax,rax
ret




# I think rcx will have env
.globl __longj
__longj:
mov rax, [rcx + 8*7]
mov rsp, rax
mov rax, [rcx + 8*1]
mov [rsp], rax
mov rbx, [rcx + 8*2]
mov rdx, [rcx + 8*4]
mov r8, [rcx + 8*5]
mov r9, [rcx + 8*6]
mov rsi, [rcx + 8*8]
mov rdi, [rcx + 8*9]
mov r10, [rcx + 8*10]
mov r11, [rcx + 8*11]
mov r12, [rcx + 8*12]
mov r13, [rcx + 8*13]
mov r14, [rcx + 8*14]
mov r15, [rcx + 8*15]
mov rbp, [rcx + 8*16]
mov rax, [rcx]
mov rcx, [rcx + 8*3]
ret







# These routines were copied (and then modified) from bcclib.asm generated
# by the public domain cc64, and are used for cc64
# (Converted to PDAS .intel_syntax noprefix by guessing.)

.globl m$ufloat_r64u32
m$ufloat_r64u32:
	mov ecx, ecx					# clear top half (already done if value just moved there)
	cvtsi2sd xmm15, rcx
	ret

.globl m$ufloat_r32u32
m$ufloat_r32u32:
	mov ecx, ecx
	cvtsi2ss xmm15, rcx
	ret

.globl m$ufloat_r64u64
m$ufloat_r64u64:
#jjj: jmp jjj
#mov rax, 0x1234567
#mov rax, 1234567
#xorpd xmm15, xmm15
#ret

	cmp ecx, 0
	jl fl1
#number is positive, so can treat like i64
	cvtsi2sd xmm15, rcx
	ret
fl1:						#negative value
	and rcx, QWORD PTR mask63[rip]		#clear top bit (subtract 2**63)
	cvtsi2sd xmm15, rcx
	addsd xmm15, QWORD PTR offset64[rip]	#(add 2**63 back to result)
	ret

.globl m$ufloat_r32u64
m$ufloat_r32u64:
	cmp ecx, 0
	jl fl2
#number is positive, so can treat like i64
	cvtsi2ss xmm15, rcx
	ret
fl2:						#negative value
	and rcx, QWORD PTR mask63[rip]		#clear top bit (subtract 2**63)
	cvtsi2ss xmm15, rcx
	addss xmm15, DWORD PTR offset32[rip]	#(add 2**63 back to result)
	ret

.globl m$pushcallback
m$pushcallback:
	incd __ncallbacks[rip]
        push r12
	mov r12,__ncallbacks[rip]
	shl r12,6					#8x8 bytes is size per entry
	lea r11,callbackstack[rip]
	add r11,r12
        pop r12

	mov [r11],rbx
	mov [r11+8],rsi
	mov [r11+16],rdi
	mov [r11+24],r12
	mov [r11+32],r13
	mov [r11+40],r14
	mov [r11+48],r15
	ret

.globl m$popcallback
m$popcallback:
        mov r12,__ncallbacks[rip]
	shl r12,6					#8x8 bytes is size per entry
	lea r11,callbackstack[rip]
	add r11,r12
	mov rbx,[r11]
	mov rsi,[r11+8]
	mov rdi,[r11+16]
	mov r12,[r11+24]
	mov r13,[r11+32]
	mov r14,[r11+40]
	mov r15,[r11+48]
	decd __ncallbacks[rip]
	ret

.data

.globl _fltused
.p2align 2
_fltused:
.space 4

.section rdata
mask63:
	.quad 0x7fffffffffffffff
offset64:
	.quad 9223372036854775808		# 2**63 as r64
offset32:
	.quad 9223372036854775808		# 2**63 as r32


.bss
callbackstack:
	.space 576			#8-level stack
#	resb 5'120'000

.globl __ncallbacks
__ncallbacks:
	.space 8
