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
#include "microjit/x/amd64/AMD64CodegenGC.hpp"

#include <stdint.h>
#include <string.h>
#include "env/StackMemoryRegion.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/CodeGenerator_inlines.hpp"
#include "codegen/GCStackAtlas.hpp"
#include "codegen/GCStackMap.hpp"
#include "codegen/Linkage.hpp"
#include "codegen/Linkage_inlines.hpp"
#include "compile/Compilation.hpp"
#include "control/Options.hpp"
#include "control/Options_inlines.hpp"
#include "env/ObjectModel.hpp"
#include "env/CompilerEnv.hpp"
#include "env/TRMemory.hpp"
#include "env/jittypes.h"
#include "il/AutomaticSymbol.hpp"
#include "il/Node.hpp"
#include "il/ParameterSymbol.hpp"
#include "il/ResolvedMethodSymbol.hpp"
#include "il/Symbol.hpp"
#include "il/SymbolReference.hpp"
#include "infra/Assert.hpp"
#include "infra/BitVector.hpp"
#include "infra/IGNode.hpp"
#include "infra/InterferenceGraph.hpp"
#include "infra/List.hpp"
#include "microjit/utils.hpp"
#include "microjit/x/amd64/AMD64Codegen.hpp"
#include "ras/Debug.hpp"

