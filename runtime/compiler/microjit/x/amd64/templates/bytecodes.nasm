; add all labels to global list if intending to use in CodeGenerator
%include "utils.nasm"

; rsp is the stack base for the java value stack pointer.
; r10 will hold the java value stack pointer.
; r11 stores the accumulator
; r12 stores the value which will act on the accumulator
; r13 will hold the java reference stack pointer
; r14 will hold a pointer to the start of the local array
; r15 stores values loaded from memory for storing on the stack

; used to solve intial problem static add(II)I
template_start addAndReturn
    add rax, rsi    ; add second argument to first
    ret             ; return
template_end addAndReturn

template_start debugBreakpoint
    int3           ; trigger hardware interrupt
template_end debugBreakpoint

; template_start iLoad0Template
;     sub r10, 8      ; add 1 slot to stack (8 bytes)
;     mov [r10], rax  ; push 1st argument (rax) onto stack 
; template_end iLoad0Template

; template_start iLoad1Template
;     sub r10, 8      ; add 1 slot to stack (8 bytes)
;     mov [r10], rsi  ; move 2nd argument into first location in stack
; template_end iLoad1Template

; template_start iLoad2Template
;     sub r10, 8      ; add 1 slot to stack (8 bytes)
;     mov [r10], rdx  ; move 3rd argument into first location in stack
; template_end iLoad2Template

;All loads (value or reference) require adding stack space of 8 bytes
;  and the moving of values from a stack slot to a temp register
template_start loadTemplatePrologue
    sub r10, 8
    mov r15, [r14 + 0xefbeadde]
template_end loadTemplatePrologue

;The destination of a load template depends on whether it is a value
;  or a reference
template_start vLoadTemplate
    mov [r10], r15
template_end vLoadTemplate

;Not all stores start from the same stack
template_start vStoreTemplatePrologue
    mov r15, [r10]
    add r10, 8
template_end vStoreTemplatePrologue

;The destination of a store is always a stack slot
template_start storeTemplate
    mov [r14 + 0xefbeadde], r15
template_end storeTemplate

template_start iAddTemplate
    mov r11, [r10]  ; pop first value off java stack into the accumulator
    add r10, 8      ; which means reducing the stack size by 1 slot (8 bytes)
    mov r12, [r10]  ; copy second value to the value register
    add r11, r12    ; add the value to the accumulator
    mov [r10], r11  ; write the accumulator over the second arg.
template_end iAddTemplate

template_start iReturnTemplate
    mov rax, [r10]  ; move the stack top into the Private Linkage return register
    ret             ; return from the JITed method.
template_end iReturnTemplate
