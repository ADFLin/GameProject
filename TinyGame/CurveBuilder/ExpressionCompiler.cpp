#include "ExpressionCompiler.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>

#if ENABLE_FPU_CODE

#include "Assembler.h"
#include "PlatformConfig.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"


class ExecutableHeapManager
{
public:
	ExecutableHeapManager()
	{
		mHandle = ::HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 1 * 1024 * 1204, 128 * 1024 * 1204);
	}

	~ExecutableHeapManager()
	{
		::HeapDestroy(mHandle);
	}

	static ExecutableHeapManager& Get() { static ExecutableHeapManager sInstance; return sInstance; }

	void* alloc(size_t size)
	{
		return ::HeapAlloc(mHandle, 0, size);
	}
	void* realloc(void* p, size_t size)
	{
		return ::HeapReAlloc(mHandle, 0, p, size);
	}

	void free(void* p)
	{
		::HeapFree(mHandle, 0, p);
	}

	HANDLE mHandle;
};

static void* AllocExecutableMemory(size_t size)
{
	return ExecutableHeapManager::Get().alloc(size);
}
static void* ReallocExecutableMemory(void* ptr, size_t size)
{
	return ExecutableHeapManager::Get().realloc(ptr, size);
}
static void  FreeExecutableMemory(void* ptr)
{
	return ExecutableHeapManager::Get().free(ptr);
}
#else
static void* AllocExecutableMemory(size_t size)
{
	return ::malloc(size);
}
static void* ReallocExecutableMemory(void* ptr, size_t size)
{
	return realloc(ptr, size);
}
static void  FreeExecutableMemory(void* ptr)
{
	::free(ptr);
}

#endif

#define VALUE_PTR qword_ptr

//#include "FPUCode.h"
using namespace Asmeta;

class AsmCodeGenerator : public Asmeta::AssemblerT< AsmCodeGenerator >
	                   , public ExprParse
{
public:

	using Asm = AssemblerT< AsmCodeGenerator >;
	void   emitByte(uint8 byte1) { mData->pushCode(byte1); }
	void   emitWord(uint16 val) { mData->pushCodeT(val); }
	void   emitWord(uint8 byte1, uint8 byte2) { mData->pushCode(byte1, byte2); }
	void   emitDWord(uint32 val) { mData->pushCode(val); }
	void   emitQWord(uint64 val) { mData->pushCode(val); }
	void   emitPtr(void* ptr) { mData->pushCode(ptr); }

	uint32 getOffset() { return mData->getCodeLength(); }
	void setCodeData(ExecutableCode* data) { mData = data; }

	ExecutableCode* mData;
	int           mNumInstruction;
};

class FPUCodeGeneratorBase : public AsmCodeGenerator
{
public:
	using ValueType = double;

	static int const FpuRegNum = 8;

	struct StackValue
	{
		ValueLayout layout;
		int32       offset;
	};
	TArray< StackValue >  mInputStack;
	TArray< StackValue >  mConstStack;
	Asmeta::Label           mConstLabel;
	TArray< uint8 >    mConstStorage;

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
			mConstStorage.resize(mConstStorage.size() + GetLayoutSize(val.layout));
			val.assignTo(&mConstStorage[stackValue.offset]);
		}

		return idx;
	}

	struct ValueInfo
	{
		ValueInfo()
		{
			type == TOKEN_NONE;
			idxStack = INDEX_NONE;
		}
		TokenType type;
		union
		{
			VariableInfo const* var;
			InputInfo const*    input;
			int     idxCV;
		};
		int     idxStack;
	};

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mInputStack.resize(numInput);
		//#TODO : consider func call type
		// ebp + 0 : esp
#if 1
		SysInt offset = 8;
		for( int i = 0; i < numInput; ++i )
		{
			mInputStack[i].layout = inputLayouts[i];
			mInputStack[i].offset = offset;
			offset += GetLayoutSize(inputLayouts[i]);
		}
#else
		SysInt offset = 0;
		for (int i = 0; i < numInput; ++i)
		{
			offset += GetLayoutSize(inputLayouts[i]);
			mInputStack[i].layout = inputLayouts[i];
			mInputStack[i].offset = offset;
		}
