#ifndef AMD64LINKAGE_HPP
#define AMD64LINKAGE_HPP

#include <cstdint>
#include "env/VMJ9.h"
#include "../../utils.hpp"

// linkage properties
#define CallerCleanup                              0x0001
#define RightToLeft                                0x0002
#define IntegersInRegisters                        0x0004
#define LongsInRegisters                           0x0008
#define FloatsInRegisters                          0x0010
#define EightBytePointers                          0x0020
#define EightByteParmSlots                         0x0040
#define LinkageRegistersAssignedByCardinalPosition 0x0080
#define CallerFrameAllocatesSpaceForLinkageRegs    0x0100
#define AlwaysDedicateFramePointerRegister         0x0200
#define NeedsThunksForIndirectCalls                0x0400
#define UsesPushesForPreservedRegs                 0x0800
#define ReservesOutgoingArgsInPrologue             0x1000
#define UsesRegsForHelperArgs                      0x2000

// register flags
#define Preserved                   0x01
#define IntegerReturn               0x02
#define IntegerArgument             0x04
#define FloatReturn                 0x08
#define FloatArgument               0x10
#define CallerAllocatesBackingStore 0x20
#define CalleeVolatile              0x40


/*
| Architecture 	| Endian | Return address 	| Argument registers 	
| x86-64 		| Little | Stack 			| RAX RSI RDX RCX 

| Return value registers 						| Preserved registers 	| vTable index 			|
| EAX (32-bit) RAX (64-bit) XMM0 (float/double) | RBX R9-R15 			| R8 (receiver in RAX) 	|
*/

namespace MJIT {

uint32_t const NUM_REGISTERS = 32;

enum
   {
   RETURN_ADDRESS_SIZE=8,
   NUM_INTEGER_LINKAGE_REGS=4,
   NUM_FLOAT_LINKAGE_REGS=8,
   };

namespace RealRegister {

	typedef int RegNum;
	//No one should modify these.
	const RegNum NoReg                   = 0;
  
	// The order of the GPRS registers is defined by the linkage
	// convention, and the VM may rely on it (eg. for GC).
	const RegNum eax                    = 1;
	const RegNum FirstGPR               = eax;
	const RegNum ebx                    = 2;
	const RegNum ecx                    = 3;
	const RegNum edx                    = 4;
	const RegNum edi                    = 5;
	const RegNum esi                    = 6;
	const RegNum ebp                    = 7;
	const RegNum esp                    = 8;
	const RegNum r8                     = 9;
	const RegNum r9                     = 10;
	const RegNum r10                    = 11;
	const RegNum r11                    = 12;
	const RegNum r12                    = 13;
	const RegNum r13                    = 14;
	const RegNum r14                    = 15;
	const RegNum r15                    = 16;
	const RegNum LastGPR                = r15;
	const RegNum Last8BitGPR            = r15;
	const RegNum LastAssignableGPR      = r15;
 
	const RegNum vfp                    = 17;
 
	const RegNum FPRMaskOffset          = LastGPR;
	const RegNum st0                    = 18;
	const RegNum FirstFPR               = st0;
	const RegNum st1                    = 19;
	const RegNum st2                    = 20;
	const RegNum st3                    = 21;
	const RegNum st4                    = 22;
	const RegNum st5                    = 23;
	const RegNum st6                    = 24;
	const RegNum st7                    = 25;
	const RegNum LastFPR                = st7;
	const RegNum LastAssignableFPR      = st7;

	const RegNum mm0                    = 26;
	const RegNum FirstMMXR              = mm0;
	const RegNum mm1                    = 27;
	const RegNum mm2                    = 28;
	const RegNum mm3                    = 29;
	const RegNum mm4                    = 30;
	const RegNum mm5                    = 31;
	const RegNum mm6                    = 32;
	const RegNum mm7                    = 33;
	const RegNum LastMMXR               = mm7;

	const RegNum XMMRMaskOffset         = LastGPR;
	const RegNum xmm0                   = 34;
	const RegNum FirstXMMR              = xmm0;
	const RegNum xmm1                   = 35;
	const RegNum xmm2                   = 36;
	const RegNum xmm3                   = 37;
	const RegNum xmm4                   = 38;
	const RegNum xmm5                   = 39;
	const RegNum xmm6                   = 40;
	const RegNum xmm7                   = 41;
	const RegNum xmm8                   = 42;
	const RegNum xmm9                   = 43;
	const RegNum xmm10                  = 44;
	const RegNum FirstSpillReg          = xmm10;
	const RegNum xmm11                  = 45;
	const RegNum xmm12                  = 46;
	const RegNum xmm13                  = 47;
	const RegNum xmm14                  = 48;
	const RegNum xmm15                  = 49;
	const RegNum LastSpillReg           = xmm15;
	const RegNum LastXMMR               = xmm15;

