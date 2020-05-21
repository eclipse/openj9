/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/
#include "compile/OMRCompilation.hpp"
#include "AMD64Linkage.hpp"
#include <cstring>

#define INTERPRETER_CLOBBERS_XMMS          (!debug("interpreterClobbersXmms"))


/*
| Architecture | Endian | Return address | Argument registers |
| x86-64       | Little | Stack          | RAX RSI RDX RCX    |

| Return value registers                        | Preserved registers | vTable index         |
| EAX (32-bit) RAX (64-bit) XMM0 (float/double) | RBX R9-R15          | R8 (receiver in RAX) |
*/

MJIT::Linkage::Linkage(struct J9JITConfig* jitConfig, J9VMThread* vmThread, TR::FilePointer* logFileFP)
{
    _fej9 = TR_J9VMBase::get(jitConfig, vmThread);
    const MJIT::RealRegister::RegNum noReg = MJIT::RealRegister::NoReg;
    uint8_t p;

    _properties._logFileFP = logFileFP;

    MJIT::RealRegister::RegNum metaReg = MJIT::RealRegister::ebp;

    _properties._properties =
          EightBytePointers | EightByteParmSlots
        | IntegersInRegisters | LongsInRegisters | FloatsInRegisters
        | NeedsThunksForIndirectCalls
        | UsesRegsForHelperArgs
        ;

#if defined(J9SW_NEEDS_JIT_2_INTERP_CALLEE_ARG_POP)
    _properties._properties |= CallerCleanup | ReservesOutgoingArgsInPrologue;
#endif

    // Integer arguments

    p=0;
    _properties._firstIntegerArgumentRegister = p;
    _properties._argumentRegisters[p++]       = MJIT::RealRegister::eax;
    _properties._argumentRegisters[p++]       = MJIT::RealRegister::esi;
    _properties._argumentRegisters[p++]       = MJIT::RealRegister::edx;
    _properties._argumentRegisters[p++]       = MJIT::RealRegister::ecx;
    MJIT_ASSERT(logFileFP, p == NUM_INTEGER_LINKAGE_REGS, "assertion failure");

    _properties._numIntegerArgumentRegisters = NUM_INTEGER_LINKAGE_REGS;

    // Float arguments

    _properties._firstFloatArgumentRegister = p;

    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm0;
    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm1;
    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm2;
    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm3;
    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm4;
    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm5;
    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm6;
    _properties._argumentRegisters[p++] = MJIT::RealRegister::xmm7;

    MJIT_ASSERT(logFileFP, p == NUM_INTEGER_LINKAGE_REGS + NUM_FLOAT_LINKAGE_REGS, "assertion failure");
    _properties._numFloatArgumentRegisters = NUM_FLOAT_LINKAGE_REGS;



    // Preserved

    p=0;
    _properties._preservedRegisters[p++]    = MJIT::RealRegister::ebx;
    _properties._preservedRegisterMapForGC  = MJIT::RealRegisterMask::ebxMask;

    int32_t lastPreservedRegister = 9; // changed to 9 for liberty, it used to be 15

    _properties._preservedRegisters[p++] = MJIT::RealRegister::r9; 
    _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::r9Mask; // 9

    _properties._numberOfPreservedGPRegisters = p;

    if (!INTERPRETER_CLOBBERS_XMMS){
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm8;
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm9;
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm10;
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm11;
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm12;
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm13;
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm14;
        _properties._preservedRegisters[p++] = MJIT::RealRegister::xmm15;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm8Mask;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm9Mask;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm10Mask;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm11Mask;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm12Mask;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm13Mask;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm14Mask;
        _properties._preservedRegisterMapForGC |= MJIT::RealRegisterMask::xmm15Mask;
    }

    _properties._numberOfPreservedXMMRegisters   = p - _properties._numberOfPreservedGPRegisters;
    _properties._maxRegistersPreservedInPrologue = p;
    _properties._preservedRegisters[p++]         = metaReg;
    _properties._preservedRegisters[p++]         = MJIT::RealRegister::esp;
    _properties._numPreservedRegisters           = p;

    // Other

    _properties._returnRegisters[0] = MJIT::RealRegister::eax;
    _properties._returnRegisters[1] = MJIT::RealRegister::xmm0;
    _properties._returnRegisters[2] = noReg;

    _properties._scratchRegisters[0] = MJIT::RealRegister::edi;
    _properties._scratchRegisters[1] = MJIT::RealRegister::r8;
    _properties._numScratchRegisters = 2;

    _properties._vtableIndexArgumentRegister = MJIT::RealRegister::r8;
    _properties._j9methodArgumentRegister = MJIT::RealRegister::edi;
    _properties._framePointerRegister = MJIT::RealRegister::esp;
    _properties._methodMetaDataRegister = metaReg;

    _properties._numberOfVolatileGPRegisters  = 6; // rax, rsi, rdx, rcx, rdi, r8
    _properties._numberOfVolatileXMMRegisters = INTERPRETER_CLOBBERS_XMMS? 16 : 8; // xmm0-xmm15 : xmm0-xmm7

    // Offsets relative to where the frame pointer *would* point if we had one;
    // namely, the local with the highest address (ie. the "first" local)

    _properties._offsetToFirstParm = RETURN_ADDRESS_SIZE;
    _properties._offsetToFirstLocal = 0;

    // TODO: Need a better way to build the flags so they match the info above
    //

    memset(_properties._registerFlags, 0, sizeof(_properties._registerFlags));

    // Integer arguments/return

    _properties._registerFlags[MJIT::RealRegister::eax]      = IntegerArgument | IntegerReturn;
    _properties._registerFlags[MJIT::RealRegister::esi]      = IntegerArgument;
    _properties._registerFlags[MJIT::RealRegister::edx]      = IntegerArgument;
    _properties._registerFlags[MJIT::RealRegister::ecx]      = IntegerArgument;

    // Float arguments/return

    _properties._registerFlags[MJIT::RealRegister::xmm0]     = FloatArgument | FloatReturn;

    _properties._registerFlags[MJIT::RealRegister::xmm1]  = FloatArgument;
    _properties._registerFlags[MJIT::RealRegister::xmm2]  = FloatArgument;
    _properties._registerFlags[MJIT::RealRegister::xmm3]  = FloatArgument;
    _properties._registerFlags[MJIT::RealRegister::xmm4]  = FloatArgument;
    _properties._registerFlags[MJIT::RealRegister::xmm5]  = FloatArgument;
    _properties._registerFlags[MJIT::RealRegister::xmm6]  = FloatArgument;
    _properties._registerFlags[MJIT::RealRegister::xmm7]  = FloatArgument;

    // Preserved

    _properties._registerFlags[MJIT::RealRegister::ebx]  = Preserved;
    _properties._registerFlags[MJIT::RealRegister::esp]  = Preserved;
    _properties._registerFlags[metaReg]                  = Preserved;

    _properties._registerFlags[MJIT::RealRegister::r9]   = Preserved;

    if(!INTERPRETER_CLOBBERS_XMMS){
        _properties._registerFlags[MJIT::RealRegister::xmm8]   = Preserved;
        _properties._registerFlags[MJIT::RealRegister::xmm9]   = Preserved;
        _properties._registerFlags[MJIT::RealRegister::xmm10]  = Preserved;
        _properties._registerFlags[MJIT::RealRegister::xmm11]  = Preserved;
        _properties._registerFlags[MJIT::RealRegister::xmm12]  = Preserved;
        _properties._registerFlags[MJIT::RealRegister::xmm13]  = Preserved;
        _properties._registerFlags[MJIT::RealRegister::xmm14]  = Preserved;
        _properties._registerFlags[MJIT::RealRegister::xmm15]  = Preserved;
    }

    p = 0;
    _properties._allocationOrder[p++] = MJIT::RealRegister::edi;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r8;
    _properties._allocationOrder[p++] = MJIT::RealRegister::ecx;
    _properties._allocationOrder[p++] = MJIT::RealRegister::edx;
    _properties._allocationOrder[p++] = MJIT::RealRegister::esi;
    _properties._allocationOrder[p++] = MJIT::RealRegister::eax;
/* 
This commented block and the following one are kept because we do not yet know if we need to support the " == 2" case
However, a full run of the test program in GDB shows that TR::Machine::enableNewPickRegister always returned true
    if (TR::Machine::enableNewPickRegister()){

        // Volatiles that aren't linkage regs

        if (TR::Machine::numGPRRegsWithheld(cg) == 0){
            _properties._allocationOrder[p++] = MJIT::RealRegister::edi;
            _properties._allocationOrder[p++] = MJIT::RealRegister::r8;
        }

        else{
            MJIT_ASSERT(logFileFP, TR::Machine::numGPRRegsWithheld(cg) == 2, "numRegsWithheld: only 0 and 2 currently supported");
        }

        // Linkage regs in reverse order

        _properties._allocationOrder[p++] = MJIT::RealRegister::ecx;
        _properties._allocationOrder[p++] = MJIT::RealRegister::edx;
        _properties._allocationOrder[p++] = MJIT::RealRegister::esi;
        _properties._allocationOrder[p++] = MJIT::RealRegister::eax;
    }
*/
    // Preserved regs

    _properties._allocationOrder[p++] = MJIT::RealRegister::ebx;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r9;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r10;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r11;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r12;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r13;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r14;
    _properties._allocationOrder[p++] = MJIT::RealRegister::r15;


    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm7;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm6;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm5;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm4;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm3;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm2;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm1;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm0;
/*
    if (TR::Machine::enableNewPickRegister()){

        // Linkage regs in reverse order

        if (TR::Machine::numRegsWithheld(cg) == 0){
            _properties._allocationOrder[p++] = MJIT::RealRegister::xmm7;
            _properties._allocationOrder[p++] = MJIT::RealRegister::xmm6;

        }

        else{
            MJIT_ASSERT(logFileFP, TR::Machine::numRegsWithheld(cg) == 2, "numRegsWithheld: only 0 and 2 currently supported");
        }

        _properties._allocationOrder[p++] = MJIT::RealRegister::xmm5;
        _properties._allocationOrder[p++] = MJIT::RealRegister::xmm4;
        _properties._allocationOrder[p++] = MJIT::RealRegister::xmm3;
        _properties._allocationOrder[p++] = MJIT::RealRegister::xmm2;
        _properties._allocationOrder[p++] = MJIT::RealRegister::xmm1;
        _properties._allocationOrder[p++] = MJIT::RealRegister::xmm0;
    }
*/
    // Preserved regs

    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm8;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm9;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm10;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm11;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm12;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm13;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm14;
    _properties._allocationOrder[p++] = MJIT::RealRegister::xmm15;
}
