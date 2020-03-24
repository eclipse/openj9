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
DECLARE_TEMPLATE(callByteRel);
DECLARE_TEMPLATE(call4ByteRel);
DECLARE_TEMPLATE(jump4ByteRel);
DECLARE_TEMPLATE(nopInstruction);
DECLARE_TEMPLATE(movRDIImm64);
DECLARE_TEMPLATE(movEDIImm32);
DECLARE_TEMPLATE(movRAXImm64);
DECLARE_TEMPLATE(movEAXImm32);
DECLARE_TEMPLATE(jumpRDI);
DECLARE_TEMPLATE(jumpRAX);

// bytecodes
DECLARE_TEMPLATE(debugBreakpoint);
DECLARE_TEMPLATE(loadTemplatePrologue);
DECLARE_TEMPLATE(vLoadTemplate); // Load a value
DECLARE_TEMPLATE(vStoreTemplatePrologue); // Store a value
DECLARE_TEMPLATE(storeTemplate);
DECLARE_TEMPLATE(iAddTemplate);
DECLARE_TEMPLATE(iSubTemplate);
DECLARE_TEMPLATE(iMulTemplate);
DECLARE_TEMPLATE(iDivTemplate);
DECLARE_TEMPLATE(iReturnTemplate);

static void 
debug_print_hex(char* buffer, unsigned long long size)
{
    printf("Start of dump:\n");
    while(size--){
        printf("%x ", 0xc0 & *(buffer++));
    }
    printf("\nEnd of dump:");
}

inline uint32_t align(uint32_t number, uint32_t requirement, TR::FilePointer* fp)
{
    MJIT_ASSERT(fp, requirement && ((requirement & (requirement -1)) == 0), "INCORRECT ALIGNMENT");
    return (number + requirement - 1) & ~(requirement - 1);
}

static bool 
getRequiredAlignment(uintptr_t cursor, uintptr_t boundary, uintptr_t margin, uintptr_t* alignment)
{
    if((boundary & (boundary-1)) != 0)
        return true;
    *alignment = (-cursor - margin) & (boundary-1);
    return false;
}

bool
MJIT::nativeSignature(J9Method* method, char *resultBuffer)
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
			case '(':										/* start of signature -- skip */
				continue;
			case ')':										/* End of signature -- done args, find return type */
				parsingReturnType = TRUE;
				continue;
			case MJIT::CLASSNAME_TYPE_CHARACTER:
				nextType = MJIT::CLASSNAME_TYPE_CHARACTER;
				while(J9UTF8_DATA(methodSig)[i++] != ';') {}		/* a type string - loop scanning for ';' to end it - i points past ';' when done loop */
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
				continue;				/* go back to top of loop for next char */
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
				break;			/* from the while loop */
			} else {
				resultBuffer[arg] = MJIT::CLASSNAME_TYPE_CHARACTER;
				arg++;
				processingBracket = FALSE;
			}
		} else if(parsingReturnType) {
			resultBuffer[0] = nextType;
			break;			/* from the while loop */
		} else {
			resultBuffer[arg] = nextType;
			arg++;
		}
	}

	resultBuffer[1] = MJIT::CLASSNAME_TYPE_CHARACTER; 	/* the JNIEnv */
	resultBuffer[2] = MJIT::CLASSNAME_TYPE_CHARACTER;	/* the jobject or jclass */
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
patchImm8(char* buffer, U_64 imm){
    *(((unsigned long long int*)buffer)-1) = imm;
}

/**
 * Assuming that 'buffer' is the end of the immediate value location,
 * this function replaces the value supplied value 'imm'
 * 
 * Assumes 32-bit value
 */
static void
patchImm4(char* buffer, U_32 imm){
    *(((unsigned int*)buffer)-1) = imm;
}

/**
 * Assuming that 'buffer' is the end of the immediate value location,
 * this function replaces the value supplied value 'imm'
 * 
 * Assumes 16-bit value
 */