TR::GCStackAtlas*
MJIT::CodeGenGC::createStackAtlas(TR::Compilation *comp, ParamTable *paramTable, LocalTable *localTable)
   {
   //remove before attempting to use function
   TR::GCStackAtlas * atlas = NULL;

   // --------------------------------------------------------------------------------
   // Construct the parameter map for mapped reference parameters
   //
   intptrj_t stackSlotSize = TR::Compiler->om.sizeofReferenceAddress();
   U_16 paramCount = paramTable->getParamCount();
   U_16 ParamSlots = paramTable->getTotalParamSize()/stackSlotSize;
   TR_GCStackMap *parameterMap = new (comp->trHeapMemory(), paramCount) TR_GCStackMap(paramCount);

   int32_t firstMappedParmOffsetInBytes = -1;
   for(int i=0; i<paramCount; i++){
      ParamTableEntry entry;
      MJIT_ASSERT(_logFileFP, paramTable->getEntry(i, &entry), "Bad index for table entry");
      if(entry.isReference){
         firstMappedParmOffsetInBytes = entry.offset;
         break;
      }
   }

   firstMappedParmOffsetInBytes = 
      (firstMappedParmOffsetInBytes >= 0) ? firstMappedParmOffsetInBytes : 0;

   for(int i=0; i<paramCount; i++){
      ParamTableEntry entry;
      MJIT_ASSERT(_logFileFP, paramTable->getEntry(i, &entry), "Bad index for table entry");
      if(entry.isReference){
         int32_t entryOffsetInBytes = entry.offset;
         parameterMap->setBit(((entryOffsetInBytes-firstMappedParmOffsetInBytes)/stackSlotSize));
         if(!entry.onStack){
            parameterMap->setRegisterBits(TR::RealRegister::gprMask((TR::RealRegister::RegNum)entry.regNo));
         }
      }
   }

   // --------------------------------------------------------------------------------
   // Construct the local map for mapped reference locals
   //
   U_16 localCount = localTable->getLocalCount();
   TR_GCStackMap *localMap = new (comp->trHeapMemory(), localCount) TR_GCStackMap(localCount);

   for(int i=0; i<localCount; i++){
      
   }


   /*
   // Assign a GC map index to each reference parameter and each reference local.
   // Stack mapping will have to map the stack in a way that honours these indices
   //
   TR::ResolvedMethodSymbol * methodSymbol = comp->getMethodSymbol();

   // From hereon, any stack memory allocations will expire / die when the function returns
   //
   TR::StackMemoryRegion stackMemoryRegion(*comp->trMemory());

   // --------------------------------------------------------------------------------
   // First map the parameters - the mapping of parameters is constrained by
   // the linkage, so we depend on whether the linkage maps the parameters
   // right to left or left to right.
   // We assume that the parameters are mapped contiguously
   //

   intptrj_t stackSlotSize = TR::Compiler->om.sizeofReferenceAddress();
   int32_t firstMappedParmOffsetInBytes = 0;
   int32_t parmOffsetInBytes;
   int32_t numParmSlots = methodSymbol->getNumParameterSlots();

   TR_ASSERT(firstMappedParmOffsetInBytes >= 0, "firstMappedParmOffsetInBytes must be positive or 0");
   TR_ASSERT(numParmSlots >= 0, "numParmSlots must be positive or 0");

   // --------------------------------------------------------------------------------
   // Now assign GC map indices to parameters depending on the linkage mapping.
   // At the same time populate the parameter map.
   //

   // slotIndex is the zero-based index into the GC map bit vector for a reference
   // parameter or auto.
   //
   int32_t slotIndex = 0;

   parameterIterator.reset();
   for (parmCursor = parameterIterator.getFirst(); parmCursor; parmCursor = parameterIterator.getNext())
      {
      if (comp->getOption(TR_MimicInterpreterFrameShape) || comp->getOption(TR_FullSpeedDebug))
         {
         parmCursor->setParmHasToBeOnStack();
         }

      if ((parmCursor->isReferencedParameter() || comp->getOption(TR_MimicInterpreterFrameShape) || comp->getOption(TR_FullSpeedDebug)) &&
          parmCursor->isCollectedReference())
         {
         parmOffsetInBytes = parmCursor->getParameterOffset();

         // Normalize the parameter offset on the stack to a zero-based index
         // into the GC map.
         //
         slotIndex = ;
         parmCursor->setGCMapIndex(slotIndex);

         if (parmCursor->getLinkageRegisterIndex()<0 ||
             parmCursor->getAssignedGlobalRegisterIndex()<0 ||
             _bodyLinkage->hasToBeOnStack(parmCursor))
            {
            parameterMap->setBit(((parmOffsetInBytes-firstMappedParmOffsetInBytes)/stackSlotSize));
            }
         }
      }

   // At this point, slotIndex will cover either all the parameter slots or just
   // the range of parameters that contain collected references.
   //
   slotIndex = numParmSlots;

   // --------------------------------------------------------------------------------
   // Now assign a GC map index to reference locals. When the stack is mapped,
   // these locals will have to be mapped contiguously in the stack according to
   // this index.
   //
   // Locals that need initialization during the method prologue are mapped first,
   // then the ones that do not need initialization.
   //

   ListIterator<TR::AutomaticSymbol> automaticIterator(&methodSymbol->getAutomaticList());
   TR::AutomaticSymbol * localCursor;

   int32_t numberOfPendingPushSlots = 0;
   if (comp->getOption(TR_MimicInterpreterFrameShape))
      {
      for (localCursor = automaticIterator.getFirst(); localCursor; localCursor = automaticIterator.getNext())
         {
         int32_t assignedIndex = localCursor->getGCMapIndex();
         if (assignedIndex >= 0)
            {
            int32_t nextIndex = assignedIndex + TR::Symbol::convertTypeToNumberOfSlots(localCursor->getDataType());
            if (nextIndex > slotIndex)
               slotIndex = nextIndex;
            }
         }

      // There can be holes at the end of auto list, where the auto was declared
      // and was never used
      //
      if (slotIndex < methodSymbol->getFirstJitTempIndex())
         {
         slotIndex = methodSymbol->getFirstJitTempIndex();
         }

      numberOfPendingPushSlots = slotIndex - methodSymbol->getFirstJitTempIndex();
      TR_ASSERT(numberOfPendingPushSlots >= 0, "assertion failure");

      for (localCursor = automaticIterator.getFirst(); localCursor; localCursor = automaticIterator.getNext())
         {
         int32_t assignedIndex = localCursor->getGCMapIndex();
         if (assignedIndex >= 0)
            {
            // FSD requires the JIT to mimic the interpreter stack.
            //    i.e. for doubles, interpreter expects 2 full slots, even on 64bit platforms.
            // Hence, the JIT must calculate the number of slots required by interpreter and
            // not the number of address-sized slots required to store the data type.
            //
            // Note: convertTypeToNumberOfSlots does not handle aggregate types,
            // but is safe in this scenario because:
            //    1. Symbols of aggregate types have negative GC map indices.
            //    2. FSD runs at no-opt => no escape analysis => no aggregates.
            //
            localCursor->setGCMapIndex(slotIndex -
                                       assignedIndex -
                                       TR::Symbol::convertTypeToNumberOfSlots(localCursor->getDataType()) +
                                       numParmSlots
                                       );
            if (debug("traceFSDStackMap"))
               {
               diagnostic("FSDStackMap: Auto moved from index %d to %d\n", assignedIndex, localCursor->getGCMapIndex());
               }
            }
         }
      }

   // Iniialize colour mapping for locals compaction
   //
   int32_t *colourToGCIndexMap = 0;


   // --------------------------------------------------------------------------------
   // Map uninitialized reference locals that are not stack allocated objects
   //
   for (localCursor = automaticIterator.getFirst(); localCursor; localCursor = automaticIterator.getNext())
      {
      if (localCursor->getGCMapIndex() < 0 &&
          localCursor->isCollectedReference() &&
          !localCursor->isLocalObject() &&
          !localCursor->isInitializedReference() &&
          !localCursor->isInternalPointer() &&
          !localCursor->isPinningArrayPointer())
         {

         localCursor->setGCMapIndex(slotIndex);
         slotIndex += localCursor->getNumberOfSlots();
         }
      }

   int32_t numberOfSlotsToBeInitialized = slotIndex - numParmSlots;

   // --------------------------------------------------------------------------------
   // Map initialized reference locals and stack allocated objects
   //
   int32_t numLocalObjectPaddingSlots = 0;
   bool localObjectsFound = false;

   for (localCursor = automaticIterator.getFirst(); localCursor; localCursor = automaticIterator.getNext())
      {
      if (localCursor->getGCMapIndex() < 0 &&
          localCursor->isCollectedReference() &&
          (localCursor->isLocalObject() || localCursor->isInitializedReference()) &&
          !localCursor->isInternalPointer() &&
          !localCursor->isPinningArrayPointer())
         {

         if (localCursor->isLocalObject())
            {
            localObjectsFound = true;
            int32_t localObjectAlignment = comp->fej9()->getLocalObjectAlignmentInBytes();
            if (localObjectAlignment > stackSlotSize &&
                (comp->target().cpu.isX86() || comp->target().cpu.isPower() || comp->target().cpu.isZ()))
               {
               // We only get here in compressedrefs mode
               int32_t gcMapIndexAlignment = localObjectAlignment / stackSlotSize;
               int32_t remainder = (slotIndex - numParmSlots) % gcMapIndexAlignment;
               if (remainder)
                  {
                  slotIndex += gcMapIndexAlignment - remainder;
                  numLocalObjectPaddingSlots += gcMapIndexAlignment - remainder;
                  traceMsg(comp, "GC index of local object %p is adjusted by +%d, and is %d now\n",localCursor, gcMapIndexAlignment - remainder, slotIndex);
                  }
               }
            }

         localCursor->setGCMapIndex(slotIndex);
         slotIndex += localCursor->getNumberOfSlots();
         }
      }

   int32_t totalSlotsInMap = slotIndex;

   // --------------------------------------------------------------------------------
   // Construct and populate the stack map for a method.  Start with all parameters
   // and locals being live, and selectively unmark slots that are not live.
   //
   TR_GCStackMap * localMap = new (comp->trHeapMemory(), totalSlotsInMap) TR_GCStackMap(totalSlotsInMap);
   localMap->copy(parameterMap);

   // Set all the local references to be live
   //
   int32_t i;
   for (i = numParmSlots; i < totalSlotsInMap; ++i)
      {
      localMap->setBit(i);
      }

   // Reset the bits for non-reference objects
   //
   if (comp->getOption(TR_MimicInterpreterFrameShape))
      {
      for (localCursor = automaticIterator.getFirst(); localCursor; localCursor = automaticIterator.getNext())
         {
         if (localCursor->getGCMapIndex() >= 0 &&
             (!localCursor->isCollectedReference() ||
              localCursor->isInternalPointer() ||
             localCursor->isPinningArrayPointer()))
            {
            localMap->resetBit(localCursor->getGCMapIndex());
            if (localCursor->getSize() > stackSlotSize)
               {
               localMap->resetBit(localCursor->getGCMapIndex() + 1);
               }
            }
         }
      }

   // --------------------------------------------------------------------------
   // Construct and populate the local object stack map
   //
   // Reset the bits for parts of local objects that are not collected slots
   //
   TR_GCStackAllocMap *localObjectStackMap = NULL;

   if (localObjectsFound)
      {
      for (localCursor = automaticIterator.getFirst(); localCursor; localCursor = automaticIterator.getNext())
         {
         if (localCursor->isCollectedReference() && localCursor->isLocalObject())
            {
            TR::AutomaticSymbol * localObject = localCursor->getLocalObjectSymbol();
            slotIndex = localObject->getGCMapIndex();

            if (!localObjectStackMap)
               {
               localObjectStackMap = new (comp->trHeapMemory(), totalSlotsInMap) TR_GCStackAllocMap(totalSlotsInMap);
               }

            localObjectStackMap->setBit(slotIndex);

            int32_t * collectedSlots = localObject->getReferenceSlots();
            int32_t i = 0;

            while (*collectedSlots > 0)
               {
               int32_t collectedSlotIndex = *collectedSlots;

               // collectedSlotIndex is measured in terms of object field sizes,
               // and we want GC map slot sizes, which are not necessarily the same
               // (ie. compressed refs)
               //
               collectedSlotIndex = collectedSlotIndex * TR::Compiler->om.sizeofReferenceField() / stackSlotSize;
               for ( ; i < collectedSlotIndex; ++i)
                  {
                  localMap->resetBit(slotIndex+i);
                  }

               localMap->resetBit(slotIndex+i);
               i = collectedSlotIndex+1;
               collectedSlots++;
               }

            for ( ; i < localObject->getSize()/stackSlotSize; ++i)
               {
               localMap->resetBit(slotIndex+i);
               }
            }
         }
      }

   comp->setMethodStackMap(localMap);

   // --------------------------------------------------------------------------
   // Now create the stack atlas
   //
   TR::GCStackAtlas * atlas = new (comp->trHeapMemory()) TR::GCStackAtlas(numParmSlots, totalSlotsInMap, self()->trMemory());
   atlas->setParmBaseOffset(firstMappedParmOffsetInBytes);
   atlas->setParameterMap(parameterMap);
   atlas->setLocalMap(localMap);
   atlas->setStackAllocMap(localObjectStackMap);
   atlas->setNumberOfSlotsToBeInitialized(numberOfSlotsToBeInitialized);
   atlas->setIndexOfFirstSpillTemp(totalSlotsInMap);
   atlas->setInternalPointerMap(0);
   atlas->setNumberOfPendingPushSlots(numberOfPendingPushSlots);
   atlas->setNumberOfPaddingSlots(numLocalObjectPaddingSlots);
   */
   return atlas;
   }