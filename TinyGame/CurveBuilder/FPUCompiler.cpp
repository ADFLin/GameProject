#include "FPUCompiler.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "Assembler.h"
#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
static void* AllocExecutableMemory(size_t size)
{
	return ::VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
}
static void  FreeExecutableMemory(void* ptr)
{
	::VirtualFree(ptr, 0, MEM_RELEASE);
}
#else
static void* AllocExecutableMemory(size_t size)
{
	return ::malloc(size);
}
static void  FreeExecutableMemory(void* ptr)
{
	::free(ptr);
}
#endif

//#include "FPUCode.h"
using namespace Asmeta;

class FPUCodeTemplate : public Asmeta::AssemblerT< FPUCodeTemplate >
	                  , public ExprParse
{
	typedef AssemblerT< FPUCodeTemplate > Asm;
	typedef double ValueType;
#define VALUE_PTR qword_ptr

public:
	void codeInit();
	void codeConstValue(ValueType const&val);
	void codeVar(ValueType* varPtr);
	void codeInput(uint8 inputIndex);
	void codeFunction(FunInfo const& info);
	void codeBinaryOp(TokenType type,bool isReverse);
	void codeUnaryOp(TokenType type);
	void codeEnd();
	void setCodeData(FPUCodeData* data){  mData = data; }

protected:
	static int const FpuRegNum = 8;
	void  checkStackPrevValPtr();
	void  resolvePushValues( TokenType type )
	{




	}
	template< class Type > 
	void  evalBinaryOp( TokenType type , bool isReverse , Type const& src );

	FPUCodeData* mData;

public:
	void   emitByte( uint8 byte1 ){ mData->pushCode( byte1 ); }
	void   emitWord( uint16 val ){ mData->pushCodeT( val ); }
	void   emitWord( uint8 byte1 , uint8 byte2 ){ mData->pushCode( byte1 , byte2 ); }
	void   emitDWord(uint32 val) { mData->pushCode(val); }
	void   emitPtr ( void* ptr ){ mData->pushCode( ptr ); }
	
	uint32 getOffset(){ return mData->getCodeLength(); }

	

	struct PushValue
	{
		PushValue()
		{
			locLoad = -1;
		}
		// ( - -2 ) Extra EBP -1 no Load (0 - 7) REG
		//ExprParse::Unit unit;
		int locLoad;
	};

	Asmeta::Label            mConstLabel;
	std::vector< double >    mConstTable;
	std::vector< PushValue > mPushValues;
	int mIdxPushVarStart;
	int mIdxPushConstStart;
	std::vector< int > mStacks;


	int           mIdxNextPush;
	int           mNumPushValue;

	int           mIdxRegValueOffset;
	TokenType     mPrevType;
	int           mNumVarStack;
	int           mNumInstruction;

	int           mIdxRegSave;

	double*       mPrevVarPtr;
	int           mPrevIdxCV;
	uint8         mPrevInputIndex;
};

FPUCompiler::FPUCompiler()
{
	mOptimization = false;
}

double FooTest(double x, double y)
{
	return x + y;
}

float gc;
float ga = 1.2f;
float gb;
__declspec(noinline) static void foo()
{

	float a1 = 1;
	float a2 = 2;
	float a3 = 3;
	float a4 = 4;
	float a5 = 5;
	float a6 = 6;
	float a7 = 7;
	float a8 = 8;
	float a9 = 9;

	//__asm
	//{
	//	fld a1;
	//	fld a2;
	//	fld a3;
	//	fld a4;
	//	fld a5;
	//	fld a6;
	//	fld a7;
	//	fld a8;
	//	fld a9;

	//	fstp a1;
	//}

	gc = ga * ga * ga;
}