	const RegNum AllFPRegisters         = 50;
	const RegNum ByteReg                = 51;
	const RegNum BestFreeReg            = 52;
	const RegNum SpilledReg             = 53;
	const RegNum NumRegisters           = 54;

	const RegNum NumXMMRegisters        = LastXMMR - FirstXMMR + 1;
	const RegNum MaxAssignableRegisters = NumXMMRegisters + (LastAssignableGPR - FirstGPR + 1) - 1; // -1 for stack pointer

} //namespace RealRegister

inline char*
getRealRegString(RealRegister::RegNum reg){
	switch(reg){
	case RealRegister::NoReg:
		return "NoReg";
	case RealRegister::eax  :
		return "eax";
	case RealRegister::ebx  :
		return "ebx";
	case RealRegister::ecx  :
		return "ecx";
	case RealRegister::edx  :
		return "edx";
	case RealRegister::edi  :
		return "edi";
	case RealRegister::esi  :
		return "esi";
	case RealRegister::ebp  :
		return "ebp";
	case RealRegister::esp  :
		return "esp";
	case RealRegister::r8   :
		return "r8";
	case RealRegister::r9   :
		return "r9";
	case RealRegister::r10  :
		return "r10";
	case RealRegister::r11  :
		return "r11";
	case RealRegister::r12  :
		return "r12";
	case RealRegister::r13  :
		return "r13";
	case RealRegister::r14  :
		return "r14";
	case RealRegister::r15  :
		return "r15";
	case RealRegister::vfp  :
		return "vfp";
	case RealRegister::st0  :
		return "st0";
	case RealRegister::st1  :
		return "st1";
	case RealRegister::st2  :
		return "st2";
	case RealRegister::st3  :
		return "st3";
	case RealRegister::st4  :
		return "st4";
	case RealRegister::st5  :
		return "st5";
	case RealRegister::st6  :
		return "st6";
	case RealRegister::st7  :
		return "st7";
	case RealRegister::mm0  :
		return "mm0";
	case RealRegister::mm1  :
		return "mm1";
	case RealRegister::mm2  :
		return "mm2";
	case RealRegister::mm3  :
		return "mm3";
	case RealRegister::mm4  :
		return "mm4";
	case RealRegister::mm5  :
		return "mm5";
	case RealRegister::mm6  :
		return "mm6";
	case RealRegister::mm7  :
		return "mm7";
	case RealRegister::xmm0 :
		return "xmm0";
	case RealRegister::xmm1 :
		return "xmm1";
	case RealRegister::xmm2 :
		return "xmm2";
	case RealRegister::xmm3 :
		return "xmm3";
	case RealRegister::xmm4 :
		return "xmm4";
	case RealRegister::xmm5 :
		return "xmm5";
	case RealRegister::xmm6 :
		return "xmm6";
	case RealRegister::xmm7 :
		return "xmm7";
	case RealRegister::xmm8 :
		return "xmm8";
	case RealRegister::xmm9 :
		return "xmm9";
	case RealRegister::xmm10:
		return "xmm10";
	case RealRegister::xmm11:
		return "xmm11";
	case RealRegister::xmm12:
		return "xmm12";
	case RealRegister::xmm13:
		return "xmm13";
	case RealRegister::xmm14:
		return "xmm14";
	case RealRegister::xmm15:
		return "xmm15";
	default:
		return "Not a valid register";
	}
}

namespace RealRegisterMask {

	typedef int RegMask;
	const RegMask noRegMask         = 0x00000000;
  
	// GPR
	//
	const RegMask eaxMask           = 0x00000001;
	const RegMask ebxMask           = 0x00000002;
	const RegMask ecxMask           = 0x00000004;
	const RegMask edxMask           = 0x00000008;
	const RegMask ediMask           = 0x00000010;
	const RegMask esiMask           = 0x00000020;
	const RegMask ebpMask           = 0x00000040;
	const RegMask espMask           = 0x00000080;

