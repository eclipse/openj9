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
#include "j9.h"
#include "env/VMJ9.h"

#include "compile/OMRCompilation.hpp"
#include "microjit/x/amd64/AMD64Codegen.hpp"
#include "runtime/CodeCacheManager.hpp"

#define MJIT_JITTED_BODY_INFO_PTR_SIZE 8
#define MJIT_SAVE_AREA_SIZE 2
#define MJIT_LINKAGE_INFO_SIZE 4
#define GENERATE_SWITCH_TO_INTERP_PREPROLOGUE 1

#define BIT_MASK_32 0xffffffff
#define BIT_MASK_64 0xffffffffffffffff

//Debug Params, comment/uncomment undef to get omit/enable debugging info
#define MJIT_DEBUG 1
//#undef MJIT_DEBUG

#ifdef MJIT_DEBUG 
#define MJIT_DEBUG_MAP_PARAMS 1 
#else 
#undef MJIT_DEBUG_MAP_PARAMS 
#endif
#ifdef MJIT_DEBUG 
#define MJIT_DEBUG_BC_WALKING 1
#define MJIT_DEBUG_BC_LOG(logFile, text) trfprintf(logFile, text)
#else 
#undef MJIT_DEBUG_BC_WALKING 
#define MJIT_DEBUG_BC_LOG(logFile, text) do {} while(0)
#endif

#define STACKCHECKBUFFER 512

#include <string.h>
#include <cstdio>

#define COPY_TEMPLATE(buffer, INSTRUCTION, size) do {\
    memcpy(buffer, INSTRUCTION, INSTRUCTION##Size);\
    buffer=(buffer+INSTRUCTION##Size);\
    size += INSTRUCTION##Size;\
} while(0)

#define PATCH_RELATIVE_ADDR(buffer, INSTRUCTION, absAddress) do {\
    intptr_t relativeAddress = (intptr_t)(buffer);\
    intptr_t minAddress = (relativeAddress < (intptr_t)(absAddress)) ? relativeAddress : (intptr_t)(absAddress);\
    intptr_t maxAddress = (relativeAddress > (intptr_t)(absAddress)) ? relativeAddress : (intptr_t)(absAddress);\
    intptr_t absDistance = maxAddress - minAddress;\
    relativeAddress = (relativeAddress < (intptr_t)(absAddress)) ? absDistance : (-1*(intptr_t)(absDistance));\
    patchImm4(buffer, (U_32)(relativeAddress & 0x00000000ffffffff));\
} while(0)

// Using char[] to signify that we are treating this
// like data, even though they are instructions.
#define EXTERN_TEMPLATE(TEMPLATE_NAME) extern char TEMPLATE_NAME[]
#define EXTERN_TEMPLATE_SIZE(TEMPLATE_NAME) extern unsigned short TEMPLATE_NAME##Size
#define DECLARE_TEMPLATE(TEMPLATE_NAME) EXTERN_TEMPLATE(TEMPLATE_NAME);\
    EXTERN_TEMPLATE_SIZE(TEMPLATE_NAME)


// Helpers linked from nathelpers.
extern "C" void j2iTransition();
extern "C" void samplingRecompileMethod();
extern "C" int jitStackOverflow();

// Labels linked from templates.
DECLARE_TEMPLATE(movRSPR10);
DECLARE_TEMPLATE(movR10R14);
DECLARE_TEMPLATE(subR10Imm4);
DECLARE_TEMPLATE(addR10Imm4);
DECLARE_TEMPLATE(addR14Imm4);
DECLARE_TEMPLATE(subR14Imm4);
DECLARE_TEMPLATE(subRSPImm8);
DECLARE_TEMPLATE(subRSPImm4);
DECLARE_TEMPLATE(subRSPImm2);
DECLARE_TEMPLATE(subRSPImm1);
DECLARE_TEMPLATE(addRSPImm8);
DECLARE_TEMPLATE(addRSPImm4);
DECLARE_TEMPLATE(addRSPImm2);
DECLARE_TEMPLATE(addRSPImm1);
DECLARE_TEMPLATE(jbe4ByteRel);
DECLARE_TEMPLATE(cmpRspRbpDerefOffset);
DECLARE_TEMPLATE(loadEBPOffset);
DECLARE_TEMPLATE(loadESPOffset);
DECLARE_TEMPLATE(loadEAXOffset);
DECLARE_TEMPLATE(loadESIOffset);
DECLARE_TEMPLATE(loadEDXOffset);
DECLARE_TEMPLATE(loadECXOffset);
DECLARE_TEMPLATE(loadEBXOffset);
DECLARE_TEMPLATE(loadEBPOffset);
DECLARE_TEMPLATE(loadESPOffset);
DECLARE_TEMPLATE(loadRAXOffset);
DECLARE_TEMPLATE(loadRSIOffset);
DECLARE_TEMPLATE(loadRDXOffset);
DECLARE_TEMPLATE(loadRCXOffset);
DECLARE_TEMPLATE(loadRBXOffset);
DECLARE_TEMPLATE(loadRBPOffset);
DECLARE_TEMPLATE(loadRSPOffset);
DECLARE_TEMPLATE(loadR9Offset);
DECLARE_TEMPLATE(loadR10Offset);
DECLARE_TEMPLATE(loadR11Offset);
DECLARE_TEMPLATE(loadR12Offset);
DECLARE_TEMPLATE(loadR13Offset);
DECLARE_TEMPLATE(loadR14Offset);
DECLARE_TEMPLATE(loadR15Offset);
DECLARE_TEMPLATE(loadXMM0Offset);
DECLARE_TEMPLATE(loadXMM1Offset);
DECLARE_TEMPLATE(loadXMM2Offset);
DECLARE_TEMPLATE(loadXMM3Offset);
DECLARE_TEMPLATE(loadXMM4Offset);
DECLARE_TEMPLATE(loadXMM5Offset);
DECLARE_TEMPLATE(loadXMM6Offset);
DECLARE_TEMPLATE(loadXMM7Offset);
DECLARE_TEMPLATE(saveEAXOffset);
DECLARE_TEMPLATE(saveESIOffset);
DECLARE_TEMPLATE(saveEDXOffset);
DECLARE_TEMPLATE(saveECXOffset);
DECLARE_TEMPLATE(saveEBXOffset);
DECLARE_TEMPLATE(saveEBPOffset);
DECLARE_TEMPLATE(saveESPOffset);
DECLARE_TEMPLATE(saveRAXOffset);
DECLARE_TEMPLATE(saveRSIOffset);
DECLARE_TEMPLATE(saveRDXOffset);
DECLARE_TEMPLATE(saveRCXOffset);
DECLARE_TEMPLATE(saveRBXOffset);
DECLARE_TEMPLATE(saveRBPOffset);
DECLARE_TEMPLATE(saveRSPOffset);
DECLARE_TEMPLATE(saveR9Offset);
DECLARE_TEMPLATE(saveR10Offset);
DECLARE_TEMPLATE(saveR11Offset);
DECLARE_TEMPLATE(saveR12Offset);
DECLARE_TEMPLATE(saveR13Offset);
DECLARE_TEMPLATE(saveR14Offset);
DECLARE_TEMPLATE(saveR15Offset);
DECLARE_TEMPLATE(saveXMM0Offset);
DECLARE_TEMPLATE(saveXMM1Offset);
DECLARE_TEMPLATE(saveXMM2Offset);
DECLARE_TEMPLATE(saveXMM3Offset);
DECLARE_TEMPLATE(saveXMM4Offset);
DECLARE_TEMPLATE(saveXMM5Offset);
DECLARE_TEMPLATE(saveXMM6Offset);
DECLARE_TEMPLATE(saveXMM7Offset);
DECLARE_TEMPLATE(saveXMM0Local);
DECLARE_TEMPLATE(saveXMM1Local);
DECLARE_TEMPLATE(saveXMM2Local);
DECLARE_TEMPLATE(saveXMM3Local);
DECLARE_TEMPLATE(saveXMM4Local);
DECLARE_TEMPLATE(saveXMM5Local);
DECLARE_TEMPLATE(saveXMM6Local);
DECLARE_TEMPLATE(saveXMM7Local);
DECLARE_TEMPLATE(saveRAXLocal);
DECLARE_TEMPLATE(saveRSILocal);
DECLARE_TEMPLATE(saveRDXLocal);
DECLARE_TEMPLATE(saveRCXLocal);
DECLARE_TEMPLATE(saveR11Local);
DECLARE_TEMPLATE(callByteRel);
DECLARE_TEMPLATE(call4ByteRel);
DECLARE_TEMPLATE(jump4ByteRel);
DECLARE_TEMPLATE(nopInstruction);
DECLARE_TEMPLATE(movRDIImm64);
DECLARE_TEMPLATE(movEDIImm32);
DECLARE_TEMPLATE(movRAXImm64);
DECLARE_TEMPLATE(movEAXImm32);
DECLARE_TEMPLATE(movRSPOffsetR11);
DECLARE_TEMPLATE(jumpRDI);
DECLARE_TEMPLATE(jumpRAX);
DECLARE_TEMPLATE(paintRegister);
DECLARE_TEMPLATE(paintLocal);


