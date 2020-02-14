; add all labels to global list if intending to use in CodeGenerator
%include "utils.nasm"

declare_template debugBreakpoint
declare_template iLoad0Template
declare_template iLoad1Template
declare_template iLoad2Template
declare_template iAddTemplate
declare_template iSubTemplate
declare_template iMulTemplate
declare_template iDivTemplate
declare_template iReturnTemplate

; rsp is the stack base for the java stack pointer.
; r10 will hold the java stack pointer.
; r11 stores the accumulator
; r12 stores the value which will act on the accumulator

debugBreakpoint:
    int3           ; trigger hardware interrupt

iLoad0Template:
    sub r10, 8      ; add 1 slot to stack (8 bytes)
    mov [r10], rax  ; push 1st argument (rax) onto stack (todo: replace with load from local-array)

iLoad1Template:
    sub r10, 8      ; add 1 slot to stack (8 bytes)
    mov [r10], rsi  ; move 2nd argument into first location in stack (todo: replace with load from local-array)

iLoad2Template:
    sub r10, 8      ; add 1 slot to stack (8 bytes)
    mov [r10], rdx  ; move 3rd argument into first location in stack

iAddTemplate:
    mov r11, [r10]  ; pop first value off java stack into the accumulator
    add r10, 8      ; which means reducing the stack size by 1 slot (8 bytes)
    mov r12, [r10]  ; copy second value to the value register
    add r11, r12    ; add the value to the accumulator
    mov [r10], r11  ; write the accumulator over the second arg.

iSubTemplate:
    mov r12, [r10]  ; copy top value of stack in the value register
    add r10, 8      ; reduce the stack size by 1 slot (8 bytes)
    mov r11, [r10]  ; copy second value to the accumulator register
    sub r11, r12    ; subtract the value from the accumulator
    mov [r10], r11  ; write the accumulator over the second arg.    

iMulTemplate:
    mov r11, [r10]  ; pop first value off java stack into the accumulator
    add r10, 8      ; which means reducing the stack size by 1 slot (8 bytes)
    mov r12, [r10]  ; copy second value to the value register
    imul r11, r12   ; multiply accumulator by the value to the accumulator
    mov [r10], r11  ; write the accumulator over the second arg.

iDivTemplate:
    mov r12, [r10]        ; copy top value of stack in the value register (divisor)
    add r10, 8            ; reduce the stack size by 1 slot (8 bytes)
    mov eax, [r10]        ; copy second value (dividend) to eax
    cdq                   ; extend sign bit from rax to rdx
    idiv r12d             ; divide rdx:rax by lower 32bits of r12 value 
    mov [r10], eax        ; store the quotient in accumulator

iReturnTemplate:
    mov rax, [r10]  ; move the stack top into the Private Linkage return register
    ret             ; return from the JITed method. (TODO: is ret correct here?)
endOfBytecodeTemplates:

debugBreakpointSize:    dw  iLoad0Template          -   debugBreakpoint
iLoad0TemplateSize:     dw  iLoad1Template          -   iLoad0Template
iLoad1TemplateSize:     dw  iLoad2Template          -   iLoad1Template
iLoad2TemplateSize:     dw  iAddTemplate            -   iLoad2Template
iAddTemplateSize:       dw  iSubTemplate            -   iAddTemplate
iSubTemplateSize:       dw  iMulTemplate            -   iSubTemplate
iMulTemplateSize:       dw  iDivTemplate            -   iMulTemplate
iDivTemplateSize:       dw  iReturnTemplate         -   iDivTemplate
iReturnTemplateSize:    dw  endOfBytecodeTemplates  -   iReturnTemplate