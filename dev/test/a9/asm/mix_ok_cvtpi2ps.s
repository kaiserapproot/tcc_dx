# cvtpi2ps really takes an mm source and an xmm destination (OPT_MMX and OPT_SSE in the table): it must still assemble.
    .text
    cvtpi2ps %mm1, %xmm2
