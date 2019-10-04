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

class AsmCodeGenerator : public Asmeta::AssemblerT< FPUCodeGeneratorV0 >
	                   , public ExprParse
{
public:

	typedef AssemblerT< FPUCodeGeneratorV0 > Asm;

	void   emitByte(uint8 byte1) { mData->pushCode(byte1); }
	void   emitWord(uint16 val) { mData->pushCodeT(val); }
	void   emitWord(uint8 byte1, uint8 byte2) { mData->pushCode(byte1, byte2); }
	void   emitDWord(uint32 val) { mData->pushCode(val); }
	void   emitPtr(void* ptr) { mData->pushCode(ptr); }

	uint32 getOffset() { return mData->getCodeLength(); }
	void setCodeData(ExecutableCode* data) { mData = data; }

	ExecutableCode* mData;
	int           mNumInstruction;
};

class FPUCodeGeneratorBase : public AsmCodeGenerator
{
public:
	typedef double ValueType;
#define VALUE_PTR qword_ptr
	static int const FpuRegNum = 8;

	struct StackValue
	{
		ValueLayout layout;
		SysInt      offset;
	};
	std::vector< StackValue >  mInputStack;
	std::vector< StackValue >  mConstStack;
	Asmeta::Label           mConstLabel;
	std::vector< uint8 >    mConstStorage;

	int addConstValue(ConstValueInfo const& val)
	{

		int idx = 0;
		int size = (int)mConstStack.size();
		for( ; idx < size; ++idx )
		{
			auto const& stackValue = mConstStack[idx];
			if( stackValue.layout == val.layout && val.isSameValue(&mConstStorage[stackValue.offset]) )
				break;
		}

		if( idx == size )
		{
			StackValue stackValue;
			stackValue.layout = val.layout;
			if( mConstStack.empty() )
			{
				stackValue.offset = 0;
			}
			else
			{
				stackValue.offset = mConstStack.back().offset + GetLayoutSize(mConstStack.back().layout);
			}
			mConstStack.push_back(stackValue);
			mConstStorage.resize(mConstStack.size() + GetLayoutSize(val.layout));
			val.assignTo(&mConstStorage[stackValue.offset]);
		}

		return idx;
	}

