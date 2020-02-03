; add all labels to global list if intending to use in CodeGenerator
; TODO: Do this with macros so that you can't accidently miss type a label.

%include "utils.nasm"

declare_template movRSPR10
declare_template subR10Imm4
declare_template addR10Imm4
declare_template saveEAXOffset
declare_template saveESIOffset
declare_template saveEDXOffset
declare_template saveECXOffset
declare_template saveEBXOffset
declare_template saveEBPOffset
declare_template saveESPOffset
declare_template loadEAXOffset
declare_template loadESIOffset
declare_template loadEDXOffset
declare_template loadECXOffset
declare_template loadEBXOffset
declare_template loadEBPOffset
declare_template loadESPOffset
declare_template je4ByteRel
declare_template jeByteRel
declare_template cmpRspRbpDerefOffset
declare_template saveRAXOffset
declare_template saveRSIOffset
declare_template saveRDXOffset
declare_template saveRCXOffset
declare_template saveRBXOffset
declare_template saveRBPOffset
declare_template saveRSPOffset
declare_template saveR9Offset
declare_template saveR10Offset
declare_template saveR11Offset
declare_template saveR12Offset
declare_template saveR13Offset
declare_template saveR14Offset
declare_template saveR15Offset
declare_template saveXMM0Offset
declare_template saveXMM1Offset
declare_template saveXMM2Offset
declare_template saveXMM3Offset
declare_template saveXMM4Offset
declare_template saveXMM5Offset
declare_template saveXMM6Offset
declare_template saveXMM7Offset
declare_template loadRAXOffset
declare_template loadRSIOffset
declare_template loadRDXOffset
declare_template loadRCXOffset
declare_template loadRBXOffset
declare_template loadRBPOffset
declare_template loadRSPOffset
declare_template loadR9Offset
declare_template loadR10Offset
declare_template loadR11Offset
declare_template loadR12Offset
declare_template loadR13Offset
declare_template loadR14Offset
declare_template loadR15Offset
declare_template loadXMM0Offset
declare_template loadXMM1Offset
declare_template loadXMM2Offset
declare_template loadXMM3Offset
declare_template loadXMM4Offset
declare_template loadXMM5Offset
declare_template loadXMM6Offset
declare_template loadXMM7Offset
declare_template callByteRel
declare_template call4ByteRel
declare_template jump4ByteRel
declare_template nopInstruction
declare_template movRDIImm64
declare_template movEDIImm32
declare_template movRAXImm64
declare_template movEAXImm32
declare_template jumpRDI
declare_template jumpRAX
declare_template jbe4ByteRel
declare_template subRSPImm8
declare_template subRSPImm4
declare_template subRSPImm2
declare_template subRSPImm1
declare_template addRSPImm8
declare_template addRSPImm4
declare_template addRSPImm2
declare_template addRSPImm1

;
;Create templates
;

section .text
movRSPR10:
    mov r10, rsp; Used for allocating space on the stack.
subR10Imm4:
    sub r10, 0x7fffffff; Used for allocating space on the stack.
addR10Imm4:
    add r10, 0x7fffffff; Used for allocating space on the stack.
saveRBPOffset:
    mov [rsp+0xff], rbp; Used ff as all other labels look valid and 255 will be rare.
saveEBPOffset:
    mov [rsp+0xff], ebp; Used ff as all other labels look valid and 255 will be rare.
saveRSPOffset:
saveESPOffset:
    mov [rsp+0xff], rsp; Used ff as all other labels look valid and 255 will be rare.
loadRBPOffset:
    mov rbp, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadEBPOffset:
    mov ebp, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadRSPOffset:
loadESPOffset:
    mov rsp, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
saveEAXOffset:
    mov [rsp+0xff], eax; Used ff as all other labels look valid and 255 will be rare.
saveESIOffset:
    mov [rsp+0xff], esi; Used ff as all other labels look valid and 255 will be rare.
saveEDXOffset:
    mov [rsp+0xff], edx; Used ff as all other labels look valid and 255 will be rare.
saveECXOffset:
    mov [rsp+0xff], ecx; Used ff as all other labels look valid and 255 will be rare.
