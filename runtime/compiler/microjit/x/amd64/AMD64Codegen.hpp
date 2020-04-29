/*******************************************************************************
 * Copyright (c) 2000, 2020 IBM Corp. and others
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
#ifndef MJIT_AMD64CODEGEN_HPP
#define MJIT_AMD64CODEGEN_HPP

#include <cstdint>
#include "microjit/SideTables.hpp"
#include "microjit/utils.hpp"
#include "microjit/x/amd64/AMD64Linkage.hpp"
#include "microjit/x/amd64/AMD64CodegenGC.hpp"
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
        TR::Compilation* _comp;
        MJIT::CodeGenGC* _mjitCGGC;
        TR::GCStackAtlas* _atlas;

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
         * @param bci            Bytecode iterator from which the bytecode was retreived
         * @return               size of generated code; 0 if method failed
         */
        buffer_size_t
        generateStore(
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
         * @param bci            Bytecode iterator from which the bytecode was retreived
         * @return               size of generated code; 0 if method failed
         */
        buffer_size_t
        generatePutStatic(
            char* buffer, 
            TR_ResolvedMethod* method, 
            TR_J9ByteCodeIterator* bci
        );

        /**
         * Generates a store instruction based on the type.
         *
         * @param buffer         code buffer
         * @param method         method for introspection
         * @param bci            Bytecode iterator from which the bytecode was retreived
         * @return               size of generated code; 0 if method failed
         */
        buffer_size_t
        generateGetStatic(
            char* buffer, 
            TR_ResolvedMethod* method, 
            TR_J9ByteCodeIterator* bci
        );

        /**
         * Generates a return instruction based on the type.
         * 
         * @param buffer        code buffer
         * @param dt            The data type to be returned by the TR-compiled method
         * @return              size of generated code; 0 if method failed
         */
        buffer_size_t
        generateReturn(
            char* buffer, 
            TR::DataType dt
        );

        LocalTable makeLocalTable(TR_J9ByteCodeIterator*, MJIT::LocalTableEntry*, U_16, int32_t);

    public:
        CodeGenerator() = delete;
        CodeGenerator(J9MicroJITConfig*, J9VMThread*, TR::FilePointer*, TR_J9VMBase&, ParamTable*, TR::Compilation*, MJIT::CodeGenGC*);

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
            char**,
            TR_J9ByteCodeIterator*
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

        /**
         * Get a pointer to the Stack Atlas
         */
        TR::GCStackAtlas* getStackAtlas();
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