	struct ValueInfo
	{
		ValueInfo()
		{
			type == TOKEN_NONE;
			idxStack = -1;
		}
		TokenType type;
		union
		{
			VariableInfo const* var;
			int     idxCV;
			int     idxInput;
		};
		int     idxStack;
	};

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mInputStack.resize(numInput);
		SysInt offset = 0;
		for( int i = 0; i < numInput; ++i )
		{
			offset += GetLayoutSize(inputLayouts[i]);
			mInputStack[i].layout = inputLayouts[i];
			mInputStack[i].offset = offset;
		}
		mConstStorage.clear();
		mConstStack.clear();
	}

	template< class Type >
	int  emitBOP(TokenType type, bool isReverse, Type const& src)
	{
		switch( type )
		{
		case BOP_ADD:
			Asm::fadd(src);
			return 1;
		case BOP_MUL:
			Asm::fmul(src);
			return 1;
		case BOP_SUB:
			if( isReverse )
				Asm::fsubr(src);
			else
				Asm::fsub(src);
			return 1;
		case BOP_DIV:
			if( isReverse )
				Asm::fdivr(src);
			else
				Asm::fdiv(src);
			return 1;
		case BOP_COMMA:
			Asm::fstp(st(0));
			Asm::fld(src);
			return 2;
		}
		return 0;
	}

	template< class Type >
	int  emitBOPInt(TokenType type, bool isReverse, Type const& src)
	{
		switch( type )
		{
		case BOP_ADD:
			Asm::fiadd(src);
			return 1;
		case BOP_MUL:
			Asm::fimul(src);
			return 1;
		case BOP_SUB:
			if( isReverse )
				Asm::fisubr(src);
			else
				Asm::fisub(src);
			return 1;
		case BOP_DIV:
			if( isReverse )
				Asm::fidivr(src);
			else
				Asm::fidiv(src);
			return 1;
		case BOP_COMMA:
			Asm::fstp(st(0));
			Asm::fild(src);
			return 2;
		}
		return 0;
	}

	template< class Type >
	int emitBOPPop(TokenType type, bool isReverse, Type const& src)
	{
		switch( type )
		{
		case BOP_ADD:
			Asm::faddp(src);
			return 1;
		case BOP_MUL:
			Asm::fmulp(src);
			return 1;
		case BOP_SUB:
			if( isReverse )
				Asm::fsubrp(src);
			else
				Asm::fsubp(src);
			return 1;
		case BOP_DIV:
			if( isReverse )
				Asm::fdivrp(src);
			else
				Asm::fdivp(src);
			return 1;
		case BOP_COMMA:
			Asm::fstp(src);
			return 1;
		}
		return 0;
	}

	template< class ...Args >
	int emitBOPWithLayout(TokenType opType, bool isReverse, ValueLayout layout, Args&& ...args)
	{
		switch( layout )
		{
		case ValueLayout::Double: return emitBOP(opType, isReverse, qword_ptr(args...)); break;
		case ValueLayout::Float:  return emitBOP(opType, isReverse, dword_ptr(args...)); break;
		case ValueLayout::Int32:  return emitBOPInt(opType, isReverse, dword_ptr(args...)); break;
		default:
			assert(0);
		}
		return 0;
	}

	template< class ...Args >
	int emitStoreValueWithLayout(ValueLayout layout, Args&& ...args)
	{
		switch( layout )
		{
		case ValueLayout::Double: Asm::fst(qword_ptr(args...)); break;
		case ValueLayout::Float:  Asm::fst(dword_ptr(args...)); break;
		case ValueLayout::Int32:  Asm::fist(dword_ptr(args...)); break;
		default:
			assert(0);
		}
		return 1;
	}

	template< class ...Args >
	int emitStoreValuePopWithLayout(ValueLayout layout, Args&& ...args)
	{
		switch( layout )
		{
		case ValueLayout::Double: Asm::fstp(qword_ptr(args...)); break;
		case ValueLayout::Float:  Asm::fstp(dword_ptr(args...)); break;
		case ValueLayout::Int32:  Asm::fistp(dword_ptr(args...)); break;
		default:
			assert(0);
		}
		return 1;
	}

	template< class ...Args >
	int emitLoadValueWithLayout(ValueLayout layout, Args&& ...args)
	{
		switch( layout )
		{
		case ValueLayout::Double: Asm::fld(qword_ptr(args...)); break;
		case ValueLayout::Float:  Asm::fld(dword_ptr(args...)); break;
		case ValueLayout::Int32:  Asm::fild(dword_ptr(args...)); break;
		default:
			return 0;
		}
		return 1;
	}

	int emitLoadValue(ValueInfo const& info)
	{
		switch( info.type )
		{
		case VALUE_VARIABLE:
			return emitLoadValueWithLayout(info.var->layout, info.var->ptr);
		case VALUE_INPUT:
			{
				StackValue& inputValue = mInputStack[info.idxInput];
				return emitLoadValueWithLayout(inputValue.layout, ebp, inputValue.offset);
			}
		case VALUE_CONST:
			{
				StackValue& constValue = mConstStack[info.idxCV];
				return emitLoadValueWithLayout(constValue.layout, &mConstLabel, constValue.offset);
			}
		}

		return 0;
	}
};


class FPUCodeGeneratorV0 : public FPUCodeGeneratorBase
{
	
	typedef FPUCodeGeneratorBase BaseClass;
public:

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		BaseClass::codeInit(numInput, inputLayouts);
		//m_pData->eval();
		mNumVarStack = 0;
		mNumInstruction = 0;

		mPrevValue.type = TOKEN_NONE;
		mPrevValue.var = nullptr;


		mData->clearCode();
		Asm::clearLink();

		Asm::push(ebp);
		Asm::mov(ebp, esp);
		mNumInstruction += 2;

