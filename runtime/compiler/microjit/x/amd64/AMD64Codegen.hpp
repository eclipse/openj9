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
| EAX (32-bit) RAX (64-bit)         | RBX R9                        | RAX RSI RDX RCX 

| Float Return value registers  | Float Preserved registers | Float Argument registers
| XMM0                          | XMM8-XMM15        		| XMM0-XMM7 
*/

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
calculateOffset(RegisterStack* stack)
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
initParamStack(RegisterStack* stack)
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
addParamIntToStack(RegisterStack* stack, U_16 size)
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
addParamFloatToStack(RegisterStack* stack, U_16 size)
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
removeParamIntFromStack(RegisterStack* stack, U_16 *size)
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
removeParamFloatFromStack(RegisterStack* stack, U_16 *size)
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
    //Stored here so we can look up when saving to local array
    char slots;
    bool isReference;
    bool onStack;
};

inline ParamTableEntry
makeRegisterEntry(int32_t regNo, int32_t stackOffset, U_16 size, bool isRef){
    ParamTableEntry entry;
    entry.regNo = regNo;
    entry.offset = stackOffset;
    entry.onStack = false;
    entry.slots = size/8;
    entry.isReference = isRef;
    return entry;
}

inline ParamTableEntry
makeStackEntry(int32_t stackOffset, U_16 size, bool isRef){
    ParamTableEntry entry;
    entry.regNo = MJIT::RealRegister::NoReg;
    entry.offset = stackOffset;
    entry.onStack = true;
    entry.slots = size/8;
    entry.isReference = isRef;
    return entry;
}

class ParamTable
{
    private:
        ParamTableEntry* _tableEntries;
        U_16 _paramCount;
        RegisterStack* _registerStack;
    public:
        ParamTable(ParamTableEntry*, U_16, RegisterStack*);
        bool getEntry(U_16, ParamTableEntry*);
        U_16 getTotalParamSize();
        U_16 getParamCount();
};

using LocalTableEntry=ParamTableEntry;

class LocalTable
{
    private:
        LocalTableEntry* _tableEntries;
        U_16 _localCount;
    public:
        LocalTable(LocalTableEntry*, U_16);
        bool getEntry(U_16, LocalTableEntry*);
        U_16 getTotalLocalSize();
        U_16 getLocalCount();
};

RegisterStack 
mapIncomingParams(char*, U_16, int*, ParamTableEntry*, U_16, TR::FilePointer*);

bool
nativeSignature(J9Method* method, char *resultBuffer);

int
getParamCount(char *typeString, U_16 maxLength);

class CodeGenerator {
    private:
        Linkage _linkage;
        TR::FilePointer *_logFileFP;
        TR_J9VMBase& _vm;
        TR::CodeCache *_codeCache;
        int32_t _stackPeakSize;
        ParamTable* _paramTable;

        buffer_size_t generateSwitchToInterpPrePrologue(
            char*,
            J9Method*,
            buffer_size_t,
            buffer_size_t,
            char*,
            U_16
        );

        buffer_size_t generateEpologue(char*);

        buffer_size_t loadArgsFromStack(
            char*,
            buffer_size_t
        );

        buffer_size_t saveArgsOnStack(
            char*,
            buffer_size_t
        );

        buffer_size_t
        saveArgsInLocalArray(
            char*,
            buffer_size_t
        );

        /**
         * Generates a load instruction based on the type.
         *
         * @param buffer         code buffer
         * @param method         method for introspection
         * @param bc             Bytecode that generated the load instruction
         * @return               size of generated code; 0 if method failed
         */
        buffer_size_t
        generateLoad(
            char* buffer,
            TR_ResolvedMethod* method,
            TR_J9ByteCode bc,
            TR_J9ByteCodeIterator* bci
        );

        /**
         * Generates a store instruction based on the type.
         *
         * @param buffer         code buffer
         * @param method         method for introspection
         * @param bc             Bytecode that generated the load instruction
         * @return               size of generated code; 0 if method failed
         */
        buffer_size_t
        generateStore(
            char* buffer,
            TR_ResolvedMethod* method,
            TR_J9ByteCode bc,
            TR_J9ByteCodeIterator* bci
        );

    public:
        CodeGenerator() = delete;
        CodeGenerator(J9MicroJITConfig*, J9VMThread*, TR::FilePointer*, TR_J9VMBase&, ParamTable*);

        inline void 
        setPeakStackSize(int32_t newSize)
        {
            _stackPeakSize = newSize;
        }

        inline int32_t 
        getPeakStackSize()
        {
            return _stackPeakSize;
        }

        buffer_size_t 
        generatePrePrologue(
            char*,
            J9Method*,
            char**,
            char**,
            TR_PersistentJittedBodyInfo**
        );

        buffer_size_t 
        generatePrologue(
            char*,
            J9Method*,
            char**,
            char*,
            char*,
            char**
        );

        buffer_size_t 
        generateColdArea(
            char*,
            J9Method*,
            char*
        );

        /**
         * Generates the body for a method.
         * 
         * @param buffer         code buffer
         * @param bci            byte code iterator
         * @param method         method for introspection
         * @return               size of generated code; 0 if method failed
         */
        buffer_size_t 
        generateBody(
            char* buffer,
            TR_ResolvedMethod* method,
            TR_J9ByteCodeIterator* bci
        );

        /**
         * Generate an int3 for signaling debug breakpoint for Linux x86
         * @param buffer    code buffer
         */
        buffer_size_t 
        generateDebugBreakpoint(char* buffer);
        
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
        U_8 * 
        allocateCodeCache(
            int32_t length,
            TR_J9VMBase *vmBase,
            J9VMThread *vmThread
        );

        /**
         * Get the code cache 
         */
        TR::CodeCache* getCodeCache();
};

class MJITCompilationFailure: public virtual std::exception
   {
public:
   MJITCompilationFailure() { }
   virtual const char* what() const throw()
      {
      return "Unable to compile method.";
      }
   };


} //namespace MJIT

#endif
