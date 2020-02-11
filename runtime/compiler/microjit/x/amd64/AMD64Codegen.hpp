#ifndef AMD64CODEGEN_HPP
#define AMD64CODEGEN_HPP

#include <cstdint>
#include "AMD64Linkage.hpp"
#include "microjit/utils.hpp"
#include "oti/j9generated.h"
#include "control/RecompilationInfo.hpp"
#include "ilgen/J9ByteCodeIterator.hpp"
#include "env/IO.hpp"

typedef unsigned int buffer_size_t;
namespace MJIT {

enum TypeCharacters {
    BYTE_TYPE_CHARACTER = 'B',
    CHAR_TYPE_CHARACTER = 'C',
    DOUBLE_TYPE_CHARACTER = 'D',
    FLOAT_TYPE_CHARACTER = 'F',
    INT_TYPE_CHARACTER = 'I',
    LONG_TYPE_CHARACTER = 'J',
    CLASSNAME_TYPE_CHARACTER = 'L',
    SHORT_TYPE_CHARACTER = 'S',
    BOOLEAN_TYPE_CHARACTER = 'Z',
    VOID_TYPE_CHARACTER = 'V',
};

inline static U_16
typeSignatureSize(char typeBuffer)
{
    switch(typeBuffer){
        case MJIT::BYTE_TYPE_CHARACTER:
        case MJIT::CHAR_TYPE_CHARACTER:
        case MJIT::INT_TYPE_CHARACTER:
        case MJIT::FLOAT_TYPE_CHARACTER:
        case MJIT::CLASSNAME_TYPE_CHARACTER:
        case MJIT::SHORT_TYPE_CHARACTER:
        case MJIT::BOOLEAN_TYPE_CHARACTER:
            return 8;
        case MJIT::DOUBLE_TYPE_CHARACTER:
        case MJIT::LONG_TYPE_CHARACTER:
            return 16;
        default:
            return 0;
    }
}

/*
| Architecture 	| Endian | Return address | vTable index |
| x86-64 		| Little | Stack 		  | R8 (receiver in RAX)

| Integer Return value registers    | Integer Preserved registers   | Integer Argument registers
| EAX (32-bit) RAX (64-bit)         | RBX R9-R15                    | RAX RSI RDX RCX 

| Float Return value registers  | Float Preserved registers | Float Argument registers
| XMM0                          | XMM8-XMM15        		| XMM0-XMM7 
*/

struct ParamTable {
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
calculateOffset(ParamTable* table)
{
    return  8*(table->useRAX  +
               table->useRSI  +
               table->useRDX  +
               table->useRCX  +
               table->useXMM0 +
               table->useXMM1 +
               table->useXMM2 +
               table->useXMM3 +
               table->useXMM4 +
               table->useXMM5 +
               table->useXMM6 +
               table->useXMM7 +
               table->stackSlotsUsed);
}

inline void
initParamTable(ParamTable* table)
{
    table->useRAX         = 0;
    table->useRSI         = 0;
    table->useRDX         = 0;
    table->useRCX         = 0;
    table->useXMM0        = 0;
    table->useXMM1        = 0;
    table->useXMM2        = 0;
    table->useXMM3        = 0;
    table->useXMM4        = 0;
    table->useXMM5        = 0;
    table->useXMM6        = 0;
    table->useXMM7        = 0;
    table->stackSlotsUsed = 0;
}

inline void
addParamInt(ParamTable* table, U_16 size)
{
    if(!table->useRAX){
        table->useRAX = size/8;
    } else if(!table->useRSI){
        table->useRSI = size/8;
    } else if(!table->useRDX){
        table->useRDX = size/8;
    } else if(!table->useRCX){
        table->useRCX = size/8;
    } else {
        table->stackSlotsUsed += size/8;
    }
}

inline void
addParamFloat(ParamTable* table, U_16 size)
{
    if(!table->useXMM0){
        table->useXMM0 = size/8;
    } else if(!table->useXMM1){
        table->useXMM1 = size/8;
    } else if(!table->useXMM2){
        table->useXMM2 = size/8;
    } else if(!table->useXMM3){
        table->useXMM3 = size/8;
    } else if(!table->useXMM4){
        table->useXMM4 = size/8;
    } else if(!table->useXMM5){
        table->useXMM5 = size/8;
    } else if(!table->useXMM6){
        table->useXMM6 = size/8;
    } else if(!table->useXMM7){
        table->useXMM7 = size/8;
    } else {
        table->stackSlotsUsed += size/8;
    }
}

inline int
removeParamInt(ParamTable* table, U_16 *size)
{
    if(table->useRAX){
        *size = table->useRAX*8;
        table->useRAX = 0;
        return RealRegister::eax;
    } else if(table->useRSI){
        *size = table->useRSI*8;
        table->useRSI = 0;
        return RealRegister::esi;
    } else if(table->useRDX){
        *size = table->useRDX*8;
        table->useRDX = 0;
        return RealRegister::edx;
    } else if(table->useRCX){
        *size = table->useRCX*8;
        table->useRCX = 0;
        return RealRegister::ecx;
    } else {
        *size = 0;
        return RealRegister::NoReg;
    }
}

inline int
removeParamFloat(ParamTable* table, U_16 *size)
{
    if(table->useXMM0){
        *size = table->useXMM0*8;
        table->useXMM0 = 0;
        return RealRegister::xmm0;
    } else if(table->useXMM1){
        *size = table->useXMM1*8;
        table->useXMM1 = 0;
        return RealRegister::xmm1;
    } else if(table->useXMM2){
        *size = table->useXMM2*8;
        table->useXMM2 = 0;
        return RealRegister::xmm2;
    } else if(table->useXMM3){
        *size = table->useXMM3*8;
        table->useXMM3 = 0;
        return RealRegister::xmm3;
    } else if(table->useXMM4){
        *size = table->useXMM4*8;
        table->useXMM4 = 0;
        return RealRegister::xmm4;
    } else if(table->useXMM5){
        *size = table->useXMM5*8;
        table->useXMM5 = 0;
        return RealRegister::xmm5;
    } else if(table->useXMM6){
        *size = table->useXMM6*8;
        table->useXMM6 = 0;
        return RealRegister::xmm6;
    } else if(table->useXMM7){
        *size = table->useXMM7*8;
        table->useXMM7 = 0;
        return RealRegister::xmm7;
    } else {
        *size = 0;
        return RealRegister::NoReg;
    }
}

class CodeGenerator {
    private:
        Linkage _linkage;
        TR::FilePointer *_logFileFP;
        TR_J9VMBase& _vm;
        TR::CodeCache *_codeCache;
        ParamTable mapIncomingParms(char*, U_16, int*);
        buffer_size_t generateSwitchToInterpPrePrologue(char*, J9Method*, buffer_size_t, buffer_size_t, char*, U_16);
        buffer_size_t generateEpologue(char*, int32_t);
        buffer_size_t loadArgsFromStack(ParamTable*, char*, buffer_size_t, char*);
        buffer_size_t saveArgsOnStack(ParamTable*, char*, buffer_size_t, char*);
    public:
        CodeGenerator() = delete;
        CodeGenerator(J9MicroJITConfig*, J9VMThread*, TR::FilePointer*, TR_J9VMBase&);