saveEBXOffset:
    mov [rsp+0xff], ebx; Used ff as all other labels look valid and 255 will be rare.
loadEAXOffset:
    mov eax, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadESIOffset:
    mov esi, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadEDXOffset:
    mov edx, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadECXOffset:
    mov ecx, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadEBXOffset:
    mov ebx, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
addRSPImm8:
    add rsp, 0x7fffffffffffffff; Used for allocating space on the stack.
addRSPImm4:
    add rsp, 0x7fffffff; Used for allocating space on the stack.
addRSPImm2:
    add rsp, 0x7fff; Used for allocating space on the stack.
addRSPImm1:
    add rsp, 0x7f; Used for allocating space on the stack.
je4ByteRel:
    je 0xff;
jeByteRel:
    je 0xff;
subRSPImm8:
    sub rsp, 0x7fffffffffffffff; Used for allocating space on the stack.
subRSPImm4:
    sub rsp, 0x7fffffff; Used for allocating space on the stack.
subRSPImm2:
    sub rsp, 0x7fff; Used for allocating space on the stack.
subRSPImm1:
    sub rsp, 0x7f; Used for allocating space on the stack.
jbe4ByteRel:
    jbe 0xefbeadde; Used for generating jumps to far labels.
cmpRspRbpDerefOffset:
    cmp rsp, [rbp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadRAXOffset:
    mov rax, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadRSIOffset:
    mov rsi, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadRDXOffset:
    mov rdx, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadRCXOffset:
    mov rcx, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadRBXOffset:
    mov rbx, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadR9Offset:
    mov r9, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadR10Offset:
    mov r10, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadR11Offset:
    mov r11, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadR12Offset:
    mov r12, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadR13Offset:
    mov r13, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadR14Offset:
    mov r14, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadR15Offset:
    mov r15, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM0Offset:
    movq xmm0, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM1Offset:
    movq xmm1, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM2Offset:
    movq xmm2, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM3Offset:
    movq xmm3, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM4Offset:
    movq xmm4, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM5Offset:
    movq xmm5, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM6Offset:
    movq xmm6, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
loadXMM7Offset:
    movq xmm7, [rsp+0xff]; Used ff as all other labels look valid and 255 will be rare.
saveRAXOffset:
    mov [rsp+0xff], rax; Used ff as all other labels look valid and 255 will be rare.
saveRSIOffset:
    mov [rsp+0xff], rsi; Used ff as all other labels look valid and 255 will be rare.
saveRDXOffset:
    mov [rsp+0xff], rdx; Used ff as all other labels look valid and 255 will be rare.
saveRCXOffset:
    mov [rsp+0xff], rcx; Used ff as all other labels look valid and 255 will be rare.
saveRBXOffset:
    mov [rsp+0xff], rbx; Used ff as all other labels look valid and 255 will be rare.
saveR9Offset:
    mov [rsp+0xff], r9; Used ff as all other labels look valid and 255 will be rare.
saveR10Offset:
    mov [rsp+0xff], r10; Used ff as all other labels look valid and 255 will be rare.
saveR11Offset:
    mov [rsp+0xff], r11; Used ff as all other labels look valid and 255 will be rare.
saveR12Offset:
    mov [rsp+0xff], r12; Used ff as all other labels look valid and 255 will be rare.
saveR13Offset:
    mov [rsp+0xff], r13; Used ff as all other labels look valid and 255 will be rare.
saveR14Offset:
    mov [rsp+0xff], r14; Used ff as all other labels look valid and 255 will be rare.
saveR15Offset:
    mov [rsp+0xff], r15; Used ff as all other labels look valid and 255 will be rare.
saveXMM0Offset:
    movq [rsp+0xff], xmm0; Used ff as all other labels look valid and 255 will be rare.
saveXMM1Offset:
    movq [rsp+0xff], xmm1; Used ff as all other labels look valid and 255 will be rare.
saveXMM2Offset:
    movq [rsp+0xff], xmm2; Used ff as all other labels look valid and 255 will be rare.
saveXMM3Offset:
    movq [rsp+0xff], xmm3; Used ff as all other labels look valid and 255 will be rare.
saveXMM4Offset:
    movq [rsp+0xff], xmm4; Used ff as all other labels look valid and 255 will be rare.
saveXMM5Offset:
    movq [rsp+0xff], xmm5; Used ff as all other labels look valid and 255 will be rare.
saveXMM6Offset:
    movq [rsp+0xff], xmm6; Used ff as all other labels look valid and 255 will be rare.
saveXMM7Offset:
    movq [rsp+0xff], xmm7; Used ff as all other labels look valid and 255 will be rare.
call4ByteRel:
    call 0xefbeadde; Used for generating jumps to far labels.
callByteRel:
    call 0xff; Used for generating jumps to nearby labels. Used ff as all other labels look valid and 255 will be rare.
jump4ByteRel:
    jmp 0xefbeadde; Used for generating jumps to far labels.
nopInstruction:
    nop; Used for alignment. One byte wide 
movRDIImm64:
    mov rdi, 0xefbeaddeefbeadde; looks like 'deadbeef' in objdump
movEDIImm32:
    mov edi, 0xefbeadde; looks like 'deadbeef' in objdump
movRAXImm64:
    mov rax, 0xefbeaddeefbeadde; looks like 'deadbeef' in objdump
movEAXImm32:
    mov eax, 0xefbeadde; looks like 'deadbeef' in objdump
jumpRDI:
    jmp rdi; Useful for jumping to absolute addresses. Store in RDI then generate this.
jumpRAX:
    jmp rax; Useful for jumping to absolute addresses. Store in RAX then generate this.
lastLabel:

;
;Calculate the size of all templates
;

section .data
;TODO: change convention to support creation of a macro to automate this

movRSPR10Size:              dw subR10Imm4           - movRSPR10
subR10Imm4Size:             dw addR10Imm4           - subR10Imm4
addR10Imm4Size:             dw saveRBPOffset        - addR10Imm4
saveRBPOffsetSize:          dw saveEBPOffset        - saveRBPOffset
saveEBPOffsetSize:          dw saveRSPOffset        - saveEBPOffset
saveRSPOffsetSize:          dw loadRBPOffset        - saveRSPOffset
saveESPOffsetSize:          dw loadRBPOffset        - saveESPOffset
loadRBPOffsetSize:          dw loadEBPOffset        - loadRBPOffset
loadEBPOffsetSize:          dw loadESPOffset        - loadEBPOffset
loadRSPOffsetSize:          dw saveEAXOffset        - loadRSPOffset
loadESPOffsetSize:          dw saveEAXOffset        - loadESPOffset
saveEAXOffsetSize:          dw saveESIOffset        - saveEAXOffset
saveESIOffsetSize:          dw saveEDXOffset        - saveESIOffset
saveEDXOffsetSize:          dw saveECXOffset        - saveEDXOffset
saveECXOffsetSize:          dw saveEBXOffset        - saveECXOffset
saveEBXOffsetSize:          dw loadEAXOffset        - saveEBXOffset
loadEAXOffsetSize:          dw loadESIOffset        - loadEAXOffset
loadESIOffsetSize:          dw loadEDXOffset        - loadESIOffset
loadEDXOffsetSize:          dw loadECXOffset        - loadEDXOffset
loadECXOffsetSize:          dw loadEBXOffset        - loadECXOffset
loadEBXOffsetSize:          dw addRSPImm8           - loadEBXOffset
addRSPImm8Size:             dw addRSPImm4           - addRSPImm8
addRSPImm4Size:             dw addRSPImm2           - addRSPImm4
addRSPImm2Size:             dw addRSPImm1           - addRSPImm2
addRSPImm1Size:             dw je4ByteRel           - addRSPImm1
je4ByteRelSize:             dw jeByteRel            - je4ByteRel
jeByteRelSize:              dw subRSPImm8           - jeByteRel
subRSPImm8Size:             dw subRSPImm4           - subRSPImm8
subRSPImm4Size:             dw subRSPImm2           - subRSPImm4
subRSPImm2Size:             dw subRSPImm1           - subRSPImm2
subRSPImm1Size:             dw jbe4ByteRel          - subRSPImm1
jbe4ByteRelSize:            dw cmpRspRbpDerefOffset - jbe4ByteRel
cmpRspRbpDerefOffsetSize:   dw loadRAXOffset        - cmpRspRbpDerefOffset
loadRAXOffsetSize:          dw loadRSIOffset        - loadRAXOffset
loadRSIOffsetSize:          dw loadRDXOffset        - loadRSIOffset
loadRDXOffsetSize:          dw loadRCXOffset        - loadRDXOffset
loadRCXOffsetSize:          dw loadRBXOffset        - loadRCXOffset
loadRBXOffsetSize:          dw loadR9Offset         - loadRBXOffset
loadR9OffsetSize:           dw loadR10Offset        - loadR9Offset
loadR10OffsetSize:          dw loadR11Offset        - loadR10Offset
loadR11OffsetSize:          dw loadR12Offset        - loadR11Offset
loadR12OffsetSize:          dw loadR13Offset        - loadR12Offset
loadR13OffsetSize:          dw loadR14Offset        - loadR13Offset
loadR14OffsetSize:          dw loadR15Offset        - loadR14Offset
loadR15OffsetSize:          dw loadXMM0Offset       - loadR15Offset
loadXMM0OffsetSize:         dw loadXMM1Offset       - loadXMM0Offset
loadXMM1OffsetSize:         dw loadXMM2Offset       - loadXMM1Offset
loadXMM2OffsetSize:         dw loadXMM3Offset       - loadXMM2Offset
loadXMM3OffsetSize:         dw loadXMM4Offset       - loadXMM3Offset
loadXMM4OffsetSize:         dw loadXMM5Offset       - loadXMM4Offset
loadXMM5OffsetSize:         dw loadXMM6Offset       - loadXMM5Offset
loadXMM6OffsetSize:         dw loadXMM7Offset       - loadXMM6Offset
loadXMM7OffsetSize:         dw saveRAXOffset        - loadXMM7Offset
saveRAXOffsetSize:          dw saveRSIOffset        - saveRAXOffset
saveRSIOffsetSize:          dw saveRDXOffset        - saveRSIOffset
saveRDXOffsetSize:          dw saveRCXOffset        - saveRDXOffset
saveRCXOffsetSize:          dw saveRBXOffset        - saveRCXOffset
saveRBXOffsetSize:          dw saveR9Offset         - saveRBXOffset
saveR9OffsetSize:           dw saveR10Offset        - saveR9Offset
saveR10OffsetSize:          dw saveR11Offset        - saveR10Offset
saveR11OffsetSize:          dw saveR12Offset        - saveR11Offset
saveR12OffsetSize:          dw saveR13Offset        - saveR12Offset
saveR13OffsetSize:          dw saveR14Offset        - saveR13Offset
saveR14OffsetSize:          dw saveR15Offset        - saveR14Offset
saveR15OffsetSize:          dw saveXMM0Offset       - saveR15Offset
saveXMM0OffsetSize:         dw saveXMM1Offset       - saveXMM0Offset
saveXMM1OffsetSize:         dw saveXMM2Offset       - saveXMM1Offset
saveXMM2OffsetSize:         dw saveXMM3Offset       - saveXMM2Offset
saveXMM3OffsetSize:         dw saveXMM4Offset       - saveXMM3Offset
saveXMM4OffsetSize:         dw saveXMM5Offset       - saveXMM4Offset
saveXMM5OffsetSize:         dw saveXMM6Offset       - saveXMM5Offset
saveXMM6OffsetSize:         dw saveXMM7Offset       - saveXMM6Offset
saveXMM7OffsetSize:         dw call4ByteRel         - saveXMM7Offset
call4ByteRelSize:           dw callByteRel          - call4ByteRel
callByteRelSize:            dw jump4ByteRel         - callByteRel
jump4ByteRelSize:           dw nopInstruction       - jump4ByteRel
nopInstructionSize:         dw movRDIImm64          - nopInstruction
movRDIImm64Size:            dw movEDIImm32          - movRDIImm64
movEDIImm32Size:            dw movRAXImm64          - movEDIImm32
movRAXImm64Size:            dw movEAXImm32          - movRAXImm64
movEAXImm32Size:            dw jumpRDI              - movEAXImm32
jumpRDISize:                dw jumpRAX              - jumpRDI
jumpRAXSize:                dw lastLabel            - jumpRAX