		// ??
		//mData->pushCode(MSTART);
		//++m_NumInstruction;
	}

	void codeConstValue(ConstValueInfo const& val)
	{
		checkStackPrevValPtr();
		//find if have the same value before

		int idx = addConstValue( val );

		mPrevValue.type = VALUE_CONST;
		mPrevValue.idxCV = idx;
		mPrevValue.idxStack = findStack(mPrevValue);
	}

	void codeVar(VariableInfo const& varInfo)
	{
		checkStackPrevValPtr();

		mPrevValue.type = VALUE_VARIABLE;
		mPrevValue.var = &varInfo;
		mPrevValue.idxStack = findStack(mPrevValue);
	}

	void codeInput(uint8 inputIndex)
	{
		checkStackPrevValPtr();

		mPrevValue.type = VALUE_INPUT;
		mPrevValue.idxInput = inputIndex;
		mPrevValue.idxStack = findStack(mPrevValue);
	}

	void codeFunction(FuncInfo const& info)
	{
		checkStackPrevValPtr();

		int   numParam = info.getArgNum();
		int   numSPUParam = numParam;

		unsigned espOffset = sizeof(ValueType) * numParam;

		if( numParam )
		{
			int32 paramOffset = 0;

			if( mNumVarStack > FpuRegNum )
			{
				int numCpuStack = mNumVarStack - FpuRegNum;
				int numCpuParam = std::min(numParam, numCpuStack);

				paramOffset -= numCpuStack * sizeof(ValueType);
				for( int num = 0; num < numCpuParam; ++num )
				{
					paramOffset -= sizeof(ValueType);
					Asm::fstp(VALUE_PTR(esp, int8(paramOffset)));
					Asm::fld(VALUE_PTR(ebp, int8(-(numCpuStack - num) * sizeof(ValueType))));

					mRegStack.back().type = TOKEN_NONE;
				}

				mNumInstruction += 2 * numCpuParam;
				numSPUParam = numParam - numCpuParam;
				espOffset += sizeof(ValueType) * numCpuStack;
			}

			assert(numParam < 256 / sizeof(ValueType));

			for( int num = 0; num < numSPUParam; ++num )
			{
				paramOffset -= sizeof(ValueType);
				Asm::fstp(VALUE_PTR(esp, int8(paramOffset)));
				mRegStack.pop_back();
			}
			mNumInstruction += 1 * numSPUParam;
		}

		Asm::sub(esp, imm8u(uint8(espOffset)));
		Asm::call(ptr((void*)&info.funcPtr));
		Asm::add(esp, imm8u(uint8(espOffset)));

		mRegStack.push_back(ValueInfo());
		mNumInstruction += 3;

		mNumVarStack -= numParam - 1;
		mPrevValue.type = TOKEN_FUN;
		mPrevValue.idxStack = -1;
	}

	void codeBinaryOp(TokenType opType, bool isReverse)
	{
		if( opType == BOP_ASSIGN )
		{
			mNumInstruction += emitStoreValueWithLayout(mPrevValue.var->layout, mPrevValue.var->ptr);
		}
		else
		{
			if( mPrevValue.idxStack != -1 )
			{
				int idxReg = mRegStack.size() - mPrevValue.idxStack - 1;
				if( idxReg == 0 && opType == BOP_COMMA )
				{

				}
				else
				{
					mNumInstruction += emitBOP(opType, isReverse, st(idxReg));
				}
				mRegStack.back().type = TOKEN_NONE;
			}
			else if( IsValue(mPrevValue.type) )
			{
				switch( mPrevValue.type )
				{
				case VALUE_VARIABLE:
					{
						mNumInstruction += emitBOPWithLayout(opType, isReverse, mPrevValue.var->layout, mPrevValue.var->ptr);
					}
					break;
				case VALUE_INPUT:
					{
						StackValue& inputValue = mInputStack[mPrevValue.idxInput];
						mNumInstruction += emitBOPWithLayout(opType, isReverse, inputValue.layout, ebp, inputValue.offset);
					}
					break;
				case VALUE_CONST:
					{
						StackValue& constValue = mConstStack[mPrevValue.idxCV];
						mNumInstruction += emitBOPWithLayout(opType, isReverse, constValue.layout , &mConstLabel, constValue.offset);
					}
					break;
				}

				mRegStack.back().type = TOKEN_NONE;
			}
			else
			{
				if( mNumVarStack > FpuRegNum )
				{
					int32 offset = -(mNumVarStack - FpuRegNum) * sizeof(ValueType);
					mNumInstruction += emitBOP(opType, isReverse, VALUE_PTR(ebp, SysInt(offset)));

					mRegStack.back().type = TOKEN_NONE;
				}
				else
				{
					mNumInstruction += emitBOPPop(opType, isReverse, st(1));
				}
				--mNumVarStack;
				mRegStack.pop_back();
			}
		}
		mPrevValue.type = TOKEN_BINARY_OP;
		mPrevValue.idxStack = -1;
	}

	void codeUnaryOp(TokenType type)
	{
		checkStackPrevValPtr();

		if( type == UOP_MINS )
		{
			Asm::fchs();
			++mNumInstruction;
			mRegStack.back().type == TOKEN_NONE;
		}

		mPrevValue.type = TOKEN_FUN;
		mPrevValue.idxStack = -1;
	}

	void codeEnd()
	{
		if( IsValue(mPrevValue.type) )
		{
			checkStackPrevValPtr();
		}

		Asm::mov(esp, ebp);
		Asm::pop(ebp);
		Asm::ret();
		mNumInstruction += 3;

		Asm::bind(&mConstLabel);

		int   num = mData->getCodeLength();
		if( !mConstStorage.empty() )
		{
			mData->pushCode(&mConstStorage[0], mConstStorage.size());
		}

		Asm::reloc(mData->mCode);
#if _DEBUG
		mData->mNumInstruction = mNumInstruction;
#endif
	}