bool FPUCompiler::compile( char const* expr , SymbolTable const& table , FPUCodeData& data , int numInput )
{
#if 0
	ga = 1.2f;
	foo();
	double x = 1;
	double y = 2;
	double c = FooTest(x, y);
#endif

	try
	{
		ExpressionParser parser;

		if ( !parser.parse( expr , table , mResult ) )
			return false;

		if (mOptimization)
			mResult.optimize();

		FPUCodeTemplate codeTemplate;
		data.mNumInput = numInput;
		codeTemplate.setCodeData( &data );
		mResult.generateCode(codeTemplate);
		return true;
	}
	catch ( ParseException&  )
	{
		return false;
	}
	catch ( std::exception& )
	{
		return false;
	}
}


template< class T >
static T subOffset( T offset )
{
	return (~offset) + 1;
}

void FPUCodeTemplate::codeInit()
{
	//m_pData->eval();

	mPrevType = TOKEN_NONE;
	mNumVarStack = 0;
	mIdxRegSave = -1;
	mNumInstruction = 0;
	mPrevVarPtr = 0;

	mConstTable.clear();
	mData->clearCode();
	mPushValues.reserve( 10 );
	Asm::clearLink();

	Asm::push( ebp );
	Asm::mov( ebp , esp );
	mNumInstruction += 2;

	// ??
	//m_pData->pushCode(MSTART);
	//++m_NumInstruction;
}

void FPUCodeTemplate::codeEnd()
{
	if ( IsValue( mPrevType ) )
	{
		checkStackPrevValPtr();
	}

	Asm::mov( esp , ebp );
	Asm::pop( ebp );
	Asm::ret();
	mNumInstruction += 3;

	Asm::bind( &mConstLabel );

	int   num = mData->getCodeLength();
	for (int i = 0 ; i < mConstTable.size(); ++i )
	{
		mData->pushCode( mConstTable[i] );
	}

	Asm::reloc( mData->mCode );
#if _DEBUG
	mData->mNumInstruction = mNumInstruction;
#endif
}


void FPUCodeTemplate::codeConstValue( ValueType const& val )
{
	checkStackPrevValPtr();
	//find if have the same value before

	int idx = 0;
	int size = (int)mConstTable.size();
	for(  ; idx < size ; ++idx )
	{
		if ( mConstTable[idx] == val )
			break;
	}

	if ( idx == size )
	{
		mConstTable.push_back( val );
	}

	mPrevIdxCV = idx;
	mPrevType = VALUE_CONST;

}

void FPUCodeTemplate::codeVar( ValueType* varPtr )
{
	checkStackPrevValPtr();
	mPrevVarPtr = varPtr;
	mPrevType   = VALUE_VARIABLE;
}

void FPUCodeTemplate::codeInput(uint8 inputIndex)
{
	checkStackPrevValPtr();
	mPrevInputIndex = inputIndex;
	mPrevType = VALUE_INPUT;
}

void FPUCodeTemplate::codeFunction( FunInfo const& info )
{
	checkStackPrevValPtr();

	int   numParam = info.numParam;
	int   numSPUParam = numParam;

	unsigned espOffset = sizeof( ValueType ) * numParam;
	int32 paramOffset = 0;

	if ( mNumVarStack > FpuRegNum )
	{
		int numCpuStack = mNumVarStack - FpuRegNum;
		int numCpuParam = std::min( numParam , numCpuStack );

		paramOffset -= numCpuStack * sizeof( ValueType );
		for ( int num = 0; num < numCpuParam ; ++num )
		{
			paramOffset -= sizeof( ValueType );

			Asm::fstp( VALUE_PTR( esp , int8( paramOffset ) ) );
			Asm::fld( VALUE_PTR( ebp , int8( -( numCpuStack - num  ) * sizeof( ValueType ) ) ) );
			mNumInstruction += 2;
		}

		numSPUParam = numParam - numCpuParam;
		espOffset += sizeof( ValueType ) * numCpuStack;
	}

	assert( numParam < 256 / sizeof( ValueType ) );

	for ( int num = 0; num < numSPUParam ; ++num )
	{
		paramOffset -= sizeof( ValueType );
		Asm::fstp( VALUE_PTR( esp , int8( paramOffset ) ) );
		++mNumInstruction;
	}

	Asm::sub( esp , imm8u( uint8( espOffset ) ) );
	Asm::call( ptr( (void*)&info.ptrFun ) );
	Asm::add( esp , imm8u( uint8( espOffset ) ) );
	mNumInstruction += 3;

	mNumVarStack -= numParam;
	mPrevType = TOKEN_FUN;
}

