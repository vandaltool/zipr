bits 64
global main
section .text
main:
	vfmaddpd xmm0, xmm1, xmm2, xmm3	
	vfmaddps xmm0, xmm1, xmm2, xmm3	
	vfmaddsd xmm0, xmm1, xmm2, xmm3	
	vfmaddss xmm0, xmm1, xmm2, xmm3	
	vfmaddsubpd xmm0, xmm1, xmm2, xmm3	
	vfmaddsubps xmm0, xmm1, xmm2, xmm3	
	vfmsubaddpd xmm0, xmm1, xmm2, xmm3	
	vfmsubaddps xmm0, xmm1, xmm2, xmm3	
	vfmsubpd xmm0, xmm1, xmm2, xmm3	
	vfmsubps xmm0, xmm1, xmm2, xmm3	
	vfmsubsd xmm0, xmm1, xmm2, xmm3	
	vfmsubss xmm0, xmm1, xmm2, xmm3	
	vfnmaddpd xmm0, xmm1, xmm2, xmm3
	vfnmaddps xmm0, xmm1, xmm2, xmm3
	vfnmaddsd xmm0, xmm1, xmm2, xmm3
	vfnmaddss xmm0, xmm1, xmm2, xmm3
	vfnmsubpd xmm0, xmm1, xmm2, xmm3
	vfnmsubps xmm0, xmm1, xmm2, xmm3
	vfnmsubsd xmm0, xmm1, xmm2, xmm3
	vfnmsubss xmm0, xmm1, xmm2, xmm3

	vfmaddpd xmm0, xmm1, [rsp+1024], xmm3	
	vfmaddps xmm0, xmm1, [rsp+1024], xmm3	
	vfmaddsd xmm0, xmm1, [rsp+1024], xmm3	
	vfmaddss xmm0, xmm1, [rsp+1024], xmm3	
	vfmaddsubpd xmm0, xmm1, [rsp+1024], xmm3	
	vfmaddsubps xmm0, xmm1, [rsp+1024], xmm3	
	vfmsubaddpd xmm0, xmm1, [rsp+1024], xmm3	
	vfmsubaddps xmm0, xmm1, [rsp+1024], xmm3	
	vfmsubpd xmm0, xmm1, [rsp+1024], xmm3	
	vfmsubps xmm0, xmm1, [rsp+1024], xmm3	
	vfmsubsd xmm0, xmm1, [rsp+1024], xmm3	
	vfmsubss xmm0, xmm1, [rsp+1024], xmm3	
	vfnmaddpd xmm0, xmm1, [rsp+1024], xmm3
	vfnmaddps xmm0, xmm1, [rsp+1024], xmm3
	vfnmaddsd xmm0, xmm1, [rsp+1024], xmm3
	vfnmaddss xmm0, xmm1, [rsp+1024], xmm3
	vfnmsubpd xmm0, xmm1, [rsp+1024], xmm3
	vfnmsubps xmm0, xmm1, [rsp+1024], xmm3
	vfnmsubsd xmm0, xmm1, [rsp+1024], xmm3
	vfnmsubss xmm0, xmm1, [rsp+1024], xmm3

	vfmaddpd xmm0, xmm1, xmm2, [rsp+1024]	
	vfmaddps xmm0, xmm1, xmm2, [rsp+1024]	
	vfmaddsd xmm0, xmm1, xmm2, [rsp+1024]	
	vfmaddss xmm0, xmm1, xmm2, [rsp+1024]	
	vfmaddsubpd xmm0, xmm1, xmm2, [rsp+1024]	
	vfmaddsubps xmm0, xmm1, xmm2, [rsp+1024]	
	vfmsubaddpd xmm0, xmm1, xmm2, [rsp+1024]	
	vfmsubaddps xmm0, xmm1, xmm2, [rsp+1024]	
	vfmsubpd xmm0, xmm1, xmm2, [rsp+1024]	
	vfmsubps xmm0, xmm1, xmm2, [rsp+1024]	
	vfmsubsd xmm0, xmm1, xmm2, [rsp+1024]	
	vfmsubss xmm0, xmm1, xmm2, [rsp+1024]	
	vfnmaddpd xmm0, xmm1, xmm2, [rsp+1024]
	vfnmaddps xmm0, xmm1, xmm2, [rsp+1024]
	vfnmaddsd xmm0, xmm1, xmm2, [rsp+1024]
	vfnmaddss xmm0, xmm1, xmm2, [rsp+1024]
	vfnmsubpd xmm0, xmm1, xmm2, [rsp+1024]
	vfnmsubps xmm0, xmm1, xmm2, [rsp+1024]
	vfnmsubsd xmm0, xmm1, xmm2, [rsp+1024]
	vfnmsubss xmm0, xmm1, xmm2, [rsp+1024]

; test ymm versions
	vfmaddpd ymm0, ymm1, ymm2, ymm3	
	vfmaddps ymm0, ymm1, ymm2, ymm3	
	vfmaddsubpd ymm0, ymm1, ymm2, ymm3	
	vfmaddsubps ymm0, ymm1, ymm2, ymm3	
	vfmsubaddpd ymm0, ymm1, ymm2, ymm3	
	vfmsubaddps ymm0, ymm1, ymm2, ymm3	
	vfmsubpd ymm0, ymm1, ymm2, ymm3	
	vfmsubps ymm0, ymm1, ymm2, ymm3	
	vfnmaddpd ymm0, ymm1, ymm2, ymm3
	vfnmaddps ymm0, ymm1, ymm2, ymm3
	vfnmsubpd ymm0, ymm1, ymm2, ymm3
	vfnmsubps ymm0, ymm1, ymm2, ymm3

	vfmaddpd ymm0, ymm1, [rsp+1024], ymm3	
	vfmaddps ymm0, ymm1, [rsp+1024], ymm3	
	vfmaddsubpd ymm0, ymm1, [rsp+1024], ymm3	
	vfmaddsubps ymm0, ymm1, [rsp+1024], ymm3	
	vfmsubaddpd ymm0, ymm1, [rsp+1024], ymm3	
	vfmsubaddps ymm0, ymm1, [rsp+1024], ymm3	
	vfmsubpd ymm0, ymm1, [rsp+1024], ymm3	
	vfmsubps ymm0, ymm1, [rsp+1024], ymm3	
	vfnmaddpd ymm0, ymm1, [rsp+1024], ymm3
	vfnmaddps ymm0, ymm1, [rsp+1024], ymm3
	vfnmsubpd ymm0, ymm1, [rsp+1024], ymm3
	vfnmsubps ymm0, ymm1, [rsp+1024], ymm3

	vfmaddpd ymm0, ymm1, ymm2, [rsp+1024]	
	vfmaddps ymm0, ymm1, ymm2, [rsp+1024]	
	vfmaddsubpd ymm0, ymm1, ymm2, [rsp+1024]	
	vfmaddsubps ymm0, ymm1, ymm2, [rsp+1024]	
	vfmsubaddpd ymm0, ymm1, ymm2, [rsp+1024]	
	vfmsubaddps ymm0, ymm1, ymm2, [rsp+1024]	
	vfmsubpd ymm0, ymm1, ymm2, [rsp+1024]	
	vfmsubps ymm0, ymm1, ymm2, [rsp+1024]	
	vfnmaddpd ymm0, ymm1, ymm2, [rsp+1024]
	vfnmaddps ymm0, ymm1, ymm2, [rsp+1024]
	vfnmsubpd ymm0, ymm1, ymm2, [rsp+1024]
	vfnmsubps ymm0, ymm1, ymm2, [rsp+1024]

