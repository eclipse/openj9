/*******************************************************************************
 * Copyright (c) 1991, 2020 IBM Corp. and others
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

#include "ArrayletObjectModel.hpp"
#include "GCExtensionsBase.hpp"
#include "ModronAssertions.h"
#include "ObjectModel.hpp"

#if defined(J9VM_GC_ENABLE_DOUBLE_MAP)
#include "ArrayletLeafIterator.hpp"
#include "ObjectAccessBarrier.hpp"
#endif /* J9VM_GC_ENABLE_DOUBLE_MAP */

bool
GC_ArrayletObjectModel::initialize(MM_GCExtensionsBase *extensions)
{
	return GC_ArrayletObjectModelBase::initialize(extensions);
}

void
GC_ArrayletObjectModel::tearDown(MM_GCExtensionsBase *extensions)
{
	GC_ArrayletObjectModelBase::tearDown(extensions);
}

void
GC_ArrayletObjectModel::AssertBadElementSize()
{
	Assert_MM_unreachable();
}

#if defined(J9VM_GC_ENABLE_DOUBLE_MAP)
void
GC_ArrayletObjectModel::AssertNotEmptyArrayletLeaves(UDATA sizeInElements, UDATA arrayletLeafCount)
{
	Assert_MM_true((0 == sizeInElements) || (arrayletLeafCount > 0));
}
#endif /* J9VM_GC_ENABLE_DOUBLE_MAP */

GC_ArrayletObjectModel::ArrayLayout
GC_ArrayletObjectModel::getArrayletLayout(J9Class* clazz, UDATA dataSizeInBytes, UDATA largestDesirableSpine)
{
	ArrayLayout layout = Illegal;
	MM_GCExtensionsBase* extensions = MM_GCExtensionsBase::getExtensions(_omrVM);
	UDATA objectAlignmentInBytes = extensions->getObjectAlignmentInBytes();

	/* the spine need not contain a pointer to the data */
	const UDATA minimumSpineSize = 0;
	UDATA minimumSpineSizeAfterGrowing = minimumSpineSize;
	if (extensions->isVLHGC()) {
		/* CMVC 170688:  Ensure that we don't try to allocate an inline contiguous array of a size which will overflow the region if it ever grows
		 * (easier to handle this case in the allocator than to special-case the collectors to know how to avoid this case)
		 * (currently, we only grow by a hashcode slot which is 4-bytes but will increase our size by the granule of alignment)
		 */
		minimumSpineSizeAfterGrowing += objectAlignmentInBytes;
	}

	/* CMVC 135307 : when checking for InlineContiguous layout, perform subtraction as adding to dataSizeInBytes could trigger overflow. */
	if ((largestDesirableSpine == UDATA_MAX) || (dataSizeInBytes <= (largestDesirableSpine - minimumSpineSizeAfterGrowing - contiguousHeaderSize()))) {
		layout = InlineContiguous;
		if(0 == dataSizeInBytes) {
			/* Zero sized NUA uses the discontiguous shape */
			layout = Discontiguous;
		}
	} else {
		UDATA arrayletLeafSize = _omrVM->_arrayletLeafSize;
		UDATA lastArrayletBytes = dataSizeInBytes & (arrayletLeafSize - 1);

		if (lastArrayletBytes > 0) {
			/* determine how large the spine would be if this were a hybrid array */
			UDATA numberArraylets = numArraylets(dataSizeInBytes);
			bool align = shouldAlignSpineDataSection(clazz);
			UDATA hybridSpineBytes = getSpineSize(clazz, Hybrid, numberArraylets, dataSizeInBytes, align);
			UDATA adjustedHybridSpineBytes = extensions->objectModel.adjustSizeInBytes(hybridSpineBytes);
			UDATA adjustedHybridSpineBytesAfterMove = adjustedHybridSpineBytes;
			if (extensions->isVLHGC()) {
				adjustedHybridSpineBytesAfterMove += objectAlignmentInBytes;
			}
#if defined(J9VM_GC_ENABLE_DOUBLE_MAP)
			if (extensions->indexableObjectModel.isDoubleMappingEnabled()) {
				layout = Discontiguous;
			} else
#endif /* J9VM_GC_ENABLE_DOUBLE_MAP */
			/* if remainder data can fit in spine, make it hybrid */
			if (adjustedHybridSpineBytesAfterMove <= largestDesirableSpine) {
				/* remainder data can fit in spine, last arrayoid pointer points to empty data section in spine */
				layout = Hybrid;
			} else {
				/* remainder data will go into an arraylet, last arrayoid pointer points to it */
				layout = Discontiguous;
			}
		} else {
			/* remainder is empty, so no arraylet allocated; last arrayoid pointer is set to MULL */
			layout = Discontiguous;
		}
	}
	return layout;
}

#if defined(J9VM_GC_ENABLE_DOUBLE_MAP)
bool
GC_ArrayletObjectModel::freeDoubleMapping(MM_EnvironmentBase *env, J9IndexableObject *objectPtr, struct J9PortVmemIdentifier *identifier)
{
	PORT_ACCESS_FROM_ENVIRONMENT(env);
	void *handle = identifier->handle;
	void *arrayletLeaveAddrs[J9_GC_ARRAYLET_ALLOC_THRESHOLD];
	void *addressToFree = NULL;
	if (NULL == handle) {
		GC_ArrayletLeafIterator arrayletLeafIterator((J9JavaVM *)_omrVM->_language_vm, objectPtr);
		GC_SlotObject *slotObject = NULL;
		uintptr_t count = 0;

		while (NULL != (slotObject = arrayletLeafIterator.nextLeafPointer())) {
			void *currentLeaf = slotObject->readReferenceFromSlot();
			/* In some corner cases the last leaf might be NULL therefore we must ignore it */
			if (NULL == currentLeaf) {
				break;
			}
			count++;
			arrayletLeaveAddrs[count] = currentLeaf;
		}
		arrayletLeaveAddrs[0] = (void *)count;
		identifier->handle = arrayletLeaveAddrs;
	} else {
		addressToFree = handle;
	}
	j9vmem_free_memory(identifier->address, identifier->size, identifier);
	if (NULL != addressToFree) {
		env->getForge()->free(addressToFree);
	}
	return true;
}
#endif /* J9VM_GC_ENABLE_DOUBLE_MAP */