template< class Type >
void FPUCodeTemplate::evalBinaryOp( TokenType type , bool isReverse , Type const& src  )
{
	switch( type )
	{
	case BOP_ADD: 
		Asm::fadd( src ); 
		break;
	case BOP_MUL:
		Asm::fmul( src ); 
		break;
	case BOP_SUB:
		if ( isReverse )
			Asm::fsubr( src ); 
		else
			Asm::fsub( src );
		break;
	case BOP_DIV:
		if ( isReverse )
			Asm::fdivr( src ); 
		else
			Asm::fdiv( src );
		break;
	case BOP_COMMA:
		Asm::fstp( st(0) );
		Asm::fld( src );
		break;
	}
}
void FPUCodeTemplate::codeBinaryOp( TokenType type , bool isReverse )
{

	if ( type == BOP_ASSIGN )
	{
		Asm::fst( VALUE_PTR( mPrevVarPtr ) );
		++mNumInstruction;
	}
	else
	{
		if ( IsValue( mPrevType ) )
		{
			switch( mPrevType )
			{
			case VALUE_CONST:
				{
					evalBinaryOp( type , isReverse , VALUE_PTR( &mConstLabel , mPrevIdxCV * sizeof( ValueType ) ) );
				}
				break;
			case VALUE_VARIABLE:
				{
					evalBinaryOp( type , isReverse , VALUE_PTR( mPrevVarPtr ) );
				}
				break;
			case VALUE_INPUT:
				{
					evalBinaryOp(type, isReverse, VALUE_PTR( ebp , int( (mPrevInputIndex + 1 ) * sizeof(ValueType) )) );
				}
				break;
			}

			++mNumInstruction;
		}
		else
		{
			if ( mNumVarStack > FpuRegNum )
			{
				int32 offset = -( mNumVarStack - FpuRegNum ) * sizeof( ValueType );
				evalBinaryOp( type , isReverse , VALUE_PTR( ebp , int32( offset ) ) );
				++mNumInstruction;
			}
			else
			{
				switch( type )
				{
				case BOP_ADD: 
					Asm::faddp( st(1) ); 
					break;
				case BOP_SUB:
					if ( isReverse )
						Asm::fsubrp( st(1) ); 
					else
						Asm::fsubp( st(1) );
					break;
				case BOP_MUL:
					Asm::fmulp( st(1) ); 
					break;
				case BOP_DIV:
					if ( isReverse )
						Asm::fdivrp( st(1) ); 
					else
						Asm::fdivp( st(1) );
					break;
				case BOP_COMMA:
					Asm::fstp( st(1) );
					break;
				}
				++mNumInstruction;
			}
			--mNumVarStack;
		}
	}
	mPrevType = TOKEN_BINARY_OP;
}

void FPUCodeTemplate::codeUnaryOp( TokenType type )
{
	checkStackPrevValPtr();

	if ( type == UOP_MINS )
	{
		Asm::fchs();
		++mNumInstruction;
	}

	mPrevType = TOKEN_FUN;
}

void FPUCodeTemplate::checkStackPrevValPtr()
{
	if ( !ExprParse::IsValue( mPrevType ) )
		return;

	bool isVar = ( mPrevType == VALUE_VARIABLE );

	++mNumVarStack;
	if ( mNumVarStack >= FpuRegNum )
	{
		//fstp        qword ptr [ebp - offset] 
		int8 offset = -( mNumVarStack - FpuRegNum ) * sizeof( ValueType );
		Asm::fstp( VALUE_PTR( ebp , offset ) );
		++mNumInstruction;
	}

	switch( mPrevType )
	{
	case VALUE_CONST:
		Asm::fld(VALUE_PTR(&mConstLabel, mPrevIdxCV * sizeof(ValueType)));
		break;
	case VALUE_VARIABLE:
		Asm::fld(VALUE_PTR(mPrevVarPtr));
		break;
	case VALUE_INPUT:
		Asm::fld(VALUE_PTR(ebp, int((mPrevInputIndex + 1) * sizeof(ValueType))));
		break;
	}

	++mNumInstruction;

}


