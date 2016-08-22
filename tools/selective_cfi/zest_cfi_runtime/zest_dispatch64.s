BITS 64

default rel

extern zest_cfi_dispatch_c

section .text
global zest_cfi_dispatch:function
zest_cfi_dispatch:

	lea rsp, [rsp-128]
	pushf
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	mov rdi, r11	; r11 is the cfi reg for x86-64, copy it to rdi for C-calling convention.
	call zest_cfi_dispatch_c wrt ..plt
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	popf
	lea rsp, [rsp+128]
	ret