	const RegMask r8Mask            = 0x00000100;
	const RegMask r9Mask            = 0x00000200;
	const RegMask r10Mask           = 0x00000400;
	const RegMask r11Mask           = 0x00000800;
	const RegMask r12Mask           = 0x00001000;
	const RegMask r13Mask           = 0x00002000;
	const RegMask r14Mask           = 0x00004000;
	const RegMask r15Mask           = 0x00008000;
	const RegMask AvailableGPRMask  = 0x0000FFFF;
  
	// FPR
	//
	const RegMask st0Mask           = 0x00000001;
	const RegMask st1Mask           = 0x00000002;
	const RegMask st2Mask           = 0x00000004;
	const RegMask st3Mask           = 0x00000008;
	const RegMask st4Mask           = 0x00000010;
	const RegMask st5Mask           = 0x00000020;
	const RegMask st6Mask           = 0x00000040;
	const RegMask st7Mask           = 0x00000080;
	const RegMask AvailableFPRMask  = 0x000000FF;
  
	// MMXR
	//
	const RegMask mm0Mask           = 0x00010000;
	const RegMask mm1Mask           = 0x00020000;
	const RegMask mm2Mask           = 0x00040000;
	const RegMask mm3Mask           = 0x00080000;
	const RegMask mm4Mask           = 0x00100000;
	const RegMask mm5Mask           = 0x00200000;
	const RegMask mm6Mask           = 0x00400000;
	const RegMask mm7Mask           = 0x00800000;
	const RegMask AvailableMMRMask  = 0x00FF0000;
  
	// XMMR
	//
	const RegMask xmm0Mask          = 0x00000001 << RealRegister::XMMRMaskOffset;
	const RegMask xmm1Mask          = 0x00000002 << RealRegister::XMMRMaskOffset;
	const RegMask xmm2Mask          = 0x00000004 << RealRegister::XMMRMaskOffset;
	const RegMask xmm3Mask          = 0x00000008 << RealRegister::XMMRMaskOffset;
	const RegMask xmm4Mask          = 0x00000010 << RealRegister::XMMRMaskOffset;
	const RegMask xmm5Mask          = 0x00000020 << RealRegister::XMMRMaskOffset;
	const RegMask xmm6Mask          = 0x00000040 << RealRegister::XMMRMaskOffset;
	const RegMask xmm7Mask          = 0x00000080 << RealRegister::XMMRMaskOffset;
	const RegMask xmm8Mask          = 0x00000100 << RealRegister::XMMRMaskOffset;
	const RegMask xmm9Mask          = 0x00000200 << RealRegister::XMMRMaskOffset;
	const RegMask xmm10Mask         = 0x00000400 << RealRegister::XMMRMaskOffset;
	const RegMask xmm11Mask         = 0x00000800 << RealRegister::XMMRMaskOffset;
	const RegMask xmm12Mask         = 0x00001000 << RealRegister::XMMRMaskOffset;
	const RegMask xmm13Mask         = 0x00002000 << RealRegister::XMMRMaskOffset;
	const RegMask xmm14Mask         = 0x00004000 << RealRegister::XMMRMaskOffset;
	const RegMask xmm15Mask         = 0x00008000 << RealRegister::XMMRMaskOffset;
	const RegMask AvailableXMMRMask = 0x0000FFFF << RealRegister::XMMRMaskOffset;

}

struct properties {

	enum {
		MaxArgumentRegisters = 30,
		MaxReturnRegisters   = 3,
		MaxVolatileRegisters = 30,
		MaxScratchRegisters  = 5
	};

	TR::FilePointer* _logFileFP;

	uint32_t                _properties;
	uint32_t                _registerFlags[NUM_REGISTERS];

	RealRegister::RegNum    _preservedRegisters[NUM_REGISTERS];
	RealRegister::RegNum    _argumentRegisters[MaxArgumentRegisters];
	RealRegister::RegNum    _returnRegisters[MaxReturnRegisters];
	RealRegister::RegNum    _volatileRegisters[MaxVolatileRegisters];
	RealRegister::RegNum    _scratchRegisters[MaxScratchRegisters];
	RealRegister::RegNum    _vtableIndexArgumentRegister; // for icallVMprJavaSendPatchupVirtual
	RealRegister::RegNum    _j9methodArgumentRegister;    // for icallVMprJavaSendStatic