protected:


	void checkStackPrevValPtr()
	{
		if( !ExprParse::IsValue(mPrevValue.type) )
			return;

		bool isVar = (mPrevValue.type == VALUE_VARIABLE);

		++mNumVarStack;
		if( mNumVarStack >= FpuRegNum )
		{
			//fstp        qword ptr [ebp - offset] 
			int8 offset = -(mNumVarStack - FpuRegNum) * sizeof(ValueType);
			Asm::fstp(VALUE_PTR(ebp, offset));
			mRegStack.pop_back();
			++mNumInstruction;
		}

		mNumInstruction += emitLoadValue(mPrevValue);
		mRegStack.push_back(mPrevValue);
	}


	ValueInfo     mPrevValue;
	int           mNumVarStack;
	std::vector< ValueInfo >    mRegStack;

	int findStack(ValueInfo const& value)
	{
		for( int i = 0; i < mRegStack.size(); ++i )
		{
			if( value.type == mRegStack[i].type )
			{
				switch( value.type )
				{
				case VALUE_CONST:
					if( value.idxCV == mRegStack[i].idxCV )
						return i;
					break;
				case VALUE_VARIABLE:
					if( value.var == mRegStack[i].var )
						return i;
					break;
				case VALUE_INPUT:
					if( value.idxInput == mRegStack[i].idxInput )
						return i;
					break;
				}
			}
		}
		return -1;
	}
};


class FPUCodeGeneratorV1 : public FPUCodeGeneratorBase
{

public:
	std::vector< RealType > mConstTable;

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mNumInstruction = 0;
		mRegStack.clear();
		mStackValues.clear();
#if 0
		ValueInfo info;
		info.type = TOKEN_NONE;
		info.idxCV = 0;
		info.idxStack = 0;
		mStackValues.push_back(info);
#endif

		mConstTable.clear();
		mData->clearCode();
		Asm::clearLink();

		Asm::push(ebp);
		Asm::mov(ebp, esp);
		mNumInstruction += 2;
	}
	void codeConstValue(RealType const&val)
	{
		int idx = 0;
		int size = (int)mConstTable.size();
		for( ; idx < size; ++idx )
		{
			if( mConstTable[idx] == val )
				break;
		}

		if( idx == size )
		{
			mConstTable.push_back(val);
		}


		ValueInfo info;
		info.type = VALUE_CONST;
		info.idxCV = idx;
		info.idxStack = -1;
		mStackValues.push_back(info);
	}
	void codeVar(VariableInfo const& varInfo)
	{
		ValueInfo info;
		info.type = VALUE_VARIABLE;
		info.var = &varInfo;
		info.idxStack = -1;
		mStackValues.push_back(info);
	}
	void codeInput(uint8 inputIndex)
	{
		ValueInfo info;
		info.type = VALUE_INPUT;
		info.idxInput = inputIndex;
		info.idxStack = -1;
		mStackValues.push_back(info);
	}

	void codeFunction(FuncInfo const& info);
	void codeBinaryOp(TokenType type, bool isReverse)
	{
		if( type == BOP_ASSIGN )
		{
			ValueInfo valueVar = mStackValues.back();
			mStackValues.pop_back();
			ValueInfo& value = mStackValues.back();
			if( value.type == TOKEN_NONE )
			{
				mNumInstruction += emitStoreValueWithLayout(valueVar.var->layout, valueVar.var->ptr);
			}
			else
			{
				uint8 indexReg = getRegIndex(value);
				if( indexReg != -1 )
				{
					Asm::fst(st(indexReg));
				}
				else
				{
					mNumInstruction += emitLoadValue(value);
					mNumInstruction += emitStoreValueWithLayout(valueVar.var->layout, valueVar.var->ptr);	
					//TODO: use move?
				}
			}
		}


	}
	void codeUnaryOp(TokenType type);
	void codeEnd()
	{
		Asm::mov(esp, ebp);
		Asm::pop(ebp);
		Asm::ret();
		mNumInstruction += 3;

		Asm::bind(&mConstLabel);
		int   num = mData->getCodeLength();
		for( int i = 0; i < mConstTable.size(); ++i )
		{
			mData->pushCode(mConstTable[i]);
		}

		Asm::reloc(mData->mCode);
#if _DEBUG
		mData->mNumInstruction = mNumInstruction;
#endif
	}