void FPUCodeData::setCode( unsigned pos , uint8 byte )
{
	assert( pos < getCodeLength() );
	mCode[ pos ] = byte;
}
void FPUCodeData::setCode( unsigned pos , void* ptr )
{
	assert( pos < getCodeLength() );
	*reinterpret_cast< void** >( mCode + pos ) = ptr; 
}

void FPUCodeData::pushCode( uint8 byte )
{
	checkCodeSize(1);
	pushCodeInternal( byte );
}

void FPUCodeData::pushCode( uint8 byte1,uint8 byte2 )
{
	checkCodeSize( 2 );
	pushCodeInternal(byte1); 
	pushCodeInternal(byte2);
}

void FPUCodeData::pushCode( uint8 byte1,uint8 byte2,uint8 byte3 )
{
	checkCodeSize( 3 );
	pushCodeInternal(byte1); 
	pushCodeInternal(byte2);
	pushCodeInternal(byte3);
}

void FPUCodeData::pushCodeInternal( uint8 byte )
{
	*mCodeEnd = byte;
	++mCodeEnd;
}


void FPUCodeData::printCode()
{
	std::cout << std::hex;
	int num = getCodeLength();
	for (int i= 0;i < num ; ++i)
	{
		int c1 = mCode[i] &  0x0f;
		int c2 = mCode[i] >> 4;
		std::cout << c1 << c2 << " ";

		if ((i+1) % 8 == 0)
			std::cout << "| ";
		if ((i+1) % 32 == 0)
			std::cout << std::endl;
	}
	std::cout << std::dec << std::endl;
}


FPUCodeData::FPUCodeData( int size )
{
	mMaxCodeSize = ( size ) ? size : 128;
	mCode = ( uint8* ) AllocExecutableMemory( mMaxCodeSize );
	assert( mCode );
	clearCode();
}


FPUCodeData::~FPUCodeData()
{
	FreeExecutableMemory( mCode );
}

double FPUCodeData::eval() const
{
	assert(mNumInput == 0);
	typedef double (__cdecl *EvalFun)();
	return reinterpret_cast< EvalFun >( &mCode[0] )();
}

double FPUCodeData::eval(double p0) const
{
	assert(mNumInput == 1);
	typedef double(__cdecl *EvalFun)(double);
	return reinterpret_cast< EvalFun >(&mCode[0])(p0);
}

double FPUCodeData::eval(double p0, double p1) const
{
	assert(mNumInput == 2);
	typedef double(__cdecl *EvalFun)(double,double);
	return reinterpret_cast<EvalFun>(&mCode[0])(p0,p1);
}

double FPUCodeData::eval(double p0, double p1, double p2) const
{
	assert(mNumInput == 3);
	typedef double(__cdecl *EvalFun)(double,double,double);
	return reinterpret_cast<EvalFun>(&mCode[0])(p0,p1,p2);
}

void FPUCodeData::checkCodeSize( int freeSize )
{
	int codeNum = getCodeLength();
	if ( codeNum + freeSize <= mMaxCodeSize )
		return;

	int newSize = 2 * mMaxCodeSize;
	assert( newSize >= codeNum + freeSize );

	uint8* newPtr = (uint8*)AllocExecutableMemory( newSize );
	if ( !newPtr )
		throw std::bad_alloc();

	memcpy( newPtr , mCode , codeNum );
	FreeExecutableMemory( mCode );
	mCode = newPtr;
	mCodeEnd = mCode + codeNum;
	mMaxCodeSize = newSize;
}

void FPUCodeData::clearCode()
{
	memset(mCode,0,mMaxCodeSize );
	mCodeEnd = mCode;
}
