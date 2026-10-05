# MMX and XMM operands mixed: not an x86 instruction, must be rejected (asm_gate.bat).
    .text
    pavgb %mm1, %xmm2
