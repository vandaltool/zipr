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
	; rax has the address of zest_cfi in the target module.
	pop r11
	pop r10
	pop r9
	pop r8
	pop rdi
	pop rsi
	pop rdx

	; we haven't restored all the registers here.  we leave rax and rcx to help issue the transfer.

	; decide if we need to check the transfer, or if it's allowed.
	; checking the xfer is done if the target module has a zest_cfi symbol.
	; rax contains the address, if the symbol exists.
	; rax is 0 if the symbol does not exist.
	cmp rax, 0
	je allow_xfer

check_xfer:
	mov rcx, rax 		; save zest_cfi address into rcx.
	mov rax, [rsp+8]	; restore rax.
	mov [rsp+8], rcx 	; write zest_cfi over rax location.
	pop rcx			; restore rcx.
	lea rsp, [rsp+8]	; skip over stack space for saved rax, which is now used for zest_zfi address.
	popf			; restore rest of state.
	lea rsp, [rsp+128]	; undo read zone
	jmp [rsp-128-8-8]	; jump to zest_cfi address stored on stack.  -128 for red zone, -8 for flags, -8 for rax.


allow_xfer:
	pop rcx
	pop rax
	popf
	lea rsp, [rsp+128]
	jmp r11
