BITS 64

default rel

extern zest_cfi_dispatch_c

%define cfi_dispatch_translation_cache_entries 16*1024

section .tls alloc write align=8 tls

global translation_cache_src
translation_cache_src:  times cfi_dispatch_translation_cache_entries dq 0 
translation_cache_targ: times cfi_dispatch_translation_cache_entries dq 0 


section .text
global zest_cfi_dispatch:function
zest_cfi_dispatch:

	; not saving rbx, rsp, rbp, r12-r15 as C code should save them.
	lea rsp, [rsp-128]
	pushf
	push rax
	push rcx

	; check translation cache.
	mov   rax,[rel translation_cache_src wrt ..gottpoff] 
	
	; rax contains tls address of translation_cache_src

	; hash target 
	mov   rcx, r11
	and   rcx, ((cfi_dispatch_translation_cache_entries) -1)

	cmp r11, [fs:rax+rcx*8]
	jne enter_c_code

allow_xfer_fast:
	mov rax, [fs:rax+rcx*8+cfi_dispatch_translation_cache_entries*8]
		

decide_to_check:
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




enter_c_code:
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

	
	; rax contains tls address of translation_cache_src

	; hash target 
	mov rcx, r11
	and rcx, ((cfi_dispatch_translation_cache_entries) -1)
	lea rcx, [rcx*8]
	add rcx, [rel translation_cache_src wrt ..gottpoff] 
	mov [fs:rcx], r11
	mov [fs:rcx+cfi_dispatch_translation_cache_entries*8], rax

	; resume computation
	jmp decide_to_check