        buffer_size_t generatePrePrologue(char*, J9Method*, char**, char**, TR_PersistentJittedBodyInfo**);
        buffer_size_t generatePrologue(char*, J9Method*, char**, int32_t*, char*, char*, char**);
        buffer_size_t generateColdArea(char*, J9Method*, char*, int32_t);
        /**
         * Generates the body for a method.
         * 
         * @param buffer         code buffer
         * @param bci            byte code iterator
         * @param method         method for introspection
         * @return               size of generated code; 0 if method failed
         */
        buffer_size_t generateBody(char* buffer, TR_ResolvedMethod* method, TR_J9ByteCodeIterator* bci, int32_t);

        /**
         * Generate an int3 for signaling debug breakpoint for Linux x86
         * @param buffer    code buffer
         */
        buffer_size_t generateDebugBreakpoint(char* buffer);
        
        /**
         * Allocate space in the code cache of size length and 
         * copy the buffer to the cache.
         * 
         * @return pointer to the new code cache segment or NULL if failed.
         * 
         * @param buffer    code buffer
         * @param length    length of code in the buffer
         * @param vmBase    To check if compilation should be interrupted
         * @param vmThread  Thread Id is stored in cache
         */
        U_8 * allocateCodeCache(int32_t length, TR_J9VMBase *vmBase, J9VMThread *vmThread);
};

} //namespace MJIT

#endif