// bytecodes
DECLARE_TEMPLATE(debugBreakpoint);
DECLARE_TEMPLATE(loadTemplatePrologue); //This prologue exists because the instruction in the middle is the one which must be patched.
DECLARE_TEMPLATE(loadTemplate);
DECLARE_TEMPLATE(storeTemplate);
DECLARE_TEMPLATE(getFieldTemplatePrologue);
DECLARE_TEMPLATE(getFieldTemplate);
DECLARE_TEMPLATE(staticTemplatePrologue);
DECLARE_TEMPLATE(getStaticTemplate);
DECLARE_TEMPLATE(putStaticTemplate);
DECLARE_TEMPLATE(iAddTemplate);
DECLARE_TEMPLATE(iSubTemplate);
DECLARE_TEMPLATE(iMulTemplate);
DECLARE_TEMPLATE(iDivTemplate);
DECLARE_TEMPLATE(iReturnTemplate);
DECLARE_TEMPLATE(vReturnTemplate);

static void 
debug_print_hex(char *buffer, unsigned long long size)
{
    printf("Start of dump:\n");
    while(size--){
        printf("%x ", 0xc0 & *(buffer++));
    }
    printf("\nEnd of dump:");
}

inline uint32_t align(uint32_t number, uint32_t requirement, TR::FilePointer *fp)
{
    MJIT_ASSERT(fp, requirement && ((requirement & (requirement -1)) == 0), "INCORRECT ALIGNMENT");
    return (number + requirement - 1) & ~(requirement - 1);
}

static bool 
getRequiredAlignment(uintptr_t cursor, uintptr_t boundary, uintptr_t margin, uintptr_t *alignment)
{
    if((boundary & (boundary-1)) != 0)
        return true;
    *alignment = (-cursor - margin) & (boundary-1);
    return false;
}

bool
MJIT::nativeSignature(J9Method *method, char *resultBuffer)
{
    J9UTF8 *methodSig;
    UDATA arg;
    U_16 i, ch;
    BOOLEAN parsingReturnType = FALSE, processingBracket = FALSE;
    char nextType = '\0';

    methodSig = J9ROMMETHOD_SIGNATURE(J9_ROM_METHOD_FROM_RAM_METHOD(method));
    i = 0;
    arg = 3; /* skip the return type slot and JNI standard slots, they will be filled in later. */

    while(i < J9UTF8_LENGTH(methodSig)) {
        ch = J9UTF8_DATA(methodSig)[i++];
        switch(ch) {
            case '(':                                        /* start of signature -- skip */
                continue;
            case ')':                                        /* End of signature -- done args, find return type */
                parsingReturnType = TRUE;
                continue;
            case MJIT::CLASSNAME_TYPE_CHARACTER:
                nextType = MJIT::CLASSNAME_TYPE_CHARACTER;
                while(J9UTF8_DATA(methodSig)[i++] != ';') {}        /* a type string - loop scanning for ';' to end it - i points past ';' when done loop */
                break;
            case MJIT::BOOLEAN_TYPE_CHARACTER:
                nextType = MJIT::BOOLEAN_TYPE_CHARACTER;
                break;
            case MJIT::BYTE_TYPE_CHARACTER:
                nextType = MJIT::BYTE_TYPE_CHARACTER;
                break;
            case MJIT::CHAR_TYPE_CHARACTER:
                nextType = MJIT::CHAR_TYPE_CHARACTER;
                break;
            case MJIT::SHORT_TYPE_CHARACTER:
                nextType = MJIT::SHORT_TYPE_CHARACTER;
                break;
            case MJIT::INT_TYPE_CHARACTER:
                nextType = MJIT::INT_TYPE_CHARACTER;
                break;
            case MJIT::LONG_TYPE_CHARACTER:
                nextType = MJIT::LONG_TYPE_CHARACTER;
                break;
            case MJIT::FLOAT_TYPE_CHARACTER:
                nextType = MJIT::FLOAT_TYPE_CHARACTER;
                break;
            case MJIT::DOUBLE_TYPE_CHARACTER:
                nextType = MJIT::DOUBLE_TYPE_CHARACTER;
                break;
            case '[':
                processingBracket = TRUE;
                continue;                /* go back to top of loop for next char */
            case MJIT::VOID_TYPE_CHARACTER:
                if(!parsingReturnType) {
                    return true;
                }
                nextType = MJIT::VOID_TYPE_CHARACTER;
                break;

            default:
                nextType = '\0';
                return true;
                break;
        }
        if(processingBracket) {
            if(parsingReturnType) {
                resultBuffer[0] = MJIT::CLASSNAME_TYPE_CHARACTER;
                break;            /* from the while loop */
            } else {
                resultBuffer[arg] = MJIT::CLASSNAME_TYPE_CHARACTER;
                arg++;
                processingBracket = FALSE;
            }
        } else if(parsingReturnType) {
            resultBuffer[0] = nextType;
            break;            /* from the while loop */
        } else {
            resultBuffer[arg] = nextType;
            arg++;
        }
    }

    resultBuffer[1] = MJIT::CLASSNAME_TYPE_CHARACTER;     /* the JNIEnv */
    resultBuffer[2] = MJIT::CLASSNAME_TYPE_CHARACTER;    /* the jobject or jclass */
    resultBuffer[arg] = '\0';
    return false;
}

/**
 * Assuming that 'buffer' is the end of the immediate value location,
 * this function replaces the value supplied value 'imm'
 * 
 * Assumes 64-bit value
 */
static void
patchImm8(char *buffer, U_64 imm){
    *(((unsigned long long int*)buffer)-1) = imm;
}

/**
 * Assuming that 'buffer' is the end of the immediate value location,
 * this function replaces the value supplied value 'imm'
 * 
 * Assumes 32-bit value
 */
static void
patchImm4(char *buffer, U_32 imm){
    *(((unsigned int*)buffer)-1) = imm;
}

/**
 * Assuming that 'buffer' is the end of the immediate value location,
 * this function replaces the value supplied value 'imm'
 * 
 * Assumes 16-bit value
 */
static void
patchImm2(char *buffer, U_16 imm){
    *(((unsigned short int*)buffer)-1) = imm;
}

/**
 * Assuming that 'buffer' is the end of the immediate value location,
 * this function replaces the value supplied value 'imm'
 * 
 * Assumes 8-bit value
 */
static void
patchImm1(char *buffer, U_8 imm){
    *(unsigned char*)(buffer-1) = imm;
}

/*
| Architecture | Endian | Return address | vTable index         |
| x86-64       | Little | Stack          | R8 (receiver in RAX) |

| Integer Return value registers | Integer Preserved registers | Integer Argument registers |
| EAX (32-bit) RAX (64-bit)      | RBX R9                      | RAX RSI RDX RCX            |

| Float Return value registers | Float Preserved registers | Float Argument registers |
| XMM0                         | XMM8-XMM15                | XMM0-XMM7                |
*/

inline buffer_size_t
savePreserveRegisters(char *buffer, int32_t offset)
{
    int32_t size = 0;
    COPY_TEMPLATE(buffer, saveRBXOffset, size);
    patchImm4(buffer, offset);
    COPY_TEMPLATE(buffer, saveR9Offset, size);
    patchImm4(buffer, offset-8);
    COPY_TEMPLATE(buffer, saveRBPOffset, size);
    patchImm4(buffer, offset-16);
    COPY_TEMPLATE(buffer, saveRSPOffset, size);
    patchImm4(buffer, offset-24);
    return size;
}

inline buffer_size_t
loadPreserveRegisters(char *buffer, int32_t offset)
{
    int32_t size = 0;
    COPY_TEMPLATE(buffer, loadRBXOffset, size);
    patchImm4(buffer, offset);
    COPY_TEMPLATE(buffer, loadR9Offset, size);
    patchImm4(buffer, offset-8);
    COPY_TEMPLATE(buffer, loadRBPOffset, size);
    patchImm4(buffer, offset-16);
    COPY_TEMPLATE(buffer, loadRSPOffset, size);
    patchImm4(buffer, offset-24);
    return size;
}

#define COPY_IMM1_TEMPLATE(buffer, size, value) do{\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    patchImm1(buffer, (U_8)(value));\
}while(0)

#define COPY_IMM2_TEMPLATE(buffer, size, value) do{\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    patchImm2(buffer, (U_16)(value));\
}while(0)

#define COPY_IMM4_TEMPLATE(buffer, size, value) do{\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    patchImm4(buffer, (U_32)(value));\
}while(0)

#define COPY_IMM8_TEMPLATE(buffer, size, value) do{\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    COPY_TEMPLATE(buffer, nopInstruction, size);\
    patchImm8(buffer, (U_64)(value));\
}while(0)

