; pgapre.asm - prefix code for PDOS-generic 16-bit programs
;
; I think the purpose of this is to allow some bytes to be
; removed from the start of the code and data section to
; compensate for the malloc overhead.
;
; This program written by Paul Edwards
; Released to the public domain

% .model memodel, c

extrn __crt0: proc

.stack 4000h

.data

; eyecatcher and padding
public __eyepad
__eyepad db 'XXXXXXXXXXXXXXXX'

.code

top:

public __intstart2
__intstart2 proc
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
nop
jmp __crt0
__intstart2 endp

end top
