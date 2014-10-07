%ifdef CGC
section .text


BITS 32

global cgc_terminate
cgc_terminate:
	mov    eax,0x1
	push   ebx
	mov    ebx, [esp+0x8]
	int    0x80
	pop    ebx
	ret    

global cgc_transmit
cgc_transmit:
	mov    eax,0x2
	push   ebx
	push   ecx
	push   edx
	push   esi
	mov    ebx, [esp+0x14]
	mov    ecx, [esp+0x18]
	mov    edx, [esp+0x1c]
	mov    esi, [esp+0x20]
	int    0x80
	pop    esi
	pop    edx
	pop    ecx
	pop    ebx
	ret    


global cgc_receive
cgc_receive:
	mov    eax,0x3
	push   ebx
	push   ecx
	push   edx
	push   esi
	mov    ebx, [esp+0x14]
	mov    ecx, [esp+0x18]
	mov    edx, [esp+0x1c]
	mov    esi, [esp+0x20]
	int    0x80
	pop    esi
	pop    edx
	pop    ecx
	pop    ebx
	ret    

global cgc_fdwait
cgc_fdwait:
	mov    eax,0x4
	push   ebx
	push   ecx
	push   edx
	push   esi
	push   edi
	mov    ebx, [esp+0x18]
	mov    ecx, [esp+0x1c]
	mov    edx, [esp+0x20]
	mov    esi, [esp+0x24]
	mov    edi, [esp+0x28]
	int    0x80
	pop    edi
	pop    esi
	pop    edx
	pop    ecx
	pop    ebx
	ret    

global cgc_allocate
cgc_allocate:
	mov    eax,0x5
	push   ebx
	push   ecx
	push   edx
	mov    ebx, [esp+0x10]
	mov    ecx, [esp+0x14]
	mov    edx, [esp+0x18]
	int    0x80
	pop    edx
	pop    ecx
	pop    ebx
	ret    

global deallocate
cgc_deallocate:
	mov    eax,0x6
	push   ebx
	push   ecx
	mov    ebx, [esp+0xc]
	mov    ecx, [esp+0x10]
	int    0x80
	pop    ecx
	pop    ebx
	ret    

global cgc_random
cgc_random:
	mov    eax,0x7
	push   ebx
	push   ecx
	push   edx
	mov    ebx, [esp+0x10]
	mov    ecx, [esp+0x14]
	mov    edx, [esp+0x18]
	int    0x80
	pop    edx
	pop    ecx
	pop    ebx
	ret    

%else

global syscall
syscall:
	push   edi
	push   esi
	push   ebx
	mov    edi, [esp+0x24]
	mov    esi, [esp+0x20]
	mov    edx, [esp+0x1c]
	mov    ecx, [esp+0x18]
	mov    ebx, [esp+0x14]
	mov    eax, [esp+0x10]
	int    0x80
	pop    ebx
	pop    esi
	pop    edi
	ret    


%endif