#endif
		mConstStorage.clear();
		mConstStack.clear();

		Asm::push(ebp);
		Asm::mov(ebp, esp);
		mNumInstruction += 2;
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
		if (IsPointer(layout))
		{
			Asm::mov(eax, SYSTEM_PTR(std::forward<Args>(args)...));
			return 1 + emitBOPWithBaseLayout( opType , isReverse , ToBase(layout), eax);
		}

		return emitBOPWithBaseLayout(opType , isReverse , layout , std::forward<Args>(args)...);
	}

	template< class ...Args >
	int emitBOPWithBaseLayout(TokenType opType, bool isReverse, ValueLayout layout, Args&& ...args)
	{
		switch (layout)
		{
		case ValueLayout::Double: return emitBOP(opType, isReverse, qword_ptr(std::forward<Args>(args)...));
		case ValueLayout::Float:  return emitBOP(opType, isReverse, dword_ptr(std::forward<Args>(args)...));
		case ValueLayout::Int32:  return emitBOPInt(opType, isReverse, dword_ptr(std::forward<Args>(args)...));
		default:
			NEVER_REACH("emitBOPWithBaseLayout miss case");
		}
		return 0;
	}

	template< class ...Args >
	int emitStoreValueWithLayout(ValueLayout layout, Args&& ...args)
	{
		if (IsPointer(layout))
		{
			Asm::mov(eax, SYSTEM_PTR(std::forward<Args>(args)...));
			return 1 + emitStoreValueWithBaseLayout(ToBase(layout), eax);
		}
		return emitStoreValueWithBaseLayout(layout, std::forward<Args>(args)...);
	}

	template< class ...Args >
	int emitStoreValueWithBaseLayout(ValueLayout layout, Args&& ...args)
	{
		switch (layout)
		{
		case ValueLayout::Double: Asm::fst(qword_ptr(std::forward<Args>(args)...)); return 1;
		case ValueLayout::Float:  Asm::fst(dword_ptr(std::forward<Args>(args)...)); return 1;
		case ValueLayout::Int32:  Asm::fist(dword_ptr(std::forward<Args>(args)...)); return 1;
		default:
			NEVER_REACH("emitStoreValueWithBaseLayout miss case");
		}
		return 0;
	}

	template< class ...Args >
	int emitStoreValuePopWithLayout(ValueLayout layout, Args&& ...args)
	{
		if (IsPointer(layout))
		{
			Asm::mov(eax, SYSTEM_PTR(std::forward<Args>(args)...));
			return 1 + emitStoreValuePopWithBaseLayout(ToBase(layout), eax);
		}

		return emitStoreValuePopWithBaseLayout(layout, std::forward<Args>(args)...);
	}

	template< class ...Args >
	int emitStoreValuePopWithBaseLayout(ValueLayout layout, Args&& ...args)
	{
		switch (layout)
		{
		case ValueLayout::Double: Asm::fstp(qword_ptr(std::forward<Args>(args)...)); return 1;
		case ValueLayout::Float:  Asm::fstp(dword_ptr(std::forward<Args>(args)...)); return 1;
		case ValueLayout::Int32:  Asm::fistp(dword_ptr(std::forward<Args>(args)...)); return 1;
		default:
			NEVER_REACH("emitStoreValuePopWithBaseLayout miss case");
		}
		return 0;
	}

	template< class ...Args >
	int emitLoadValueWithLayout(ValueLayout layout, Args&& ...args)
	{
		if (IsPointer(layout))
		{
			Asm::mov(eax, SYSTEM_PTR(std::forward<Args>(args)...));
			return 1 + emitLoadValueWithBaseLayout(ToBase(layout), eax);
		}

		return emitLoadValueWithBaseLayout(layout, std::forward<Args>(args)...);
	}

	template< class ...Args >
	int emitLoadValueWithBaseLayout(ValueLayout layout, Args&& ...args)
	{
		switch (layout)
		{
		case ValueLayout::Double: Asm::fld(qword_ptr(std::forward<Args>(args)...)); return 1;
		case ValueLayout::Float:  Asm::fld(dword_ptr(std::forward<Args>(args)...)); return 1;
		case ValueLayout::Int32:  Asm::fild(dword_ptr(std::forward<Args>(args)...)); return 1;
		default:
			NEVER_REACH("emitLoadValueWithBaseLayout miss case");
		}
		return 0;
	}

	int emitLoadValue(ValueInfo const& info)
	{
		switch( info.type )
		{
		case VALUE_VARIABLE:
			return emitLoadValueWithLayout(info.var->layout, info.var->ptr);
		case VALUE_INPUT:
			{
				StackValue& inputValue = mInputStack[info.input->index];
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
	
	using BaseClass = FPUCodeGeneratorBase;
public:

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mData->clearCode();
		mNumVarStack = 0;
		mNumInstruction = 0;

		mPrevValue.type = TOKEN_NONE;
		mPrevValue.var = nullptr;

		Asm::clearLink();

		BaseClass::codeInit(numInput, inputLayouts);
		//m_pData->eval();


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

	void codeInput(InputInfo const& input)
	{
		checkStackPrevValPtr();

		mPrevValue.type     = VALUE_INPUT;
		mPrevValue.input = &input;
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

				//#TODO : consider value layout
				paramOffset -= numCpuStack * sizeof(ValueType);
				for( int num = 0; num < numCpuParam; ++num )
				{
					paramOffset -= sizeof(ValueType);
					Asm::fstp(VALUE_PTR(esp, int32(paramOffset)));
					Asm::fld(VALUE_PTR(ebp, int32(-(numCpuStack - num) * sizeof(ValueType))));

					mRegStack.back().type = TOKEN_NONE;
				}

				mNumInstruction += 2 * numCpuParam;
				numSPUParam = numParam - numCpuParam;
				espOffset += sizeof(ValueType) * numCpuStack;
			}


			for( int num = 0; num < numSPUParam; ++num )
			{
				paramOffset -= sizeof(ValueType);
				Asm::fstp(VALUE_PTR(esp, int32(paramOffset)));
				mRegStack.pop_back();
			}
			mNumInstruction += 1 * numSPUParam;
		}

		Asm::sub(esp, imm8u(uint8(espOffset)));
		Asm::call(ptr((void*)&info.funcPtr));
		Asm::add(esp, imm8u(uint8(espOffset)));

		mRegStack.emplace_back();
		mNumInstruction += 3;

		mNumVarStack -= numParam - 1;
		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxStack = INDEX_NONE;
	}

	void codeBinaryOp(TokenType opType, bool isReverse)
	{
		if( opType == BOP_ASSIGN )
		{
			switch (mPrevValue.type)
			{
			case VALUE_INPUT:
				{
					StackValue& inputValue = mInputStack[mPrevValue.input->index];
					if ( !IsPointer(inputValue.layout) )
					{
						throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Input value layout is not pointer when assign");
					}
					mNumInstruction += emitStoreValueWithLayout(inputValue.layout, ebp, inputValue.offset);
				}
				break;
			case VALUE_VARIABLE:
				{
					mNumInstruction += emitStoreValueWithLayout(mPrevValue.var->layout, mPrevValue.var->ptr);
				}
				break;
			}		
		}
		else
		{
			if( mPrevValue.idxStack != INDEX_NONE)
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
						StackValue& inputValue = mInputStack[mPrevValue.input->index];
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
					mNumInstruction += emitBOP(opType, isReverse, VALUE_PTR(ebp, int32(offset)));

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
		mPrevValue.idxStack = INDEX_NONE;
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

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxStack = INDEX_NONE;
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

		int num = mData->getCodeLength();
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
	TArray< ValueInfo >    mRegStack;

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
					if( value.input == mRegStack[i].input )
						return i;
					break;
				}
			}
		}
		return INDEX_NONE;
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
		info.idxStack = INDEX_NONE;
		mStackValues.push_back(info);
	}
	void codeVar(VariableInfo const& varInfo)
	{
		ValueInfo info;
		info.type = VALUE_VARIABLE;
		info.var = &varInfo;
		info.idxStack = INDEX_NONE;
		mStackValues.push_back(info);
	}
	void codeInput(InputInfo const& input)
	{
		ValueInfo info;
		info.type = VALUE_INPUT;
		info.input = &input;
		info.idxStack = INDEX_NONE;
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
				if( indexReg != INDEX_NONE )
				{
					Asm::fst(st(indexReg));
				}
				else
				{
					mNumInstruction += emitLoadValue(value);
					mNumInstruction += emitStoreValueWithLayout(valueVar.var->layout, valueVar.var->ptr);	
					//#TODO: use move?
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
		for(RealType cValue : mConstTable)
		{
			mData->pushCode(cValue);
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

#elif ENABLE_BYTE_CODE

#define EBC_USE_FIXED_SIZE 2
#define EBC_USE_COMPOSITIVE_CODE 1

namespace EExprByteCode
{
	enum Type : uint8
	{
		Dummy,
		Const,
		Input,
		Variable,

#if EBC_USE_COMPOSITIVE_CODE

		Add,
		CAdd,
		IAdd,
		VAdd,

		Sub,
		CSub,
		ISub,
		VSub,

		SubR,
		CSubR,
		ISubR,
		VSubR,

		Mul,
		CMul,
		IMul,
		VMul,

		Div,
		CDiv,
		IDiv,
		VDiv,

		DivR,
		CDivR,
		IDivR,
		VDivR,

		TAdd,
		TMul,
#else

		Add,
		Sub,
		SubR,
		Mul,
		Div,
		DivR,
#endif
		Mins,

		FuncSymbol,
		FuncSymbolEnd = FuncSymbol + EFuncSymbol::COUNT,

		FuncCall0 = FuncSymbolEnd,
		FuncCall1,
		FuncCall2,
		FuncCall3,
		FuncCall4,
		FuncCall5,
	};
}

#define GET_CONST(CODE) (mCode->mConstValues[CODE])
#define GET_INPUT(CODE) (mInputs[CODE])
#define GET_VAR(CODE)	(*(RealType*)mCode->mPointers[CODE])

template<typename TValue>
TValue TByteCodeExecutor<TValue>::doExecute(ExecutableCode const& code)
{
	mCode = &code;

	int index = 0;
	int numCodes = code.mCodes.size();
	uint8 const* pCode = code.mCodes.data();
	uint8 const* pCodeEnd = pCode + numCodes;
	TValue  stackValues[32];
	TValue* stackValue = stackValues;
	TValue  topValue;
	switch (*pCode)
	{
	case EExprByteCode::Const:
		topValue = GET_CONST(pCode[1]);
		break;
	case EExprByteCode::Input:
		topValue = GET_INPUT(pCode[1]);
		break;
	case EExprByteCode::Variable:
		topValue = GET_VAR(pCode[1]);
		break;
	case EExprByteCode::FuncCall0:
	{
		void* funcPtr = mCode->mPointers[pCode[1]];
		topValue = (*static_cast<FuncType0>(funcPtr))();
	}
	break;
	default:
		return 0.0f;
	}
#if EBC_USE_FIXED_SIZE
	pCode += EBC_USE_FIXED_SIZE;
#else
	pCode += 2;
#endif
#define EXECUTE_CODE() execCode(pCode, stackValue, topValue)

#if EBC_USE_FIXED_SIZE
	while (pCode < pCodeEnd)
	{
		EXECUTE_CODE();
		pCode += EBC_USE_FIXED_SIZE;
	}
#else
	while (pCode < pCodeEnd)
	{
		pCode += EXECUTE_CODE();
	}
#endif
	return topValue;
}

template<typename TValue>
FORCEINLINE int TByteCodeExecutor<TValue>::execCode(uint8 const* pCode, TValue*& pValueStack, TValue& topValue)
{
	auto pushStack = [&pValueStack](TValue const& value)
	{
		*pValueStack = value;
		++pValueStack;
	};

	auto popStack = [&pValueStack]() -> TValue const&
	{
		--pValueStack;
		return *pValueStack;
	};

	switch (*pCode)
	{
	case EExprByteCode::Const:
		pushStack(topValue);
		topValue = GET_CONST(pCode[1]);
		return 2;
	case EExprByteCode::Input:
		pushStack(topValue);
		topValue = GET_INPUT(pCode[1]);
		return 2;
	case EExprByteCode::Variable:
		pushStack(topValue);
		topValue = GET_VAR(pCode[1]);
		return 2;
	case EExprByteCode::Add:
	{
		auto const& lhs = popStack();
		topValue = lhs + topValue;
	}
	return 1;
	case EExprByteCode::Sub:
	{
		auto const& lhs = popStack();
		topValue = lhs - topValue;
	}
	return 1;
	case EExprByteCode::SubR:
	{
		auto const& lhs = popStack();
		topValue = topValue - lhs;
	}
	return 1;
	case EExprByteCode::Mul:
	{
		auto const& lhs = popStack();
		topValue = lhs * topValue;
	}
	return 1;
	case EExprByteCode::Div:
	{
		auto const& lhs = popStack();
		topValue = lhs / topValue;
	}
	return 1;
	case EExprByteCode::DivR:
	{
		auto const& lhs = popStack();
		topValue = topValue / lhs;
	}
	return 1;
	case EExprByteCode::Mins:
	{
		topValue = -topValue;
	}
	return 1;

#if EBC_USE_COMPOSITIVE_CODE
	case EExprByteCode::TAdd:
	{
		topValue = topValue + topValue;
	}
	return 1;
	case EExprByteCode::TMul:
	{
		topValue = topValue * topValue;
	}
	return 1;
	case EExprByteCode::CAdd:
	{
		auto lhs = GET_CONST(pCode[1]);
		topValue = lhs + topValue;
	}
	return 2;
	case EExprByteCode::CSub:
	{
		auto lhs = GET_CONST(pCode[1]);
		topValue = lhs - topValue;
	}
	return 2;
	case EExprByteCode::CSubR:
	{
		auto lhs = GET_CONST(pCode[1]);
		topValue = topValue - lhs;
	}
	return 2;
	case EExprByteCode::CMul:
	{
		auto lhs = GET_CONST(pCode[1]);
		topValue = lhs * topValue;
	}
	return 2;
	case EExprByteCode::CDiv:
	{
		auto lhs = GET_CONST(pCode[1]);
		topValue = lhs / topValue;
	}
	return 2;
	case EExprByteCode::CDivR:
	{
		auto lhs = GET_CONST(pCode[1]);
		topValue = topValue / lhs;
	}
	return 2;
	case EExprByteCode::IAdd:
	{
		auto const& lhs = GET_INPUT(pCode[1]);
		topValue = lhs + topValue;
	}
	return 2;
	case EExprByteCode::ISub:
	{
		auto const& lhs = GET_INPUT(pCode[1]);
		topValue = lhs - topValue;
	}
	return 2;
	case EExprByteCode::ISubR:
	{
		auto const& lhs = GET_INPUT(pCode[1]);
		topValue = topValue - lhs;
	}
	return 2;
	case EExprByteCode::IMul:
	{
		auto const& lhs = GET_INPUT(pCode[1]);
		topValue = lhs * topValue;
	}
	return 2;
	case EExprByteCode::IDiv:
	{
		auto const& lhs = GET_INPUT(pCode[1]);
		topValue = lhs / topValue;
	}
	return 2;
	case EExprByteCode::IDivR:
	{
		auto const& lhs = GET_INPUT(pCode[1]);
		topValue = topValue / lhs;
	}
	return 2;
	case EExprByteCode::VAdd:
	{
		auto lhs = GET_VAR(pCode[1]);
		topValue = lhs + topValue;
	}
	return 2;
	case EExprByteCode::VSub:
	{
		auto lhs = GET_VAR(pCode[1]);
		topValue = lhs - topValue;
	}
	return 2;
	case EExprByteCode::VSubR:
	{
		auto lhs = GET_VAR(pCode[1]);
		topValue = topValue - lhs;
	}
	return 2;
	case EExprByteCode::VMul:
	{
		auto lhs = GET_VAR(pCode[1]);
		topValue = lhs * topValue;
	}
	return 2;
	case EExprByteCode::VDiv:
	{
		auto lhs = GET_VAR(pCode[1]);
		topValue = lhs / topValue;
	}
	return 2;
	case EExprByteCode::VDivR:
	{
		auto lhs = GET_VAR(pCode[1]);
		topValue = topValue / lhs;
	}
	return 2;
#endif
	case EExprByteCode::FuncSymbol + EFuncSymbol::Exp:  topValue = exp(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Ln:   topValue = log(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Sin:  topValue = sin(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Cos:  topValue = cos(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Tan:  topValue = tan(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Cot:  topValue = 1.0 / tan(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Sec:  topValue = 1.0 / cos(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Csc:  topValue = 1.0 / sin(topValue); return 1;
	case EExprByteCode::FuncSymbol + EFuncSymbol::Sqrt: topValue = sqrt(topValue); return 1;
#if 0
	case EExprByteCode::FuncCall0:
		{
			pushStack(topValue);
			void* funcPtr = mCode->mPointers[pCode[1]];
			topValue = (*static_cast<FuncType0>(funcPtr))();
		}
		return 2;
	case EExprByteCode::FuncCall1:
		if constexpr (std::is_same_v<TValue, RealType>)
		{
			void* funcPtr = mCode->mPointers[pCode[1]];
			topValue = (*static_cast<FuncType1>(funcPtr))(topValue);
		}
		else
		{
			void* funcPtr = mCode->mPointers[pCode[1]];
			topValue = FloatVector(
				(*static_cast<FuncType1>(funcPtr))(topValue[0]),
				(*static_cast<FuncType1>(funcPtr))(topValue[1]),
				(*static_cast<FuncType1>(funcPtr))(topValue[2]),
				(*static_cast<FuncType1>(funcPtr))(topValue[3]));
		}
		return 2;
	case EExprByteCode::FuncCall2:
		if constexpr (std::is_same_v<TValue, RealType>)
		{
			void* funcPtr = mCode->mPointers[pCode[1]];
			RealType params[1];
			params[0] = popStack();
			topValue = (*static_cast<FuncType2>(funcPtr))(params[0], topValue);
		}
		else
		{
			void* funcPtr = mCode->mPointers[pCode[1]];
			FloatVector params[1];
			params[0] = popStack();
			topValue = FloatVector(
				(*static_cast<FuncType2>(funcPtr))(params[0][0], topValue[0]),
				(*static_cast<FuncType2>(funcPtr))(params[0][1], topValue[1]),
				(*static_cast<FuncType2>(funcPtr))(params[0][2], topValue[2]),
				(*static_cast<FuncType2>(funcPtr))(params[0][3], topValue[3]));
		}
		return 2;
	case EExprByteCode::FuncCall3:
	{
		void* funcPtr = mCode->mPointers[pCode[1]];
		RealType params[2];
		params[0] = popStack();
		params[1] = popStack();
		topValue = (*static_cast<FuncType3>(funcPtr))(params[1], params[0], topValue);
	}
	return 2;
	case EExprByteCode::FuncCall4:
	{
		void* funcPtr = mCode->mPointers[pCode[1]];
		RealType params[3];
		params[0] = popStack();
		params[1] = popStack();
		params[2] = popStack();
		topValue = (*static_cast<FuncType4>(funcPtr))(params[2], params[1], params[0], topValue);
	}
	return 2;
	case EExprByteCode::FuncCall5:
	{
		void* funcPtr = mCode->mPointers[pCode[1]];
		RealType params[4];
		params[0] = popStack();
		params[1] = popStack();
		params[2] = popStack();
		params[3] = popStack();
		topValue = (*static_cast<FuncType5>(funcPtr))(params[3], params[2], params[1], params[0], topValue);
	}
	return 2;
#endif
	}
	return 1;
}

template RealType TByteCodeExecutor<RealType>::doExecute(ExecutableCode const& code);
template FloatVector TByteCodeExecutor<FloatVector>::doExecute(ExecutableCode const& code);

struct ExprByteCodeCompiler : public ExprParse
{
	ExecutableCode& mOutput;
	int mNumCmd = 0;

	ExprByteCodeCompiler(ExecutableCode& output)
		:mOutput(output)
	{

	}
	using TokenType = ParseResult::TokenType;

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mStacks.clear();
		mNumCmd = 0;
	}
	void codeConstValue(ConstValueInfo const&val)
	{
		CHECK(val.layout == ValueLayout::Real);
		int index = mOutput.mConstValues.findIndex(val.asReal);
		if ( index == INDEX_NONE )
		{
			index = mOutput.mConstValues.size();
			mOutput.mConstValues.push_back(val.asReal);
		}
		CHECK(index < 255);
		pushValue(EExprByteCode::Const, index);
	}

	void codeVar(VariableInfo const& varInfo)
	{
		CHECK(varInfo.layout == ValueLayout::Real);
		int index = mOutput.mPointers.findIndex(varInfo.ptr);
		if (index == INDEX_NONE)
		{
			index = mOutput.mPointers.size();
			mOutput.mPointers.push_back(varInfo.ptr);
		}
		CHECK(index < 255);
		pushValue(EExprByteCode::Variable, index);
	}

	void codeInput(InputInfo const& input)
	{
		pushValue(EExprByteCode::Input, input.index);
	}

	int mNumInputUsed = 0;
	int mNumVarUsed = 0;


	void codeFunction(FuncInfo const& info)
	{
		int index = mOutput.mPointers.findIndex(info.funcPtr);
		if (index == INDEX_NONE)
		{
			index = mOutput.mPointers.size();
			mOutput.mPointers.push_back(info.funcPtr);
		}
		CHECK(index < 255);

#if EBC_USE_COMPOSITIVE_CODE
		if (info.getArgNum() >= 1)
		{
			for (int i = 0; i < info.getArgNum(); ++i)
			{
				auto argValue = mStacks[mStacks.size() - 2 + info.getArgNum() - i];
				if (argValue.byteCode != EExprByteCode::Dummy)
				{
					outputCmd(argValue.byteCode, argValue.index);
				}
			}

			outputCmd(EExprByteCode::Type(EExprByteCode::FuncCall0 + info.getArgNum()), index);

			for (int i = 0; i < info.getArgNum(); ++i)
			{
				mStacks.pop_back();
			}
		}
		else
		{
			outputCmd(EExprByteCode::FuncCall0, index);
		}

		mStacks.push_back({ EExprByteCode::Dummy, 0 });
#else
		outputCmd(EExprByteCode::Type(EExprByteCode::FuncCall0 + info.getArgNum()), index);
#endif
	}

	void codeFunction(FuncSymbolInfo const& info)
	{
#if EBC_USE_COMPOSITIVE_CODE
		auto leftValue = mStacks.back();
		if (leftValue.byteCode != EExprByteCode::Dummy)
		{
			outputCmd(leftValue.byteCode, leftValue.index);
		}

		mStacks.pop_back();
#endif
		outputCmd(EExprByteCode::Type(EExprByteCode::FuncSymbol + info.id));

#if EBC_USE_COMPOSITIVE_CODE
		mStacks.push_back({ EExprByteCode::Dummy, 0 });
#endif
	}

	void pushValue(EExprByteCode::Type byteCode, uint8 index)
	{
#if EBC_USE_COMPOSITIVE_CODE
		mStacks.push_back({byteCode, index});
#else
		outputCmd(byteCode, index);
#endif
	}

	struct StackData 
	{
		EExprByteCode::Type byteCode;
		uint8 index;
	};
	TArray< StackData > mStacks;

	void codeBinaryOp(TokenType type, bool isReverse)
	{
#if EBC_USE_COMPOSITIVE_CODE
		auto rightValue = mStacks.back();
		auto leftValue  = mStacks[mStacks.size() - 2];
		if (rightValue.byteCode != EExprByteCode::Dummy)
		{
			outputCmd(rightValue.byteCode, rightValue.index);
		}

		EExprByteCode::Type opCode;
		switch (type)
		{
		case BOP_ADD: opCode = EExprByteCode::Add; break;
		case BOP_SUB: opCode = isReverse ? EExprByteCode::SubR : EExprByteCode::Sub; break;
		case BOP_MUL: opCode = EExprByteCode::Mul; break;
		case BOP_DIV: opCode = isReverse ? EExprByteCode::DivR : EExprByteCode::Div; break;
		}

		if (leftValue.byteCode == EExprByteCode::Dummy)
		{
			outputCmd(opCode);
		}
		else
		{
			if (leftValue.byteCode == rightValue.byteCode && leftValue.index == rightValue.index &&
			    (type == BOP_ADD || type == BOP_MUL))
			{
				outputCmd(type == BOP_ADD ? EExprByteCode::TAdd : EExprByteCode::TMul);
			}
			else
			{
				outputCmd(EExprByteCode::Type(opCode + leftValue.byteCode), leftValue.index);
			}
		}

		mStacks.pop_back();
		mStacks.pop_back();
#else
		switch (type)
		{
		case BOP_ADD: outputCmd(EExprByteCode::Add); break;
		case BOP_SUB: outputCmd(isReverse ? EExprByteCode::SubR : EExprByteCode::Sub); break;
		case BOP_MUL: outputCmd(EExprByteCode::Mul); break;
		case BOP_DIV: outputCmd(isReverse ? EExprByteCode::DivR : EExprByteCode::Div); break;
		}
#endif

#if EBC_USE_COMPOSITIVE_CODE
		mStacks.push_back({EExprByteCode::Dummy, 0});
#endif
	}
	
	void codeUnaryOp(TokenType type)
	{
#if EBC_USE_COMPOSITIVE_CODE
		auto leftValue = mStacks.back();
		if (leftValue.byteCode != EExprByteCode::Dummy)
		{
			outputCmd(leftValue.byteCode, leftValue.index);
		}

		mStacks.pop_back();
#endif
		switch (type)
		{
		case UOP_MINS: outputCmd(EExprByteCode::Mins); break;
		}
#if EBC_USE_COMPOSITIVE_CODE
		mStacks.push_back({ EExprByteCode::Dummy, 0 });
#endif
	}

	void outputCmd(EExprByteCode::Type a)
	{
		++mNumCmd;
		mOutput.mCodes.push_back(a);
#if EBC_USE_FIXED_SIZE >= 2
		mOutput.mCodes.push_back(0);
#endif
#if EBC_USE_FIXED_SIZE >= 3
		mOutput.mCodes.push_back(0);
#endif
	}
	void outputCmd(EExprByteCode::Type a, uint8 b)
	{
		++mNumCmd;
		mOutput.mCodes.push_back(a);
		mOutput.mCodes.push_back(b);
#if EBC_USE_FIXED_SIZE >= 3
		mOutput.mCodes.push_back(0);
#endif
	}
	void outputCmd(EExprByteCode::Type a, uint8 b, uint c)
	{
		++mNumCmd;
		mOutput.mCodes.push_back(a);
		mOutput.mCodes.push_back(b);
		mOutput.mCodes.push_back(c);
	}

	void codeEnd()
	{
		LogMsg("Num Cmd = %d" , mNumCmd);
#if EBC_USE_FIXED_SIZE
		std::string debugMeg;
		for( int i = 0 ; i < mOutput.mCodes.size(); i += EBC_USE_FIXED_SIZE )
		{
			debugMeg += "[";
			
			switch (mOutput.mCodes[i])
			{
			case EExprByteCode::Const: debugMeg += "C"; break;
			case EExprByteCode::Input: debugMeg += "I"; break;
			case EExprByteCode::Variable:  debugMeg += "V"; break;
			case EExprByteCode::Add: debugMeg += "+"; break;
			case EExprByteCode::Sub: debugMeg += "-"; break;
			case EExprByteCode::SubR: debugMeg += "-r"; break;
			case EExprByteCode::Mul: debugMeg += "*"; break;
			case EExprByteCode::Div: debugMeg += "/"; break;
			case EExprByteCode::DivR: debugMeg += "/r"; break;
			case EExprByteCode::Mins: debugMeg += "Mins"; break;
#if EBC_USE_COMPOSITIVE_CODE
			case EExprByteCode::CAdd: debugMeg += "C+"; break;
			case EExprByteCode::CSub: debugMeg += "C-"; break;
			case EExprByteCode::CSubR:debugMeg += "C-r"; break;
			case EExprByteCode::CMul: debugMeg += "C*"; break;
			case EExprByteCode::CDiv: debugMeg += "C/"; break;
			case EExprByteCode::CDivR:debugMeg += "C/r"; break;
			case EExprByteCode::IAdd: debugMeg += "I+"; break;
			case EExprByteCode::ISub: debugMeg += "I-"; break;
			case EExprByteCode::ISubR:debugMeg += "I-r"; break;
			case EExprByteCode::IMul: debugMeg += "I*"; break;
			case EExprByteCode::IDiv: debugMeg += "I/"; break;
			case EExprByteCode::IDivR:debugMeg += "I/r"; break;
			case EExprByteCode::VAdd: debugMeg += "V+"; break;
			case EExprByteCode::VSub:debugMeg += "V-"; break;
			case EExprByteCode::VSubR:debugMeg += "V-r"; break;
			case EExprByteCode::VMul:debugMeg += "V*"; break;
			case EExprByteCode::VDiv:debugMeg += "V/"; break;
			case EExprByteCode::VDivR:debugMeg += "V/r"; break;
			case EExprByteCode::TAdd:debugMeg += "T+"; break;
			case EExprByteCode::TMul:debugMeg += "T*"; break;
#endif
			case EExprByteCode::FuncSymbol + EFuncSymbol::Exp:  debugMeg += "exp"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Ln:   debugMeg += "ln"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Sin:  debugMeg += "sin"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Cos:  debugMeg += "cos"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Tan:  debugMeg += "tan"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Cot:  debugMeg += "cot"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Sec:  debugMeg += "sec"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Csc:  debugMeg += "csc"; break;
			case EExprByteCode::FuncSymbol + EFuncSymbol::Sqrt: debugMeg += "sqrt"; break;
			}
			debugMeg += "]";
		}
		LogMsg("Cmd = %s", debugMeg.c_str());
#endif
	}
};



#endif

ExpressionCompiler::ExpressionCompiler()
{
	mOptimization = true;
}

bool ExpressionCompiler::compile( char const* expr , SymbolTable const& table , ExecutableCode& data , int numInput , ValueLayout inputLayouts[] )
{
	try
	{
		ExpressionParser parser;
		if (!parser.parse(expr, table, mResult))
			return false;

#if 0
		if (mOptimization)
			mResult.optimize();
#endif
#if ENABLE_FPU_CODE
		FPUCodeGeneratorV0 generator;
		generator.setCodeData( &data );
		mResult.generateCode(generator , numInput, inputLayouts);
#elif ENABLE_BYTE_CODE
		ExprByteCodeCompiler generator(data);
		mResult.generateCode(generator, numInput, inputLayouts);
#else
		data.mCodes = mResult.getPostfixCodes();
#endif
		return true;
	}
	catch ( ExprParseException& e)
	{
		LogMsg("Compile error : (%d) : %s", e.errorCode, e.what());
		return false;
	}
	catch ( std::exception& )
	{
		return false;
	}
}

#if ENABLE_FPU_CODE

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
	FMemory::Copy(mCodeEnd, data, size);
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

#endif


ExecutableCode::ExecutableCode( int size )
{
#if ENABLE_FPU_CODE
	mMaxCodeSize = ( size ) ? size : 64;
	mCode = ( uint8* ) AllocExecutableMemory( mMaxCodeSize );
	assert( mCode );
	clearCode();
#endif
}


ExecutableCode::~ExecutableCode()
{
#if ENABLE_FPU_CODE
	FreeExecutableMemory( mCode );
#endif
}

#if ENABLE_FPU_CODE
void ExecutableCode::checkCodeSize( int freeSize )
{
	int codeNum = getCodeLength();
	if ( codeNum + freeSize <= mMaxCodeSize )
		return;

	int newSize = 2 * mMaxCodeSize;
	assert( newSize >= codeNum + freeSize );

	uint8* newPtr = (uint8*)ReallocExecutableMemory(mCode , newSize);
	if (!newPtr)
	{
		throw ExprParseException(eAllocFailed, "");
	}

	mCode = newPtr;
	mCodeEnd = mCode + codeNum;
	mMaxCodeSize = newSize;
}
#endif

void ExecutableCode::clearCode()
{
#if ENABLE_FPU_CODE
	FMemory::Set(mCode, 0, mMaxCodeSize);
	mCodeEnd = mCode;
#elif ENABLE_BYTE_CODE
	mCodes.clear();
	mPointers.clear();
	mConstValues.clear();
#else
	mCodes.clear();
#endif
}