MJIT::RegisterStack 
MJIT::mapIncomingParams(
    char *typeString, 
    U_16 maxLength, 
    int *error_code, 
    ParamTableEntry *paramTable, 
    U_16 paramCount, 
    TR::FilePointer *logFileFP
){
#ifdef MJIT_DEBUG_MAP_PARAMS
    trfprintf(logFileFP, "MapIncomingParams Start:\n");
#endif
    MJIT::RegisterStack stack;
    initParamStack(&stack);


    intptr_t offset = 0;

#ifdef MJIT_DEBUG_MAP_PARAMS
    trfprintf(logFileFP, "  maxLength: %04x\n", maxLength);
    trfprintf(logFileFP, "  MapIncomingParamsLoop Start:\n");
#endif
    for(int i = 0; i<paramCount; i++){
#ifdef MJIT_DEBUG_MAP_PARAMS
        trfprintf(logFileFP, "    i: %04x\n", i);
#endif
        //3rd index of typeString is first param.
        char typeChar = typeString[i+3];
        U_16 currentOffset = calculateOffset(&stack);
        U_16 size = MJIT::typeSignatureSize(typeChar);
        if(typeChar == MJIT::LONG_TYPE_CHARACTER && i == paramCount - 1){
            // TR requires last argument of type long uses 1 slot!
            size = 8;
        }
#ifdef MJIT_DEBUG_MAP_PARAMS
        trfprintf(logFileFP, "    ParamTableSize: %04x\n", size);
#endif
        int regNo = RealRegister::NoReg;
        bool isRef = false;
        switch(typeChar){
        case MJIT::CLASSNAME_TYPE_CHARACTER:
            isRef = true;
        case MJIT::BYTE_TYPE_CHARACTER:
        case MJIT::CHAR_TYPE_CHARACTER:
        case MJIT::INT_TYPE_CHARACTER:
        case MJIT::SHORT_TYPE_CHARACTER:
        case MJIT::BOOLEAN_TYPE_CHARACTER:
        case MJIT::LONG_TYPE_CHARACTER:
            regNo = addParamIntToStack(&stack, size);
            goto makeParamTableEntry;
        case MJIT::DOUBLE_TYPE_CHARACTER:
        case MJIT::FLOAT_TYPE_CHARACTER:
            regNo = addParamFloatToStack(&stack, size);
            goto makeParamTableEntry;
        makeParamTableEntry:
            paramTable[i] = (regNo != RealRegister::NoReg) ? //If this is a register
                makeRegisterEntry(regNo, currentOffset, size, isRef): //Add a register entry
                makeStackEntry(currentOffset, size, isRef); //else it's a stack entry
                //first stack entry (1 used) should be at offset 0.
            break;
        default:
            *error_code = 2;
            return stack;
        }
#ifdef MJIT_DEBUG_MAP_PARAMS
        trfprintf(logFileFP, "    Table:\n");
        trfprintf(logFileFP, "%01x", stack.useRAX);
        trfprintf(logFileFP, "%01x", stack.useRSI);
        trfprintf(logFileFP, "%01x", stack.useRDX);
        trfprintf(logFileFP, "%01x", stack.useRCX);
        trfprintf(logFileFP, "%01x", stack.useXMM0);
        trfprintf(logFileFP, "%01x", stack.useXMM1);
        trfprintf(logFileFP, "%01x", stack.useXMM2);
        trfprintf(logFileFP, "%01x", stack.useXMM3);
        trfprintf(logFileFP, "%01x", stack.useXMM4);
        trfprintf(logFileFP, "%01x", stack.useXMM5);
        trfprintf(logFileFP, "%01x", stack.useXMM6);
        trfprintf(logFileFP, "%01x", stack.useXMM7);
        trfprintf(logFileFP, "%02x", stack.stackSlotsUsed);
#endif
    }

    return stack;
}

int
MJIT::getParamCount(char *typeString, U_16 maxLength)
{
    U_16 paramCount = 0;
    for(int i = 3; i<maxLength; i++){
        switch(typeString[i]){
        case MJIT::BYTE_TYPE_CHARACTER:
        case MJIT::CHAR_TYPE_CHARACTER:
        case MJIT::INT_TYPE_CHARACTER:
        case MJIT::CLASSNAME_TYPE_CHARACTER:
        case MJIT::SHORT_TYPE_CHARACTER:
        case MJIT::BOOLEAN_TYPE_CHARACTER:
        case MJIT::LONG_TYPE_CHARACTER:
        case MJIT::DOUBLE_TYPE_CHARACTER:
        case MJIT::FLOAT_TYPE_CHARACTER:
            paramCount++;
            break;
        default:
            return 0;
        }
    }
    return paramCount;
}

inline uint32_t 
gcd(uint32_t a, uint32_t b)
{
    while (b != 0)
    {
        uint32_t t = b;
        b = a % b;
        a = t;
    }
    return a;
}

inline uint32_t 
lcm(uint32_t a, uint32_t b)
{
    return a * b / gcd(a, b);
}

MJIT::ParamTable::ParamTable(ParamTableEntry *tableEntries, U_16 paramCount, RegisterStack *registerStack)
    : _tableEntries(tableEntries)
    , _paramCount(paramCount)
    , _registerStack(registerStack)
{}

bool
MJIT::ParamTable::getEntry(U_16 paramIndex, MJIT::ParamTableEntry *entry)
{
    bool success = paramIndex < _paramCount;
    if(success)
        *entry = _tableEntries[paramIndex];
    return success;
}

bool
MJIT::ParamTable::setEntry(U_16 paramIndex, MJIT::ParamTableEntry *entry)
{
    bool success = paramIndex < _paramCount;
    if(success)
        _tableEntries[paramIndex] = *entry;
    return success;
}

U_16
MJIT::ParamTable::getTotalParamSize()
{
    return calculateOffset(_registerStack);
}

U_16
MJIT::ParamTable::getParamCount()
{
    return _paramCount;
}

MJIT::LocalTable::LocalTable(LocalTableEntry *tableEntries, U_16 localCount)
    : _tableEntries(tableEntries)
    , _localCount(localCount)
{}

bool
MJIT::LocalTable::getEntry(U_16 localIndex, MJIT::LocalTableEntry *entry)
{
    bool success = localIndex < _localCount;
    if(success)
        *entry = _tableEntries[localIndex];
    return success;
}

U_16
MJIT::LocalTable::getTotalLocalSize()
{
    U_16 localSize = 0;
    for(int i=0; i<_localCount; i++){
        localSize += _tableEntries[i].slots*8;
    }
    return localSize;
}

U_16
MJIT::LocalTable::getLocalCount()
{
    return _localCount;
}