	uint32_t                _preservedRegisterMapForGC;
	RealRegister::RegNum    _framePointerRegister;
	RealRegister::RegNum    _methodMetaDataRegister;
	int8_t                  _offsetToFirstParm;
	int8_t                  _offsetToFirstLocal;           // Points immediately after the local with highest address

	uint8_t                 _numScratchRegisters;

	uint8_t                 _numberOfVolatileGPRegisters;
	uint8_t                 _numberOfVolatileXMMRegisters;
	uint8_t                 _numVolatileRegisters;

	uint8_t                 _numberOfPreservedGPRegisters;
	uint8_t                 _numberOfPreservedXMMRegisters;
	uint8_t                 _numPreservedRegisters;

	uint8_t                 _maxRegistersPreservedInPrologue;

	uint8_t                 _numIntegerArgumentRegisters;
	uint8_t                 _numFloatArgumentRegisters;

	uint8_t                 _firstIntegerArgumentRegister;
	uint8_t                 _firstFloatArgumentRegister;

	uint32_t                _allocationOrder[NUM_REGISTERS];

	uint32_t                _OutgoingArgAlignment;

	uint32_t                getProperties() const {return _properties;}
	uint32_t                getCallerCleanup() const {return (_properties & CallerCleanup);}
	uint32_t                passArgsRightToLeft() const {return (_properties & RightToLeft);}
	uint32_t                getIntegersInRegisters() const {return (_properties & IntegersInRegisters);}
	uint32_t                getLongsInRegisters() const {return (_properties & LongsInRegisters);}
	uint32_t                getFloatsInRegisters() const {return (_properties & FloatsInRegisters);}

	uint32_t                getAlwaysDedicateFramePointerRegister() const {return (_properties & AlwaysDedicateFramePointerRegister);}
	uint32_t                getNeedsThunksForIndirectCalls() const {return (_properties & NeedsThunksForIndirectCalls);}
	uint32_t                getUsesPushesForPreservedRegs() const {return (_properties & UsesPushesForPreservedRegs);}
	uint32_t                getReservesOutgoingArgsInPrologue() const {return (_properties & ReservesOutgoingArgsInPrologue);}
	uint32_t                getUsesRegsForHelperArgs() const {return (_properties & UsesRegsForHelperArgs);}

	uint8_t                 getPointerSize() const {return (_properties & EightBytePointers)? 8 : 4;}
	uint8_t                 getPointerShift() const {return (_properties & EightBytePointers)? 3 : 2;}

	uint8_t                 getParmSlotSize() const {return (_properties & EightByteParmSlots)? 8 : 4;}
	uint8_t                 getParmSlotShift() const {return (_properties & EightByteParmSlots)? 3 : 2;}

	uint8_t                 getGPRWidth() const { return getPointerSize();}
	uint8_t                 getRetAddressWidth() const {return getGPRWidth(); }

	uint32_t getLinkageRegistersAssignedByCardinalPosition() const
		{
		return (_properties & LinkageRegistersAssignedByCardinalPosition);
		}

	uint32_t getCallerFrameAllocatesSpaceForLinkageRegisters() const
		{
		return (_properties & CallerFrameAllocatesSpaceForLinkageRegs);
		}

	uint32_t                getRegisterFlags(RealRegister::RegNum regNum)          const {return _registerFlags[regNum];}
	uint32_t                isPreservedRegister(RealRegister::RegNum regNum)       const {return (_registerFlags[regNum] & Preserved);}
	uint32_t                isCalleeVolatileRegister(RealRegister::RegNum regNum)  const {return (_registerFlags[regNum] & CalleeVolatile);}
	uint32_t                isIntegerReturnRegister(RealRegister::RegNum regNum)   const {return (_registerFlags[regNum] & IntegerReturn);}
	uint32_t                isIntegerArgumentRegister(RealRegister::RegNum regNum) const {return (_registerFlags[regNum] & IntegerArgument);}
	uint32_t                isFloatReturnRegister(RealRegister::RegNum regNum)     const {return (_registerFlags[regNum] & FloatReturn);}
	uint32_t                isFloatArgumentRegister(RealRegister::RegNum regNum)   const {return (_registerFlags[regNum] & FloatArgument);}
	uint32_t                doesCallerAllocatesBackingStore(RealRegister::RegNum regNum) const {return (_registerFlags[regNum] & CallerAllocatesBackingStore);}

