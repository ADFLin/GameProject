#ifndef Assembler_h__
#define Assembler_h__

#include "AsmetaBase.h"

#include <cassert>
#include <vector>
#include <type_traits>

namespace Asmeta
{

	class Label
	{
	public:
		typedef Label* RefType;
		Label()
		{
			mState  = eUnused;
			mLink   = nullptr;
			mOffset = 0;
		}
		bool    isBound(){ return mState == eBound; }
		SysUint getOffset(){ return mOffset; }
	private:

		enum LinkMode
		{
			eLinkAbs ,
			eLinkRel ,
		};
		struct Link
		{
			SysInt   offset;
			SysInt   dispOffset;
			LinkMode mode; 
			char     dispSize;
			Link*    next;
		};
		enum State
		{
			eUnused  ,
			eUnbound ,
			eBound   ,
		};

		Link*  getLink(){ return mLink;  }
		void   addLink( Link* link )
		{
			link->next = mLink;
			mLink = link;
		}

#if _DEBUG
		int     mIndex;
#endif
		Link*   mLink;
		SysUint mOffset;
		State   mState;
		template< class T >
		friend class AssemblerT;
	};

	Reg8 const al = Reg8( Reg::eAL );
	Reg8 const cl = Reg8( Reg::eCL );
	Reg8 const dl = Reg8( Reg::eDL );
	Reg8 const bl = Reg8( Reg::eBL );
	Reg8 const ah = Reg8( Reg::eAH );
	Reg8 const dh = Reg8( Reg::eDH );
	Reg8 const ch = Reg8( Reg::eCH );
	Reg8 const bh = Reg8( Reg::eBH );

	Reg16 const ax = Reg16( Reg::eAX );
	Reg16 const cx = Reg16( Reg::eCX );
	Reg16 const dx = Reg16( Reg::eDX );
	Reg16 const bx = Reg16( Reg::eBX );
	Reg16 const sp = Reg16( Reg::eSP );
	Reg16 const bp = Reg16( Reg::eBP );
	Reg16 const si = Reg16( Reg::eSI );
	Reg16 const di = Reg16( Reg::eDI );

	Reg32 const eax = Reg32( Reg::eAX );
	Reg32 const ecx = Reg32( Reg::eCX );
	Reg32 const edx = Reg32( Reg::eDX );
	Reg32 const ebx = Reg32( Reg::eBX );
	Reg32 const esp = Reg32( Reg::eSP );
	Reg32 const ebp = Reg32( Reg::eBP );
	Reg32 const esi = Reg32( Reg::eSI );
	Reg32 const edi = Reg32( Reg::eDI );

#if TARGET_PLATFORM_64BITS
	Reg64 const rax = Reg64(Reg::eAX);
	Reg64 const rcx = Reg64(Reg::eCX);
	Reg64 const rdx = Reg64(Reg::eDX);
	Reg64 const rbx = Reg64(Reg::eBX);
	Reg64 const rsp = Reg64(Reg::eSP);
	Reg64 const rbp = Reg64(Reg::eBP);
	Reg64 const rsi = Reg64(Reg::eSI);
	Reg64 const rdi = Reg64(Reg::eDI);

	Reg64 const r8 = Reg64(Reg::eR8);
	Reg64 const r9 = Reg64(Reg::eR9);
	Reg64 const r10 = Reg64(Reg::eR10);
	Reg64 const r11 = Reg64(Reg::eR11);
	Reg64 const r12 = Reg64(Reg::eR12);
	Reg64 const r13 = Reg64(Reg::eR13);
	Reg64 const r14 = Reg64(Reg::eR14);
	Reg64 const r15 = Reg64(Reg::eR15);

#endif

	RegSeg const es = RegSeg( Reg::eES );
	RegSeg const cs = RegSeg( Reg::eCS );
	RegSeg const ss = RegSeg( Reg::eSS );
	RegSeg const ds = RegSeg( Reg::eDS );
	RegSeg const fs = RegSeg( Reg::eFS );
	RegSeg const gs = RegSeg( Reg::eGS );

	ASMETA_INLINE RegST st( uint8 idx = 0 ){ assert( idx < 8 ); return RegST( idx );  }

	// XMM registers (SSE)
	RegXMM const xmm0 = RegXMM(0);
	RegXMM const xmm1 = RegXMM(1);
	RegXMM const xmm2 = RegXMM(2);
	RegXMM const xmm3 = RegXMM(3);
	RegXMM const xmm4 = RegXMM(4);
	RegXMM const xmm5 = RegXMM(5);
	RegXMM const xmm6 = RegXMM(6);
	RegXMM const xmm7 = RegXMM(7);

#if TARGET_PLATFORM_64BITS
	RegXMM const xmm8 = RegXMM(8);
	RegXMM const xmm9 = RegXMM(9);
	RegXMM const xmm10 = RegXMM(10);
	RegXMM const xmm11 = RegXMM(11);
	RegXMM const xmm12 = RegXMM(12);
	RegXMM const xmm13 = RegXMM(13);
	RegXMM const xmm14 = RegXMM(14);
	RegXMM const xmm15 = RegXMM(15);
#endif

	ASMETA_INLINE RegXMM xmm(uint8 idx) { assert(idx < 16); return RegXMM(idx); }


#define DEF_REF_MEM( Type , Param , InputRef )\
	ASMETA_INLINE RefMem< Type , 0 >  ptr      ( Param ){ return RefMem< Type , 0 >( InputRef );  }\
	ASMETA_INLINE RefMem< Type , 1 >  byte_ptr ( Param ){ return RefMem< Type , 1 >( InputRef );  }\
	ASMETA_INLINE RefMem< Type , 2 >  word_ptr ( Param ){ return RefMem< Type , 2 >( InputRef );  }\
	ASMETA_INLINE RefMem< Type , 4 >  dword_ptr( Param ){ return RefMem< Type , 4 >( InputRef );  }\
	ASMETA_INLINE RefMem< Type , 8 >  qword_ptr( Param ){ return RefMem< Type , 8 >( InputRef );  }\
	ASMETA_INLINE RefMem< Type , 10 > tword_ptr( Param ){ return RefMem< Type , 10 >( InputRef );  }

	DEF_REF_MEM( Label   , Label* label        , label )
	DEF_REF_MEM( Address , Address const& addr , addr  )
	DEF_REF_MEM( RegPtr  , Reg32 const& base   , RegPtr( base ) )

	ASMETA_INLINE RefMem< LabelPtr , 0 >  ptr      ( Label* label , int32 offset ){ return RefMem< LabelPtr , 0 >( LabelPtr( label , offset ) );  }
	ASMETA_INLINE RefMem< LabelPtr , 1 >  byte_ptr ( Label* label , int32 offset ){ return RefMem< LabelPtr , 1 >( LabelPtr( label , offset ) );  }
	ASMETA_INLINE RefMem< LabelPtr , 2 >  word_ptr ( Label* label , int32 offset ){ return RefMem< LabelPtr , 2 >( LabelPtr( label , offset ) );  }
	ASMETA_INLINE RefMem< LabelPtr , 4 >  dword_ptr( Label* label , int32 offset ){ return RefMem< LabelPtr , 4 >( LabelPtr( label , offset ) );  }
	ASMETA_INLINE RefMem< LabelPtr , 8 >  qword_ptr( Label* label , int32 offset ){ return RefMem< LabelPtr , 8 >( LabelPtr( label , offset ) );  }
	ASMETA_INLINE RefMem< LabelPtr , 10 > tword_ptr( Label* label , int32 offset ){ return RefMem< LabelPtr , 10 >( LabelPtr( label , offset ) );  }


