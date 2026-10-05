# SSE / SSE2 instructions added to (or fixed in) x86_64-asm.h, one line per
# operand form, in AT&T syntax.  Used by:
#   ..\asm_gate.bat                    - TCC must assemble these to the bytes in
#                                        sse_sse2_expected.h
#   ..\manual\asm_sse_sse2_vs_clang.bat - TCC and clang must disassemble to the
#                                        same text (needs LLVM_BIN)
# Only %xmm0-%xmm7 (TCC does not know %xmm8-%xmm15).  cvtsi2ss / cvtsi2sd with
# a memory source are left out: clang needs a size suffix there that TCC does
# not take.
    .text
    .globl sse_list
sse_list:
	andps %xmm1, %xmm2
	andps 16(%rax), %xmm3
	andnps %xmm1, %xmm2
	andnps 16(%rax), %xmm3
	orps %xmm1, %xmm2
	orps 16(%rax), %xmm3
	xorps %xmm1, %xmm2
	xorps 16(%rax), %xmm3
	addss %xmm1, %xmm2
	addss 16(%rax), %xmm3
	subss %xmm1, %xmm2
	subss 16(%rax), %xmm3
	mulss %xmm1, %xmm2
	mulss 16(%rax), %xmm3
	divss %xmm1, %xmm2
	divss 16(%rax), %xmm3
	sqrtss %xmm1, %xmm2
	sqrtss 16(%rax), %xmm3
	rsqrtss %xmm1, %xmm2
	rsqrtss 16(%rax), %xmm3
	minss %xmm1, %xmm2
	minss 16(%rax), %xmm3
	maxss %xmm1, %xmm2
	maxss 16(%rax), %xmm3
	unpcklps %xmm1, %xmm2
	unpcklps 16(%rax), %xmm3
	unpckhps %xmm1, %xmm2
	unpckhps 16(%rax), %xmm3
	comiss %xmm1, %xmm2
	comiss 16(%rax), %xmm3
	ucomiss %xmm1, %xmm2
	ucomiss 16(%rax), %xmm3
	rcpps %xmm1, %xmm2
	rcpps 16(%rax), %xmm3
	rcpss %xmm1, %xmm2
	rcpss 16(%rax), %xmm3
	addpd %xmm1, %xmm2
	addpd 16(%rax), %xmm3
	addsd %xmm1, %xmm2
	addsd 16(%rax), %xmm3
	subpd %xmm1, %xmm2
	subpd 16(%rax), %xmm3
	subsd %xmm1, %xmm2
	subsd 16(%rax), %xmm3
	mulpd %xmm1, %xmm2
	mulpd 16(%rax), %xmm3
	mulsd %xmm1, %xmm2
	mulsd 16(%rax), %xmm3
	divpd %xmm1, %xmm2
	divpd 16(%rax), %xmm3
	divsd %xmm1, %xmm2
	divsd 16(%rax), %xmm3
	sqrtpd %xmm1, %xmm2
	sqrtpd 16(%rax), %xmm3
	sqrtsd %xmm1, %xmm2
	sqrtsd 16(%rax), %xmm3
	minpd %xmm1, %xmm2
	minpd 16(%rax), %xmm3
	minsd %xmm1, %xmm2
	minsd 16(%rax), %xmm3
	maxpd %xmm1, %xmm2
	maxpd 16(%rax), %xmm3
	maxsd %xmm1, %xmm2
	maxsd 16(%rax), %xmm3
	andpd %xmm1, %xmm2
	andpd 16(%rax), %xmm3
	andnpd %xmm1, %xmm2
	andnpd 16(%rax), %xmm3
	orpd %xmm1, %xmm2
	orpd 16(%rax), %xmm3
	xorpd %xmm1, %xmm2
	xorpd 16(%rax), %xmm3
	comisd %xmm1, %xmm2
	comisd 16(%rax), %xmm3
	ucomisd %xmm1, %xmm2
	ucomisd 16(%rax), %xmm3
	unpcklpd %xmm1, %xmm2
	unpcklpd 16(%rax), %xmm3
	unpckhpd %xmm1, %xmm2
	unpckhpd 16(%rax), %xmm3
	cvtsd2ss %xmm1, %xmm2
	cvtsd2ss 16(%rax), %xmm3
	cvtss2sd %xmm1, %xmm2
	cvtss2sd 16(%rax), %xmm3
	cvtpd2ps %xmm1, %xmm2
	cvtpd2ps 16(%rax), %xmm3
	cvtps2pd %xmm1, %xmm2
	cvtps2pd 16(%rax), %xmm3
	cvtpd2dq %xmm1, %xmm2
	cvtpd2dq 16(%rax), %xmm3
	cvttpd2dq %xmm1, %xmm2
	cvttpd2dq 16(%rax), %xmm3
	cvtdq2pd %xmm1, %xmm2
	cvtdq2pd 16(%rax), %xmm3
	cvtdq2ps %xmm1, %xmm2
	cvtdq2ps 16(%rax), %xmm3
	cvtps2dq %xmm1, %xmm2
	cvtps2dq 16(%rax), %xmm3
	cvttps2dq %xmm1, %xmm2
	cvttps2dq 16(%rax), %xmm3
	punpcklqdq %xmm1, %xmm2
	punpcklqdq 16(%rax), %xmm3
	punpckhqdq %xmm1, %xmm2
	punpckhqdq 16(%rax), %xmm3
	addps %xmm1, %xmm2
	addps 16(%rax), %xmm3
	mulps %xmm1, %xmm2
	mulps 16(%rax), %xmm3
	sqrtps %xmm1, %xmm2
	sqrtps 16(%rax), %xmm3
	paddd %xmm1, %xmm2
	paddd 16(%rax), %xmm3
	cmpps $0x1b, %xmm1, %xmm2
	cmpps $0x4, 16(%rax), %xmm3
	cmpss $0x1b, %xmm1, %xmm2
	cmpss $0x4, 16(%rax), %xmm3
	shufps $0x1b, %xmm1, %xmm2
	shufps $0x4, 16(%rax), %xmm3
	cmppd $0x1b, %xmm1, %xmm2
	cmppd $0x4, 16(%rax), %xmm3
	cmpsd $0x1b, %xmm1, %xmm2
	cmpsd $0x4, 16(%rax), %xmm3
	shufpd $0x1b, %xmm1, %xmm2
	shufpd $0x4, 16(%rax), %xmm3
	pshufd $0x1b, %xmm1, %xmm2
	pshufd $0x4, 16(%rax), %xmm3
	pshufhw $0x1b, %xmm1, %xmm2
	pshufhw $0x4, 16(%rax), %xmm3
	pshuflw $0x1b, %xmm1, %xmm2
	pshuflw $0x4, 16(%rax), %xmm3
	paddq %xmm1, %xmm2
	paddq 16(%rax), %xmm3
	paddq %mm1, %mm2
	paddq 8(%rax), %mm3
	psubq %xmm1, %xmm2
	psubq 16(%rax), %xmm3
	psubq %mm1, %mm2
	psubq 8(%rax), %mm3
	pmuludq %xmm1, %xmm2
	pmuludq 16(%rax), %xmm3
	pmuludq %mm1, %mm2
	pmuludq 8(%rax), %mm3
	pmulhuw %xmm1, %xmm2
	pmulhuw 16(%rax), %xmm3
	pmulhuw %mm1, %mm2
	pmulhuw 8(%rax), %mm3
	psadbw %xmm1, %xmm2
	psadbw 16(%rax), %xmm3
	psadbw %mm1, %mm2
	psadbw 8(%rax), %mm3
	pavgb %xmm1, %xmm2
	pavgb 16(%rax), %xmm3
	pavgb %mm1, %mm2
	pavgb 8(%rax), %mm3
	pavgw %xmm1, %xmm2
	pavgw 16(%rax), %xmm3
	pavgw %mm1, %mm2
	pavgw 8(%rax), %mm3
	movhlps %xmm1, %xmm2
	movlhps %xmm1, %xmm2
	maskmovdqu %xmm1, %xmm2
	movlps 16(%rax), %xmm3
	movlps %xmm4, 32(%rcx)
	movhpd 16(%rax), %xmm3
	movhpd %xmm4, 32(%rcx)
	movlpd 16(%rax), %xmm3
	movlpd %xmm4, 32(%rcx)
	movss %xmm1, %xmm2
	movss 16(%rax), %xmm3
	movss %xmm4, 32(%rcx)
	movsd %xmm1, %xmm2
	movsd 16(%rax), %xmm3
	movsd %xmm4, 32(%rcx)
	movapd %xmm1, %xmm2
	movapd 16(%rax), %xmm3
	movapd %xmm4, 32(%rcx)
	movupd %xmm1, %xmm2
	movupd 16(%rax), %xmm3
	movupd %xmm4, 32(%rcx)
	movdqa %xmm1, %xmm2
	movdqa 16(%rax), %xmm3
	movdqa %xmm4, 32(%rcx)
	movdqu %xmm1, %xmm2
	movdqu 16(%rax), %xmm3
	movdqu %xmm4, 32(%rcx)
	movaps %xmm1, %xmm2
	movaps 16(%rax), %xmm3
	movaps %xmm4, 32(%rcx)
	movups %xmm1, %xmm2
	movups 16(%rax), %xmm3
	movups %xmm4, 32(%rcx)
	movntps %xmm4, 32(%rcx)
	movntpd %xmm4, 32(%rcx)
	movntdq %xmm4, 32(%rcx)
	movmskps %xmm5, %eax
	movmskpd %xmm5, %eax
	pmovmskb %xmm5, %eax
	cvtsi2ss %ecx, %xmm6
	cvtsi2ss %rcx, %xmm6
	cvtsi2sd %ecx, %xmm6
	cvtsi2sd %rcx, %xmm6
	cvtss2si %xmm1, %eax
	cvtss2si %xmm1, %rax
	cvtss2si 8(%rdx), %eax
	cvttss2si %xmm1, %eax
	cvttss2si %xmm1, %rax
	cvttss2si 8(%rdx), %eax
	cvtsd2si %xmm1, %eax
	cvtsd2si %xmm1, %rax
	cvtsd2si 8(%rdx), %eax
	cvttsd2si %xmm1, %eax
	cvttsd2si %xmm1, %rax
	cvttsd2si 8(%rdx), %eax
	pslldq $0x3, %xmm4
	psrldq $0x5, %xmm7
	pextrw $0x2, %xmm1, %eax
	pinsrw $0x3, %ecx, %xmm2
	pinsrw $0x3, 8(%rdx), %xmm2
    .globl sse_list_end
sse_list_end:
