#ifndef AsmetaBase_h__
#define AsmetaBase_h__

#include "PlatformConfig.h"
#include "Core/IntegerType.h"

#define ASMETA_STATIC_ASSERT( COND )\
struct _TESTCOND_##__LINE__{ int val [ ( COND ) ? 1 : -1 ]; };

//#define ASMETA_INLINE __forceinline 
#define ASMETA_INLINE inline


namespace Asmeta
{
#if TARGET_PLATFORM_64BITS
	typedef int64 SysInt;
	typedef uint64 SysUint;
#else
	typedef int32 SysInt;
	typedef uint32 SysUint;
#endif

	inline bool isByteInt( SysInt value ){ return -128 <= value && value <= 127; }


	class Label;

	template< int N >
	struct Int2Type {};

	enum FPMemFormat
	{
		FPMF_Real32     = 0x0,
		FPMF_Integer32  = 0x1,
		FPMF_eReal64    = 0x2,
		FPMF_eInteger16 = 0x3,
	};

	template< int Size >
	struct SPFMap {};
	template<>
	struct SPFMap< 2 >
	{ 
		static FPMemFormat constexpr IntFormat = FPMF_eInteger16;
	};
	template<>
	struct SPFMap< 4 >
	{
		static FPMemFormat constexpr RealFormat = FPMF_Real32;
		static FPMemFormat constexpr IntFormat = FPMF_Integer32;
	};
	template<>
	struct SPFMap< 8 >
	{
		static FPMemFormat constexpr RealFormat = FPMF_eReal64;
	};

	enum FlagModRM
	{
		MOD_M      = 0x00,
		MOD_DISP8  = 0x01,
		MOD_DISP32 = 0x02, 
		MOD_R      = 0x03, 

		RM_EAX     = 0x00,
		RM_ECX     = 0x01,
		RM_EDX     = 0x02,
		RM_EBX     = 0x03,
		RM_MD_SIB  = 0x04,
		RM_R_ESP   = 0x04,
		RM_M_DISP  = 0x05,
		RM_DR_EBP  = 0x05,
		RM_EDI     = 0x06,
		RM_ESI     = 0x07,
	};

	enum FlagSIB
	{
		SIB_NOINDEX = 0x04,
		SIB_NOBASE  = 0x05, //MOD == 00
	};

#define MOD_RM_BYTE( MODE , OP , RM )\
	uint8( ( (MODE) << 6 ) | ( (OP) << 3 ) | ( RM ) ) 
#define SIB_BYTE( SCALE , INDEX , BASE )\
	uint8( ( (SCALE) << 6 ) | ( INDEX ) << 3 | ( BASE ) )

	class Reg
	{
	public:
		enum Code
		{
			eAX = 0x0, eCX = 0x1, eDX = 0x2, eBX = 0x3,
			eSP = 0x4, eBP = 0x5, eSI = 0x6, eDI = 0x7,

			eAL = 0x0, eCL = 0x1, eDL = 0x2, eBL = 0x3,
			eAH = 0x0, eCH = 0x5, eDH = 0x6, eBH = 0x7,

			eES = 0x0 , eCS = 0x1, eSS = 0x2, eDS = 0x3, eFS = 0x4 , eGS = 0x5 ,
		};

	};


	class RegSeg
	{
	public:
		RegSeg( Reg::Code code ): mCode( code ){}
		Reg::Code code() const { return mCode; }
	private:
		Reg::Code mCode;
	};

	template< int Size >
	class RegX86
	{
	public:
		enum { size = Size };
		RegX86( Reg::Code code ): mCode( code ){}
		Reg::Code code() const { return mCode; }
	private:
		Reg::Code mCode;
	};

	class Reg8 : public RegX86< 1 >
	{
	public:
		Reg8( Reg::Code type ): RegX86< 1 >( type ){}
	};

	class Reg16 : public RegX86< 2 >
	{
	public:
		Reg16( Reg::Code type ): RegX86< 2 >( type ){}
	};

	class Reg32 : public RegX86< 4 >
	{
	public:
		Reg32( Reg::Code type ): RegX86< 4 >( type ){}
	};

	class Reg64 : public RegX86< 8 >
	{
	public:
		Reg64(Reg::Code type) : RegX86< 8 >(type) {}
	};

	class RegST
	{
	public:
		RegST( uint8 idx ): mIndex( idx ){}
		uint8 index() const { return mIndex; }
	private:
		uint8 mIndex;
	};

	enum CondTestField
	{
		CTF_O  = 0x0 , CTF_NO  = 0x1 , CTF_B  = 0x2 , CTF_NB  = 0x3, 
		CTF_E  = 0x4 , CTF_NE  = 0x5 , CTF_BE = 0x6 , CTF_NBE = 0x7, 
		CTF_S  = 0x8 , CTF_NS  = 0x9 , CTF_P  = 0xa , CTF_NP  = 0xb, 
		CTF_L  = 0xc , CTF_NL  = 0xd , CTF_LE = 0xe , CTF_NLE = 0xf,
		CTF_NAE = CTF_B  , CTF_AE = CTF_NB  , CTF_Z  = CTF_E  , CTF_NZ = CTF_NE  , 
		CTF_NA  = CTF_BE , CTF_A  = CTF_NBE , CTF_PE = CTF_P  , CTF_PO = CTF_NP  , 
		CTF_NGE = CTF_L  , CTF_GE = CTF_NL  , CTF_NG = CTF_LE , CTF_G  = CTF_NLE ,
	};

	class Address
	{
	public:
		typedef Address const& RefType;
		Address( void* ptr ):ptr(ptr){}
		void* value() const { return ptr; }
	private:
		void* ptr;
	};

	class LabelPtr
	{
	public:
		typedef LabelPtr RefType;

		LabelPtr( Label* label , SysInt offset = 0 )
			:mLabel( label ) , mOffset( offset ){}
	private:
		Label* mLabel;
		SysInt mOffset;
		template< class T >
		friend class AssemblerT;
	};

	class RegPtr
	{
	public:
		typedef RegPtr RefType;
		RegPtr( Reg32 const& base )                                                 {  init( base );  }
		RegPtr( Reg32 const& base , int8 disp )                                     {  init( base , MOD_DISP8 , disp );  }
		RegPtr( Reg32 const& base , SysInt disp )                                   {  init( base , GetDispMode(disp), disp );  }
		RegPtr( Reg32 const& base , Reg32 const& index , uint8 shift )              {  init( base , MOD_M , index , shift , 0 );  }
		RegPtr( Reg32 const& base , Reg32 const& index , uint8 shift , SysInt disp ){  init( base , GetDispMode(disp), index , shift , disp );  }
		RegPtr( Reg32 const& base , Reg32 const& index , uint8 shift , int8 disp )  {  init( base , MOD_DISP8 , index , shift , disp );  }
	private:

		static FlagModRM GetDispMode(SysInt disp)
		{
			return (int8(disp) == disp) ? MOD_DISP8: MOD_DISP32 ;
		}

		void init( Reg32 const& base )
		{
			switch( base.code() )
			{
			case Reg::eSP:
				mModByte = MOD_RM_BYTE( MOD_M , 0 , RM_MD_SIB );
				mSIBByte = SIB_BYTE( 0 , SIB_NOINDEX , Reg::eSP );
				break;
			case Reg::eBP:
				mModByte = MOD_RM_BYTE( MOD_DISP8 , 0 , RM_DR_EBP );
				mSIBByte = 0;
				break;
			default:
				mModByte = MOD_RM_BYTE( MOD_M , 0 , base.code() );
				mSIBByte = 0;
				break;
			}
			mDisp = 0;
		}
		void init( Reg32 const& base , uint8 mod , SysInt disp )
		{
			assert( mod == MOD_DISP8 || mod == MOD_DISP32 );

			if ( base.code() == Reg::eSP )
			{
				mModByte = MOD_RM_BYTE( mod  , 0 , RM_MD_SIB );
				mSIBByte = SIB_BYTE( 0 , SIB_NOINDEX , Reg::eSP );
			}
			else
			{
				mModByte = MOD_RM_BYTE( mod  , 0 , base.code() );
				mSIBByte = 0;
			}
			mDisp = disp;
		}

		void init( Reg32 const& base , uint8 mod , Reg32 const& index , uint8 shift , SysInt disp )
		{
			assert( base.code() != SIB_NOBASE );
			assert( index.code() != SIB_NOINDEX );
			assert( shift < 4 );
			mModByte = MOD_RM_BYTE( mod , 0 , RM_MD_SIB );
			mSIBByte = SIB_BYTE( shift , index.code() , base.code() );
			mDisp    = disp;
		}
		uint8   mModByte;
		uint8   mSIBByte;
		SysInt  mDisp;
		template< class T >
		friend  class AssemblerT;
	};

	template< class Ref , int Size >
	class RefMem
	{
	public:
		typedef typename Ref::RefType RefType;
		RefMem( RefType ref ):mRef( ref ){}
		RefType reference() const { return mRef ; }
	private:
		RefType mRef; 
	};
	uint8 const PUI_PATTERN = 0xd8;



	enum FPInstCode
	{
#define FP_INIST_LIST( op )\
	op( fgADD  , 0x0 , 0x0 )\
	op( fgMUL  , 0x0 , 0x1 )\
	op( fgCOM  , 0x0 , 0x2 )\
	op( fgCOMP , 0x0 , 0x3 )\
	op( fgSUB  , 0x0 , 0x4 )\
	op( fgDIV  , 0x0 , 0x6 )\
	op( fgFLD  , 0x1 , 0x0 )\
	op( fgST   , 0x1 , 0x2 )\
	op( fgSTP  , 0x1 , 0x3 )\

#define ENUM_OP( A , B , C ) A,
		FP_INIST_LIST(ENUM_OP)
#undef  ENUM_OP
	};
	struct FPInist
	{
		uint8 opA;
		uint8 opB;
	};
	static FPInist const gFPInistMap[] = 
	{
#define CODE_OP_DATA_OP( CODE , oA , oB ) 	{ oA , oB } ,
		FP_INIST_LIST(CODE_OP_DATA_OP)
#undef CODE_OP_DATA_OP
	};
#define FP_INIST_OPA( CODE ) gFPInistMap[ CODE ].opA
#define FP_INIST_OPB( CODE ) gFPInistMap[ CODE ].opB

	enum IntInstCode
	{
#define INT_INIST_LIST( op )\
	op( igADD , 0x00 , 0x80  , 0x0 )\
	op( igOR  , 0x08 , 0x80  , 0x1 )\
	op( igADC , 0x10 , 0x80  , 0x2 )\
	op( igSBB , 0x18 , 0x80  , 0x3 )\
	op( igAND , 0x20 , 0x80  , 0x4 )\
	op( igSUB , 0x28 , 0x80  , 0x5 )\
	op( igXOR , 0x30 , 0x80  , 0x6 )\
	op( igCMP , 0x38 , 0x80  , 0x7 )\
	\
	op( igNOT  , 0xf6 , 0x00  , 0x2 )\
	op( igNEG  , 0xf6 , 0x00  , 0x3 )\
	op( igMUL  , 0xf6 , 0x00  , 0x4 )\
	op( igIMUL , 0xf6 , 0x00  , 0x5 )\
	op( igDIV  , 0xf6 , 0x00  , 0x6 )\
	op( igIDIV , 0xf6 , 0x00  , 0x7 )\
	\
	op( igROL  , 0xd0 , 0xc0  , 0x0 )\
	op( igROR  , 0xd0 , 0xc0  , 0x1 )\
	op( igRCL  , 0xd0 , 0xc0  , 0x2 )\
	op( igRCR  , 0xd0 , 0xc0  , 0x3 )\
	op( igSHL  , 0xd0 , 0xc0  , 0x4 )\
	op( igSHR  , 0xd0 , 0xc0  , 0x5 )\
	op( igSAL  , 0xd0 , 0xc0  , 0x4 )\
	op( igSAR  , 0xd0 , 0xc0  , 0x7 )\
	\
	op( igPUSH , 0xff , 0x68  , 0x6 )\
	op( igPOP  , 0x8f , 0x00  , 0x0 )\
	op( igCALL , 0xff , 0x00  , 0x2 )\
	op( igJCC  , 0x70 , 0x0f  , 0x8 )\
	\
	op( igINS  , 0x6c , 0x00  , 0x0 )\
	op( igLODS , 0xac , 0x00  , 0x0 )\
	op( igMOVS , 0xa4 , 0x00  , 0x0 )\
	op( igMOV  , 0x88 , 0x00  , 0x0 )\
	op( igOUTS , 0x6e , 0x00  , 0x0 )\
	op( igSTOS , 0xaa , 0x00  , 0x0 )\
	op( igCMPS , 0xa6 , 0x00  , 0x0 )\
	op( igSCAS , 0xae , 0x00  , 0x0 )\
	\
	op( igTEST , 0x84 , 0xf6  , 0x0 )\

#define ENUM_OP( A , B , C , D ) A,
		INT_INIST_LIST(ENUM_OP)
#undef  ENUM_OP
	};

	struct IntInist
	{
		uint8 opA;
		uint8 opB;
		uint8 opR;
	};
	static IntInist const gIntInistMap[] = 
	{
#define CODE_OP_DATA_OP( CODE , oA , oB , oR ) 	{ oA , oB , oR } ,
		INT_INIST_LIST(CODE_OP_DATA_OP)
#undef CODE_OP_DATA_OP
	};

#define INT_INIST_OPA( CODE ) gIntInistMap[ CODE ].opA
#define INT_INIST_OPB( CODE ) gIntInistMap[ CODE ].opB
#define INT_INIST_OPR( CODE ) gIntInistMap[ CODE ].opR

	class ImmediateBase
	{
	public:
		ASMETA_INLINE ImmediateBase( SysInt val , bool s = false ):mVal( val ),mSigned( s ){}
		SysInt  value() const { return mVal; }
	protected:
		SysInt  mVal;
		bool    mSigned;
	};

	template< int ImmSize >
	class Immediate : public ImmediateBase
	{
	public:
		template< class T >
		ASMETA_INLINE Immediate( T val , bool s = false ):ImmediateBase( (SysUint) val , s ){}
		uint8 size() const { return ImmSize; }
	};

	template<>
	class Immediate<0> : public ImmediateBase
	{
	public:
		template< class T >
		ASMETA_INLINE Immediate( T val , bool s = false ):ImmediateBase( (SysUint) val , s ){}
		uint8 size() const { return isByteInt( mVal ) ? 1 : 4 ; }
	};

	template< int Size >
	class Disp
	{
	public:
		template< class T >
		Disp( T val ):mVal( SysInt( val ) ){}
		SysInt value() const { return mVal; }
	private:
		SysInt mVal;
	};

}//namespace Asmeta

#endif // AsmetaBase_h__
