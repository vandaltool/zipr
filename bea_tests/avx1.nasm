bits 64
blendvpd xmm2,xmm1,xmm0

vblendvpd xmm2,xmm1,xmm0,xmm0
vblendvpd xmm2,xmm1,xmm0
vblendvpd ymm2,ymm1,ymm0,ymm0
vblendvpd ymm2,ymm1,ymm0

vcvtsi2sd xmm9,xmm10,ecx
vcvtsi2sd xmm9,xmm10,rcx
vcvtsi2sd xmm9,xmm10,dword [rdi]
vcvtsi2sd xmm9,xmm10,qword [rdi]


; can't figure out how to encode this w/o manually specifying the bytes, as nasm prefers the VEX encoding
; which disassembles slightly differently (although, still correctly)
; pextrb byte [rax],xmm1,0x33
db 0x66, 0x0f, 0x3a, 0x14,0x08,0x33
vpextrb [rax],xmm1,0x33

; can't figure out how to encode this w/o manually specifying the bytes, as nasm prefers the VEX encoding
; which disassembles slightly differently (although, still correctly)
; pextrw byte [rax],xmm1,0x33
db 0x66, 0x0f, 0x3a, 0x15,0x08,0x33
vpextrw [rax],xmm1,0x33

; can't figure out how to encode this w/o manually specifying the bytes, as nasm prefers the VEX encoding
; which disassembles slightly differently (although, still correctly)
; pextrd [rax],xmm1,0x33
db 0x66, 0x0f, 0x3a, 0x16,0x08,0x33		; no REX 
db 0x66, 0x40, 0x0f, 0x3a, 0x16, 0x08, 0x33 	; REX.W=1 -> pextrd
vpextrd [rax],xmm1,0x33

db 0x66, 0x48, 0x0f, 0x3a, 0x16, 0x08, 0x33 ; REX.W=1 -> pextrq
vpextrq [rax],xmm1,0x33

; here
vpextrb rax,xmm1,0x33

; can't figure out how to encode this w/o manually specifying the bytes, as nasm prefers the VEX encoding
; which disassembles slightly differently (although, still correctly)
; pextrw byte rax,xmm1,0x33
db 0x66, 0x0f, 0xc5, 0xc1, 0x33
vpextrw rax,xmm1,0x33

pextrq rax,xmm1,0x33
pextrb eax,xmm1,0x33
pextrw eax,xmm1,0x33
pextrd eax,xmm1,0x33

vpextrd rax,xmm1,0x33
vpextrq rax,xmm1,0x33
vpextrb eax,xmm1,0x33
vpextrw eax,xmm1,0x33
vpextrd eax,xmm1,0x33

vcvtpd2ps xmm0,xmm1
vcvtpd2ps xmm0,oword [rsi]
vcvtpd2ps xmm0,ymm1
vcvtpd2ps xmm0,yword [rsi]
; vcvtpd2ps xmm0,[rsi]

vcvtpd2dq xmm0,xmm1
vcvtpd2dq xmm0,oword [rsi]
vcvtpd2dq xmm0,ymm1
vcvtpd2dq xmm0,yword [rsi]
; vcvtpd2dq xmm0,[rsi]

vcvttpd2dq xmm0,xmm1
vcvttpd2dq xmm0,oword [rsi]
vcvttpd2dq xmm0,ymm1
vcvttpd2dq xmm0,yword [rsi]
; vcvttpd2dq xmm0,[rsi]