	uint32_t                getKilledAndNonReturn(RealRegister::RegNum regNum) const {return ((_registerFlags[regNum] & (Preserved | IntegerReturn | FloatReturn)) == 0);}
	uint32_t                getPreservedRegisterMapForGC() const {return _preservedRegisterMapForGC;}

	RealRegister::RegNum    getPreservedRegister(uint32_t index) const {return _preservedRegisters[index];}

	RealRegister::RegNum    getArgument(uint32_t index) const {return _argumentRegisters[index];}

	RealRegister::RegNum    getIntegerReturnRegister()  const {return _returnRegisters[0];}
	RealRegister::RegNum    getLongLowReturnRegister()  const {return _returnRegisters[0];}
	RealRegister::RegNum    getLongHighReturnRegister() const {return _returnRegisters[2];}
	RealRegister::RegNum    getFloatReturnRegister()    const {return _returnRegisters[1];}
	RealRegister::RegNum    getDoubleReturnRegister()   const {return _returnRegisters[1];}
	RealRegister::RegNum    getFramePointerRegister()   const {return _framePointerRegister;}
	RealRegister::RegNum    getMethodMetaDataRegister() const {return _methodMetaDataRegister;}

	int32_t getOffsetToFirstParm() const {return _offsetToFirstParm;}

	int32_t getOffsetToFirstLocal() const {return _offsetToFirstLocal;}

	uint8_t getNumIntegerArgumentRegisters() const {return _numIntegerArgumentRegisters;}
	uint8_t getNumFloatArgumentRegisters()   const {return _numFloatArgumentRegisters;}
	uint8_t getNumPreservedRegisters()       const {return _numPreservedRegisters;}
	uint8_t getNumVolatileRegisters()        const {return _numVolatileRegisters;}
	uint8_t getNumScratchRegisters()         const {return _numScratchRegisters;}

	uint32_t *getRegisterAllocationOrder() const {return (uint32_t *)_allocationOrder;}

	uint32_t getOutgoingArgAlignment() const   {return _OutgoingArgAlignment;}
	uint32_t setOutgoingArgAlignment(uint32_t s)   {return (_OutgoingArgAlignment = s);}

	RealRegister::RegNum getIntegerArgumentRegister(uint8_t index) const
		{
		MJIT_ASSERT(_logFileFP, index < getNumIntegerArgumentRegisters(), "assertion failure");
		return _argumentRegisters[_firstIntegerArgumentRegister + index];
		}

	RealRegister::RegNum getFloatArgumentRegister(uint8_t index) const
		{
		MJIT_ASSERT(_logFileFP, index < getNumFloatArgumentRegisters(), "assertion failure");
		return _argumentRegisters[_firstFloatArgumentRegister + index];
		}

	RealRegister::RegNum getArgumentRegister(uint8_t index, bool isFloat) const
		{
		return isFloat? getFloatArgumentRegister(index) : getIntegerArgumentRegister(index);
		}

	RealRegister::RegNum getIntegerScratchRegister(uint8_t index) const
		{
		MJIT_ASSERT(_logFileFP, index < getNumScratchRegisters(), "assertion failure");
		return _scratchRegisters[index];
		}

	RealRegister::RegNum      getVTableIndexArgumentRegister() const
		{
		return _vtableIndexArgumentRegister;
		}

	RealRegister::RegNum      getJ9MethodArgumentRegister() const
		{
		return _j9methodArgumentRegister;
		}

	uint8_t getMaxRegistersPreservedInPrologue() const { return _maxRegistersPreservedInPrologue; }

	uint32_t getNumberOfVolatileGPRegisters()  const {return _numberOfVolatileGPRegisters;}
	uint32_t getNumberOfVolatileXMMRegisters() const {return _numberOfVolatileXMMRegisters;}

	uint32_t getNumberOfPreservedGPRegisters()  const {return _numberOfPreservedGPRegisters;}
	uint32_t getNumberOfPreservedXMMRegisters() const {return _numberOfPreservedXMMRegisters;}

};

class Linkage {
public:
	struct properties _properties;
    TR_J9VMBase* _fej9;
	Linkage() = delete;
	Linkage(J9MicroJITConfig*, J9VMThread*, TR::FilePointer* logFileFP);
	inline TR_J9VMBase* getFrontEnd() {
		return _fej9;
	}
};

} // namespace MJIT
#endif