protected:

	uint8 getRegIndex(ValueInfo const& info)
	{
		while( info.idxStack != 0 )
		{


		}


	}
	std::vector< int > mRegStack;
	std::vector< ValueInfo > mStackValues;

};

FPUCompiler::FPUCompiler()
{
	mOptimization = false;
}

__declspec(noinline) double FooTest(double x, double y)
{
	return x + y;
}

__declspec(noinline) double FooTest2(double x, double y)
{
	return x - y;
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

#if 0
	__asm
	{
		fld a1;
		fld a2;
		fld a3;
		fld a4;
		fld a5;
		fld a6;
		fld a7;
		fld a8;
		fld a9;

		fstp a1;
	}
#endif

	gc = ga * ga * ga;
}


double gC;
bool FPUCompiler::compile( char const* expr , SymbolTable const& table , ExecutableCode& data , int numInput , ValueLayout inputLayouts[] )
{
#if 1
	ga = 1.2f;
	foo();
	double x = 1;
	double y = 2;

	double(*pFun)(double x, double y);
	pFun = rand() % 2 ? FooTest : FooTest2;
	gc = pFun(x, y);

	LogMsg("%f", gc);
#endif

	try
	{
		ExpressionParser parser;

		if ( !parser.parse( expr , table , mResult ) )
			return false;

		if (mOptimization)
			mResult.optimize();

		FPUCodeGeneratorV0 generator;
		data.mNumInput = numInput;
		generator.setCodeData( &data );
		mResult.generateCode(generator , numInput, inputLayouts);
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


void ExecutableCode::setCode( unsigned pos , uint8 byte )
{
	assert( pos < getCodeLength() );
	mCode[ pos ] = byte;
}
void ExecutableCode::setCode( unsigned pos , void* ptr )
{
	assert( pos < getCodeLength() );
	*reinterpret_cast< void** >( mCode + pos ) = ptr; 
}

void ExecutableCode::pushCode( uint8 byte )
{
	checkCodeSize(1);
	pushCodeInternal( byte );
}

void ExecutableCode::pushCode( uint8 byte1,uint8 byte2 )
{
	checkCodeSize( 2 );
	pushCodeInternal(byte1); 
	pushCodeInternal(byte2);
}

void ExecutableCode::pushCode( uint8 byte1,uint8 byte2,uint8 byte3 )
{
	checkCodeSize( 3 );
	pushCodeInternal(byte1); 
	pushCodeInternal(byte2);
	pushCodeInternal(byte3);
}

void ExecutableCode::pushCode(uint8 const* data, int size)
{
	checkCodeSize(size);
	memcpy(mCodeEnd, data, size);
	mCodeEnd += size;
}

void ExecutableCode::pushCodeInternal( uint8 byte )
{
	*mCodeEnd = byte;
	++mCodeEnd;
}


void ExecutableCode::printCode()
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


ExecutableCode::ExecutableCode( int size )
{
	mMaxCodeSize = ( size ) ? size : 128;
	mCode = ( uint8* ) AllocExecutableMemory( mMaxCodeSize );
	assert( mCode );
	clearCode();
}


ExecutableCode::~ExecutableCode()
{
	FreeExecutableMemory( mCode );
}

void ExecutableCode::checkCodeSize( int freeSize )
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

void ExecutableCode::clearCode()
{
	memset(mCode,0,mMaxCodeSize );
	mCodeEnd = mCode;
}