buffer_size_t
MJIT::CodeGenerator::saveArgsInLocalArray(
    char *buffer,
    buffer_size_t stack_alloc_space
){
    buffer_size_t saveSize = 0;
    U_16 slot = 0;
    RealRegister::RegNum regNum = RealRegister::NoReg;
    int index = 0;
    for(; index<_paramTable->getParamCount(); index++){
        ParamTableEntry entry;
        MJIT_ASSERT(_logFileFP, _paramTable->getEntry(index, &entry), "Bad index for table entry");
        if(entry.onStack)
            break;
        int32_t regNum = entry.regNo;
        switch(regNum){
            case RealRegister::xmm0:
                COPY_TEMPLATE(buffer, saveXMM0Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::xmm1:
                COPY_TEMPLATE(buffer, saveXMM1Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::xmm2:
                COPY_TEMPLATE(buffer, saveXMM2Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::xmm3:
                COPY_TEMPLATE(buffer, saveXMM3Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::xmm4:
                COPY_TEMPLATE(buffer, saveXMM4Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::xmm5:
                COPY_TEMPLATE(buffer, saveXMM5Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::xmm6:
                COPY_TEMPLATE(buffer, saveXMM6Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::xmm7:
                COPY_TEMPLATE(buffer, saveXMM7Local, saveSize);
                goto PatchAndBreak;
            case RealRegister::eax:
                COPY_TEMPLATE(buffer, saveRAXLocal,  saveSize);
                goto PatchAndBreak;
            case RealRegister::esi:
                COPY_TEMPLATE(buffer, saveRSILocal,  saveSize);
                goto PatchAndBreak;
            case RealRegister::edx:
                COPY_TEMPLATE(buffer, saveRDXLocal,  saveSize);
                goto PatchAndBreak;
            case RealRegister::ecx:
                COPY_TEMPLATE(buffer, saveRCXLocal,  saveSize);
                goto PatchAndBreak;
            PatchAndBreak:
                patchImm4(buffer, (U_32)((slot*8) & BIT_MASK_32));
                break;
            case RealRegister::NoReg:
                break;
        }
        slot += entry.slots;
    }
    
    for(; index<_paramTable->getParamCount(); index++){
        ParamTableEntry entry;
        MJIT_ASSERT(_logFileFP, _paramTable->getEntry(index, &entry), "Bad index for table entry");
        uintptr_t offset = entry.offset;
        //Our linkage templates for loading from the stack assume that the offset will always be less than 0xff.
        //  This is however not always true for valid code. If this becomes a problem in the future we will
        //  have to add support for larger offsets by creating new templates that use 2 byte offsets from rsp.
        //  Whether that will be a change to the current tempaltes, or new templates with supporting code
        //  is a decision yet to be determined.
        MJIT_ASSERT(_logFileFP, (offset + stack_alloc_space) < uintptr_t(0xff), "Offset too large, add support for larger offsets");
        COPY_TEMPLATE(buffer, movRSPOffsetR11, saveSize);
        patchImm1(buffer, (U_8)(offset+stack_alloc_space));
        COPY_TEMPLATE(buffer, saveR11Local, saveSize);
        patchImm4(buffer, (U_32)((slot*8) & BIT_MASK_32));
        slot += entry.slots;
    }
    return saveSize;
}

buffer_size_t 
MJIT::CodeGenerator::saveArgsOnStack(
    char *buffer, 
    buffer_size_t stack_alloc_space
){
    buffer_size_t saveArgsSize = 0;
    U_16 offset = _paramTable->getTotalParamSize();
    RealRegister::RegNum regNum = RealRegister::NoReg;
    for(int i=0; i<_paramTable->getParamCount(); i++){
        ParamTableEntry entry;
        MJIT_ASSERT(_logFileFP, _paramTable->getEntry(i, &entry), "Bad index for table entry");
        if(entry.onStack)
            break;
        int32_t regNum = entry.regNo;
        switch(regNum){
            case RealRegister::xmm0:
                COPY_TEMPLATE(buffer, saveXMM0Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm1:
                COPY_TEMPLATE(buffer, saveXMM1Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm2:
                COPY_TEMPLATE(buffer, saveXMM2Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm3:
                COPY_TEMPLATE(buffer, saveXMM3Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm4:
                COPY_TEMPLATE(buffer, saveXMM4Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm5:
                COPY_TEMPLATE(buffer, saveXMM5Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm6:
                COPY_TEMPLATE(buffer, saveXMM6Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm7:
                COPY_TEMPLATE(buffer, saveXMM7Offset, saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::eax:
                COPY_TEMPLATE(buffer, saveRAXOffset,  saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::esi:
                COPY_TEMPLATE(buffer, saveRSIOffset,  saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::edx:
                COPY_TEMPLATE(buffer, saveRDXOffset,  saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::ecx:
                COPY_TEMPLATE(buffer, saveRCXOffset,  saveArgsSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::NoReg:
                break;
        }
        offset -= entry.slots*8;
    }
    return saveArgsSize;
}

buffer_size_t 
MJIT::CodeGenerator::loadArgsFromStack(
    char *buffer, 
    buffer_size_t stack_alloc_space
){
    U_16 offset = _paramTable->getTotalParamSize();
    buffer_size_t argLoadSize = 0;
    RealRegister::RegNum regNum = RealRegister::NoReg;
    for(int i=0; i<_paramTable->getParamCount(); i++){
        ParamTableEntry entry;
        MJIT_ASSERT(_logFileFP, _paramTable->getEntry(i, &entry), "Bad index for table entry");
        if(entry.onStack)
            break;
        int32_t regNum = entry.regNo;
        switch(regNum){
            case RealRegister::xmm0:
                COPY_TEMPLATE(buffer, loadXMM0Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm1:
                COPY_TEMPLATE(buffer, loadXMM1Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm2:
                COPY_TEMPLATE(buffer, loadXMM2Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm3:
                COPY_TEMPLATE(buffer, loadXMM3Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm4:
                COPY_TEMPLATE(buffer, loadXMM4Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm5:
                COPY_TEMPLATE(buffer, loadXMM5Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm6:
                COPY_TEMPLATE(buffer, loadXMM6Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::xmm7:
                COPY_TEMPLATE(buffer, loadXMM7Offset, argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::eax:
                COPY_TEMPLATE(buffer, loadRAXOffset,  argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::esi:
                COPY_TEMPLATE(buffer, loadRSIOffset,  argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::edx:
                COPY_TEMPLATE(buffer, loadRDXOffset,  argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::ecx:
                COPY_TEMPLATE(buffer, loadRCXOffset,  argLoadSize);
                patchImm4(buffer, (U_32)((offset+ stack_alloc_space) & 0xffffffff));
                break;
            case RealRegister::NoReg:
                break;
        }
        offset -= entry.slots*8;
    }
    return argLoadSize;
}

MJIT::CodeGenerator::CodeGenerator(
    struct J9JITConfig *config, 
    J9VMThread *thread, 
    TR::FilePointer *fp, 
    TR_J9VMBase &vm, 
    ParamTable *paramTable,
    TR::Compilation *comp,
    MJIT::CodeGenGC *mjitCGGC
    ):_linkage(Linkage(config, thread, fp))
    ,_logFileFP(fp)
    ,_vm(vm)
    ,_codeCache(NULL)
    ,_stackPeakSize(0)
    ,_paramTable(paramTable)
    ,_comp(comp)
    ,_mjitCGGC(mjitCGGC)
    ,_atlas(NULL)
{
    _linkage._properties.setOutgoingArgAlignment(lcm(16, _vm.getLocalObjectAlignmentInBytes()));
}

TR::GCStackAtlas*
MJIT::CodeGenerator::getStackAtlas()
{
    return _atlas;
}

buffer_size_t
MJIT::CodeGenerator::generateSwitchToInterpPrePrologue(
    char *buffer,
    J9Method *method,
    buffer_size_t boundary,
    buffer_size_t margin,
    char *typeString,
    U_16 maxLength
){

    uintptr_t startOfMethod = (uintptr_t)buffer;
    buffer_size_t switchSize = 0;

    // Store the J9Method address in edi and store the label of this jitted method.
    uintptr_t j2iTransitionPtr = (uintptr_t)j2iTransition;
    uintptr_t methodPtr = (uintptr_t)method;
    COPY_TEMPLATE(buffer, movRDIImm64, switchSize);
    patchImm8(buffer, methodPtr);

    //if (comp->getOption(TR_EnableHCR))
    //   comp->getStaticHCRPICSites()->push_front(prev);

    // Pass args on stack, expects the method to run in RDI
    buffer_size_t saveArgSize = saveArgsOnStack(buffer, 0);
    buffer += saveArgSize;
    switchSize += saveArgSize;

    // Generate jump to the TR_i2jTransition function
    COPY_TEMPLATE(buffer, jump4ByteRel, switchSize);
    PATCH_RELATIVE_ADDR(buffer, jump4ByteRel, j2iTransitionPtr);

    margin += 2;
    uintptr_t requiredAlignment = 0;
    if(getRequiredAlignment((uintptr_t)buffer, boundary, margin, &requiredAlignment)){
        buffer = (char*)startOfMethod;
        return 0;
    }
    for(U_8 i = 0; i<requiredAlignment; i++){
        COPY_TEMPLATE(buffer, nopInstruction, switchSize);
    }

    //Generate relative jump to start
    char distance = (char)(((uintptr_t)buffer - startOfMethod) & 0x00000000000000FF);
    COPY_TEMPLATE(buffer, jump4ByteRel, switchSize);
    patchImm4(buffer, (unsigned char)distance);
    
    return switchSize;
}

buffer_size_t
MJIT::CodeGenerator::generatePrePrologue(
    char *buffer, 
    J9Method *method, 
    char **magicWordLocation,
    char **first2BytesPatchLocation,
    TR_PersistentJittedBodyInfo **bodyInfo
){
    // Return size (in bytes) of pre-proglue on success

    int preprologueSize = 0;
    int alignmentMargin = 0;
    int alignmentBoundary = 8;
    uintptr_t endOfPreviousMethod = (uintptr_t)buffer;

    J9ROMClass *romClass = J9_CLASS_FROM_METHOD(method)->romClass;
    J9ROMMethod *romMethod = J9_ROM_METHOD_FROM_RAM_METHOD(method);
    U_16 maxLength = J9UTF8_LENGTH(J9ROMMETHOD_SIGNATURE(romMethod));
    char typeString[maxLength];
    if(nativeSignature(method, typeString)){
        return 0;
    }

    // If sampling, add sampling size to margin. 

    // Save area for the first two bytes of the method + jitted body info pointer + linkageinfo in margin.
    alignmentMargin += MJIT_SAVE_AREA_SIZE + MJIT_JITTED_BODY_INFO_PTR_SIZE + MJIT_LINKAGE_INFO_SIZE;


    // Make sure the startPC at least 4-byte aligned.  This is important, since the VM
    // depends on the alignment (it uses the low order bits as tag bits).
    //
    // TODO: `mustGenerateSwitchToInterpreterPrePrologue` checks that the code generator's compilation is not:
    //         `usesPreexistence`,
    //         `getOption(TR_EnableHCR)`,
    //         `!fej9()->isAsyncCompilation`,
    //         `getOption(TR_FullSpeedDebug)`,
    //
    if(GENERATE_SWITCH_TO_INTERP_PREPROLOGUE){ // TODO: Replace with a check that perfroms the above checks.
        //generateSwitchToInterpPrePrologue will align data for use
        char *old_buff = buffer;
        preprologueSize += generateSwitchToInterpPrePrologue(buffer, method, alignmentBoundary, alignmentMargin, typeString, maxLength);
        buffer += preprologueSize;

        trfprintf(_logFileFP, "\ngenerateSwitchToInterpPrePrologue: %d\n", preprologueSize);
        for(int32_t i = 0; i < preprologueSize; i++){
           trfprintf(_logFileFP, "%02x\n", ((unsigned char)old_buff[i]) & (unsigned char)0xff);
        }
    } else {
        uintptr_t requiredAlignment = 0;
        if(getRequiredAlignment((uintptr_t)buffer, alignmentBoundary, alignmentMargin, &requiredAlignment)){
            buffer = (char*)endOfPreviousMethod;
            return 0;
        }
        for(U_8 i = 0; i<requiredAlignment; i++){
            COPY_TEMPLATE(buffer, nopInstruction, preprologueSize);
        }
    }


    // On 64 bit target platforms 
    // Copy the first two bytes of the method, in case we need to un-patch them
    COPY_IMM2_TEMPLATE(buffer, preprologueSize, 0xcccc);
    *first2BytesPatchLocation = buffer;


    //if we are using sampling, generate the call instruction to patch the sampling mathod
    // Note: the displacement of this call gets patched, but thanks to the
    // alignment constraint on the startPC, it is automatically aligned to a
    // 4-byte boundary, which is more than enough to ensure it can be patched
    // atomically.
    //
    COPY_TEMPLATE(buffer, call4ByteRel, preprologueSize);
    PATCH_RELATIVE_ADDR(buffer, call4ByteRel, samplingRecompileMethod);

    // The address of the persistent method info is inserted in the pre-prologue
    // information. If the method is not to be compiled again a null value is
    // inserted.
    *bodyInfo = TR_PersistentJittedBodyInfo::allocate(
        nullptr,
        TR_Hotness::noOpt, 
        false, 
        nullptr);
    COPY_IMM8_TEMPLATE(buffer, preprologueSize, *bodyInfo);


    // Allow 4 bytes for private linkage return type info. Allocate the 4 bytes
    // even if the linkage is not private, so that all the offsets are
    // predictable.
    //TODO: This is missing something, does not match the TR magic word.
    uint32_t magicWord = 0;
    switch(typeString[0]){
        case MJIT::VOID_TYPE_CHARACTER:
            COPY_IMM4_TEMPLATE(buffer, preprologueSize, magicWord | TR_VoidReturn);
            break;
        case MJIT::BYTE_TYPE_CHARACTER:
        case MJIT::SHORT_TYPE_CHARACTER:
        case MJIT::INT_TYPE_CHARACTER:
            COPY_IMM4_TEMPLATE(buffer, preprologueSize, magicWord | TR_IntReturn);
            break;
        case MJIT::LONG_TYPE_CHARACTER:
            COPY_IMM4_TEMPLATE(buffer, preprologueSize, magicWord | TR_LongReturn);
            break;
        case MJIT::CLASSNAME_TYPE_CHARACTER:
            COPY_IMM4_TEMPLATE(buffer, preprologueSize, magicWord | TR_ObjectReturn);
            break;
        case MJIT::FLOAT_TYPE_CHARACTER:
            COPY_IMM4_TEMPLATE(buffer, preprologueSize, magicWord | TR_FloatXMMReturn);
            break;
        case MJIT::DOUBLE_TYPE_CHARACTER:
            COPY_IMM4_TEMPLATE(buffer, preprologueSize, magicWord | TR_DoubleXMMReturn);
            break;
    }
    *magicWordLocation = buffer;
    return preprologueSize;
}

MJIT::LocalTable 
MJIT::CodeGenerator::makeLocalTable(TR_J9ByteCodeIterator *bci, MJIT::LocalTableEntry *localTableEntries, U_16 entries, int32_t offsetToFirstLocal)
{
    int32_t totalSize = 0;

    //Use defaults that help catch errors when debugging
    int32_t localIndex = -1; // Indexes are 0 based and positive
    int32_t gcMapOffset = 0; // Offset will always be greater than 0
    U_16 size = 0; //Sizes are all multiples of 8 and greater than 0
    bool isRef = false; //No good default here, both options are valid.

    for(TR_J9ByteCode bc = bci->first(); bc != J9BCunknown; bc = bci->next()){
        /* It's okay to overwrite entries here.
         * We are not storing whether the entry is a used for 
         * load or a store because it could be both. Identifying entries 
         * that we have already created would require zeroing the 
         * array before use and/or remembering which entries we've created.
         * Both might be as much work as just recreating entries, depending 
         * on how many times a local is stored/loaded during a method.
         * This may be worth doing later, but requires some research first.
         */
        gcMapOffset = 0;
        switch(bc){
            MakeEntry:
                gcMapOffset = offsetToFirstLocal + totalSize;
                localTableEntries[localIndex] = makeLocalTableEntry(localIndex, gcMapOffset, size, isRef);
                totalSize += size;
                break;
            case J9BCiload:
            case J9BCistore:
            case J9BCfstore:
            case J9BCfload:
                localIndex = (int32_t)bci->nextByte();
                size = 8;
                goto MakeEntry;
            case J9BCiload0:
            case J9BCistore0:
            case J9BCfload0:
            case J9BCfstore0:
                localIndex = 0;
                size = 8;
                goto MakeEntry;
            case J9BCiload1:
            case J9BCistore1:
            case J9BCfstore1:
            case J9BCfload1:
                localIndex = 1;
                size = 8;
                goto MakeEntry;
            case J9BCiload2:
            case J9BCistore2:
            case J9BCfstore2:
            case J9BCfload2:
                localIndex = 2;
                size = 8;
                goto MakeEntry;
            case J9BCiload3:
            case J9BCistore3:
            case J9BCfstore3:
            case J9BCfload3:
                localIndex = 3;
                size = 8;
                goto MakeEntry;
            case J9BClstore:
            case J9BClload:
            case J9BCdstore:
            case J9BCdload:
                localIndex = (int32_t)bci->nextByte();
                size = 16;
                goto MakeEntry;
            case J9BClstore0:
            case J9BClload0:
            case J9BCdload0:
            case J9BCdstore0:
                localIndex = 0;
                size = 16;
                goto MakeEntry;
            case J9BClstore1:
            case J9BCdstore1:
            case J9BCdload1:
            case J9BClload1:
                localIndex = 1;
                size = 16;
                goto MakeEntry;
            case J9BClstore2:
            case J9BClload2:
            case J9BCdstore2:
            case J9BCdload2:
                localIndex = 2;
                size = 16;
                goto MakeEntry;
            case J9BClstore3:
            case J9BClload3:
            case J9BCdstore3:
            case J9BCdload3:
                localIndex = 3;
                size = 16;
                goto MakeEntry;
            case J9BCaload:
                localIndex = (int32_t)bci->nextByte();
                size = 8;
                isRef = true;
                goto MakeEntry;
            case J9BCastore:
            case J9BCaload0:
            case J9BCastore0:
                localIndex = 0;
                size = 8;
                isRef = true;
                goto MakeEntry;
            case J9BCaload1:
            case J9BCastore1:
                localIndex = 1;
                size = 8;
                isRef = true;
                goto MakeEntry;
            case J9BCaload2:
            case J9BCastore2:
                localIndex = 2;
                size = 8;
                isRef = true;
                goto MakeEntry;
            case J9BCaload3:
            case J9BCastore3:
                localIndex = 3;
                size = 8;
                isRef = true;
                goto MakeEntry;
            default:
                //This is weak. We should be finding all bytecodes
                //  which put values in the bytecodes of the method
                //  and explicitly skip the correct number of bytecodes
                break;
        }
    }
    MJIT::LocalTable localTable(localTableEntries, entries);
    return localTable;
}

/**
 * Write the prologue to the buffer and return the size written to the buffer.
 */
buffer_size_t
MJIT::CodeGenerator::generatePrologue(
    char *buffer, 
    J9Method *method, 
    char **jitStackOverflowJumpPatchLocation,
    char *magicWordLocation,
    char *first2BytesPatchLocation,
    char **firstInstLocation,
    TR_J9ByteCodeIterator *bci
){
    int prologueSize = 0;
    uintptr_t prologueStart = (uintptr_t)buffer;

    J9ROMClass *romClass = J9_CLASS_FROM_METHOD(method)->romClass;
    J9ROMMethod *romMethod = J9_ROM_METHOD_FROM_RAM_METHOD(method);
    U_16 maxLength = J9UTF8_LENGTH(J9ROMMETHOD_SIGNATURE(romMethod));
    char typeString[maxLength];
    if(nativeSignature(method, typeString)){
        return 0;
    }

    //Same order as saving (See generateSwitchToInterpPrePrologue)
    buffer_size_t loadArgSize = loadArgsFromStack(buffer, 0);
    buffer += loadArgSize;
    prologueSize += loadArgSize;

    uint32_t magicWord = *(magicWordLocation-4); //We save the end of the location for patching, not the start.
    magicWord |= (((uintptr_t)buffer) - prologueStart) << 16;
    patchImm4(magicWordLocation, magicWord);
    
    //check for stack overflow
    *firstInstLocation = buffer;
    COPY_TEMPLATE(buffer, cmpRspRbpDerefOffset, prologueSize);
    patchImm4(buffer, (U_32)(0x50));
    patchImm2(first2BytesPatchLocation, *(uint16_t*)(firstInstLocation));
    COPY_TEMPLATE(buffer, jbe4ByteRel, prologueSize);
    *jitStackOverflowJumpPatchLocation = (char*)buffer;

    // Compute frame size
    //
    // allocSize: bytes to be subtracted from the stack pointer when allocating the frame
    // peakSize:  maximum bytes of stack this method might consume before encountering another stack check
    //
    struct properties properties = _linkage._properties;
    //Max amount of data to be preserved. It is overkill, but as long as the prologue/epilogue save/load everything
    //  it is a workable solution for now. Later try to determine what registers need to be preserved ahead of time.
    U_32 preservedRegsSize = properties._numPreservedRegisters * properties.getPointerSize();
    const int32_t pointerSize = properties.getPointerSize();
    const int32_t localSize = (romMethod->argCount + romMethod->tempCount)*pointerSize;
    MJIT_ASSERT(_logFileFP, localSize >= 0, "assertion failure");
    int32_t frameSize = localSize + preservedRegsSize + ( _linkage._properties.getReservesOutgoingArgsInPrologue() ? properties.getPointerSize() : 0 );
    uint32_t stackSize = frameSize + properties.getRetAddressWidth();
    uint32_t adjust = align(stackSize, properties.getOutgoingArgAlignment(), _logFileFP) - stackSize;
    auto allocSize = frameSize + adjust;

    //return address is allocated by call instruction
    // Here we conservatively assume there is a call in this method that will require space for its return address
    _stackPeakSize = 
        allocSize + //space for stack frame
        properties.getPointerSize() + //space for return address, conservatively expecting a call
        _stackPeakSize; //space for mjit value stack (set in CompilationInfoPerThreadBase::mjit)

    // Small: entire stack usage fits in STACKCHECKBUFFER, so if sp is within
    // the soft limit before buying the frame, then the whole frame will fit
    // within the hard limit.
    // 
    // Medium: the additional stack required after bumping the sp fits in 
    // STACKCHECKBUFFER, so if sp after the bump is within the soft limit, the 
    // whole frame will fit within the hard limit. 
    //
    // Large: No shortcuts.  Calculate the maximum extent of stack needed and
    // compare that against the soft limit.  (We have to use the soft limit here
    // if for no other reason than that's the one used for asyncchecks.)
    //

    COPY_TEMPLATE(buffer, subRSPImm4, prologueSize);
    patchImm4(buffer, _stackPeakSize);

    int savePreserveSize = savePreserveRegisters(buffer, _stackPeakSize-8);
    buffer += savePreserveSize;
    prologueSize += savePreserveSize;

    buffer_size_t saveArgSize = saveArgsOnStack(buffer, _stackPeakSize);
    buffer += saveArgSize;
    prologueSize += saveArgSize;

    /*
     * MJIT stack frames must conform to TR stack frame shapes. However, so long as we are able to have our frames walked, we should be
     * free to allocate more things on the stack before the required data which is used for walking. The following is the general shape of the
     * initial MJIT stack frame
     * +-----------------------------+
     * |             ...             |
     * | Paramaters passed by caller | <-- The space for saving params passed in registers is also here
     * |             ...             |
     * +-----------------------------+
     * |        Return Address       | <-- RSP points here before we sub the _stackPeakSize
     * +-----------------------------+ <-- Caller/Callee stack boundary.
     * |             ...             |
     * |     Preserved Registers     |
     * |             ...             |
     * +-----------------------------+
     * |             ...             |
     * |       Local Variables       |
     * |             ...             | <-- R10 points here at the end of the prologue, R14 always points here.
     * +-----------------------------+ 
     * |             ...             |
     * |    MJIT Computation Stack   |
     * |             ...             |
     * +-----------------------------+ <-- RSP points here (as far as we currently know) after we sub _stackPeakSize
     * |             ...             |
     * | Paramaters passed to callee | <-- This is not implemented yet, because we do not yet support calling functions.
     * |             ...             |
     * +-----------------------------+
     * |        Return Address       | <-- This space has been saved, but isn't being used because we do not yet call functions.
     * +-----------------------------+ <-- End of stack frame
     */

    //Set up MJIT value stack
    COPY_TEMPLATE(buffer, movRSPR10, prologueSize);
    COPY_TEMPLATE(buffer, addR10Imm4, prologueSize);
    patchImm4(buffer, (U_32)(romMethod->maxStack*8)); 
    //TODO: Find a way to cleanly preserve this value before overriding it in the _stackAllocSize

    //Set up MJIT local array
    COPY_TEMPLATE(buffer, movR10R14, prologueSize);

    // Move parameters to where the method body will expect to find them
    buffer_size_t saveSize = saveArgsInLocalArray(buffer, _stackPeakSize);
    prologueSize += saveSize;
    if (!saveSize && romMethod->argCount) return 0;

    int32_t firstLocalOffset = (preservedRegsSize+8);
    U_16 localVariableCount = romMethod->argCount + romMethod->tempCount;
    MJIT::LocalTableEntry localTableEntries[localVariableCount];
    LocalTable localTable = makeLocalTable(bci, localTableEntries, localVariableCount, firstLocalOffset);

    //The Parameters are now in the local array, so we update the entries in the ParamTable with their values from the LocalTable
    for(int i=0; i<_paramTable->getParamCount(); i++){
        ParamTableEntry entry;
        //The indexes should be the same in both tables
        //  because (as far as we know) the parameters
        //  are indexed the same as locals and parameters
        localTable.getEntry(i, &entry);
        _paramTable->setEntry(i, &entry);
    }

    TR::GCStackAtlas *atlas = _mjitCGGC->createStackAtlas(_comp, _paramTable, &localTable);
    if (!atlas) return 0;
    _atlas = atlas;

    //Support to paint allocated frame slots.
    if (( _comp->getOption(TR_PaintAllocatedFrameSlotsDead) || _comp->getOption(TR_PaintAllocatedFrameSlotsFauxObject) )  && allocSize!=0) {
        uint64_t paintValue = 0;

        //Paint the slots with deadf00d
        if (_comp->getOption(TR_PaintAllocatedFrameSlotsDead)) {
            paintValue = (uint64_t)CONSTANT64(0xdeadf00ddeadf00d);
        } else { //Paint stack slots with a arbitrary object aligned address.
            paintValue = ((uintptr_t) ((uintptr_t)_comp->getOptions()->getHeapBase() + (uintptr_t) 4096));
        }

        COPY_TEMPLATE(buffer, paintRegister, prologueSize);
        patchImm8(buffer, paintValue);
        for(int32_t i=_paramTable->getParamCount(); i<localTable.getLocalCount(); i++){
            LocalTableEntry entry;
            localTable.getEntry(i, &entry);
            COPY_TEMPLATE(buffer, paintLocal, prologueSize);
            patchImm4(buffer, entry.localArrayIndex * pointerSize);
        }
    }
    
    return prologueSize;
}

buffer_size_t
MJIT::CodeGenerator::generateEpologue(char *buffer){
    int epologueSize = loadPreserveRegisters(buffer, _stackPeakSize-8);
    buffer += epologueSize;
    COPY_TEMPLATE(buffer, addRSPImm4, epologueSize);
    patchImm4(buffer, _stackPeakSize);
    return epologueSize;
}

// Macros to clean up the switch case for generate body
#define loadCasePrologue(byteCodeCaseType)\
case TR_J9ByteCode::byteCodeCaseType
#define loadCaseBody(byteCodeCaseType)\
    trfprintf(_logFileFP, #byteCodeCaseType "\n");\
    goto GenericLoadCall
#define storeCasePrologue(byteCodeCaseType)\
case TR_J9ByteCode::byteCodeCaseType
#define storeCaseBody(byteCodeCaseType)\
    trfprintf(_logFileFP, #byteCodeCaseType "\n");\
    goto GenericStoreCall
 
buffer_size_t 
MJIT::CodeGenerator::generateBody(char *buffer, TR_ResolvedMethod *method, TR_J9ByteCodeIterator *bci){
    buffer_size_t codeGenSize = 0;
    buffer_size_t calledCGSize = 0;
    for(TR_J9ByteCode bc = bci->first(); bc != J9BCunknown; bc = bci->next())
    {
        switch(bc) {
            loadCasePrologue(J9BCiload0):
            loadCaseBody(J9BCiload0);
            loadCasePrologue(J9BCiload1):
            loadCaseBody(J9BCiload1);
            loadCasePrologue(J9BCiload2):
            loadCaseBody(J9BCiload2);
            loadCasePrologue(J9BCiload3):
            loadCaseBody(J9BCiload3);
            loadCasePrologue(J9BCiload):
            loadCaseBody(J9BCiload);
            loadCasePrologue(J9BClload0):
            loadCaseBody(J9BClload0);
            loadCasePrologue(J9BClload1):
            loadCaseBody(J9BClload1);
            loadCasePrologue(J9BClload2):
            loadCaseBody(J9BClload2);
            loadCasePrologue(J9BClload3):
            loadCaseBody(J9BClload3);
            loadCasePrologue(J9BClload):
            loadCaseBody(J9BClload);
            loadCasePrologue(J9BCfload0):
            loadCaseBody(J9BCfload0);
            loadCasePrologue(J9BCfload1):
            loadCaseBody(J9BCfload1);
            loadCasePrologue(J9BCfload2):
            loadCaseBody(J9BCfload2);
            loadCasePrologue(J9BCfload3):
            loadCaseBody(J9BCfload3);
            loadCasePrologue(J9BCfload):
            loadCaseBody(J9BCfload);
            loadCasePrologue(J9BCdload0):
            loadCaseBody(J9BCdload0);
            loadCasePrologue(J9BCdload1):
            loadCaseBody(J9BCdload1);
            loadCasePrologue(J9BCdload2):
            loadCaseBody(J9BCdload2);
            loadCasePrologue(J9BCdload3):
            loadCaseBody(J9BCdload3);
            loadCasePrologue(J9BCdload):
            loadCaseBody(J9BCdload);
            loadCasePrologue(J9BCaload0):
            loadCaseBody(J9BCaload0);
            loadCasePrologue(J9BCaload1):
            loadCaseBody(J9BCaload1);
            loadCasePrologue(J9BCaload2):
            loadCaseBody(J9BCaload2);
            loadCasePrologue(J9BCaload3):
            loadCaseBody(J9BCaload3);
            loadCasePrologue(J9BCaload):
            loadCaseBody(J9BCaload);
            GenericLoadCall:
                if(calledCGSize = generateLoad(buffer, method, bc, bci)){
                    buffer += calledCGSize;
                    codeGenSize += calledCGSize;
                } else {
                    trfprintf(_logFileFP, "Unsupported load bytecode %d\n", bc);
                    return 0;
                }
                break;
            storeCasePrologue(J9BCistore0):
            storeCaseBody(J9BCistore0);
            storeCasePrologue(J9BCistore1):
            storeCaseBody(J9BCistore1);
            storeCasePrologue(J9BCistore2):
            storeCaseBody(J9BCistore2);
            storeCasePrologue(J9BCistore3):
            storeCaseBody(J9BCistore3);
            storeCasePrologue(J9BCistore):
            storeCaseBody(J9BCistore);
            storeCasePrologue(J9BClstore0):
            storeCaseBody(J9BClstore0);
            storeCasePrologue(J9BClstore1):
            storeCaseBody(J9BClstore1);
            storeCasePrologue(J9BClstore2):
            storeCaseBody(J9BClstore2);
            storeCasePrologue(J9BClstore3):
            storeCaseBody(J9BClstore3);
            storeCasePrologue(J9BClstore):
            storeCaseBody(J9BClstore);
            storeCasePrologue(J9BCfstore0):
            storeCaseBody(J9BCfstore0);
            storeCasePrologue(J9BCfstore1):
            storeCaseBody(J9BCfstore1);
            storeCasePrologue(J9BCfstore2):
            storeCaseBody(J9BCfstore2);
            storeCasePrologue(J9BCfstore3):
            storeCaseBody(J9BCfstore3);
            storeCasePrologue(J9BCfstore):
            storeCaseBody(J9BCfstore);
            storeCasePrologue(J9BCdstore0):
            storeCaseBody(J9BCdstore0);
            storeCasePrologue(J9BCdstore1):
            storeCaseBody(J9BCdstore1);
            storeCasePrologue(J9BCdstore2):
            storeCaseBody(J9BCdstore2);
            storeCasePrologue(J9BCdstore3):
            storeCaseBody(J9BCdstore3);
            storeCasePrologue(J9BCdstore):
            storeCaseBody(J9BCdstore);
            storeCasePrologue(J9BCastore0):
            storeCaseBody(J9BCastore0);
            storeCasePrologue(J9BCastore1):
            storeCaseBody(J9BCastore1);
            storeCasePrologue(J9BCastore2):
            storeCaseBody(J9BCastore2);
            storeCasePrologue(J9BCastore3):
            storeCaseBody(J9BCastore3);
            storeCasePrologue(J9BCastore):
            storeCaseBody(J9BCastore);
            GenericStoreCall:
                if(calledCGSize = generateStore(buffer, method, bc, bci)){
                    buffer += calledCGSize;
                    codeGenSize += calledCGSize;
                } else {
                    trfprintf(_logFileFP, "Unsupported store bytecode %d\n", bc);
                    return 0;
                }
                break;
            case TR_J9ByteCode::J9BCgetfield:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCgetfield\n");
                COPY_TEMPLATE(buffer, getFieldTemplatePrologue, codeGenSize);
                patchImm4(buffer, ((U_32)(bci->next2Bytes()))*8);
                COPY_TEMPLATE(buffer, getFieldTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCgetstatic:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCgetstatic\n");
                if(calledCGSize = generateGetStatic(buffer, method, bci)){
                    buffer += calledCGSize;
                    codeGenSize += calledCGSize;
                } else {
                    trfprintf(_logFileFP, "Unsupported getstatic bytecode %d\n", bc);
                    return 0;
                }
                break;
            case TR_J9ByteCode::J9BCputstatic:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCputstatic\n");
                if(calledCGSize = generatePutStatic(buffer, method, bci)){
                    buffer += calledCGSize;
                    codeGenSize += calledCGSize;
                } else {
                    trfprintf(_logFileFP, "Unsupported putstatic bytecode %d\n", bc);
                    return 0;
                }
                break;
            case TR_J9ByteCode::J9BCiadd:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCiadd\n");
                COPY_TEMPLATE(buffer, iAddTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCisub:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCisub\n");
                COPY_TEMPLATE(buffer, iSubTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCimul:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCimul\n");
                COPY_TEMPLATE(buffer, iMulTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCidiv:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCidiv\n");
                COPY_TEMPLATE(buffer, iDivTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCladd:
                trfprintf(_logFileFP, "J9BCladd\n");
                COPY_TEMPLATE(buffer, iAddTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BClsub:
                trfprintf(_logFileFP, "J9BClsub\n");
                COPY_TEMPLATE(buffer, iSubTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BClmul:
                trfprintf(_logFileFP, "J9BClmul\n");
                COPY_TEMPLATE(buffer, iMulTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCgenericReturn:
                MJIT_DEBUG_BC_LOG(_logFileFP, "J9BCgenericReturn\n");
                if(calledCGSize = generateReturn(buffer, method->returnType()))
                {
                    buffer += calledCGSize;
                    codeGenSize += calledCGSize;               
                }
                else if(method->returnType() == TR::Int64)
                {
                    trfprintf(_logFileFP, "Return type : Int64\n"); 
                    buffer_size_t epilogueSize = generateEpologue(buffer);
                    buffer += epilogueSize;
                    codeGenSize += epilogueSize;
                    COPY_TEMPLATE(buffer, iReturnTemplate, codeGenSize);                
                }
                else
                {
                    trfprintf(_logFileFP, "Unknown Return type: %d\n", method->returnType());
                    return 0;                  
                }
                break;
            default:
                trfprintf(_logFileFP, "no match for bytecode %d\n", bc);
                return 0;
        }
    }
    return codeGenSize;
}

buffer_size_t
MJIT::CodeGenerator::generateReturn(char *buffer, TR::DataType dt)
{
    buffer_size_t returnSize = 0;
    buffer_size_t calledCGSize = 0;
    switch(dt){
        case TR::Int32:
        case TR::Int64:
            calledCGSize = generateEpologue(buffer);
            buffer += calledCGSize;
            returnSize += calledCGSize;
            COPY_TEMPLATE(buffer, iReturnTemplate, returnSize); 
            break;
        case TR::NoType:
            calledCGSize = generateEpologue(buffer);
            buffer += calledCGSize;
            returnSize += calledCGSize;
            COPY_TEMPLATE(buffer, vReturnTemplate, returnSize); 
            break;
        case TR::Int8:
        case TR::Int16:
        case TR::Float:
        case TR::Double:
        case TR::Address:
            break;
    }
    return returnSize;
}

buffer_size_t
MJIT::CodeGenerator::generateLoad(char *buffer, TR_ResolvedMethod *method, TR_J9ByteCode bc, TR_J9ByteCodeIterator *bci)
{
    char *signature = method->signatureChars();
    int index = -1;
    buffer_size_t loadSize = 0;
    switch(bc) {
        GenerateTemplate:
            COPY_TEMPLATE(buffer, loadTemplatePrologue, loadSize);
            patchImm4(buffer, (U_32)(index*8));
            COPY_TEMPLATE(buffer, loadTemplate, loadSize);
            break;
        case TR_J9ByteCode::J9BClload0:
        case TR_J9ByteCode::J9BCfload0:
        case TR_J9ByteCode::J9BCdload0:
        case TR_J9ByteCode::J9BCiload0:
        case TR_J9ByteCode::J9BCaload0:
            index = 0;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClload1:
        case TR_J9ByteCode::J9BCfload1:
        case TR_J9ByteCode::J9BCdload1:
        case TR_J9ByteCode::J9BCiload1:
        case TR_J9ByteCode::J9BCaload1:
            index = 1;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClload2:
        case TR_J9ByteCode::J9BCfload2:
        case TR_J9ByteCode::J9BCdload2:
        case TR_J9ByteCode::J9BCiload2:
        case TR_J9ByteCode::J9BCaload2:
            index = 2;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClload3:
        case TR_J9ByteCode::J9BCfload3:
        case TR_J9ByteCode::J9BCdload3:
        case TR_J9ByteCode::J9BCiload3:
        case TR_J9ByteCode::J9BCaload3:
            index = 3;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClload:
        case TR_J9ByteCode::J9BCfload:
        case TR_J9ByteCode::J9BCdload:
        case TR_J9ByteCode::J9BCiload:
        case TR_J9ByteCode::J9BCaload:
            index = bci->nextByte();
            goto GenerateTemplate;
    }
    return loadSize;
}

buffer_size_t
MJIT::CodeGenerator::generateStore(char *buffer, TR_ResolvedMethod *method, TR_J9ByteCode bc, TR_J9ByteCodeIterator *bci)
{
    char *signature = method->signatureChars();
    int index = -1;
    buffer_size_t storeSize = 0;
    switch(bc) {
        GenerateTemplate:
            COPY_TEMPLATE(buffer, storeTemplate, storeSize);
            patchImm4(buffer, (U_32)(index*8));
            break;
        case TR_J9ByteCode::J9BClstore0:
        case TR_J9ByteCode::J9BCfstore0:
        case TR_J9ByteCode::J9BCdstore0:
        case TR_J9ByteCode::J9BCistore0:
        case TR_J9ByteCode::J9BCastore0:
            index = 0;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClstore1:
        case TR_J9ByteCode::J9BCfstore1:
        case TR_J9ByteCode::J9BCdstore1:
        case TR_J9ByteCode::J9BCistore1:
        case TR_J9ByteCode::J9BCastore1:
            index = 1;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClstore2:
        case TR_J9ByteCode::J9BCfstore2:
        case TR_J9ByteCode::J9BCdstore2:
        case TR_J9ByteCode::J9BCistore2:
        case TR_J9ByteCode::J9BCastore2:
            index = 2;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClstore3:
        case TR_J9ByteCode::J9BCfstore3:
        case TR_J9ByteCode::J9BCdstore3:
        case TR_J9ByteCode::J9BCistore3:
        case TR_J9ByteCode::J9BCastore3:
            index = 3;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClstore:
        case TR_J9ByteCode::J9BCfstore:
        case TR_J9ByteCode::J9BCdstore:
        case TR_J9ByteCode::J9BCistore:
        case TR_J9ByteCode::J9BCastore:
            index = bci->nextByte();
            goto GenerateTemplate;
    }
    return storeSize;
}

buffer_size_t
MJIT::CodeGenerator::generateGetStatic(char *buffer, TR_ResolvedMethod *method, TR_J9ByteCodeIterator *bci)
{
    buffer_size_t getStaticSize = 0;
    int32_t cpIndex = (int32_t)bci->next2Bytes();

    //Attempt to resolve the address at compile time. This can fail, and if it does we must fail the compilation for now.
    //In the future research project we could attempt runtime resolution. TR does this with self modifying code.
    void * dataAddress;
    //These are all from TR and likely have implications on how we use this address. However, we do not know that as of yet.
    TR::DataType type = TR::NoType;
    bool isVolatile, isFinal, isPrivate, isUnresolvedInCP;
    bool resolved = method->staticAttributes(_comp, cpIndex, &dataAddress, &type, &isVolatile, &isFinal, &isPrivate, false, &isUnresolvedInCP);
    if(!resolved) return 0;

    //Generate template and instantiate immediate values
    COPY_TEMPLATE(buffer, staticTemplatePrologue, getStaticSize);
    MJIT_ASSERT(_logFileFP, (U_64)dataAddress <= 0x00000000ffffffff, "data is above 4-byte boundary");
    patchImm4(buffer, (U_32)((U_64)dataAddress & 0x00000000ffffffff));
    COPY_TEMPLATE(buffer, getStaticTemplate, getStaticSize);

    return getStaticSize;
}

buffer_size_t
MJIT::CodeGenerator::generatePutStatic(char *buffer, TR_ResolvedMethod *method, TR_J9ByteCodeIterator *bci)
{
    buffer_size_t putStaticSize = 0;
    int32_t cpIndex = (int32_t)bci->next2Bytes();

    //Attempt to resolve the address at compile time. This can fail, and if it does we must fail the compilation for now.
    //In the future research project we could attempt runtime resolution. TR does this with self modifying code.
    void * dataAddress;
    //These are all from TR and likely have implications on how we use this address. However, we do not know that as of yet.
    TR::DataType type = TR::NoType;
    bool isVolatile, isFinal, isPrivate, isUnresolvedInCP;
    bool resolved = method->staticAttributes(_comp, cpIndex, &dataAddress, &type, &isVolatile, &isFinal, &isPrivate, true, &isUnresolvedInCP);
    if(!resolved) return 0;

    //Generate template and instantiate immediate values
    COPY_TEMPLATE(buffer, staticTemplatePrologue, putStaticSize);
    MJIT_ASSERT(_logFileFP, (U_64)dataAddress <= 0x00000000ffffffff, "data is above 4-byte boundary");
    patchImm4(buffer, (U_32)((U_64)dataAddress & 0x00000000ffffffff));
    COPY_TEMPLATE(buffer, putStaticTemplate, putStaticSize);
    
    return putStaticSize;
}

buffer_size_t
MJIT::CodeGenerator::generateColdArea(char *buffer, J9Method *method, char *jitStackOverflowJumpPatchLocation){
    buffer_size_t coldAreaSize = 0;
    PATCH_RELATIVE_ADDR(jitStackOverflowJumpPatchLocation, jbe4ByteRel, (intptr_t)buffer);
    COPY_TEMPLATE(buffer, movEDIImm32, coldAreaSize);
    patchImm4(buffer, _stackPeakSize);

    COPY_TEMPLATE(buffer, call4ByteRel, coldAreaSize);
    PATCH_RELATIVE_ADDR(buffer, call4ByteRel, (intptr_t)jitStackOverflow);

    COPY_TEMPLATE(buffer, jump4ByteRel, coldAreaSize);
    PATCH_RELATIVE_ADDR(buffer, jump4ByteRel, jitStackOverflowJumpPatchLocation);

    return coldAreaSize;
}

buffer_size_t
MJIT::CodeGenerator::generateDebugBreakpoint(char *buffer) {
    buffer_size_t codeGenSize = 0;
    COPY_TEMPLATE(buffer, debugBreakpoint, codeGenSize);
    return codeGenSize;
}

TR::CodeCache*
MJIT::CodeGenerator::getCodeCache(){ 
    return _codeCache; 
}

U_8 *
MJIT::CodeGenerator::allocateCodeCache(int32_t length, TR_J9VMBase *vmBase, J9VMThread *vmThread)
   {
   TR::CodeCacheManager *manager = TR::CodeCacheManager::instance();
   int32_t compThreadID = vmBase->getCompThreadIDForVMThread(vmThread);
   int32_t numReserved;
   
   if(!_codeCache) 
      {
      _codeCache = manager->reserveCodeCache(false, length, compThreadID, &numReserved);
      if (!_codeCache)
         {
         return NULL;
         }
      } 
    uint8_t *coldCode;
    U_8 *codeStart = manager->allocateCodeMemory(length, 0, &_codeCache, &coldCode, false);
    if(!codeStart)
        {
            return NULL;
        }

    return codeStart;
   
   }