static void
patchImm2(char* buffer, U_16 imm){
    *(((unsigned short int*)buffer)-1) = imm;
}

/**
 * Assuming that 'buffer' is the end of the immediate value location,
 * this function replaces the value supplied value 'imm'
 * 
 * Assumes 8-bit value
 */
static void
patchImm1(char* buffer, U_8 imm){
    *(unsigned char*)(buffer-1) = imm;
}

/*
| Architecture 	| Endian | Return address | vTable index |
| x86-64 		| Little | Stack 		  | R8 (receiver in RAX)

| Integer Return value registers    | Integer Preserved registers   | Integer Argument registers
| EAX (32-bit) RAX (64-bit)         | RBX R9                        | RAX RSI RDX RCX 

| Float Return value registers  | Float Preserved registers | Float Argument registers
| XMM0                          | XMM8-XMM15        		| XMM0-XMM7 
*/

inline buffer_size_t
savePreserveRegisters(char* buffer, int32_t offset)
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
loadPreserveRegisters(char* buffer, int32_t offset)
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

#undef MJIT_DEBUG_MAP_PARAMS
MJIT::RegisterStack 
MJIT::mapIncomingParams(char* typeString, U_16 maxLength, int* error_code, MJIT::ParamTableEntry* paramTable, U_16 paramCount)
{
#ifdef MJIT_DEBUG_MAP_PARAMS
    trfprintf(_logFileFP, "MapIncomingParams Start:\n");
#endif
    MJIT::RegisterStack stack;
    initParamStack(&stack);


    intptr_t offset = 0;

#ifdef MJIT_DEBUG_MAP_PARAMS
    trfprintf(_logFileFP, "  maxLength: %04x\n", maxLength);
    trfprintf(_logFileFP, "  MapIncomingParamsLoop Start:\n");
#endif
    for(int i = 0; i<paramCount; i++){
#ifdef MJIT_DEBUG_MAP_PARAMS
        trfprintf(_logFileFP, "    i: %04x\n", i);
#endif
        //3rd index of typeString is first param.
        char typeChar = typeString[i+3];
        U_16 size = MJIT::typeSignatureSize(typeChar);
#ifdef MJIT_DEBUG_MAP_PARAMS
        trfprintf(_logFileFP, "    ParamTableSize: %04x\n", size);
#endif
        int regNo = RealRegister::NoReg;
        switch(typeChar){
        case MJIT::BYTE_TYPE_CHARACTER:
        case MJIT::CHAR_TYPE_CHARACTER:
        case MJIT::INT_TYPE_CHARACTER:
        case MJIT::CLASSNAME_TYPE_CHARACTER:
        case MJIT::SHORT_TYPE_CHARACTER:
        case MJIT::BOOLEAN_TYPE_CHARACTER:
        case MJIT::LONG_TYPE_CHARACTER:
            regNo = MJIT::addParamIntToStack(&stack, size);
            goto makeParamTableEntry;
        case MJIT::DOUBLE_TYPE_CHARACTER:
        case MJIT::FLOAT_TYPE_CHARACTER:
            regNo = MJIT::addParamFloatToStack(&stack, size);
            goto makeParamTableEntry;
        makeParamTableEntry:
            paramTable[i] = (regNo != RealRegister::NoReg) ? //If this is a register
                makeRegisterEntry(regNo, size): //Add a register entry
                makeStackEntry((stack.stackSlotsUsed-1)*8, size); //else it's a stack entry
                //first stack entry (1 used) should be at offset 0.
            break;
        default:
            *error_code = 2;
            return stack;
        }
#ifdef MJIT_DEBUG_MAP_PARAMS
        trfprintf(_logFileFP, "    Table:\n");
        trfprintf(_logFileFP, "%01x", stack.useRAX);
        trfprintf(_logFileFP, "%01x", stack.useRSI);
        trfprintf(_logFileFP, "%01x", stack.useRDX);
        trfprintf(_logFileFP, "%01x", stack.useRCX);
        trfprintf(_logFileFP, "%01x", stack.useXMM0);
        trfprintf(_logFileFP, "%01x", stack.useXMM1);
        trfprintf(_logFileFP, "%01x", stack.useXMM2);
        trfprintf(_logFileFP, "%01x", stack.useXMM3);
        trfprintf(_logFileFP, "%01x", stack.useXMM4);
        trfprintf(_logFileFP, "%01x", stack.useXMM5);
        trfprintf(_logFileFP, "%01x", stack.useXMM6);
        trfprintf(_logFileFP, "%01x", stack.useXMM7);
        trfprintf(_logFileFP, "%02x", stack.stackSlotsUsed);
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


MJIT::ParamTable::ParamTable(ParamTableEntry* tableEntries, U_16 paramCount, RegisterStack* registerStack)
    : _tableEntries(tableEntries)
    , _paramCount(paramCount)
    , _registerStack(registerStack)
{}

MJIT::ParamTableEntry
MJIT::ParamTable::getEntry(U_16 paramIndex)
{
    return _tableEntries[paramIndex];
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

buffer_size_t
MJIT::CodeGenerator::saveArgsInLocalArray(
    char* buffer
){
    buffer_size_t saveSize = 0;
    U_16 slot = 0;
    RealRegister::RegNum regNum = RealRegister::NoReg;
    for(int i=0; i<_paramTable->getParamCount(); i++){
        ParamTableEntry entry = _paramTable->getEntry(i);
        if(entry.onStack)
            break;
        int regNum = (int)entry.address;
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
    //TODO Copy values from the stack to the local array here.
    return saveSize;
}

buffer_size_t 
MJIT::CodeGenerator::saveArgsOnStack(
    char* buffer, 
    buffer_size_t stack_alloc_space
){
    buffer_size_t saveArgsSize = 0;
    U_16 offset = _paramTable->getTotalParamSize();
    RealRegister::RegNum regNum = RealRegister::NoReg;
    for(int i=0; i<_paramTable->getParamCount(); i++){
        ParamTableEntry entry = _paramTable->getEntry(i);
        if(entry.onStack)
            break;
        int regNum = (int)entry.address;
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
    char* buffer, 
    buffer_size_t stack_alloc_space
){
    U_16 offset = _paramTable->getTotalParamSize();
    buffer_size_t argLoadSize = 0;
    RealRegister::RegNum regNum = RealRegister::NoReg;
    for(int i=0; i<_paramTable->getParamCount(); i++){
        ParamTableEntry entry = _paramTable->getEntry(i);
        if(entry.onStack)
            break;
        int regNum = (int)entry.address;
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

MJIT::CodeGenerator::CodeGenerator(J9MicroJITConfig* config, J9VMThread* thread, TR::FilePointer* fp, TR_J9VMBase& vm, ParamTable* paramTable)
    :_linkage(Linkage(config, thread, fp))
    ,_logFileFP(fp)
    ,_vm(vm)
    ,_codeCache(NULL)
    ,_stackPeakSize(0)
    ,_paramTable(paramTable)
{
    _linkage._properties.setOutgoingArgAlignment(lcm(16, _vm.getLocalObjectAlignmentInBytes()));
}

buffer_size_t
MJIT::CodeGenerator::generateSwitchToInterpPrePrologue(
    char* buffer,
    J9Method* method,
    buffer_size_t boundary,
    buffer_size_t margin,
    char* typeString,
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
    char* buffer, 
    J9Method* method, 
    char** magicWordLocation,
    char** first2BytesPatchLocation,
    TR_PersistentJittedBodyInfo** bodyInfo
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
        char* old_buff = buffer;
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

/**
 * Write the prologue to the buffer and return the size written to the buffer.
 */
buffer_size_t
MJIT::CodeGenerator::generatePrologue(
    char* buffer, 
    J9Method* method, 
    char** jitStackOverflowJumpPatchLocation,
    char* magicWordLocation,
    char* first2BytesPatchLocation,
    char** firstInstLocation
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
    const int32_t localSize = (romMethod->argCount + romMethod->tempCount)*8;
    const int32_t pointerSize = properties.getPointerSize();
    MJIT_ASSERT(_logFileFP, localSize >= 0, "assertion failure");
    int32_t frameSize = localSize + preservedRegsSize + ( _linkage._properties.getReservesOutgoingArgsInPrologue() ? properties.getPointerSize() : 0 );
    uint32_t stackSize = frameSize + properties.getRetAddressWidth();
    uint32_t adjust = align(stackSize, properties.getOutgoingArgAlignment(), _logFileFP) - stackSize;
    auto allocSize = frameSize + adjust;

    //return address is allocated by call instruction
    // Here we conservatively assume there is a call in this method that will require space for its return address
    _stackPeakSize = 
        allocSize + //space for stack frame
        properties.getPointerSize() + //space for return address
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
    TR::GCStackAtlas *atlas = cg()->getStackAtlas();

    if (atlas) {
        uint32_t  numberOfParmSlots = atlas->getNumberOfParmSlotsMapped();
        TR_GCStackMap *map;
        if (_properties.getNumIntegerArgumentRegisters() == 0) {
            map = atlas->getParameterMap();
        } else {
            map = new (trHeapMemory(), numberOfParmSlots) TR_GCStackMap(numberOfParmSlots);
            map->copy(atlas->getParameterMap());

            // Before this point, the parameter stack considers all parms to be on the stack.
            // Fix it to have register parameters in registers.
            TR::ParameterSymbol *paramCursor = paramIterator.getFirst();

            for (paramCursor = paramIterator.getFirst();
                paramCursor != NULL;
                paramCursor = paramIterator.getNext()
            ){
                int32_t  intRegArgIndex = paramCursor->getLinkageRegisterIndex();
                if (intRegArgIndex >= 0                   &&
                    paramCursor->isReferencedParameter()  &&
                    paramCursor->isCollectedReference()) {
                    // In FSD, the register parameters have already been backspilled.
                    // They exist in both registers and on the stack.
                    //
                    if (!parmsHaveBeenBackSpilled)
                        map->resetBit(paramCursor->getGCMapIndex());

                    map->setRegisterBits(TR::RealRegister::gprMask((getProperties().getIntegerArgumentRegister(intRegArgIndex))));
                }
            }
        }

        if (jitOverflowCheck)
            jitOverflowCheck->setGCMap(map);

        atlas->setParameterMap(map);
    }

    //Support to paint allocated frame slots.
    if (( comp()->getOption(TR_PaintAllocatedFrameSlotsDead) || comp()->getOption(TR_PaintAllocatedFrameSlotsFauxObject) )  && allocSize!=0) {
        uint32_t paintValue32 = 0;
        uint64_t paintValue64 = 0;

        TR::RealRegister *paintReg = NULL;
        TR::RealRegister *frameSlotIndexReg = machine()->getRealRegister(TR::RealRegister::edi);
        uint32_t paintBound = 0;
        uint32_t paintSlotsOffset = 0;
        uint32_t paintSize = allocSize-sizeof(uintptrj_t);

        //Paint the slots with deadf00d
        if (comp()->getOption(TR_PaintAllocatedFrameSlotsDead)) {
            if (TR::Compiler->target.is64Bit())
                paintValue64 = (uint64_t)CONSTANT64(0xdeadf00ddeadf00d);
            else
                paintValue32 = 0xdeadf00d;
        } else { //Paint stack slots with a arbitrary object aligned address.
            if (TR::Compiler->target.is64Bit()) {
                paintValue64 = ((uintptrj_t) ((uintptrj_t)comp()->getOptions()->getHeapBase() + (uintptrj_t) 4096));
            } else {
                paintValue32 = ((uintptrj_t) ((uintptrj_t)comp()->getOptions()->getHeapBase() + (uintptrj_t) 4096));
            }
        }

        TR::LabelSymbol   *startLabel = generateLabelSymbol(cg());

        //Load the 64 bit paint value into a paint reg.
        paintReg = machine()->getRealRegister(TR::RealRegister::r8);
        cursor = new (trHeapMemory()) TR::AMD64RegImm64Instruction(cursor, MOV8RegImm64, paintReg, paintValue64, cg());


        //Perform the paint.
        cursor = new (trHeapMemory()) TR::X86RegImmInstruction(cursor, MOVRegImm4(), frameSlotIndexReg, paintSize, cg());
        cursor = new (trHeapMemory()) TR::X86LabelInstruction(cursor, LABEL, startLabel, cg());
        if (TR::Compiler->target.is64Bit())
            cursor = new (trHeapMemory()) TR::X86MemRegInstruction(cursor, S8MemReg, generateX86MemoryReference(espReal, frameSlotIndexReg, 0,(uint8_t) paintSlotsOffset, cg()), paintReg, cg());
        else
            cursor = new (trHeapMemory()) TR::X86MemImmInstruction(cursor, SMemImm4(), generateX86MemoryReference(espReal, frameSlotIndexReg, 0,(uint8_t) paintSlotsOffset, cg()), paintValue32, cg());
        cursor = new (trHeapMemory()) TR::X86RegImmInstruction(cursor, SUBRegImms(), frameSlotIndexReg, sizeof(intptr_t),cg());
        cursor = new (trHeapMemory()) TR::X86RegImmInstruction(cursor, CMPRegImm4(), frameSlotIndexReg, paintBound, cg());
        cursor = new (trHeapMemory()) TR::X86LabelInstruction(cursor, JGE4, startLabel,cg());
    }

    // Save preserved regs
    cursor = savePreservedRegisters(cursor);

    // Insert some counters
    cursor = cg()->generateDebugCounter(cursor, "cg.prologues:#preserved", preservedRegsSize >> getProperties().getParmSlotShift(), TR::DebugCounter::Expensive);
    cursor = cg()->generateDebugCounter(cursor, "cg.prologues:inline", 1, TR::DebugCounter::Expensive);

    // Initialize any local pointers that could otherwise confuse the GC.
    TR::RealRegister *framePointer = machine()->getRealRegister(TR::RealRegister::vfp);
    if (atlas) {
        TR_ASSERT(_properties.getNumScratchRegisters() >= 2, "Need two scratch registers to initialize reference locals");
        TR::RealRegister *loopReg = machine()->getRealRegister(properties.getIntegerScratchRegister(1));

        int32_t numReferenceLocalSlotsToInitialize = atlas->getNumberOfSlotsToBeInitialized();
        int32_t numInternalPointerSlotsToInitialize = 0;

        if (atlas->getInternalPointerMap()) {
            numInternalPointerSlotsToInitialize = atlas->getNumberOfDistinctPinningArrays() +
                                                  atlas->getInternalPointerMap()->getNumInternalPointers();
        }

        if (numReferenceLocalSlotsToInitialize > 0 || numInternalPointerSlotsToInitialize > 0) {
            cursor = new (trHeapMemory()) TR::X86RegRegInstruction(cursor, XORRegReg(), scratchReg, scratchReg, cg());

            // Initialize locals that are live on entry
            if (numReferenceLocalSlotsToInitialize > 0) {
                cursor = initializeLocals(cursor,
                                          atlas->getLocalBaseOffset(),
                                          numReferenceLocalSlotsToInitialize,
                                          properties.getPointerSize(),
                                          framePointer, 
                                          scratchReg, 
                                          loopReg,
                                          cg());
            }

            // Initialize internal pointers and their pinning arrays
            if (numInternalPointerSlotsToInitialize > 0){
                cursor = initializeLocals(cursor,
                                          atlas->getOffsetOfFirstInternalPointer(),
                                          numInternalPointerSlotsToInitialize,
                                          properties.getPointerSize(),
                                          framePointer, 
                                          scratchReg, 
                                          loopReg,
                                          cg());
            }
        }
    }
*/
    //Set up MJIT value stack
    COPY_TEMPLATE(buffer, movRSPR10, prologueSize);
    COPY_TEMPLATE(buffer, addR10Imm4, prologueSize);
    patchImm4(buffer, (U_32)(romMethod->maxStack*8)); 
    //TODO: Find a way to cleanly preserve this value before overriding it in the _stackAllocSize

    //Set up MJIT local array
    COPY_TEMPLATE(buffer, movR10R14, prologueSize);

    // Move parameters to where the method body will expect to find them
    buffer_size_t saveSize = saveArgsInLocalArray(buffer);
    prologueSize += saveSize;
    if (!saveSize) return 0;
    
    return prologueSize;
}

buffer_size_t
MJIT::CodeGenerator::generateEpologue(char* buffer){
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
MJIT::CodeGenerator::generateBody(char* buffer, TR_ResolvedMethod* method, TR_J9ByteCodeIterator* bci){
    buffer_size_t codeGenSize = 0;
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
            GenericLoadCall:
                if(buffer_size_t loadSize = generateLoad(buffer, method, bc, bci)){
                    buffer += loadSize;
                    codeGenSize += loadSize;
                } else {
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
            GenericStoreCall:
                if(buffer_size_t loadSize = generateStore(buffer, method, bc, bci)){
                    buffer += loadSize;
                    codeGenSize += loadSize;
                } else {
                    return 0;
                }
                break;
            case TR_J9ByteCode::J9BCiadd:
                trfprintf(_logFileFP, "J9BCiadd\n");
                COPY_TEMPLATE(buffer, iAddTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCisub:
                trfprintf(_logFileFP, "J9BCisub\n");
                COPY_TEMPLATE(buffer, iSubTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCimul:
                trfprintf(_logFileFP, "J9BCimul\n");
                COPY_TEMPLATE(buffer, iMulTemplate, codeGenSize);
                break;
            case TR_J9ByteCode::J9BCidiv:
                trfprintf(_logFileFP, "J9BCidiv\n");
                COPY_TEMPLATE(buffer, iDivTemplate, codeGenSize);
                break;    
            case TR_J9ByteCode::J9BCgenericReturn:
                trfprintf(_logFileFP, "J9BCgenericReturn\n");
                if(method->returnType() == TR::Int32)
                {
                    trfprintf(_logFileFP, "Return type : Int32\n"); 
                    buffer_size_t epilogueSize = generateEpologue(buffer);
                    buffer += epilogueSize;
                    codeGenSize += epilogueSize;
                    COPY_TEMPLATE(buffer, iReturnTemplate, codeGenSize);                
                }
                else
                {
                    trfprintf(_logFileFP, "Unknown Return type: %d\n", method->returnType());                    
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
MJIT::CodeGenerator::generateLoad(char* buffer, TR_ResolvedMethod* method, TR_J9ByteCode bc, TR_J9ByteCodeIterator* bci)
{
    char* signature = method->signatureChars();
    int index = -1;
    buffer_size_t loadSize = 0;
    switch(bc) {
        GenerateTemplate:
            COPY_TEMPLATE(buffer, loadTemplatePrologue, loadSize);
            patchImm4(buffer, (U_32)(index*8));
            COPY_TEMPLATE(buffer, vLoadTemplate, loadSize);
            break;
		case TR_J9ByteCode::J9BClload0:
		case TR_J9ByteCode::J9BCfload0:
		case TR_J9ByteCode::J9BCdload0:
        case TR_J9ByteCode::J9BCiload0:
            index = 0;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClload1:
        case TR_J9ByteCode::J9BCfload1:
        case TR_J9ByteCode::J9BCdload1:
        case TR_J9ByteCode::J9BCiload1:
            index = 1;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClload2:
        case TR_J9ByteCode::J9BCfload2:
        case TR_J9ByteCode::J9BCdload2:
        case TR_J9ByteCode::J9BCiload2:
            index = 2;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClload3:
        case TR_J9ByteCode::J9BCfload3:
        case TR_J9ByteCode::J9BCdload3:
        case TR_J9ByteCode::J9BCiload3:
            index = 3;
            goto GenerateTemplate;
		case TR_J9ByteCode::J9BClload:
		case TR_J9ByteCode::J9BCfload:
		case TR_J9ByteCode::J9BCdload:
		case TR_J9ByteCode::J9BCiload:
            index = bci->nextByte();
            goto GenerateTemplate;
		//case TR_J9ByteCode::ALOAD
		//case TR_J9ByteCode::ALOAD_0
		//case TR_J9ByteCode::ALOAD_1
		//case TR_J9ByteCode::ALOAD_2
		//case TR_J9ByteCode::ALOAD_3
    }
    return loadSize;
}

buffer_size_t
MJIT::CodeGenerator::generateStore(char* buffer, TR_ResolvedMethod* method, TR_J9ByteCode bc, TR_J9ByteCodeIterator* bci)
{
    char* signature = method->signatureChars();
    int index = -1;
    buffer_size_t storeSize = 0;
    switch(bc) {
        GenerateTemplate:
            COPY_TEMPLATE(buffer, vStoreTemplatePrologue, storeSize);
            COPY_TEMPLATE(buffer, storeTemplate, storeSize);
            patchImm4(buffer, (U_32)(index*8));
            break;
		case TR_J9ByteCode::J9BClstore0:
		case TR_J9ByteCode::J9BCfstore0:
		case TR_J9ByteCode::J9BCdstore0:
        case TR_J9ByteCode::J9BCistore0:
            index = 0;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClstore1:
        case TR_J9ByteCode::J9BCfstore1:
        case TR_J9ByteCode::J9BCdstore1:
        case TR_J9ByteCode::J9BCistore1:
            index = 1;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClstore2:
        case TR_J9ByteCode::J9BCfstore2:
        case TR_J9ByteCode::J9BCdstore2:
        case TR_J9ByteCode::J9BCistore2:
            index = 2;
            goto GenerateTemplate;
        case TR_J9ByteCode::J9BClstore3:
        case TR_J9ByteCode::J9BCfstore3:
        case TR_J9ByteCode::J9BCdstore3:
        case TR_J9ByteCode::J9BCistore3:
            index = 3;
            goto GenerateTemplate;
		case TR_J9ByteCode::J9BClstore:
		case TR_J9ByteCode::J9BCfstore:
		case TR_J9ByteCode::J9BCdstore:
		case TR_J9ByteCode::J9BCistore:
            index = bci->nextByte();
            goto GenerateTemplate;
		//case TR_J9ByteCode::ALOAD
		//case TR_J9ByteCode::ALOAD_0
		//case TR_J9ByteCode::ALOAD_1
		//case TR_J9ByteCode::ALOAD_2
		//case TR_J9ByteCode::ALOAD_3
    }
    return storeSize;
}

buffer_size_t
MJIT::CodeGenerator::generateColdArea(char* buffer, J9Method* method, char* jitStackOverflowJumpPatchLocation){
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
MJIT::CodeGenerator::generateDebugBreakpoint(char* buffer) {
    buffer_size_t codeGenSize = 0;
    COPY_TEMPLATE(buffer, debugBreakpoint, codeGenSize);
    return codeGenSize;
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

   
