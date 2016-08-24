BITS 64

default rel

extern zest_cfi_dispatch_c

section .text
global zest_cfi_dispatch:function
zest_cfi_dispatch:

	; not saving rbx, rsp, rbp, r12-r15 as C code should save them.
	lea rsp, [rsp-128]
	pushf
	push rax
	push rcx
	push rdx
	push rsi
	push rdi
	push r8
	push r9
	push r10
	push r11
	mov rdi, r11	; r11 is the cfi reg for x86-64, copy it to rdi for C-calling convention.
	call zest_cfi_dispatch_c wrt ..plt
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rax
	popf
	lea rsp, [rsp+128]
	ret