	ASMETA_INLINE RefMem< RegPtr , 0 >  ptr      ( Reg32 const& base , int8 disp ){ return RefMem< RegPtr , 0 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 1 >  byte_ptr ( Reg32 const& base , int8 disp ){ return RefMem< RegPtr , 1 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 2 >  word_ptr ( Reg32 const& base , int8 disp ){ return RefMem< RegPtr , 2 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 4 >  dword_ptr( Reg32 const& base , int8 disp ){ return RefMem< RegPtr , 4 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 8 >  qword_ptr( Reg32 const& base , int8 disp ){ return RefMem< RegPtr , 8 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 10 > tword_ptr( Reg32 const& base , int8 disp ){ return RefMem< RegPtr , 10 >( RegPtr( base , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr , 0 >  ptr      ( Reg32 const& base , int32 disp ){ return RefMem< RegPtr , 0 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 1 >  byte_ptr ( Reg32 const& base , int32 disp ){ return RefMem< RegPtr , 1 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 2 >  word_ptr ( Reg32 const& base , int32 disp ){ return RefMem< RegPtr , 2 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 4 >  dword_ptr( Reg32 const& base , int32 disp ){ return RefMem< RegPtr , 4 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 8 >  qword_ptr( Reg32 const& base , int32 disp ){ return RefMem< RegPtr , 8 >( RegPtr( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 10 > tword_ptr( Reg32 const& base , int32 disp ){ return RefMem< RegPtr , 10 >( RegPtr( base , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr , 0 >  ptr      ( Reg32 const& base , Reg32 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr , 0 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 1 >  byte_ptr ( Reg32 const& base , Reg32 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr , 1 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 2 >  word_ptr ( Reg32 const& base , Reg32 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr , 2 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 4 >  dword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr , 4 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 8 >  qword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr , 8 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 10 > tword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr , 10 >( RegPtr( base , index , shift , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr , 0 >  ptr      ( Reg32 const& base , Reg32 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr , 0 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 1 >  byte_ptr ( Reg32 const& base , Reg32 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr , 1 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 2 >  word_ptr ( Reg32 const& base , Reg32 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr , 2 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 4 >  dword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr , 4 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 8 >  qword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr , 8 >( RegPtr( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr , 10 > tword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr , 10 >( RegPtr( base , index , shift , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr , 0 >  ptr      ( Reg32 const& base , Reg32 const& index , uint8 shift ){ return RefMem< RegPtr , 0 >( RegPtr( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr , 1 >  byte_ptr ( Reg32 const& base , Reg32 const& index , uint8 shift ){ return RefMem< RegPtr , 1 >( RegPtr( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr , 2 >  word_ptr ( Reg32 const& base , Reg32 const& index , uint8 shift ){ return RefMem< RegPtr , 2 >( RegPtr( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr , 4 >  dword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift ){ return RefMem< RegPtr , 4 >( RegPtr( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr , 8 >  qword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift ){ return RefMem< RegPtr , 8 >( RegPtr( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr , 10 > tword_ptr( Reg32 const& base , Reg32 const& index , uint8 shift ){ return RefMem< RegPtr , 10 >( RegPtr( base , index , shift ) );  }

#if TARGET_PLATFORM_64BITS
	// Memory addressing helper functions for 64-bit registers (Reg64)
	DEF_REF_MEM( RegPtr64, Reg64 const& base, RegPtr64( base ) )

	ASMETA_INLINE RefMem< RegPtr64 , 0 >  ptr      ( Reg64 const& base , int8 disp ){ return RefMem< RegPtr64 , 0 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 1 >  byte_ptr ( Reg64 const& base , int8 disp ){ return RefMem< RegPtr64 , 1 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 2 >  word_ptr ( Reg64 const& base , int8 disp ){ return RefMem< RegPtr64 , 2 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 4 >  dword_ptr( Reg64 const& base , int8 disp ){ return RefMem< RegPtr64 , 4 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 8 >  qword_ptr( Reg64 const& base , int8 disp ){ return RefMem< RegPtr64 , 8 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 10 > tword_ptr( Reg64 const& base , int8 disp ){ return RefMem< RegPtr64 , 10 >( RegPtr64( base , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr64 , 0 >  ptr      ( Reg64 const& base , int32 disp ){ return RefMem< RegPtr64 , 0 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 1 >  byte_ptr ( Reg64 const& base , int32 disp ){ return RefMem< RegPtr64 , 1 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 2 >  word_ptr ( Reg64 const& base , int32 disp ){ return RefMem< RegPtr64 , 2 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 4 >  dword_ptr( Reg64 const& base , int32 disp ){ return RefMem< RegPtr64 , 4 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 8 >  qword_ptr( Reg64 const& base , int32 disp ){ return RefMem< RegPtr64 , 8 >( RegPtr64( base , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 10 > tword_ptr( Reg64 const& base , int32 disp ){ return RefMem< RegPtr64 , 10 >( RegPtr64( base , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr64 , 0 >  ptr      ( Reg64 const& base , Reg64 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr64 , 0 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 1 >  byte_ptr ( Reg64 const& base , Reg64 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr64 , 1 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 2 >  word_ptr ( Reg64 const& base , Reg64 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr64 , 2 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 4 >  dword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr64 , 4 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 8 >  qword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr64 , 8 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 10 > tword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift , int32 disp ){ return RefMem< RegPtr64 , 10 >( RegPtr64( base , index , shift , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr64 , 0 >  ptr      ( Reg64 const& base , Reg64 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr64 , 0 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 1 >  byte_ptr ( Reg64 const& base , Reg64 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr64 , 1 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 2 >  word_ptr ( Reg64 const& base , Reg64 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr64 , 2 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 4 >  dword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr64 , 4 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 8 >  qword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr64 , 8 >( RegPtr64( base , index , shift , disp ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 10 > tword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift , int8 disp ){ return RefMem< RegPtr64 , 10 >( RegPtr64( base , index , shift , disp ) );  }

	ASMETA_INLINE RefMem< RegPtr64 , 0 >  ptr      ( Reg64 const& base , Reg64 const& index , uint8 shift ){ return RefMem< RegPtr64 , 0 >( RegPtr64( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 1 >  byte_ptr ( Reg64 const& base , Reg64 const& index , uint8 shift ){ return RefMem< RegPtr64 , 1 >( RegPtr64( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 2 >  word_ptr ( Reg64 const& base , Reg64 const& index , uint8 shift ){ return RefMem< RegPtr64 , 2 >( RegPtr64( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 4 >  dword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift ){ return RefMem< RegPtr64 , 4 >( RegPtr64( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 8 >  qword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift ){ return RefMem< RegPtr64 , 8 >( RegPtr64( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 10 > tword_ptr( Reg64 const& base , Reg64 const& index , uint8 shift ){ return RefMem< RegPtr64 , 10 >( RegPtr64( base , index , shift ) );  }
	ASMETA_INLINE RefMem< RegPtr64 , 16 > xmmword_ptr(Reg64 const& base, int32 disp) { return RefMem< RegPtr64 , 16 >(RegPtr64(base, disp)); }
	ASMETA_INLINE RefMem< RegPtr64 , 16 > xmmword_ptr(Reg64 const& base, Reg64 const& index, uint8 shift, int32 disp) { return RefMem< RegPtr64 , 16 >(RegPtr64(base, index, shift, disp)); }
#endif

#if TARGET_PLATFORM_64BITS
#	define SYSTEM_PTR qword_ptr
	//ASMETA_INLINE Disp< 8 > disp64(void* val) { return Disp< 8 >(val); }
#else
#	define SYSTEM_PTR dword_ptr
	ASMETA_INLINE Disp< 4 > disp32(void* val) { return Disp< 4 >(val); }
#endif

	ASMETA_INLINE Disp< 4 > disp32( uint32 val ){ return Disp< 4 >( val ); }
	ASMETA_INLINE Disp< 2 > disp16( uint16 val ){ return Disp< 2 >( val ); }
	ASMETA_INLINE Disp< 1 > disp8( uint8 val ) { return Disp< 1 >( val ); }

	ASMETA_INLINE Disp< 4 > disp( void* val ){ return Disp< 4 >( val ); }
	ASMETA_INLINE Disp< 4 > disp( uint32 val ){ return Disp< 4 >( val ); }
	ASMETA_INLINE Disp< 2 > disp( uint16 val ){ return Disp< 2 >( val ); }
	ASMETA_INLINE Disp< 1 > disp( uint8 val ) { return Disp< 1 >( val ); }

	ASMETA_INLINE RefMem< LabelPtr , 16 > xmmword_ptr(Label* label, int32 offset) { return RefMem< LabelPtr , 16 >(LabelPtr(label, offset)); }
	ASMETA_INLINE RefMem< RegPtr , 16 > xmmword_ptr(Reg32 const& base, int32 disp) { return RefMem< RegPtr , 16 >(RegPtr(base, disp)); }

	ASMETA_INLINE Immediate< 4 > imm32( int32 val ){ return Immediate< 4 >( val , true ); }
	ASMETA_INLINE Immediate< 2 > imm16( int16 val ){ return Immediate< 2 >( val , true ); }
	ASMETA_INLINE Immediate< 1 > imm8( int8 val ){ return Immediate< 1 >( val , true ); }
	ASMETA_INLINE Immediate< 4 > imm32u( uint32 val ){ return Immediate< 4 >( val ); }
	ASMETA_INLINE Immediate< 2 > imm16u( uint16 val ){ return Immediate< 2 >( val ); }
	ASMETA_INLINE Immediate< 1 > imm8u( uint8 val ){ return Immediate< 1 >( val ); }

	ASMETA_INLINE Immediate< 0 > imm( int32 val ){ return Immediate< 0 >( val , true ); }
	ASMETA_INLINE Immediate< 0 > imm( int16 val ){ return Immediate< 0 >( val , true ); }
	ASMETA_INLINE Immediate< 0 > imm( int8 val ){ return Immediate< 0 >( val , true ); }
	ASMETA_INLINE Immediate< 0 > imm( uint32 val ){ return Immediate< 0 >( val ); }
	ASMETA_INLINE Immediate< 0 > imm( uint16 val ){ return Immediate< 0 >( val ); }
	ASMETA_INLINE Immediate< 0 > imm( uint8 val ){ return Immediate< 0 >( val ); }

#if TARGET_PLATFORM_64BITS
	ASMETA_INLINE Immediate< 8 > imm64( int64 val ){ return Immediate< 8 >( val , true ); }
	ASMETA_INLINE Immediate< 8 > imm64u( uint64 val ){ return Immediate< 8 >( val ); }
#endif


	template< class T >
	class AssemblerT
	{
		T* _this(){ return static_cast< T* >( this );  }
	public:	
		void     emitByte( uint8 byte ){}
		void     emitWord( uint8 byte1 , uint8 byte2 ){}
		void     emitWord( uint16 val ){}
		void     emitDWord( uint32 val ){}
#if TARGET_PLATFORM_64BITS
		void     emitQWord(uint64 val) {}
#endif
		void     emitPtr ( void* ptr ){}
		uint32   getOffset(){ assert( 0 ); return 0; }

	public:
		AssemblerT()
		{ 

		}

		~AssemblerT()
		{
			clearLink();
		}

		ASMETA_INLINE void aaa(){  encodeByteInist( 0x37 );  }
		ASMETA_INLINE void aad(){  encodeWordInist( 0xd5 , 0x0a ); }
		ASMETA_INLINE void aam(){  encodeWordInist( 0xd4 , 0x0a ); }
		ASMETA_INLINE void aas(){  encodeByteInist( 0x37 );  }

		ASMETA_INLINE void arpl(){  encodeByteInist( 0x37 );  }

		template< int Size >
		ASMETA_INLINE void mov( RegX86< Size > const& dst , RegX86< Size > const& src )      { encodeIntInist< Size >( igMOV , dst , src.code() ); }
		template< class Ref , int Size >
		ASMETA_INLINE void mov( RefMem< Ref , Size > const& dst , RegX86< Size > const& src ){  encodeIntInist< Size >( igMOV , dst , src.code() ); }
		template< class Ref , int Size >
		ASMETA_INLINE void mov( RegX86< Size > const& dst , RefMem< Ref , Size > const& src ){  encodeIntInistR< Size >( igMOV , src , dst.code() ); }
		
		template< class Ref, int Size , int Size2 >
		ASMETA_INLINE void mov(RegX86< Size > const& dst, RefMem< Ref, Size2 > const& src) { encodeIntInistR< Size >(igMOV, src, dst.code()); }

		ASMETA_INLINE void mov( RegX86< 2 > const& dst ,  RegSeg const& src ){  encodeIntInistRM< 2 >( 0x8c , dst , src.code()  ); }
		ASMETA_INLINE void mov( RegSeg const& dst , RegX86< 2 > const& src ) {  encodeIntInistRM< 2 >( 0x8e , src  , dst.code()  ); }
		template< class Ref >
		ASMETA_INLINE void mov( RefMem< Ref , 2 > const& dst ,  RegSeg const& src ){  encodeIntInistRM< 2 >( 0x8c , dst.ref() , src.code() ); }
		template< class Ref >
		ASMETA_INLINE void mov( RegSeg const& dst , RefMem< Ref , 2 >const& src ) {  encodeIntInistRM< 2 >( 0x8e , src.ref()  , dst.code() ); }

#if TARGET_PLATFORM_64BITS
		// movabs r64, imm64 - move 64-bit immediate value to 64-bit register
		// Opcode: REX.W + B8+rd io
		ASMETA_INLINE void movabs( Reg64 const& dst, Immediate< 8 > const& imm )
		{
			// REX.W prefix is required for 64-bit operand
			encodeREXPrefixSingle<8>(dst.code());
			_this()->emitByte(0xB8 | Reg::lowBits(dst.code()));
			_this()->emitQWord(imm.value());
		}

		// movabs r64, imm64 - alternative using raw int64
		ASMETA_INLINE void movabs( Reg64 const& dst, int64 imm )
		{
			movabs(dst, imm64(imm));
		}

		// movabs r64, imm64 - alternative using raw uint64
		ASMETA_INLINE void movabs( Reg64 const& dst, uint64 imm )
		{
			movabs(dst, imm64u(imm));
		}

		// LEA r64, m - Load Effective Address
		template< int Size >
		ASMETA_INLINE void lea( Reg64 const& dst, RefMem< RegPtr64, Size > const& src )
		{
			encodeREXPrefixMem<8>(dst.code(), src.reference());
			_this()->emitByte(0x8D);
			encodeModRM(src.reference(), Reg::lowBits(dst.code()));
		}
#endif


#define DEF_ALU_INST( NAME , CODE )\
	template< int Size >\
	ASMETA_INLINE void NAME( RegX86< Size > const& dst , RegX86< Size > const& src )            {  encodeIntInist< Size >( CODE , dst , src.code() ); }\
	template< class Ref , int Size >\
	ASMETA_INLINE void NAME( RefMem< Ref , Size > const& dst , RegX86< Size > const& src )      {  encodeIntInist< Size >( CODE , dst , src.code() ); }\
	template< class Ref , int Size >\
	ASMETA_INLINE void NAME( RegX86< Size > const& dst , RefMem< Ref , Size > const& src )      {  encodeIntInistR< Size >( CODE , src , dst.code() ); }\
	template< int Size , int ImmSize >\
	ASMETA_INLINE void NAME( RegX86< Size > const& dst , Immediate< ImmSize > const& imm )      {  encodeALUInist< Size >( CODE , dst , imm );  }\
	template< class Ref , int Size , int ImmSize >\
	ASMETA_INLINE void NAME( RefMem< Ref , Size > const& dst , Immediate< ImmSize > const& imm ){  encodeIntInistSW< Size >( CODE , dst , imm );  }

		DEF_ALU_INST(adc, igADC);
		DEF_ALU_INST(add, igADD);
		DEF_ALU_INST(sub, igSUB);
		DEF_ALU_INST(and, igAND);
		DEF_ALU_INST( or, igOR);
		DEF_ALU_INST(xor, igXOR);

#undef DEF_ALU_INST


		template< int Size >
		ASMETA_INLINE void cmp( RegX86< Size > const& dst , RegX86< Size > const& src )            {  encodeIntInist< Size >( igCMP , dst , src.code() ); }
		template< class Ref , int Size >
		ASMETA_INLINE void cmp( RefMem< Ref , Size > const& dst , RegX86< Size > const& src )      {  encodeIntInist< Size >( igCMP , dst , src.code() ); }
		template< class Ref , int Size >
		ASMETA_INLINE void cmp( RegX86< Size > const& dst , RefMem< Ref , Size > const& src )      {  encodeIntInistR< Size >( igCMP , src  , dst.code() ); }
		template< int Size , int ImmSize >	
		ASMETA_INLINE void cmp( RegX86< Size > const& dst , Immediate< ImmSize > const& imm )      {  encodeALUInist< Size >( igCMP , dst , imm );  }
		template< class Ref , int Size , int ImmSize >	
		ASMETA_INLINE void cmp( RefMem< Ref , Size > const& dst , Immediate< ImmSize > const& imm ){  encodeIntInistSW< Size >( igCMP , dst , imm );  }

		template< int Size >
		ASMETA_INLINE void test( RegX86< Size > const& reg1 , RegX86< Size > const& reg2 )     { encodeIntInist< Size >( igTEST , reg1 , reg2.code() ); }
		template< class Ref ,int Size >
		ASMETA_INLINE void test(  RefMem< Ref , Size > const& mem , RegX86< Size > const& reg ){ encodeIntInist< Size >( igTEST , mem , reg.code() ); }
		template< class Ref ,int Size >
		ASMETA_INLINE void test(  RegX86< Size > const& reg , RefMem< Ref , Size > const& mem ){ encodeIntInist< Size >( igTEST , mem , reg.code() ); }
		template< class Ref ,int Size , int ImmSize >
		ASMETA_INLINE void test(  RefMem< Ref , Size > const& mem , Immediate< ImmSize > const& imm ){  encodeIntInistSW< Size >( igTEST , mem , imm );  }
		template< int Size , int ImmSize >
		ASMETA_INLINE void test( RegX86< Size > const& reg , Immediate< ImmSize > const& imm )
		{
			if ( reg.code() == Reg::eAX )
				encodeAccumInist< Size >( 0xa8 , imm );
			else
				encodeIntInistRMI< Size >( igTEST , reg , imm , 0 , 1 );
		}

		template< class Ref , int Size >
		ASMETA_INLINE void neg( RefMem< Ref , Size > const& src ){  encodeIntInist2< Size >( igNEG , src ); }
		template< int Size >
		ASMETA_INLINE void neg( RegX86< Size > const& src )      {  encodeIntInist2< Size >( igNEG , src ); }

		template< class Ref , int Size >
		ASMETA_INLINE void not( RefMem< Ref , Size > const& src ){  encodeIntInist2< Size >( igNOT , src ); }
		template< int Size >
		ASMETA_INLINE void not( RegX86< Size > const& src )      {  encodeIntInist2< Size >( igNOT , src ); }

		template< class Ref , int Size >
		ASMETA_INLINE void mul( RefMem< Ref , Size > const& src ){  encodeIntInist2< Size >( igMUL , src ); }
		template< int Size >
		ASMETA_INLINE void mul( RegX86< Size > const& src )      {  encodeIntInist2< Size >( igMUL , src ); }

		template< class Ref , int Size >
		ASMETA_INLINE void imul( RefMem< Ref , Size > const& src ){  encodeIntInist2< Size >( igIMUL , src ); }
		template< int Size >
		ASMETA_INLINE void imul( RegX86< Size > const& src )      {  encodeIntInist2< Size >( igIMUL , src ); }

		template< class Ref , int Size >
		ASMETA_INLINE void imul( RegX86< Size > const& src , RefMem< Ref , Size > const& mem )
		{  static_assert( Size != 1 ) ; encodeIntInist4< Size >( 0x0f , 0xaf , mem.ref() , src.code() ); }
		template< int Size >
		ASMETA_INLINE void imul( RegX86< Size > const& src , RegX86< Size > const& reg )
		{  static_assert( Size != 1 ) ; encodeIntInist4< Size >( 0x0f , 0xaf , reg , src.code() ); }
		template< int Size , int ImmSize >
		ASMETA_INLINE void imul( RegX86< Size > const& dst , Immediate< ImmSize > const& imm ){ imul( dst ,dst ,imm ); }

		template< int Size , class Ref , int ImmSize >
		ASMETA_INLINE void imul( RegX86< Size > const& dst , RefMem< Ref , Size > const& mem , Immediate< ImmSize > const& imm )
		{  static_assert( Size != 1 ); encodeIntInistRMI< Size >( 0x69 , mem , dst.code() , imm ); }
		template< int Size , int ImmSize >
		ASMETA_INLINE void imul( RegX86< Size > const& dst , RegX86< Size > const& reg , Immediate< ImmSize > const& imm )
		{  static_assert( Size != 1 ) ; encodeIntInistRMI< Size >( 0x69 , reg , dst.code() , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void idiv( RefMem< Ref , Size > const& src ){  encodeIntInist2< Size >( igIDIV , src ); }
		template< int Size >
		ASMETA_INLINE void idiv( RegX86< Size > const& src )      {  encodeIntInist2< Size >( igIDIV , src ); }

		template< class Ref , int Size >
		ASMETA_INLINE void div( RefMem< Ref , Size > const& src ){  encodeIntInist2< Size >( igIDIV , src ); }
		template< int Size >
		ASMETA_INLINE void div( RegX86< Size > const& src )      {  encodeIntInist2< Size >( igIDIV , src ); }

		template< class Ref , int Size >
		ASMETA_INLINE void shl( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSHL , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void shl( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSHL , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void shl( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igSHL , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void shl( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igSHL , dst , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void shr( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSHR , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void shr( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSHR , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void shr( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igSHR , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void shr( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igSHR , dst , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void sal( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSAL , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void sal( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSAL , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void sal( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igSAL , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void sal( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igSAL , dst , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void sar( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSAR , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void sar( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igSAR , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void sar( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igSAR , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void sar( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igSAR , dst , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void rol( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igROL , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void rol( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igROL , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rol( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igROL , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void rol( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igROL , dst , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void ror( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igROR , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void ror( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igROR , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void ror( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igROR , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void ror( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igROR , dst , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void rcl( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igRCL , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void rcl( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igRCL , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rcl( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igRCL , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void rcl( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igRCL , dst , imm ); }

		template< class Ref , int Size >
		ASMETA_INLINE void rcr( RefMem< Ref , Size > const& dst , RegX86< 1 > const& src  )   {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igRCR , dst , 0x2 ); }
		template< int Size >
		ASMETA_INLINE void rcr( RegX86< Size > const& dst , RegX86< 1 > const& src )         {  assert( src.code() == Reg::eCL ); encodeIntInist2< Size >( igRCR , dst , 0x2 ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rcr( RefMem< Ref , Size > const& dst , Immediate< 1 > const& imm ){  encodeSRInist< Size >( igRCR , dst.ref() , imm ); }
		template< int Size >
		ASMETA_INLINE void rcr( RegX86< Size > const& dst , Immediate< 1 > const& imm )      {  encodeSRInist< Size >( igRCR , dst , imm ); }


		template< int Size , class RMType >
		ASMETA_INLINE void encodeSRInist( IntInstCode code , RMType const& dst , Immediate< 1 > const& imm )
		{
			if ( imm.value() == 1 )
				encodeIntInist2< Size >( code , dst );
			else
				encodeIntInist< Size >( code , dst , imm , 0 );
		}

		template< class Ref , int Size >
		ASMETA_INLINE void movs( RefMem< Ref , Size > const&  ){  encodeIntInistW< Size >( igMOVS ); }
		ASMETA_INLINE void movsb(){  encodeIntInistW< 1 >( igMOVS ); }
		ASMETA_INLINE void movsw(){  encodeIntInistW< 2 >( igMOVS ); }
		ASMETA_INLINE void movsd(){  encodeIntInistW< 4 >( igMOVS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rep_movs( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf3 );  encodeIntInistW< Size >( igMOVS ); }
		ASMETA_INLINE void rep_movsb(){  encodeByteInist( 0xf3 );  encodeIntInistW< 1 >( igMOVS ); }
		ASMETA_INLINE void rep_movsw(){  encodeByteInist( 0xf3 );  encodeIntInistW< 2 >( igMOVS ); }
		ASMETA_INLINE void rep_movsd(){  encodeByteInist( 0xf3 );  encodeIntInistW< 4 >( igMOVS ); }

		template< class Ref , int Size >
		ASMETA_INLINE void ins( RefMem< Ref , Size > const&  ){  encodeIntInistW< Size >( igINS ); }
		ASMETA_INLINE void insb(){  encodeIntInistW< 1 >( igINS ); }
		ASMETA_INLINE void insw(){  encodeIntInistW< 2 >( igINS ); }
		ASMETA_INLINE void insd(){  encodeIntInistW< 4 >( igINS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rep_ins( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf3 );  encodeIntInist< Size >( igINS ); }
		ASMETA_INLINE void rep_insb(){  encodeByteInist( 0xf3 );  encodeIntInistW< 1 >( igINS ); }
		ASMETA_INLINE void rep_insw(){  encodeByteInist( 0xf3 );  encodeIntInistW< 2 >( igINS ); }
		ASMETA_INLINE void rep_insd(){  encodeByteInist( 0xf3 );  encodeIntInistW< 4 >( igINS ); }

		template< class Ref , int Size >
		ASMETA_INLINE void lods( RefMem< Ref , Size > const&  ){  encodeIntInistW< Size >( igLODS ); }
		ASMETA_INLINE void lodsb(){  encodeIntInistW< 1 >( igLODS ); }
		ASMETA_INLINE void lodsw(){  encodeIntInistW< 2 >( igLODS ); }
		ASMETA_INLINE void lodsd(){  encodeIntInistW< 4 >( igLODS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rep_lods( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf3 );  encodeIntInistW< Size >( igLODS ); }
		ASMETA_INLINE void rep_lodsb(){  encodeByteInist( 0xf3 );  encodeIntInistW< 1 >( igLODS ); }
		ASMETA_INLINE void rep_lodsw(){  encodeByteInist( 0xf3 );  encodeIntInistW< 2 >( igLODS ); }
		ASMETA_INLINE void rep_lodsd(){  encodeByteInist( 0xf3 );  encodeIntInistW< 4 >( igLODS ); }

		template< class Ref , int Size >
		ASMETA_INLINE void outs( RefMem< Ref , Size > const&  ){  encodeIntInistW< Size >( igOUTS ); }
		ASMETA_INLINE void outsb(){  encodeIntInistW< 1 >( igOUTS ); }
		ASMETA_INLINE void outsw(){  encodeIntInistW< 2 >( igOUTS ); }
		ASMETA_INLINE void outsd(){  encodeIntInistW< 4 >( igOUTS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rep_outs( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf3 );  encodeIntInistW< Size >( igOUTS ); }
		ASMETA_INLINE void rep_outsb(){  encodeByteInist( 0xf3 );  encodeIntInistW< 1 >( igOUTS ); }
		ASMETA_INLINE void rep_outsw(){  encodeByteInist( 0xf3 );  encodeIntInistW< 2 >( igOUTS ); }
		ASMETA_INLINE void rep_outsd(){  encodeByteInist( 0xf3 );  encodeIntInistW< 4 >( igOUTS ); }

		template< class Ref , int Size >
		ASMETA_INLINE void stos( RefMem< Ref , Size > const&  ){  encodeIntInistW< Size >( igSTOS ); }
		ASMETA_INLINE void stosb(){  encodeIntInistW< 1 >( igSTOS ); }
		ASMETA_INLINE void stosw(){  encodeIntInistW< 2 >( igSTOS ); }
		ASMETA_INLINE void stosd(){  encodeIntInistW< 4 >( igSTOS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void rep_stos( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf3 );  encodeIntInistW< Size >( igSTOS ); }
		ASMETA_INLINE void rep_stosb(){  encodeByteInist( 0xf3 );  encodeIntInistW< 1 >( igSTOS ); }
		ASMETA_INLINE void rep_stosw(){  encodeByteInist( 0xf3 );  encodeIntInistW< 2 >( igSTOS ); }
		ASMETA_INLINE void rep_stosd(){  encodeByteInist( 0xf3 );  encodeIntInistW< 4 >( igSTOS ); }

		template< class Ref , int Size >
		ASMETA_INLINE void scas( RefMem< Ref , Size > const&  ){  encodeIntInistW< Size >( igSCAS ); }
		ASMETA_INLINE void scasb(){  encodeIntInistW< 1 >( igSCAS ); }
		ASMETA_INLINE void scasw(){  encodeIntInistW< 2 >( igSCAS ); }
		ASMETA_INLINE void scasd(){  encodeIntInistW< 4 >( igSCAS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void repe_scas( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf3 );  encodeIntInistW< Size >( igSCAS ); }
		ASMETA_INLINE void repe_scasb(){  encodeByteInist( 0xf3 );  encodeIntInistW< 1 >( igSCAS ); }
		ASMETA_INLINE void repe_scasw(){  encodeByteInist( 0xf3 );  encodeIntInistW< 2 >( igSCAS ); }
		ASMETA_INLINE void repe_scasd(){  encodeByteInist( 0xf3 );  encodeIntInistW< 4 >( igSCAS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void repne_scas( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf2 );  encodeIntInistW< Size >( igSCAS ); }
		ASMETA_INLINE void repne_scasb(){  encodeByteInist( 0xf2 );  encodeIntInistW< 1 >( igSCAS ); }
		ASMETA_INLINE void repne_scasw(){  encodeByteInist( 0xf2 );  encodeIntInistW< 2 >( igSCAS ); }
		ASMETA_INLINE void repne_scasd(){  encodeByteInist( 0xf2 );  encodeIntInistW< 4 >( igSCAS ); }

		template< class Ref , int Size >
		ASMETA_INLINE void cmps( RefMem< Ref , Size > const&  ){  encodeIntInistW< Size >( igCMPS ); }
		ASMETA_INLINE void c(){  encodeIntInistW< 1 >( igCMPS ); }
		ASMETA_INLINE void cmpsw(){  encodeIntInistW< 2 >( igCMPS ); }
		ASMETA_INLINE void cmpsd(){  encodeIntInistW< 4 >( igCMPS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void repe_cmps( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf3 );  encodeIntInistW< Size >( igCMPS ); }
		ASMETA_INLINE void repe_cmpsb(){  encodeByteInist( 0xf3 );  encodeIntInistW< 1 >( igCMPS ); }
		ASMETA_INLINE void repe_cmpsw(){  encodeByteInist( 0xf3 );  encodeIntInistW< 2 >( igCMPS ); }
		ASMETA_INLINE void repe_cmpsd(){  encodeByteInist( 0xf3 );  encodeIntInistW< 4 >( igCMPS ); }
		template< class Ref , int Size >
		ASMETA_INLINE void repne_cmps( RefMem< Ref , Size > const& ){ encodeByteInist( 0xf2 );  encodeIntInistW< Size >( igCMPS ); }
		ASMETA_INLINE void repne_cmpsb(){  encodeByteInist( 0xf2 );  encodeIntInistW< 1 >( igCMPS ); }
		ASMETA_INLINE void repne_cmpsw(){  encodeByteInist( 0xf2 );  encodeIntInistW< 2 >( igCMPS ); }
		ASMETA_INLINE void repne_cmpsd(){  encodeByteInist( 0xf2 );  encodeIntInistW< 4 >( igCMPS ); }

		ASMETA_INLINE void clc(){ encodeByteInist( 0xf8 ); }
		ASMETA_INLINE void cld(){ encodeByteInist( 0xfc ); }
		ASMETA_INLINE void cli(){ encodeByteInist( 0xfa ); }
		ASMETA_INLINE void stc(){ encodeByteInist( 0xf8 | 0x1 ); }
		ASMETA_INLINE void std(){ encodeByteInist( 0xfc | 0x1 ); }
		ASMETA_INLINE void sti(){ encodeByteInist( 0xfa | 0x1 ); }

		template< class Ref , int Size >
		ASMETA_INLINE void push( RefMem< Ref , Size > const& mem ) { encodePushPopInist< Size >( igPUSH , mem ); }
		template< int Size >
		ASMETA_INLINE void push( RegX86< Size > const& reg )       {  encodePushPopInist< Size >( igPUSH , reg ); }
		template< int ImmSize >
		ASMETA_INLINE void push( Immediate< ImmSize > const& imm ) {  encodeIntInist< ImmSize >( igPUSH , imm );  }

		template< class Ref , int Size >
		ASMETA_INLINE void pop( RefMem< Ref , Size > const& mem )  {  encodePushPopInist< Size >( igPOP , mem ); }
		template< int Size >
		ASMETA_INLINE void pop( RegX86< Size > const& reg )        {  encodePushPopInist< Size >( igPOP , reg ); }

		ASMETA_INLINE void ret() { encodeByteInist( 0xc3 ); }
		ASMETA_INLINE void ret( Immediate< 2 > const& imm )
		{ 
			encodeByteInist( 0xc2 );
			encodeImmediateNoForce( imm );
		}

		template< int Size >
		ASMETA_INLINE void xchg( RegX86< Size > const& reg1 , RegX86< Size > const& reg2 )
		{
			if ( reg1.code() == Reg::eAX )
				encodeIntInistW< Size >( 0x90 | reg2.code() );
			else if ( reg2.code() == Reg::eAX )
				encodeIntInistW< Size >( 0x90 | reg1.code() );
			else
				encodeIntInistWRM< Size >( 0x86 , reg2 , reg1.code() );
		}

		template< class Ref ,int Size >
		ASMETA_INLINE void xchg( RegX86< Size > const& reg , RefMem< Ref , Size > const& mem ){  encodeIntInistWRM< Size >( 0x86 , mem , reg.code() );  }
		template< class Ref ,int Size >
		ASMETA_INLINE void xchg( RefMem< Ref , Size > const& mem , RegX86< Size > const& reg ){ xchg( reg , mem ); }

		template< int Size >
		ASMETA_INLINE void xadd( RegX86< Size > const& reg1 , RegX86< Size > const& reg2 )     {  encodeByteInist( 0x0f ); encodeIntInistWRM< Size >( 0xc0 , reg2 , reg1.code() );  }
		template< class Ref ,int Size >
		ASMETA_INLINE void xadd(  RefMem< Ref , Size > const& mem , RegX86< Size > const& reg ){  encodeByteInist( 0x0f ); encodeIntInistWRM< Size >( 0xc0 , mem , reg.code() ); }

		template< int DispSize >
		ASMETA_INLINE void  jo  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_O , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jno ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NO , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jb  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_B , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnb ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NB , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  je  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_E , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jne ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jbe ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_BE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnbe( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NBE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  js  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_S , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jns ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NS , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jp  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_P , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnp ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NP , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jl  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_L , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnl ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NL , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jle ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_LE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnle( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NLE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jae ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_AE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnae( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NAE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jz  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_Z , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnz ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NZ , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  ja  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_A , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jna ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NA , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jpe ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_PE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jpo ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_PO , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jge ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_GE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jnge( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NGE , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jg  ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_G , disp );  }
		template< int DispSize >
		ASMETA_INLINE void  jng ( Disp< DispSize > const& disp ){  encodeJccInist( CTF_NG , disp );  }


		template< int DispSize >
		ASMETA_INLINE void encodeJccInist( CondTestField cond , Disp< DispSize > const& disp )
		{
			encodeJccInistImpl( cond , Int2Type< DispSize >() );
			encodeDisp( disp );
		}

		ASMETA_INLINE void encodeJccInistImpl( CondTestField cond , Int2Type< 4 > )
		{
			_this()->emitByte( INT_INIST_OPB( igJCC ) );
			_this()->emitByte( ( INT_INIST_OPR( igJCC ) << 4 ) | cond );
		}
		ASMETA_INLINE void encodeJccInistImpl( CondTestField cond  , Int2Type< 2 > )
		{
			_this()->emitByte( INT_INIST_OPA( igJCC ) | cond );
		}

		template< int Size , class RMType >
		ASMETA_INLINE void encodePushPopInist( IntInstCode code , RMType const& rm )
		{
			assert( code == igPOP || code == igPUSH );
#if TARGET_PLATFORM_64BITS
			static_assert( Size == 2 || Size == 4 || Size == 8 );
			if constexpr (Size == 8)
			{
				// In 64-bit mode, push/pop 64-bit registers use the short encoding (50+r for push, 58+r for pop)
				// or FF /6 for push r/m64, 8F /0 for pop r/m64
				if constexpr (std::is_same_v<RMType, RegX86<8>>)
				{
					// Short encoding for register push/pop (no REX needed unless extended register)
					uint8 baseOp = (code == igPUSH) ? 0x50 : 0x58;
					if (Reg::isExtended(rm.code()))
						encodeREXPrefix(0, 0, 0, 1);
					_this()->emitByte(baseOp | Reg::lowBits(rm.code()));
				}
				else
				{
					// Memory operand - use FF /6 for push or 8F /0 for pop
					encodeIntInist2< Size >( code , rm );
				}
			}
			else
#else
			static_assert( Size == 2 || Size == 4 );
#endif
			{
				encodeIntInist2< Size >( code , rm );
			}
		}

		template< class Ref , int Size >
		ASMETA_INLINE void call( RefMem< Ref , Size > const& ptr ){  encodeIntInist2< 4 >( igCALL , ptr ); }
		ASMETA_INLINE void call( Reg32 const& reg )               {  encodeIntInist2< 4 >( igCALL , reg ); }

#if TARGET_PLATFORM_64BITS
		// x64 call instruction: call r64
		ASMETA_INLINE void call( Reg64 const& reg )
		{
			// In 64-bit mode, CALL r64 uses FF /2 with no REX.W needed (default 64-bit)
			// But REX.B is needed for extended registers (R8-R15)
			if (Reg::isExtended(reg.code()))
				encodeREXPrefix(0, 0, 0, 1);
			_this()->emitByte(0xFF);
			encodeModRM(reg, Reg::eDX); // /2 = opcode extension in reg field
		}

		// x64 call with 64-bit memory operand
		template< int Size >
		ASMETA_INLINE void call( RefMem< RegPtr64, Size > const& ptr )
		{
			encodeREXPrefixMem<Size>(Reg::eDX, ptr.reference());  // /2 in reg field
			_this()->emitByte(0xFF);
			encodeModRM(ptr.reference(), Reg::eDX);
		}
#endif


		ASMETA_INLINE void encodeByteInist( uint8 opA )
		{
			_this()->emitByte( opA );
		}

		ASMETA_INLINE void encodeWordInist( uint8 opA , uint8 opB )
		{
			_this()->emitWord( opA , opB );
		}

		template< int Size >
		ASMETA_INLINE void encodeIntInist( IntInstCode code )
		{
			encodeIntInist< Size >( INT_INIST_OPA( code ) );
		}

		template< int Size >
		ASMETA_INLINE void encodeIntInistW( IntInstCode code )
		{
			encodeIntInistW< Size >( INT_INIST_OPA( code ) );
		}

		template< int Size , class RMType >
		ASMETA_INLINE void encodeIntInist( IntInstCode code , RMType const& rm , Reg::Code reg  )
		{
			encodeIntInistWRM< Size >( INT_INIST_OPA( code ) , rm , reg );
		}

		template< int Size , class RMType >
		ASMETA_INLINE void encodeIntInistR( IntInstCode code , RMType const& rm , Reg::Code reg  )
		{
			encodeIntInistWRM< Size >( INT_INIST_OPA( code ) | ( 0x1 << 1 ) , rm , reg );
		}

		template< int Size , class RMType >
		ASMETA_INLINE void encodeIntInist2( IntInstCode code , RMType const& rm , uint8 addCode = 0 )
		{
			encodeIntInistWRM< Size >( INT_INIST_OPA( code ) | addCode , rm , INT_INIST_OPR( code ) );
		}

		template< int Size , int ImmSize >
		ASMETA_INLINE void encodeALUInist( IntInstCode code , RegX86< Size > const& reg , Immediate< ImmSize > const& imm )
		{
			if ( reg.code() == Reg::eAX )
			{
#if TARGET_PLATFORM_64BITS
				if constexpr (Size == 8)
				{
					// For 64-bit RAX operations, emit REX.W prefix first
					encodeREXPrefix(1, 0, 0, 0);
					_this()->emitByte(INT_INIST_OPA(code) | 0x04 | 0x01);
					encodeImmediateForce<4>(imm);
				}
				else
#endif
				{
					encodeAccumInist< Size >( INT_INIST_OPA( code ) | 0x04 , imm );
				}
			}
			else
				encodeIntInistSW< Size >( code , reg , imm );
		}


		template< int Size >
		ASMETA_INLINE void encodeIntInist( uint8 op )
		{
			encodeInt16Prefix< Size >();
			 _this()->emitByte( op );
		}

		template< int Size >
		ASMETA_INLINE void encodeIntInistS( uint8 op )
		{
			uint8 s = Size > imm.size() ? 1 : 0 ;
			encodeIntInist< Size >( op | ( s << 1 ) );
		}

		template< int Size , int ImmSize >
		ASMETA_INLINE void encodeIntInistSW( uint8 op , Immediate< ImmSize > const& imm )
		{
			uint8 s = ( Size > 1 && imm.size() == 1 ) ? 1 : 0 ;
			encodeIntInistW< Size >( op | ( s << 1 ) );
		}

		template< int Size >
		ASMETA_INLINE void encodeIntInistW( uint8 op )
		{
			encodeInt16Prefix< Size >();
			if constexpr (Size == 1)
			{
				_this()->emitByte(op);
			}
			else
			{
				_this()->emitByte(op | 0x01);
			}
		}

		template< int Size , class RMType >
		ASMETA_INLINE void encodeIntInist4( uint8 opA , uint8 opB , RMType const& rm , uint8 reg )
		{
			encodeInt16Prefix< Size >();
			//encodeSemgentPrefix( rm );
			_this()->emitByte( opA );
			_this()->emitByte( opB );
			encodeModRM( rm , reg );
		}

		template< int Size , class RMType >
		ASMETA_INLINE void encodeIntInistWRM( uint8 op , RMType const& rm , uint8 reg )
		{
#if TARGET_PLATFORM_64BITS
			if constexpr (Size == 8)
			{
				// For 64-bit operations, emit REX.W prefix
				// We need to check if we're dealing with register or memory operand
				if constexpr (std::is_same_v<RMType, RegX86<8>>)
				{
					encodeREXPrefixReg<Size>((Reg::Code)reg, rm.code());
				}
				else if constexpr (std::is_same_v<RMType, RegPtr64>)
				{
					encodeREXPrefixMem<Size>((Reg::Code)reg, rm);
				}
				else
				{
					// Default: just emit REX.W
					encodeREXPrefix(1, Reg::isExtended((Reg::Code)reg) ? 1 : 0, 0, 0);
				}
				_this()->emitByte(op | 0x01);
				encodeModRM(rm, reg);
			}
			else
#endif
			{
				encodeIntInistW< Size >( op );
				encodeModRM( rm , reg );
			}
		}

		template< int Size , class RMType >
		ASMETA_INLINE void encodeIntInistRM( uint8 op , RMType const& rm , uint8 reg  )
		{
			encodeIntInist< Size >( op );
			encodeModRM( rm , reg );
		}

		template< int Size >
		ASMETA_INLINE void encodeInt16Prefix()
		{ 
			if constexpr (Size == 2)
			{
				_this()->emitByte(0x66);
			}
		}

		template< int Size , int ImmSize >
		ASMETA_INLINE void encodeAccumInist( uint8 opA , Immediate< ImmSize > const& imm )
		{
			encodeIntInistW< Size >( opA );
			encodeImmediateForce< Size >( imm );
		}

		template< int Size , int ImmSize >
		ASMETA_INLINE void encodeIntInist( IntInstCode code , Immediate< ImmSize > const& imm )
		{
			encodeInt16Prefix< Size >();
			static_assert( Size >= ImmSize );
			encodeIntInistS< Size >( INT_INIST_OPB( code ) );
			encodeImmediateNoForce( imm );
		}

		template< int Size , class RMType , int ImmSize >
		ASMETA_INLINE void encodeIntInistSW( IntInstCode code , RMType const& rm , Immediate< ImmSize > const& imm  )
		{
			static_assert( Size >= ImmSize );
#if TARGET_PLATFORM_64BITS
			if constexpr (Size == 8)
			{
				// For 64-bit operations, emit REX.W prefix
				if constexpr (std::is_same_v<RMType, RegX86<8>>)
				{
					uint8 b = Reg::isExtended(rm.code()) ? 1 : 0;
					encodeREXPrefix(1, 0, 0, b);
				}
				else if constexpr (std::is_same_v<RMType, RegPtr64>)
				{
					encodeREXPrefixMem<Size>(INT_INIST_OPR(code), rm);
				}
				else
				{
					encodeREXPrefix(1, 0, 0, 0);
				}
				// Use imm32 sign-extended for 64-bit operations (opcode 0x81)
				// s bit = 0 for imm32
				uint8 s = (imm.size() == 1) ? 1 : 0;
				_this()->emitByte(INT_INIST_OPB(code) | (s << 1) | 0x01);
				encodeModRM(rm, INT_INIST_OPR(code));
				if (s)
					encodeImmediateForce<1>(imm);
				else
					encodeImmediateForce<4>(imm);
			}
			else
#endif
			{
				encodeIntInistSW< Size >( INT_INIST_OPB( code ) , imm );
				encodeModRM( rm , INT_INIST_OPR( code ) );
				encodeImmediateNotForce( imm );
			}
		}

		template< int Size , class RMType , int ImmSize >
		ASMETA_INLINE void encodeIntInistW( IntInstCode code , RMType const& rm , Immediate< ImmSize > const& imm  )
		{
			static_assert( Size >= ImmSize );
			encodeIntInistW< Size >( INT_INIST_OPB( code ) );
			encodeModRM( rm , INT_INIST_OPR( code ) );
			encodeImmediateNotForce( imm );
		}

		template< int Size , class RMType , int ImmSize >
		ASMETA_INLINE void encodeIntInistWF( IntInstCode code , RMType const& rm , Immediate< ImmSize > const& imm )
		{
			static_assert( Size >= ImmSize );
			encodeIntInistW< Size >( INT_INIST_OPB( code ) );
			encodeModRM( rm , INT_INIST_OPR( code ) );
			encodeImmediateForce< Size >( imm );
		}

		template< int Size , class RMType , int ImmSize >
		ASMETA_INLINE void encodeIntInistSWF( IntInstCode code , RMType const& rm , Immediate< ImmSize > const& imm )
		{
			static_assert( Size >= ImmSize );
			encodeIntInistSW< Size >( INT_INIST_OPB( code ) , imm );
			encodeModRM( rm , INT_INIST_OPR( code ) );
			encodeImmediateForce< Size >( imm );
		}

		template< int Size , class RMType , int ImmSize >
		ASMETA_INLINE void encodeIntInistRMI( uint8 op , RMType const& rm , uint8 opR , Immediate< ImmSize > const& imm  )
		{
			static_assert( Size >= ImmSize );
			encodeInt16Prefix< Size >();
			uint8 s = ( Size > imm.size() ) ? 1 : 0;
			_this()->emitByte( op | ( s << 1 ) );
			encodeModRM( rm , opR );
			encodeImmediateNoForce( imm );
		}

		template< int Size , class RMType , int ImmSize >
		ASMETA_INLINE void encodeIntInistRMI( uint8 op , RMType const& rm , uint8 opR , Immediate< ImmSize > const& imm , bool usageSignExtend  )
		{
			static_assert( Size >= ImmSize );
			encodeInt16Prefix< Size >();
			uint8 s = ( usageSignExtend ) ? ( Size > imm.size() ? 1 : 0 ) : 0;
			_this()->emitByte( op | ( s << 1 ) );
			encodeModRM( rm , opR );
			encodeImmediateNoForce( imm );
		}

		template< int Size , class RMType , int ImmSize >
		ASMETA_INLINE void encodeIntInistFRMI( uint8 op , RMType const& rm , uint8 opR , Immediate< ImmSize > const& imm , bool usageSignExtend  /*true*/  )
		{
			static_assert( Size >= ImmSize );
			encodeInt16Prefix< Size >();
			uint8 s = ( usageSignExtend ) ? ( Size > imm.size() ? 1 : 0 ) : 0;
			_this()->emitByte( op | ( s << 1 ) );
			encodeModRM( rm , opR );
			encodeImmediateForce< Size >( imm );
		}

#if TARGET_PLATFORM_64BITS
		// Emit REX prefix byte: 0100WRXB
		// W = 1 for 64-bit operand size
		// R = extension of ModR/M reg field (bit 3 of register)
		// X = extension of SIB index field
		// B = extension of ModR/M r/m field or SIB base field
		void encodeREXPrefix(uint8 w, uint8 r, uint8 x, uint8 b)
		{
			_this()->emitByte( ( 0x4 << 4 ) | ( w << 3 ) | ( r << 2 ) | ( x << 1 ) | b );
		}

		// Emit REX prefix for register-to-register operations
		template< int Size >
		ASMETA_INLINE void encodeREXPrefixReg(Reg::Code reg, Reg::Code rm)
		{
			if constexpr (Size == 8)
			{
				uint8 r = Reg::isExtended(reg) ? 1 : 0;
				uint8 b = Reg::isExtended(rm) ? 1 : 0;
				encodeREXPrefix(1, r, 0, b); // W=1 for 64-bit
			}
			else
			{
				// For smaller sizes, only emit REX if extended registers are used
				uint8 r = Reg::isExtended(reg) ? 1 : 0;
				uint8 b = Reg::isExtended(rm) ? 1 : 0;
				if (r || b)
					encodeREXPrefix(0, r, 0, b);
			}
		}

		// Emit REX prefix for memory operations with RegPtr64
		template< int Size >
		ASMETA_INLINE void encodeREXPrefixMem(Reg::Code reg, RegPtr64 const& mem)
		{
			uint8 r = Reg::isExtended(reg) ? 1 : 0;
			if constexpr (Size == 8)
			{
				encodeREXPrefix(1, r, mem.mREX_X, mem.mREX_B);
			}
			else
			{
				if (r || mem.mREX_X || mem.mREX_B)
					encodeREXPrefix(0, r, mem.mREX_X, mem.mREX_B);
			}
		}

		// Emit REX prefix for single register operations (like push, pop)
		template< int Size >
		ASMETA_INLINE void encodeREXPrefixSingle(Reg::Code code)
		{
			if constexpr (Size == 8)
			{
				uint8 b = Reg::isExtended(code) ? 1 : 0;
				encodeREXPrefix(1, 0, 0, b);
			}
			else
			{
				uint8 b = Reg::isExtended(code) ? 1 : 0;
				if (b)
					encodeREXPrefix(0, 0, 0, b);
			}
		}

		// 64-bit integer instruction encoding with REX prefix
		template< int Size, class RMType >
		ASMETA_INLINE void encodeIntInist64(IntInstCode code, RMType const& rm, Reg::Code reg)
		{
			static_assert(Size == 8, "This function is for 64-bit operations");
			encodeREXPrefixReg<Size>(reg, rm.code());
			_this()->emitByte(INT_INIST_OPA(code) | 0x01); // W bit in opcode
			encodeModRM(rm, reg);
		}

		template< int Size >
		ASMETA_INLINE void encodeIntInist64(IntInstCode code, RegPtr64 const& rm, Reg::Code reg)
		{
			static_assert(Size == 8, "This function is for 64-bit operations");
			encodeREXPrefixMem<Size>(reg, rm);
			_this()->emitByte(INT_INIST_OPA(code) | 0x01);
			encodeModRM(rm, reg);
		}

		template< int Size >
		ASMETA_INLINE void encodeIntInist64R(IntInstCode code, RegPtr64 const& rm, Reg::Code reg)
		{
			static_assert(Size == 8, "This function is for 64-bit operations");
			encodeREXPrefixMem<Size>(reg, rm);
			_this()->emitByte(INT_INIST_OPA(code) | 0x03); // W bit + direction
			encodeModRM(rm, reg);
		}
#endif
	public:
		template< class Ref , int Size >
		ASMETA_INLINE void fld( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgFLD , SPFMap< Size >::RealFormat , mem.reference()  );  }
		template< class Ref >
		ASMETA_INLINE void fld( RefMem< Ref , 10 > const& mem )   {  encodeFPInst( FPMemFormat( 0x1 ) , 0x1 , 0x5 , mem.reference() );  }

		ASMETA_INLINE void fld( RegST const& dst )                {  encodeFPInst( fgFLD , 0 , 0 , 0 , dst ); }

		template< class Ref, int Size >
		ASMETA_INLINE void fild(RefMem< Ref, Size > const& mem) { encodeFPInst(fgFLD, SPFMap< Size >::IntFormat, mem.reference()); }

		ASMETA_INLINE void fld1()   {  encodeFPInst( 0 , 0x08 ); }
		ASMETA_INLINE void fldl2t() {  encodeFPInst( 0 , 0x09 ); }
		ASMETA_INLINE void fldl2e() {  encodeFPInst( 0 , 0x0a ); }
		ASMETA_INLINE void fldpi()  {  encodeFPInst( 0 , 0x0b ); }
		ASMETA_INLINE void fldlg2() {  encodeFPInst( 0 , 0x0c ); }
		ASMETA_INLINE void fldln2() {  encodeFPInst( 0 , 0x0d ); }
		ASMETA_INLINE void fldz()   {  encodeFPInst( 0 , 0x0e ); }

		ASMETA_INLINE void fchs()   {  encodeFPInst( 0 , 0x00 ); }
		ASMETA_INLINE void fsqrt()  {  encodeFPInst( 0 , 0x1a ); }
		ASMETA_INLINE void frndint(){  encodeFPInst( 0 , 0x1c ); }
		ASMETA_INLINE void fsin()   {  encodeFPInst( 0 , 0x1e ); }
		ASMETA_INLINE void fcos()   {  encodeFPInst( 0 , 0x1f ); }
		ASMETA_INLINE void fsincos(){  encodeFPInst( 0 , 0x1b ); }

		template< class Ref , int Size >
		ASMETA_INLINE void fstp( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgSTP , SPFMap< Size >::RealFormat , mem.reference() );  }
		template< class Ref >
		ASMETA_INLINE void fstp( RefMem< Ref , 10 > const& mem )   {  encodeFPInst( FPMemFormat( 0x1 ) , 0x1 , 0x7 , mem.reference() );  }
		ASMETA_INLINE void fstp( RegST const& dst )                {  encodeFPInst( fgSTP , 1 , 0 , 0 , dst ); } 

		template< class Ref , int Size >
		ASMETA_INLINE void fst( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgST , SPFMap< Size >::RealFormat, mem.reference() );  }
		ASMETA_INLINE void fst( RegST const& dst )                {  encodeFPInst( fgST , 1 , 0 , 0 , dst ); } 

		template< class Ref , int Size >
		ASMETA_INLINE void fcom( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgCOM , SPFMap< Size >::RealFormat, mem.reference() );  }
		ASMETA_INLINE void fcom( RegST const& dst )                {  encodeFPInst( fgCOM , 0 , 0 , 0 , dst ); } 

		template< class Ref , int Size >
		ASMETA_INLINE void fcomp( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgCOMP , SPFMap< Size >::RealFormat, mem.reference() );  }
		ASMETA_INLINE void fcomp( RegST const& dst )                {  encodeFPInst( fgCOMP , 0 , 0 , 0 , dst ); } 

		template< class Ref , int Size >
		ASMETA_INLINE void ficom( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgCOM , SPFMap< Size >::IntFormat, mem.reference() );  }
		template< class Ref , int Size >
		ASMETA_INLINE void ficomp( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgCOMP , SPFMap< Size >::IntFormat , mem.reference() );  }

		template< class Ref , int Size >
		ASMETA_INLINE void fistp( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgSTP , SPFMap< Size >::IntFormat , mem.reference() );  }
		template< class Ref >
		ASMETA_INLINE void fistp( RefMem< Ref , 8 > const& mem )    {  encodeFPInst( FPMemFormat( 0x3 ) , 0x1 , 0x7 , mem.reference() );  }

		template< class Ref , int Size >
		ASMETA_INLINE void fist( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgST , SPFMap< Size >::IntFormat , mem.reference() );  }

		template< class Ref , int Size  >
		ASMETA_INLINE void fadd( RefMem< Ref , Size > const& mem )     {  encodeFPInst( fgADD , SPFMap< Size >::RealFormat, mem.reference()  );  }
		ASMETA_INLINE void fadd( RegST const& dst , RegST const& src = st()) {  encodeFPInst( fgADD , dst , src ,  0 , 0 );  }
		ASMETA_INLINE void faddp( RegST const& dst = st(1) )           {  encodeFPInst( fgADD , 1 , 1 , 0 , dst );  }
		template< class Ref , int Size >
		ASMETA_INLINE void fiadd( RefMem< Ref , Size > const& mem )    {  encodeFPInst( fgADD , SPFMap< Size >::IntFormat , mem.reference() );  }

		template< class Ref , int Size  >
		ASMETA_INLINE void fmul( RefMem< Ref , Size > const& mem )     {  encodeFPInst( fgMUL , SPFMap< Size >::RealFormat, mem.reference()  );  }
		ASMETA_INLINE void fmul( RegST const& dst , RegST const& src = st()) {  encodeFPInst( fgMUL , dst , src ,  0 , 0 );  }
		ASMETA_INLINE void fmulp( RegST const& dst = st(1) )           {  encodeFPInst( fgMUL , 1 , 1 , 0 , dst );  }
		template< class Ref , int Size >
		ASMETA_INLINE void fimul( RefMem< Ref , Size > const& mem )    {  encodeFPInst( fgMUL , SPFMap< Size >::IntFormat , mem.reference() );  }

		template< class Ref , int Size  >
		ASMETA_INLINE void fsub( RefMem< Ref , Size > const& mem )     {  encodeFPInst( fgSUB , SPFMap< Size >::RealFormat, mem.reference()  );  }
		ASMETA_INLINE void fsub( RegST const& dst , RegST const& src = st()) {  encodeFPInst( fgSUB , dst , src ,  0 , 1 );  }
		ASMETA_INLINE void fsubp( RegST const& dst = st(1) )           {  encodeFPInst( fgSUB , 1 , 1 , 1 , dst );  }
		template< class Ref , int Size >
		ASMETA_INLINE void fisub( RefMem< Ref , Size > const& mem )    {  encodeFPInst( fgSUB , SPFMap< Size >::IntFormat , mem.reference() );  }

		template< class Ref , int Size  >
		ASMETA_INLINE void fsubr( RefMem< Ref , Size > const& mem )     {  encodeFPInst( fgSUB , SPFMap< Size >::RealFormat, mem.reference() , 0x1 );  }
		ASMETA_INLINE void fsubr( RegST const& dst , RegST const& src = st()) {  encodeFPInst( fgSUB , dst , src ,  1 , 1 );  }
		ASMETA_INLINE void fsubrp( RegST const& dst = st(1) )           {  encodeFPInst( fgSUB , 1 , 1 , 0 , dst );  }
		template< class Ref , int Size >
		ASMETA_INLINE void fisubr( RefMem< Ref , Size > const& mem )    {  encodeFPInst( fgSUB , SPFMap< Size >::IntFormat , mem.reference() , 0x1 );  }

		template< class Ref , int Size  >
		ASMETA_INLINE void fdiv( RefMem< Ref , Size > const& mem )     {  encodeFPInst( fgDIV , SPFMap< Size >::RealFormat, mem.reference()  );  }
		ASMETA_INLINE void fdiv( RegST const& dst , RegST const& src = st()) {  encodeFPInst( fgDIV , dst , src ,  0 , 1 );  }
		ASMETA_INLINE void fdivp( RegST const& dst = st(1) )           {  encodeFPInst( fgDIV , 1 , 1 , 1 , dst );  }
		template< class Ref , int Size >
		ASMETA_INLINE void fidiv( RefMem< Ref , Size > const& mem )    {  encodeFPInst( fgDIV , SPFMap< Size >::IntFormat , mem.reference() );  }

		template< class Ref , int Size  >
		ASMETA_INLINE void fdivr( RefMem< Ref , Size > const& mem )     {  encodeFPInst( fgDIV , SPFMap< Size >::RealFormat, mem.reference() , 0x1 );  }
		ASMETA_INLINE void fdivr( RegST const& dst , RegST const& src = st()) {  encodeFPInst( fgDIV , dst , src , 1 , 1 );  }
		ASMETA_INLINE void fdivrp( RegST const& dst = st(1) )           {  encodeFPInst( fgDIV , 1 , 1 , 0 , dst );  }
		template< class Ref , int Size >
		ASMETA_INLINE void fidivr( RefMem< Ref , Size > const& mem ) {  encodeFPInst( fgDIV , SPFMap< Size >::IntFormat , mem.reference() , 0x1 );  }

		ASMETA_INLINE void fxch( RegST const& dst ){  encodeFPInst( 0 , 0 , 0x1 , 0x1 , 0 , dst ); } 

	protected:

		ASMETA_INLINE void encodeFPInst( FPInstCode code , RegST const& dst , RegST const& src , uint8 r , uint8 mask , uint8 pop = 0 )
		{
			assert( dst.index() == 0 || src.index() == 0 );
			if ( dst.index() == 0 )
				encodeFPInst( code , 0 , pop , ( 0 + r ) & mask , src );
			else
				encodeFPInst( code , 1 , pop , ( 1 + r ) & mask , dst );
		}

		template< class RefPtr >
		ASMETA_INLINE void encodeFPInst( uint8 opA , uint8 opB , RefPtr const& refPtr )
		{
			_this()->emitByte( PUI_PATTERN | ( opA  << 1 ) | 0x1 );
			encodeModRM( refPtr , opB );
		}

		ASMETA_INLINE void encodeFPInst( FPInstCode code , uint8 dest , uint8 pop , uint8 reverse , RegST const& st )
		{
			encodeFPInst( dest , pop  , FP_INIST_OPA( code ) , FP_INIST_OPB( code ) , reverse , st  );
		}

		template< class RefPtr >
		ASMETA_INLINE void encodeFPInst( FPInstCode code , FPMemFormat format , RefPtr const& refPtr , uint8 modifyB = 0 )
		{
			encodeFPInst( format , FP_INIST_OPA( code ) , FP_INIST_OPB( code ) | modifyB  , refPtr );
		}

		template< class RefPtr >
		ASMETA_INLINE void encodeFPInst( FPMemFormat format , uint8 opA , uint8 opB , RefPtr const& refPtr )
		{
			_this()->emitByte( uint8( PUI_PATTERN | ( format << 1 ) | opA ) );
			encodeModRM( refPtr , opB );
		}

		//    15-11       10        9        8        7-6    5       3       2 - 0
		//    11011      dest      pop      opA   |   11    opB   reverse    st(i)
		ASMETA_INLINE void encodeFPInst( uint8 dest , uint8 pop , uint8 opA , uint8 opB , uint8 reverse , RegST const& st )
		{
			_this()->emitWord( 
				uint8( PUI_PATTERN | ( dest << 2 ) | ( pop << 1 ) | opA ) ,
				uint8( 0xc0 | ( (opB | reverse ) << 3 ) | st.index() ) );
		}

		ASMETA_INLINE void encodeFPInst( uint8 s , uint8 op )
		{
			_this()->emitWord( 
				uint8( PUI_PATTERN | ( s << 1 ) | 0x1 )  , 
				uint8( 0xe0 | op ) );
		}

		//=============================================================================
		// SSE Instructions
		//=============================================================================
	public:
		// SSE prefix bytes
		static constexpr uint8 SSE_PREFIX_NONE = 0x00;   // Packed single (PS)
		static constexpr uint8 SSE_PREFIX_66 = 0x66;     // Packed double (PD)
		static constexpr uint8 SSE_PREFIX_F3 = 0xF3;     // Scalar single (SS)
		static constexpr uint8 SSE_PREFIX_F2 = 0xF2;     // Scalar double (SD)

		//-----------------------------------------------------------------------------
		// MOVSS - Move Scalar Single-Precision Floating-Point Values
		//-----------------------------------------------------------------------------
		// movss xmm1, xmm2
		ASMETA_INLINE void movss(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_F3, 0x0F, 0x10, dst, src);
		}
		// movss xmm, m32
		template< class Ref >
		ASMETA_INLINE void movss(RegXMM const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_F3, 0x0F, 0x10, dst, src);
		}
		// movss m32, xmm
		template< class Ref >
		ASMETA_INLINE void movss(RefMem< Ref, 4 > const& dst, RegXMM const& src)
		{
			encodeSSEMem(SSE_PREFIX_F3, 0x0F, 0x11, src, dst);
		}

		//-----------------------------------------------------------------------------
		// MOVSD - Move Scalar Double-Precision Floating-Point Values
		//-----------------------------------------------------------------------------
		// movsd xmm1, xmm2
		ASMETA_INLINE void movsd(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_F2, 0x0F, 0x10, dst, src);
		}
		// movsd xmm, m64
		template< class Ref >
		ASMETA_INLINE void movsd(RegXMM const& dst, RefMem< Ref, 8 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_F2, 0x0F, 0x10, dst, src);
		}
		// movsd m64, xmm
		template< class Ref >
		ASMETA_INLINE void movsd(RefMem< Ref, 8 > const& dst, RegXMM const& src)
		{
			encodeSSEMem(SSE_PREFIX_F2, 0x0F, 0x11, src, dst);
		}

		//-----------------------------------------------------------------------------
		// MOVAPS/MOVUPS - Move Aligned/Unaligned Packed Single-Precision
		//-----------------------------------------------------------------------------
		ASMETA_INLINE void movaps(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_NONE, 0x0F, 0x28, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movaps(RegXMM const& dst, RefMem< Ref, 16 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_NONE, 0x0F, 0x28, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movaps(RefMem< Ref, 16 > const& dst, RegXMM const& src)
		{
			encodeSSEMem(SSE_PREFIX_NONE, 0x0F, 0x29, src, dst);
		}

		ASMETA_INLINE void movups(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_NONE, 0x0F, 0x10, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movups(RegXMM const& dst, RefMem< Ref, 16 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_NONE, 0x0F, 0x10, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movups(RefMem< Ref, 16 > const& dst, RegXMM const& src)
		{
			encodeSSEMem(SSE_PREFIX_NONE, 0x0F, 0x11, src, dst);
		}

		//-----------------------------------------------------------------------------
		// SHUFPS - Shuffle Packed Single-Precision Floating-Point Values
		//-----------------------------------------------------------------------------
		ASMETA_INLINE void shufps(RegXMM const& dst, RegXMM const& src, uint8 imm8)
		{
			encodeSSEImm(SSE_PREFIX_NONE, 0x0F, 0xC6, dst, src, imm8);
		}
		template< class Ref >
		ASMETA_INLINE void shufps(RegXMM const& dst, RefMem< Ref, 16 > const& src, uint8 imm8)
		{
			encodeSSEImmMem(SSE_PREFIX_NONE, 0x0F, 0xC6, dst, src, imm8);
		}

		//-----------------------------------------------------------------------------
		// INSERTPS - Insert Scalar Single-Precision Floating-Point Value
		//-----------------------------------------------------------------------------
		ASMETA_INLINE void insertps(RegXMM const& dst, RegXMM const& src, uint8 imm8)
		{
			encodeSSEImm(SSE_PREFIX_66, 0x0F, 0x3A, 0x21, dst, src, imm8);
		}
		template< class Ref >
		ASMETA_INLINE void insertps(RegXMM const& dst, RefMem< Ref, 4 > const& src, uint8 imm8)
		{
			encodeSSEImmMem(SSE_PREFIX_66, 0x0F, 0x3A, 0x21, dst, src, imm8);
		}

		//-----------------------------------------------------------------------------
		// MOVAPD/MOVUPD - Move Aligned/Unaligned Packed Double-Precision
		//-----------------------------------------------------------------------------
		ASMETA_INLINE void movapd(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_66, 0x0F, 0x28, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movapd(RegXMM const& dst, RefMem< Ref, 16 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_66, 0x0F, 0x28, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movapd(RefMem< Ref, 16 > const& dst, RegXMM const& src)
		{
			encodeSSEMem(SSE_PREFIX_66, 0x0F, 0x29, src, dst);
		}

		ASMETA_INLINE void movupd(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_66, 0x0F, 0x10, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movupd(RegXMM const& dst, RefMem< Ref, 16 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_66, 0x0F, 0x10, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void movupd(RefMem< Ref, 16 > const& dst, RegXMM const& src)
		{
			encodeSSEMem(SSE_PREFIX_66, 0x0F, 0x11, src, dst);
		}

		//-----------------------------------------------------------------------------
		// SSE Arithmetic - Scalar Single (SS)
		//-----------------------------------------------------------------------------
#define DEF_SSE_ARITH_SS(NAME, OPCODE) \
		ASMETA_INLINE void NAME(RegXMM const& dst, RegXMM const& src) { encodeSSE(SSE_PREFIX_F3, 0x0F, OPCODE, dst, src); } \
		template< class Ref > ASMETA_INLINE void NAME(RegXMM const& dst, RefMem< Ref, 4 > const& src) { encodeSSEMem(SSE_PREFIX_F3, 0x0F, OPCODE, dst, src); }

		DEF_SSE_ARITH_SS(addss, 0x58)
		DEF_SSE_ARITH_SS(subss, 0x5C)
		DEF_SSE_ARITH_SS(mulss, 0x59)
		DEF_SSE_ARITH_SS(divss, 0x5E)
		DEF_SSE_ARITH_SS(minss, 0x5D)
		DEF_SSE_ARITH_SS(maxss, 0x5F)
		DEF_SSE_ARITH_SS(sqrtss, 0x51)
		DEF_SSE_ARITH_SS(rcpss, 0x53)
		DEF_SSE_ARITH_SS(rsqrtss, 0x52)

#undef DEF_SSE_ARITH_SS


		//-----------------------------------------------------------------------------
		// SSE Arithmetic - Scalar Double (SD)
		//-----------------------------------------------------------------------------
#define DEF_SSE_ARITH_SD(NAME, OPCODE) \
		ASMETA_INLINE void NAME(RegXMM const& dst, RegXMM const& src) { encodeSSE(SSE_PREFIX_F2, 0x0F, OPCODE, dst, src); } \
		template< class Ref > ASMETA_INLINE void NAME(RegXMM const& dst, RefMem< Ref, 8 > const& src) { encodeSSEMem(SSE_PREFIX_F2, 0x0F, OPCODE, dst, src); }

		DEF_SSE_ARITH_SD(addsd, 0x58)
		DEF_SSE_ARITH_SD(subsd, 0x5C)
		DEF_SSE_ARITH_SD(mulsd, 0x59)
		DEF_SSE_ARITH_SD(divsd, 0x5E)
		DEF_SSE_ARITH_SD(minsd, 0x5D)
		DEF_SSE_ARITH_SD(maxsd, 0x5F)
		DEF_SSE_ARITH_SD(sqrtsd, 0x51)

#undef DEF_SSE_ARITH_SD

		//-----------------------------------------------------------------------------
		// SSE Arithmetic - Packed Single (PS)
		//-----------------------------------------------------------------------------
#define DEF_SSE_ARITH_PS(NAME, OPCODE) \
		ASMETA_INLINE void NAME(RegXMM const& dst, RegXMM const& src) { encodeSSE(SSE_PREFIX_NONE, 0x0F, OPCODE, dst, src); } \
		template< class Ref > ASMETA_INLINE void NAME(RegXMM const& dst, RefMem< Ref, 16 > const& src) { encodeSSEMem(SSE_PREFIX_NONE, 0x0F, OPCODE, dst, src); }

		DEF_SSE_ARITH_PS(addps, 0x58)
		DEF_SSE_ARITH_PS(subps, 0x5C)
		DEF_SSE_ARITH_PS(mulps, 0x59)
		DEF_SSE_ARITH_PS(divps, 0x5E)
		DEF_SSE_ARITH_PS(minps, 0x5D)
		DEF_SSE_ARITH_PS(maxps, 0x5F)
		DEF_SSE_ARITH_PS(xorps, 0x57)
		DEF_SSE_ARITH_PS(andps, 0x54)
		DEF_SSE_ARITH_PS(andnps, 0x55)
		DEF_SSE_ARITH_PS(orps, 0x56)
		DEF_SSE_ARITH_PS(sqrtps, 0x51)
		DEF_SSE_ARITH_PS(rcpps, 0x53)
		DEF_SSE_ARITH_PS(rsqrtps, 0x52)

#undef DEF_SSE_ARITH_PS

		//-----------------------------------------------------------------------------
		// SSE Arithmetic - Packed Double (PD)
		//-----------------------------------------------------------------------------
#define DEF_SSE_ARITH_PD(NAME, OPCODE) \
		ASMETA_INLINE void NAME(RegXMM const& dst, RegXMM const& src) { encodeSSE(SSE_PREFIX_66, 0x0F, OPCODE, dst, src); } \
		template< class Ref > ASMETA_INLINE void NAME(RegXMM const& dst, RefMem< Ref, 16 > const& src) { encodeSSEMem(SSE_PREFIX_66, 0x0F, OPCODE, dst, src); }

		DEF_SSE_ARITH_PD(addpd, 0x58)
		DEF_SSE_ARITH_PD(subpd, 0x5C)
		DEF_SSE_ARITH_PD(mulpd, 0x59)
		DEF_SSE_ARITH_PD(divpd, 0x5E)
		DEF_SSE_ARITH_PD(minpd, 0x5D)
		DEF_SSE_ARITH_PD(maxpd, 0x5F)
		DEF_SSE_ARITH_PD(xorpd, 0x57)
		DEF_SSE_ARITH_PD(andpd, 0x54)
		DEF_SSE_ARITH_PD(andnpd, 0x55)
		DEF_SSE_ARITH_PD(orpd, 0x56)
		DEF_SSE_ARITH_PD(sqrtpd, 0x51)

#undef DEF_SSE_ARITH_PD

		//-----------------------------------------------------------------------------
		// SSE Comparison - COMISS/COMISD (set EFLAGS)
		//-----------------------------------------------------------------------------
		ASMETA_INLINE void comiss(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_NONE, 0x0F, 0x2F, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void comiss(RegXMM const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_NONE, 0x0F, 0x2F, dst, src);
		}

		ASMETA_INLINE void comisd(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_66, 0x0F, 0x2F, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void comisd(RegXMM const& dst, RefMem< Ref, 8 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_66, 0x0F, 0x2F, dst, src);
		}

		ASMETA_INLINE void ucomiss(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_NONE, 0x0F, 0x2E, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void ucomiss(RegXMM const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_NONE, 0x0F, 0x2E, dst, src);
		}

		ASMETA_INLINE void ucomisd(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_66, 0x0F, 0x2E, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void ucomisd(RegXMM const& dst, RefMem< Ref, 8 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_66, 0x0F, 0x2E, dst, src);
		}

		//-----------------------------------------------------------------------------
		// SSE Conversion - Float <-> Double
		//-----------------------------------------------------------------------------
		// cvtss2sd xmm, xmm/m32 - Convert Scalar Single to Scalar Double
		ASMETA_INLINE void cvtss2sd(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_F3, 0x0F, 0x5A, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvtss2sd(RegXMM const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_F3, 0x0F, 0x5A, dst, src);
		}

		// cvtsd2ss xmm, xmm/m64 - Convert Scalar Double to Scalar Single
		ASMETA_INLINE void cvtsd2ss(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_F2, 0x0F, 0x5A, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvtsd2ss(RegXMM const& dst, RefMem< Ref, 8 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_F2, 0x0F, 0x5A, dst, src);
		}

		//-----------------------------------------------------------------------------
		// SSE Conversion - Integer <-> Float
		//-----------------------------------------------------------------------------
		// cvtsi2ss xmm, r32/m32 - Convert Signed Integer to Scalar Single
		ASMETA_INLINE void cvtsi2ss(RegXMM const& dst, Reg32 const& src)
		{
			encodeSSEToGP(SSE_PREFIX_F3, 0x0F, 0x2A, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvtsi2ss(RegXMM const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_F3, 0x0F, 0x2A, dst, src);
		}

		// cvtsi2sd xmm, r32/m32 - Convert Signed Integer to Scalar Double
		ASMETA_INLINE void cvtsi2sd(RegXMM const& dst, Reg32 const& src)
		{
			encodeSSEToGP(SSE_PREFIX_F2, 0x0F, 0x2A, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvtsi2sd(RegXMM const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEMem(SSE_PREFIX_F2, 0x0F, 0x2A, dst, src);
		}

#if TARGET_PLATFORM_64BITS
		// cvtsi2ss xmm, r64/m64 - Convert Signed Integer (64-bit) to Scalar Single
		ASMETA_INLINE void cvtsi2ss(RegXMM const& dst, Reg64 const& src)
		{
			encodeSSEToGP64(SSE_PREFIX_F3, 0x0F, 0x2A, dst, src);
		}

		// cvtsi2sd xmm, r64/m64 - Convert Signed Integer (64-bit) to Scalar Double
		ASMETA_INLINE void cvtsi2sd(RegXMM const& dst, Reg64 const& src)
		{
			encodeSSEToGP64(SSE_PREFIX_F2, 0x0F, 0x2A, dst, src);
		}
#endif

		// cvtss2si r32, xmm/m32 - Convert Scalar Single to Signed Integer (rounded)
		ASMETA_INLINE void cvtss2si(Reg32 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP(SSE_PREFIX_F3, 0x0F, 0x2D, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvtss2si(Reg32 const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEFromGPMem(SSE_PREFIX_F3, 0x0F, 0x2D, dst, src);
		}

		// cvtsd2si r32, xmm/m64 - Convert Scalar Double to Signed Integer (rounded)
		ASMETA_INLINE void cvtsd2si(Reg32 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP(SSE_PREFIX_F2, 0x0F, 0x2D, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvtsd2si(Reg32 const& dst, RefMem< Ref, 8 > const& src)
		{
			encodeSSEFromGPMem(SSE_PREFIX_F2, 0x0F, 0x2D, dst, src);
		}

#if TARGET_PLATFORM_64BITS
		// cvtss2si r64, xmm/m32 - Convert Scalar Single to Signed Integer 64-bit (rounded)
		ASMETA_INLINE void cvtss2si(Reg64 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP64(SSE_PREFIX_F3, 0x0F, 0x2D, dst, src);
		}

		// cvtsd2si r64, xmm/m64 - Convert Scalar Double to Signed Integer 64-bit (rounded)
		ASMETA_INLINE void cvtsd2si(Reg64 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP64(SSE_PREFIX_F2, 0x0F, 0x2D, dst, src);
		}
#endif

		// cvttss2si r32, xmm/m32 - Convert with Truncation Scalar Single to SI
		ASMETA_INLINE void cvttss2si(Reg32 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP(SSE_PREFIX_F3, 0x0F, 0x2C, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvttss2si(Reg32 const& dst, RefMem< Ref, 4 > const& src)
		{
			encodeSSEFromGPMem(SSE_PREFIX_F3, 0x0F, 0x2C, dst, src);
		}

		// cvttsd2si r32, xmm/m64 - Convert with Truncation Scalar Double to SI
		ASMETA_INLINE void cvttsd2si(Reg32 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP(SSE_PREFIX_F2, 0x0F, 0x2C, dst, src);
		}
		template< class Ref >
		ASMETA_INLINE void cvttsd2si(Reg32 const& dst, RefMem< Ref, 8 > const& src)
		{
			encodeSSEFromGPMem(SSE_PREFIX_F2, 0x0F, 0x2C, dst, src);
		}

#if TARGET_PLATFORM_64BITS
		// cvttss2si r64, xmm/m32 - Convert with Truncation Scalar Single to SI64
		ASMETA_INLINE void cvttss2si(Reg64 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP64(SSE_PREFIX_F3, 0x0F, 0x2C, dst, src);
		}

		// cvttsd2si r64, xmm/m64 - Convert with Truncation Scalar Double to SI64
		ASMETA_INLINE void cvttsd2si(Reg64 const& dst, RegXMM const& src)
		{
			encodeSSEFromGP64(SSE_PREFIX_F2, 0x0F, 0x2C, dst, src);
		}
#endif



		//-----------------------------------------------------------------------------
		// PXOR - Packed XOR (useful for zeroing XMM registers)
		//-----------------------------------------------------------------------------
		ASMETA_INLINE void pxor(RegXMM const& dst, RegXMM const& src)
		{
			encodeSSE(SSE_PREFIX_66, 0x0F, 0xEF, dst, src);
		}

	protected:
		//-----------------------------------------------------------------------------
		// SSE Encoding Helpers
		//-----------------------------------------------------------------------------
		
		// Encode SSE instruction: xmm, xmm
		ASMETA_INLINE void encodeSSE(uint8 prefix, uint8 escape, uint8 opcode, RegXMM const& dst, RegXMM const& src)
		{
			if (prefix != SSE_PREFIX_NONE)
				_this()->emitByte(prefix);
			
#if TARGET_PLATFORM_64BITS
			// Emit REX if needed for extended XMM registers (xmm8-xmm15)
			if (dst.isExtended() || src.isExtended())
			{
				uint8 r = dst.isExtended() ? 1 : 0;
				uint8 b = src.isExtended() ? 1 : 0;
				encodeREXPrefix(0, r, 0, b);
			}
#endif
			_this()->emitByte(escape);
			_this()->emitByte(opcode);
			_this()->emitByte(MOD_RM_BYTE(MOD_R, dst.lowBits(), src.lowBits()));
		}

		// Encode SSE instruction: xmm, m
		template< class Ref, int Size >
		ASMETA_INLINE void encodeSSEMem(uint8 prefix, uint8 escape, uint8 opcode, RegXMM const& xmm, RefMem< Ref, Size > const& mem)
		{
			if (prefix != SSE_PREFIX_NONE)
				_this()->emitByte(prefix);

#if TARGET_PLATFORM_64BITS
			if constexpr (std::is_same_v<Ref, RegPtr64>)
			{
				uint8 r = xmm.isExtended() ? 1 : 0;
				// Directly emit REX prefix with correct R bit for extended XMM registers
				// Cannot use encodeREXPrefixMem here because it expects a register code,
				// not a pre-computed R flag value
				if (r || mem.reference().mREX_X || mem.reference().mREX_B)
					encodeREXPrefix(0, r, mem.reference().mREX_X, mem.reference().mREX_B);
			}
			else if (xmm.isExtended())
			{
				encodeREXPrefix(0, 1, 0, 0);
			}
#endif
			_this()->emitByte(escape);
			_this()->emitByte(opcode);
			encodeModRM(mem.reference(), xmm.lowBits());
		}

		// Encode SSE instruction: xmm, r32 (for cvtsi2ss/cvtsi2sd)
		ASMETA_INLINE void encodeSSEToGP(uint8 prefix, uint8 escape, uint8 opcode, RegXMM const& xmm, Reg32 const& gp)
		{
			if (prefix != SSE_PREFIX_NONE)
				_this()->emitByte(prefix);

#if TARGET_PLATFORM_64BITS
			if (xmm.isExtended() || Reg::isExtended(gp.code()))
			{
				uint8 r = xmm.isExtended() ? 1 : 0;
				uint8 b = Reg::isExtended(gp.code()) ? 1 : 0;
				encodeREXPrefix(0, r, 0, b);
			}
#endif
			_this()->emitByte(escape);
			_this()->emitByte(opcode);
			_this()->emitByte(MOD_RM_BYTE(MOD_R, xmm.lowBits(), Reg::lowBits(gp.code())));
		}

#if TARGET_PLATFORM_64BITS
		// Encode SSE instruction: xmm, r64 (for cvtsi2ss/cvtsi2sd with 64-bit integer)
		ASMETA_INLINE void encodeSSEToGP64(uint8 prefix, uint8 escape, uint8 opcode, RegXMM const& xmm, Reg64 const& gp)
		{
			if (prefix != SSE_PREFIX_NONE)
				_this()->emitByte(prefix);

			// REX.W is required for 64-bit operand
			uint8 r = xmm.isExtended() ? 1 : 0;
			uint8 b = Reg::isExtended(gp.code()) ? 1 : 0;
			encodeREXPrefix(1, r, 0, b);

			_this()->emitByte(escape);
			_this()->emitByte(opcode);
			_this()->emitByte(MOD_RM_BYTE(MOD_R, xmm.lowBits(), Reg::lowBits(gp.code())));
		}
#endif

		// Encode SSE instruction: r32, xmm (for cvtss2si/cvtsd2si)
		ASMETA_INLINE void encodeSSEFromGP(uint8 prefix, uint8 escape, uint8 opcode, Reg32 const& gp, RegXMM const& xmm)
		{
			if (prefix != SSE_PREFIX_NONE)
				_this()->emitByte(prefix);

#if TARGET_PLATFORM_64BITS
			if (Reg::isExtended(gp.code()) || xmm.isExtended())
			{
				uint8 r = Reg::isExtended(gp.code()) ? 1 : 0;
				uint8 b = xmm.isExtended() ? 1 : 0;
				encodeREXPrefix(0, r, 0, b);
			}
#endif
			_this()->emitByte(escape);
			_this()->emitByte(opcode);
			_this()->emitByte(MOD_RM_BYTE(MOD_R, Reg::lowBits(gp.code()), xmm.lowBits()));
		}

		// Encode SSE instruction: r32, m (for cvtss2si/cvtsd2si with memory)
		template< class Ref, int Size >
		ASMETA_INLINE void encodeSSEFromGPMem(uint8 prefix, uint8 escape, uint8 opcode, Reg32 const& gp, RefMem< Ref, Size > const& mem)
		{
			if (prefix != SSE_PREFIX_NONE)
				_this()->emitByte(prefix);

#if TARGET_PLATFORM_64BITS
			if (Reg::isExtended(gp.code()))
			{
				encodeREXPrefix(0, 1, 0, 0);
			}
#endif
			_this()->emitByte(escape);
			_this()->emitByte(opcode);
			encodeModRM(mem.reference(), Reg::lowBits(gp.code()));
		}

#if TARGET_PLATFORM_64BITS
		// Encode SSE instruction with immediate: xmm, xmm, imm8
		ASMETA_INLINE void encodeSSEImm(uint8 prefix, uint8 escape, uint8 opcode, RegXMM const& dst, RegXMM const& src, uint8 imm8)
		{
			encodeSSE(prefix, escape, opcode, dst, src);
			_this()->emitByte(imm8);
		}

		// Encode SSE instruction with immediate: xmm, xmm, subopcode, imm8
		ASMETA_INLINE void encodeSSEImm(uint8 prefix, uint8 escape, uint8 escape2, uint8 opcode, RegXMM const& dst, RegXMM const& src, uint8 imm8)
		{
			if (prefix != SSE_PREFIX_NONE) _this()->emitByte(prefix);
#if TARGET_PLATFORM_64BITS
			if (dst.isExtended() || src.isExtended())
				encodeREXPrefix(0, dst.isExtended(), 0, src.isExtended());
#endif
			_this()->emitByte(escape);
			_this()->emitByte(escape2);
			_this()->emitByte(opcode);
			_this()->emitByte(MOD_RM_BYTE(MOD_R, dst.lowBits(), src.lowBits()));
			_this()->emitByte(imm8);
		}

		// Encode SSE instruction with immediate: xmm, m, imm8
		template< class Ref, int Size >
		ASMETA_INLINE void encodeSSEImmMem(uint8 prefix, uint8 escape, uint8 opcode, RegXMM const& xmm, RefMem< Ref, Size > const& mem, uint8 imm8)
		{
			encodeSSEMem(prefix, escape, opcode, xmm, mem);
			_this()->emitByte(imm8);
		}

		// Encode SSE instruction with immediate: xmm, m, subopcode, imm8
		template< class Ref, int Size >
		ASMETA_INLINE void encodeSSEImmMem(uint8 prefix, uint8 escape, uint8 escape2, uint8 opcode, RegXMM const& xmm, RefMem< Ref, Size > const& mem, uint8 imm8)
		{
			if (prefix != SSE_PREFIX_NONE) _this()->emitByte(prefix);
#if TARGET_PLATFORM_64BITS
			encodeSSEMemREX(xmm, mem);
#endif
			_this()->emitByte(escape);
			_this()->emitByte(escape2);
			_this()->emitByte(opcode);
			encodeModRM(mem.reference(), xmm.lowBits());
			_this()->emitByte(imm8);
		}

	private:
#if TARGET_PLATFORM_64BITS
		template< class Ref, int Size >
		ASMETA_INLINE void encodeSSEMemREX(RegXMM const& xmm, RefMem< Ref, Size > const& mem)
		{
			if constexpr (std::is_same_v<Ref, RegPtr64>)
			{
				uint8 r = xmm.isExtended() ? 1 : 0;
				if (r || mem.reference().mREX_X || mem.reference().mREX_B)
					encodeREXPrefix(0, r, mem.reference().mREX_X, mem.reference().mREX_B);
			}
			else if (xmm.isExtended())
			{
				encodeREXPrefix(0, 1, 0, 0);
			}
		}
#endif
	public:
		ASMETA_INLINE void encodeSSEFromGP64(uint8 prefix, uint8 escape, uint8 opcode, Reg64 const& gp, RegXMM const& xmm)
		{
			if (prefix != SSE_PREFIX_NONE)
				_this()->emitByte(prefix);

			// REX.W is required for 64-bit operand
			uint8 r = Reg::isExtended(gp.code()) ? 1 : 0;
			uint8 b = xmm.isExtended() ? 1 : 0;
			encodeREXPrefix(1, r, 0, b);

			_this()->emitByte(escape);
			_this()->emitByte(opcode);
			_this()->emitByte(MOD_RM_BYTE(MOD_R, Reg::lowBits(gp.code()), xmm.lowBits()));
		}
#endif

		template< int Size , int ImmSize >
		ASMETA_INLINE void encodeImmediate( Immediate< ImmSize > const& imm , bool forceSize )
		{
			if ( forceSize )
			{
				encodeImmediateForce< Size >( imm );
			}
			else
			{
				encodeImmediateNotForce( imm );
			}
		}

		template< int ImmSize >
		ASMETA_INLINE void encodeImmediateNotForce( Immediate< ImmSize > const& imm )
		{
			switch( imm.size() )
			{
			case 1: _this()->emitByte( imm.value() ); break;
			case 2: _this()->emitWord( imm.value() ); break;
			case 4: _this()->emitDWord( imm.value() ); break;
#if TARGET_PLATFORM_64BITS
			case 8: _this()->emitQWord( imm.value() ); break;
#endif
			}
		}

		template< int Size , int ImmSize >
		ASMETA_INLINE void encodeImmediateForce( Immediate< ImmSize > const& imm )
		{
			encodeValue<Size>( imm.value() );
		}

		template< int DispSize  >
		ASMETA_INLINE void encodeDisp( Disp< DispSize > const& disp ){  encodeValue< DispSize >( disp.value() );  }
		template< int DispSize  >
		ASMETA_INLINE void encodeDisp( SysInt value ){ encodeValue< DispSize >( value ); }

		template< int Size >
		ASMETA_INLINE void encodeValue(SysInt value) 
		{ 
			if constexpr (Size == 1)
			{
				_this()->emitByte(value);
			}
			else if constexpr (Size == 2)
			{
				_this()->emitWord(value);
			}
			else if constexpr (Size == 4)
			{
				_this()->emitDWord(value);
			}
#if TARGET_PLATFORM_64BITS
			else if constexpr (Size == 8)
			{
				_this()->emitQWord(value);
			}
#endif
		}

		template< class Ref , int Size >
		ASMETA_INLINE void encodeModRM( RefMem< Ref , Size > const& ptr , uint8 reg )
		{
			encodeModRM( ptr.reference() , reg );
		}

		ASMETA_INLINE void encodeModRM( Label* label , uint8 reg )
		{
			_this()->emitByte( MOD_RM_BYTE(  MOD_M , reg , RM_M_DISP ) );
#if TARGET_PLATFORM_64BITS
			// x64 uses RIP-relative addressing (Mod=00, R/M=101), which takes a 32-bit displacement
			addLabelLink( label , Label::eLinkRel , 4 );
			_this()->emitDWord( 0xdededede );
#else
			// x86 uses absolute addressing
			addLabelLink( label , Label::eLinkAbs , sizeof( SysInt ) );
			_this()->emitPtr( (void*)0xdededede );
#endif
		}

		ASMETA_INLINE void encodeModRM( LabelPtr const& refPtr , uint8 reg )
		{
			_this()->emitByte( MOD_RM_BYTE(  MOD_M , reg , RM_M_DISP ) );
#if TARGET_PLATFORM_64BITS
			// x64 uses RIP-relative addressing (Mod=00, R/M=101), which takes a 32-bit displacement
			// Pass the label offset as dispOffset so it gets added during relocation
			addLabelLink( refPtr.mLabel , Label::eLinkRel , 4 , refPtr.mOffset );
			_this()->emitDWord( 0xdededede );
#else
			// x86 uses absolute addressing
			addLabelLink( refPtr.mLabel , Label::eLinkAbs , sizeof( SysInt ) , refPtr.mOffset );
			_this()->emitPtr( (void*)0xdededede );
#endif
		}

		ASMETA_INLINE void encodeModRM( Address const& refPtr , uint8  reg )
		{
			_this()->emitByte( MOD_RM_BYTE( MOD_M ,  reg  , RM_M_DISP ) );
			_this()->emitPtr( refPtr.value() );
		}

		ASMETA_INLINE void encodeModRM( RegPtr const& refPtr , uint8  reg )
		{
			_this()->emitByte( uint8( refPtr.mModByte | ( reg << 3 ) ) );

			uint8 mod = refPtr.mModByte >> 6;
			assert( mod != MOD_R );
			if ( ( refPtr.mModByte & 0x7 ) == RM_MD_SIB  )
				_this()->emitByte( refPtr.mSIBByte );

			switch( mod )
			{
			case MOD_DISP8: 
				_this()->emitByte( uint8( refPtr.mDisp ) );
				break;
			case MOD_DISP32: 
				_this()->emitDWord( refPtr.mDisp ); 
				break;
			}
		}

		template< int Size >
		ASMETA_INLINE void encodeModRM(RegX86< Size > const& r, uint8 reg )
		{
			_this()->emitByte( MOD_RM_BYTE(MOD_R, reg, r.code()) );
		}
#if TARGET_PLATFORM_64BITS
		ASMETA_INLINE void encodeModRM(RegX86< 8 > const& r, uint8 reg)
		{
			// For 64-bit registers, we use low 3 bits for ModR/M
			// REX prefix should be emitted before the opcode, not here
			_this()->emitByte( MOD_RM_BYTE(MOD_R, Reg::lowBits((Reg::Code)reg), Reg::lowBits(r.code())) );
		}

		// ModR/M encoding for 64-bit memory addressing (RegPtr64)
		ASMETA_INLINE void encodeModRM(RegPtr64 const& refPtr, uint8 reg)
		{
			_this()->emitByte( uint8( refPtr.mModByte | ( Reg::lowBits((Reg::Code)reg) << 3 ) ) );

			uint8 mod = refPtr.mModByte >> 6;
			assert( mod != MOD_R );
			if ( ( refPtr.mModByte & 0x7 ) == RM_MD_SIB )
				_this()->emitByte( refPtr.mSIBByte );

			switch( mod )
			{
			case MOD_DISP8: 
				_this()->emitByte( uint8( refPtr.mDisp ) );
				break;
			case MOD_DISP32: 
				_this()->emitDWord( uint32(refPtr.mDisp) ); 
				break;
			}
		}
#endif

		bool   bind( Label* label )
		{
			if ( label->mState == Label::eBound )
				return false;

			if ( label->mState == Label::eUnused )
			{
#if _DEBUG
				label->mIndex = mLabelList.size();
#endif
				mLabelList.push_back( label );
			}

			label->mOffset   = _this()->getOffset();
			label->mState    = Label::eBound;

			return true;
		}

		bool reloc( void* baseAddr )
		{
			for( LabelList::iterator iter = mLabelList.begin();
				 iter != mLabelList.end() ; ++iter )
			{
				Label* label = *iter;
				if ( label->mState == Label::eUnbound && 
					 label->getLink() )
					 return false;

				for( auto link = label->getLink(); link; link = link->next )
				{
					refreshLinkDisp( link , label , baseAddr );
				}
			}
			return true;
		}

		void  clearLink()
		{
			for( LabelList::iterator iter = mLabelList.begin();
				iter != mLabelList.end() ; ++iter )
			{
				Label* label = *iter;
				Label::Link* link = label->getLink();
				while( link )
				{
					Label::Link* next = link->next;
					freeLabelLink( link );
					link = next;
				}

				label->mState  = Label::eUnused;
				label->mOffset = 0;
#if _DEBUG
				label->mIndex  = INDEX_NONE;
#endif
			}
			mLabelList.clear();
		}

		void adjustLabelLinks( SysInt startOffset, SysInt delta )
		{
			for( LabelList::iterator iter = mLabelList.begin();
				 iter != mLabelList.end() ; ++iter )
			{
				Label* label = *iter;
				
				// Adjust bound label offsets
				if ( label->mState == Label::eBound )
				{
					if ( label->mOffset >= (SysUint)startOffset )
						label->mOffset += delta;
				}

				// Adjust link offsets
				for( auto link = label->getLink(); link; link = link->next )
				{
					if ( link->offset >= startOffset )
					{
						link->offset += delta;
					}
				}
			}
		}

	protected:


		void freeLabelLink( Label::Link* link )
		{
			delete link;
		}
		Label::Link* createLabelLink()
		{
			return new Label::Link;
		}

		void addLabelLink( Label* label , Label::LinkMode mode , int dispSize , SysInt dispOffset = 0 )
		{
			assert(label);
			Label::Link* link = createLabelLink();

			link->mode       = mode;
			link->offset     = _this()->getOffset();
			link->dispOffset = dispOffset;
			link->dispSize   = dispSize;
			label->addLink( link );

			switch( label->mState )
			{
			case Label::eUnused:
#if _DEBUG
				label->mIndex = mLabelList.size();
#endif
				label->mState = Label::eUnbound;
				mLabelList.push_back( label );
				break;
#if _DEBUG
			default:
				assert( mLabelList[ label->mIndex ] == label );
#endif
			}
		}

		void refreshLinkDisp(  Label::Link* link  , Label* label , void* baseAddr )
		{
			void* addr = static_cast< uint8* >( baseAddr ) + link->offset;

			SysInt disp = label->mOffset;
			switch( link->mode )
			{
			case Label::eLinkAbs:
				disp += reinterpret_cast< SysInt >( baseAddr ) + link->dispOffset;
				break;
			case Label::eLinkRel:
				// RIP-relative displacement
				// disp = TargetAddress - ReturnAddress
				// TargetAddress = label->mOffset
				// ReturnAddress = link->offset + link->dispSize
				disp = label->mOffset - ( link->offset + link->dispSize );
				disp += link->dispOffset;
				break;
			}

			switch( link->dispSize )
			{
			case 1:
				*static_cast< int8*>( addr ) = int8( disp );
				break;
			case 2:
				*static_cast< int16*>( addr ) = int16( disp );
				break;
			case 4:
				*static_cast< int32*>( addr ) = int32( disp );
				break;
			}
		}


	private:

		typedef std::vector< Label* > LabelList;
		LabelList mLabelList;
	};

} //namespace asmeta

#undef FP_INIST_OPA
#undef FP_INIST_OPB
#undef INT_INIST_OPA
#undef INT_INIST_OPB
#undef INT_INIST_OPR

#endif // Assembler_h__