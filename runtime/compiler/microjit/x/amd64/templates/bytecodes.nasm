; Copyright (c) 2000, 2019 IBM Corp. and others
;
; This program and the accompanying materials are made available under
; the terms of the Eclipse Public License 2.0 which accompanies this
; distribution and is available at https://www.eclipse.org/legal/epl-2.0/
; or the Apache License, Version 2.0 which accompanies this distribution and
; is available at https://www.apache.org/licenses/LICENSE-2.0.
;
; This Source Code may also be made available under the following
; Secondary Licenses when the conditions for such availability set
; forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
; General Public License, version 2 with the GNU Classpath
; Exception [1] and GNU General Public License, version 2 with the
; OpenJDK Assembly Exception [2].
;
; [1] https://www.gnu.org/software/classpath/license.html
; [2] http://openjdk.java.net/legal/assembly-exception.html
;
; SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
%include "utils.nasm"

;MicroJIT Virtual Machine to X86-64 mapping
;  rsp: 
;    is the stack extent for the java value stack pointer.
;  r10:
;    will hold the java value stack pointer.
;  r11:
;    stores the accumulator, or
;    stores a pointer to an object
;  r12:
;    stores any value which will act on the accumulator, or
;    stores the value to be written to an object field, or
;    stores the value read from an object field
;  r13:
;    holds addresses for absolute addressing
;  r14:
;    will hold a pointer to the start of the local array
;  r15:
;    stores values loaded from memory for storing on the stack

; used to solve intial problem static add(II)I
template_start addAndReturn
    add rax, rsi    ; add second argument to first
    ret             ; return
template_end addAndReturn

template_start debugBreakpoint
    int3           ; trigger hardware interrupt
template_end debugBreakpoint

;All loads require adding stack space of 8 bytes
;  and the moving of values from a stack slot to a temp register
template_start loadTemplatePrologue
    sub r10, 8
    mov r15, [r14 + 0xefbeadde]
template_end loadTemplatePrologue

template_start loadTemplate
    mov [r10], r15
template_end loadTemplate

;Not all stores start from the same stack
template_start storeTemplate
    mov r15, [r10]
    add r10, 8
    mov [r14 + 0xefbeadde], r15
template_end storeTemplate

template_start getFieldTemplatePrologue
    mov r11, [r10]
    mov r12, [r11 + 0xefbeadde]
template_end getFieldTemplatePrologue

template_start getFieldTemplate
    xor r11, r11
    mov [r10], r12
template_end getFieldTemplate

template_start staticTemplatePrologue
    lea r13, [0xefbeadde]   ; load the absolute address of a static value
template_end staticTemplatePrologue

template_start getStaticTemplate
    sub r10, 8      ; allocate a stack slot
    mov r11, [r13]  ; load value into r11
    mov [r10], r11  ; move it onto the stack
template_end getStaticTemplate

template_start putStaticTemplate
    mov r11, [r10]  ; Move stack value into r11
    add r10, 8      ; Pop the value off the stack
    mov [r13], r11  ; Move it to memory
template_end putStaticTemplate

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

template_start vReturnTemplate
    ret             ; return from the JITed method.
template_end vReturnTemplate