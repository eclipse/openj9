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
#ifndef MJIT_SIDETABLE_HPP
#define MJIT_SIDETABLE_HPP

#include <cstdint>
#include "microjit/x/amd64/AMD64Linkage.hpp"
#include "microjit/utils.hpp"

/*
| Architecture 	| Endian | Return address | vTable index |
| x86-64 		| Little | Stack 		  | R8 (receiver in RAX)

| Integer Return value registers    | Integer Preserved registers   | Integer Argument registers
| EAX (32-bit) RAX (64-bit)         | RBX R9                        | RAX RSI RDX RCX 

| Float Return value registers  | Float Preserved registers | Float Argument registers
| XMM0                          | XMM8-XMM15        		| XMM0-XMM7 
*/
namespace MJIT {

struct RegisterStack {
    unsigned char useRAX : 2;
    unsigned char useRSI : 2;
    unsigned char useRDX : 2;
    unsigned char useRCX : 2;
    unsigned char useXMM0 : 2;
    unsigned char useXMM1 : 2;
    unsigned char useXMM2 : 2;
    unsigned char useXMM3 : 2;
    unsigned char useXMM4 : 2;
    unsigned char useXMM5 : 2;
    unsigned char useXMM6 : 2;
    unsigned char useXMM7 : 2;
    unsigned char stackSlotsUsed : 8;
};

inline U_16
calculateOffset(RegisterStack *stack)
{
    return  8*(stack->useRAX  +
               stack->useRSI  +
               stack->useRDX  +
               stack->useRCX  +
               stack->useXMM0 +
               stack->useXMM1 +
               stack->useXMM2 +
               stack->useXMM3 +
               stack->useXMM4 +
               stack->useXMM5 +
               stack->useXMM6 +
               stack->useXMM7 +
               stack->stackSlotsUsed);
}

inline void
initParamStack(RegisterStack *stack)
{
    stack->useRAX         = 0;
    stack->useRSI         = 0;
    stack->useRDX         = 0;
    stack->useRCX         = 0;
    stack->useXMM0        = 0;
    stack->useXMM1        = 0;
    stack->useXMM2        = 0;
    stack->useXMM3        = 0;
    stack->useXMM4        = 0;
    stack->useXMM5        = 0;
    stack->useXMM6        = 0;
    stack->useXMM7        = 0;
    stack->stackSlotsUsed = 0;
}

inline int
addParamIntToStack(RegisterStack *stack, U_16 size)
{
    if(!stack->useRAX){
        stack->useRAX = size/8;
        return RealRegister::eax;
    } else if(!stack->useRSI){
        stack->useRSI = size/8;
        return RealRegister::esi;
    } else if(!stack->useRDX){
        stack->useRDX = size/8;
        return RealRegister::edx;
    } else if(!stack->useRCX){
        stack->useRCX = size/8;
        return RealRegister::ecx;
    } else {
        stack->stackSlotsUsed += size/8;
        return RealRegister::NoReg;
    }
}

inline int
addParamFloatToStack(RegisterStack *stack, U_16 size)
{
    if(!stack->useXMM0){
        stack->useXMM0 = size/8;
        return RealRegister::xmm0;
    } else if(!stack->useXMM1){
        stack->useXMM1 = size/8;
        return RealRegister::xmm1;
    } else if(!stack->useXMM2){
        stack->useXMM2 = size/8;
        return RealRegister::xmm2;
    } else if(!stack->useXMM3){
        stack->useXMM3 = size/8;
        return RealRegister::xmm3;
    } else if(!stack->useXMM4){
        stack->useXMM4 = size/8;
        return RealRegister::xmm4;
    } else if(!stack->useXMM5){
        stack->useXMM5 = size/8;
        return RealRegister::xmm5;
    } else if(!stack->useXMM6){
        stack->useXMM6 = size/8;
        return RealRegister::xmm6;
    } else if(!stack->useXMM7){
        stack->useXMM7 = size/8;
        return RealRegister::xmm7;
    } else {
        stack->stackSlotsUsed += size/8;
        return RealRegister::NoReg;
    }
}

inline int
removeParamIntFromStack(RegisterStack *stack, U_16 *size)
{
    if(stack->useRAX){
        *size = stack->useRAX*8;
        stack->useRAX = 0;
        return RealRegister::eax;
    } else if(stack->useRSI){
        *size = stack->useRSI*8;
        stack->useRSI = 0;
        return RealRegister::esi;
    } else if(stack->useRDX){
        *size = stack->useRDX*8;
        stack->useRDX = 0;
        return RealRegister::edx;
    } else if(stack->useRCX){
        *size = stack->useRCX*8;
        stack->useRCX = 0;
        return RealRegister::ecx;
    } else {
        *size = 0;
        return RealRegister::NoReg;
    }
}

inline int
removeParamFloatFromStack(RegisterStack *stack, U_16 *size)
{
    if(stack->useXMM0){
        *size = stack->useXMM0*8;
        stack->useXMM0 = 0;
        return RealRegister::xmm0;
    } else if(stack->useXMM1){
        *size = stack->useXMM1*8;
        stack->useXMM1 = 0;
        return RealRegister::xmm1;
    } else if(stack->useXMM2){
        *size = stack->useXMM2*8;
        stack->useXMM2 = 0;
        return RealRegister::xmm2;
    } else if(stack->useXMM3){
        *size = stack->useXMM3*8;
        stack->useXMM3 = 0;
        return RealRegister::xmm3;
    } else if(stack->useXMM4){
        *size = stack->useXMM4*8;
        stack->useXMM4 = 0;
        return RealRegister::xmm4;
    } else if(stack->useXMM5){
        *size = stack->useXMM5*8;
        stack->useXMM5 = 0;
        return RealRegister::xmm5;
    } else if(stack->useXMM6){
        *size = stack->useXMM6*8;
        stack->useXMM6 = 0;
        return RealRegister::xmm6;
    } else if(stack->useXMM7){
        *size = stack->useXMM7*8;
        stack->useXMM7 = 0;
        return RealRegister::xmm7;
    } else {
        *size = 0;
        return RealRegister::NoReg;
    }
}

struct ParamTableEntry {
    int32_t offset;
    int32_t regNo;
    int32_t gcMapOffset;
    int32_t localArrayIndex;
    //Stored here so we can look up when saving to local array
    char slots;
    bool isReference;
    bool onStack;
};

// This function is only used for paramaters
inline ParamTableEntry
makeRegisterEntry(int32_t regNo, int32_t stackOffset, uint16_t size, bool isRef){
    ParamTableEntry entry;
    entry.regNo = regNo;
    entry.offset = stackOffset;
    entry.gcMapOffset = stackOffset;
    entry.localArrayIndex = -1;
    entry.onStack = false;
    entry.slots = size/8;
    entry.isReference = isRef;
    return entry;
}

// This function is only used for paramaters
inline ParamTableEntry
makeStackEntry(int32_t stackOffset, uint16_t size, bool isRef){
    ParamTableEntry entry;
    entry.regNo = MJIT::RealRegister::NoReg;
    entry.offset = stackOffset;
    entry.gcMapOffset = stackOffset;
    entry.localArrayIndex = -1;
    entry.onStack = true;
    entry.slots = size/8;
    entry.isReference = isRef;
    return entry;
}

class ParamTable
{
    private:
        ParamTableEntry *_tableEntries;
        U_16 _paramCount;
        RegisterStack *_registerStack;
    public:
        ParamTable(ParamTableEntry*, uint16_t, RegisterStack*);
        bool getEntry(uint16_t, ParamTableEntry*);
        bool setEntry(uint16_t, ParamTableEntry*);
        uint16_t getTotalParamSize();
        uint16_t getParamCount();
};

using LocalTableEntry=ParamTableEntry;

inline LocalTableEntry
makeLocalTableEntry(int32_t localIndex, int32_t gcMapOffset, uint16_t size, bool isRef)
{
    LocalTableEntry entry;
    entry.regNo = MJIT::RealRegister::NoReg;
    entry.offset = -1; //This field isn't used for locals
    entry.gcMapOffset = gcMapOffset; 
    entry.localArrayIndex = localIndex;
    entry.onStack = true;
    entry.slots = size/8;
    entry.isReference = isRef;
    return entry;
}
class LocalTable
{
    private:
        LocalTableEntry *_tableEntries;
        uint16_t _localCount;
    public:
        LocalTable(LocalTableEntry*, uint16_t);
        bool getEntry(uint16_t, LocalTableEntry*);
        uint16_t getTotalLocalSize();
        uint16_t getLocalCount();
};

}
#endif /* MJIT_SIDETABLE_HPP */
