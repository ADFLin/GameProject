#pragma once
#ifndef AsmCode_H_11F84952_0F61_41D0_B185_D50AA32283F5
#define AsmCode_H_11F84952_0F61_41D0_B185_D50AA32283F5

#include "../ExpressionCore.h"
#include "../SymbolTable.h"
#include "../ExpressionTree.h"
#include "../ExpressionParser.h"

#include "DataStructure/Array.h"

#include "Assembler.h"
#include "PlatformConfig.h"

#define VALUE_PTR qword_ptr


using namespace Asmeta;

class AsmCodeBuffer : public ExprParse
{
public:
	explicit AsmCodeBuffer(int size = 0);
	~AsmCodeBuffer();

	void   printCode();
	void   clear();
	int    getCodeLength() const
	{
		return int(mCodeEnd - mCode);
	}
	uint8 const* getCode() const { return mCode; }
	uint8*       getCode() { return mCode; }

protected:

	void pushCode(uint8 byte);
	void pushCode(uint8 byte1, uint8 byte2);
	void pushCode(uint8 byte1, uint8 byte2, uint8 byte3);
	void pushCode(uint8 const* data, int size);

	void pushCode(uint32 val) { pushCodeT(val); }
	void pushCode(uint64 val) { pushCodeT(val); }
	void pushCode(void* ptr) { pushCodeT(ptr); }
	void pushCode(double value) { pushCodeT(value); }

	void setCode(unsigned pos, void* ptr);
	void setCode(unsigned pos, uint8 byte);

	template < class T >
	void pushCodeT(T value)
	{
		checkCodeSize(sizeof(T));
		*reinterpret_cast<T*>(mCodeEnd) = value;
		mCodeEnd += sizeof(T);
	}
	void   pushCodeInternal(uint8 byte);
	__declspec(noinline)  void  checkCodeSize(int freeSize);

	friend class ExecutableCode;
	uint8*  mCode;
	uint8*  mCodeEnd;
	int     mMaxCodeSize;
#if _DEBUG
	int     mNumInstruction;
#endif
	friend class AsmCodeGenerator;
	friend class FPUCodeGeneratorV0;
	friend class FPUCodeGeneratorV1;
	friend class ExpressionCompiler;
	friend class SSECodeGeneratorX64;
	friend class SSESIMDCodeGeneratorX64;
};


class AsmCodeGenerator : public Asmeta::AssemblerT< AsmCodeGenerator >
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
	void   setCodeData(AsmCodeBuffer* data) { mData = data; }

	AsmCodeBuffer* mData;
	int          mNumInstruction;
};

class FPUCodeGeneratorBase : public AsmCodeGenerator
	, public ExprParse
{
public:
	using ValueType = double;
	using TokenType = ExprParse::TokenType;

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
		for (; idx < size; ++idx)
		{
			auto const& stackValue = mConstStack[idx];
			if (stackValue.layout == val.layout && val.isSameValue(&mConstStorage[stackValue.offset]))
				break;
		}

		if (idx == size)
		{
			StackValue stackValue;
			stackValue.layout = val.layout;
			if (mConstStack.empty())
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
		for (int i = 0; i < numInput; ++i)
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
		switch (type)
		{
		case BOP_ADD:
			Asm::fadd(src);
			return 1;
		case BOP_MUL:
			Asm::fmul(src);
			return 1;
		case BOP_SUB:
			if (isReverse)
				Asm::fsubr(src);
			else
				Asm::fsub(src);
			return 1;
		case BOP_DIV:
			if (isReverse)
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
		switch (type)
		{
		case BOP_ADD:
			Asm::fiadd(src);
			return 1;
		case BOP_MUL:
			Asm::fimul(src);
			return 1;
		case BOP_SUB:
			if (isReverse)
				Asm::fisubr(src);
			else
				Asm::fisub(src);
			return 1;
		case BOP_DIV:
			if (isReverse)
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
		switch (type)
		{
		case BOP_ADD:
			Asm::faddp(src);
			return 1;
		case BOP_MUL:
			Asm::fmulp(src);
			return 1;
		case BOP_SUB:
			if (isReverse)
				Asm::fsubrp(src);
			else
				Asm::fsubp(src);
			return 1;
		case BOP_DIV:
			if (isReverse)
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
			return 1 + emitBOPWithBaseLayout(opType, isReverse, ToBase(layout), eax);
		}

		return emitBOPWithBaseLayout(opType, isReverse, layout, std::forward<Args>(args)...);
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
		switch (info.type)
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


class FPUCodeGeneratorV0 : public TCodeGenerator<FPUCodeGeneratorV0>
	, public FPUCodeGeneratorBase
{

	using BaseClass = FPUCodeGeneratorBase;
public:
	using TokenType = FPUCodeGeneratorBase::TokenType;
	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mData->clear();
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

		int idx = addConstValue(val);

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

		mPrevValue.type = VALUE_INPUT;
		mPrevValue.input = &input;
		mPrevValue.idxStack = findStack(mPrevValue);
	}

	void codeFunction(FuncInfo const& info)
	{
		checkStackPrevValPtr();

		int   numParam = info.getArgNum();
		int   numSPUParam = numParam;

		unsigned espOffset = sizeof(ValueType) * numParam;

		if (numParam)
		{
			int32 paramOffset = 0;

			if (mNumVarStack > FpuRegNum)
			{
				int numCpuStack = mNumVarStack - FpuRegNum;
				int numCpuParam = std::min(numParam, numCpuStack);

				//#TODO : consider value layout
				paramOffset -= numCpuStack * sizeof(ValueType);
				for (int num = 0; num < numCpuParam; ++num)
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


			for (int num = 0; num < numSPUParam; ++num)
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
	void codeFunction(FuncSymbolInfo const& info)
	{

	}
	void codeBinaryOp(TokenType opType, bool isReverse)
	{
		if (opType == BOP_ASSIGN)
		{
			switch (mPrevValue.type)
			{
			case VALUE_INPUT:
				{
					StackValue& inputValue = mInputStack[mPrevValue.input->index];
					if (!IsPointer(inputValue.layout))
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
			if (mPrevValue.idxStack != INDEX_NONE)
			{
				int idxReg = mRegStack.size() - mPrevValue.idxStack - 1;
				if (idxReg == 0 && opType == BOP_COMMA)
				{

				}
				else
				{
					mNumInstruction += emitBOP(opType, isReverse, st(idxReg));
				}
				mRegStack.back().type = TOKEN_NONE;
			}
			else if (IsValue(mPrevValue.type))
			{
				switch (mPrevValue.type)
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
						mNumInstruction += emitBOPWithLayout(opType, isReverse, constValue.layout, &mConstLabel, constValue.offset);
					}
					break;
				}

				mRegStack.back().type = TOKEN_NONE;
			}
			else
			{
				if (mNumVarStack > FpuRegNum)
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

		if (type == UOP_MINS)
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
		if (IsValue(mPrevValue.type))
		{
			checkStackPrevValPtr();
		}

		Asm::mov(esp, ebp);
		Asm::pop(ebp);
		Asm::ret();
		mNumInstruction += 3;

		Asm::bind(&mConstLabel);

		int num = mData->getCodeLength();
		if (!mConstStorage.empty())
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
		if (!ExprParse::IsValue(mPrevValue.type))
			return;

		bool isVar = (mPrevValue.type == VALUE_VARIABLE);

		++mNumVarStack;
		if (mNumVarStack >= FpuRegNum)
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
		for (int i = 0; i < mRegStack.size(); ++i)
		{
			if (value.type == mRegStack[i].type)
			{
				switch (value.type)
				{
				case VALUE_CONST:
					if (value.idxCV == mRegStack[i].idxCV)
						return i;
					break;
				case VALUE_VARIABLE:
					if (value.var == mRegStack[i].var)
						return i;
					break;
				case VALUE_INPUT:
					if (value.input == mRegStack[i].input)
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
		mData->clear();
		Asm::clearLink();

		Asm::push(ebp);
		Asm::mov(ebp, esp);
		mNumInstruction += 2;
	}
	void codeConstValue(RealType const&val)
	{
		int idx = 0;
		int size = (int)mConstTable.size();
		for (; idx < size; ++idx)
		{
			if (mConstTable[idx] == val)
				break;
		}

		if (idx == size)
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
		if (type == BOP_ASSIGN)
		{
			ValueInfo valueVar = mStackValues.back();
			mStackValues.pop_back();
			ValueInfo& value = mStackValues.back();
			if (value.type == TOKEN_NONE)
			{
				mNumInstruction += emitStoreValueWithLayout(valueVar.var->layout, valueVar.var->ptr);
			}
			else
			{
				uint8 indexReg = getRegIndex(value);
				if (indexReg != INDEX_NONE)
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
		for (RealType cValue : mConstTable)
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
		while (info.idxStack != 0)
		{


		}


	}
	std::vector< int > mRegStack;
	std::vector< ValueInfo > mStackValues;

};

#if TARGET_PLATFORM_64BITS
//=============================================================================
// SSECodeGeneratorX64 - x64 version using SSE instructions
//=============================================================================
// Windows x64 calling convention:
// - Parameters: rcx, rdx, r8, r9 (integer), xmm0-xmm3 (floating point)
// - Return value: rax (integer), xmm0 (floating point)
// - Volatile registers: rax, rcx, rdx, r8-r11, xmm0-xmm5
// - Non-volatile: rbx, rbp, rdi, rsi, rsp, r12-r15, xmm6-xmm15
// - Shadow space: 32 bytes must be allocated on stack for first 4 params

class SSECodeGeneratorX64Base : public AsmCodeGenerator
	, public ExprParse
{
public:
	using ValueType = double;
	using TokenType = ExprParse::TokenType;

	// We can use xmm0-xmm5 freely (volatile), xmm6-xmm15 need to be saved
	static int const NumVolatileXMM = 6;  // xmm0-xmm5
	static int const NumTempXMM = 16;     // xmm0-xmm15

	// Stack offsets for saving non-volatile XMM registers (relative to rbp)
	static int const XMM14_SAVE_OFFSET = -16;  // [rbp-16]
	static int const XMM15_SAVE_OFFSET = -32;  // [rbp-32]

	struct StackValue
	{
		ValueLayout layout;
		int32       offset;
	};

	SSECodeGeneratorX64Base()
	{

	}

	TArray< StackValue >  mInputStack;
	TArray< StackValue >  mConstStack;
	Asmeta::Label         mConstLabel;
	TArray< uint8 >       mConstStorage;

	enum { MaxXMMStackSize = 10 }; // xmm6 - xmm15

	RegXMM stackXMM(int idx)
	{
		assert(idx >= 0 && idx < MaxXMMStackSize);
		return xmm(6 + idx);
	}

	int32 mXMMSaveCodeStart = 0;
	TArray<int32> mXMMSaveOffsets;
	TArray<int32> mXMMSaveInstrSizes;
	int32 mXMMSaveCodeEnd = 0;
	int32 mMaxXMMStackUsage = 0;
	int32 mReservedInputCount = 0;

	void updateStackUsage()
	{
		if ((int)mXMMStack.size() > mMaxXMMStackUsage)
			mMaxXMMStackUsage = (int)mXMMStack.size();
	}

	int addConstValue(ConstValueInfo const& val)
	{
		int idx = 0;
		int size = (int)mConstStack.size();
		for (; idx < size; ++idx)
		{
			auto const& stackValue = mConstStack[idx];
			if (stackValue.layout == val.layout && val.isSameValue(&mConstStorage[stackValue.offset]))
				break;
		}

		if (idx == size)
		{
			StackValue stackValue;
			stackValue.layout = val.layout;
			if (mConstStack.empty())
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
			type = TOKEN_NONE;
			idxXMM = INDEX_NONE;
		}
		TokenType type;
		union
		{
			VariableInfo const* var;
			InputInfo           input;
			int     idxCV;
			int     idxTemp;
		};
		int     idxXMM;  // XMM register index if value is in register
	};

	ValueInfo     mPrevValue;
	int           mNumXMMUsed;
	TArray< ValueInfo > mXMMStack;  // Stack of values in XMM registers

	// Simulated state for accurate mMaxStackDepth calculation
	TokenType     mSimulatedPrevType;
	TArray<bool>  mInputUsed;

	// Initialize code generation for x64
	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mInputStack.resize(numInput);
		for (int i = 0; i < numInput; ++i)
		{
			mInputStack[i].layout = inputLayouts[i];
			if (i < 4)
			{
				mInputStack[i].offset = 16 + i * 8;
			}
			else
			{
				mInputStack[i].offset = 16 + 32 + (i - 4) * 8;
			}
		}

		mConstStorage.clear();
		mConstStack.clear();
		mNumTemps = 0;

		mSimulatedStackDepth = 0;
		mMaxStackDepth = 0;
		mSimulatedPrevType = TOKEN_NONE;
		mInputUsed.clear();
		mInputUsed.resize(numInput, false);
	}

	void preLoadCode(ExprParse::Unit const& code)
	{
		if (code.type == VALUE_CONST)
		{
			addConstValue(code.constValue);
		}
		else if (code.type == VALUE_EXPR_TEMP)
		{
			if (mNumTemps < code.exprTemp.tempIndex + 1)
				mNumTemps = code.exprTemp.tempIndex + 1;
		}
		else if (code.type == VALUE_INPUT)
		{
			mInputUsed[code.input.index] = true;
		}

		if (code.storeIndex != -1)
		{
			if (mNumTemps < code.storeIndex + 1)
				mNumTemps = code.storeIndex + 1;
		}

		if (IsValue(code.type))
		{
			if (IsValue(mSimulatedPrevType))
			{
				mSimulatedStackDepth += 1;
			}
			mSimulatedPrevType = code.type;
		}
		else if (IsBinaryOperator(code.type))
		{
			if (IsValue(mSimulatedPrevType))
			{
				// Optimizer will use mSimulatedPrevType as second operand directly
				mSimulatedPrevType = TOKEN_NONE;
			}
			else
			{
				mSimulatedStackDepth -= 1;
			}
		}
		else if (IsUnaryOperator(code.type))
		{
			// Unary ops always process the previous value or stack top
			if (IsValue(mSimulatedPrevType))
			{
				// Value is effectively "pushed" and processed
				mSimulatedStackDepth += 1;
				mSimulatedPrevType = TOKEN_NONE;
			}
			// Result replaces stack top/prev value, depth doesn't change
		}
		else if (IsFunction(code.type))
		{
			int numArgs = (code.type == FUNC_DEF) ? code.symbol->func.getArgNum() : code.funcSymbol.numArgs;
			if (numArgs > 0)
			{
				if (IsValue(mSimulatedPrevType))
				{
					// Count effectively used stack items
					int usedFromStack = numArgs - 1;
					mSimulatedStackDepth -= usedFromStack;
					mSimulatedPrevType = TOKEN_NONE;
				}
				else
				{
					mSimulatedStackDepth -= (numArgs - 1);
				}
			}
			else
			{
				// Func with 0 args is like a value
				if (IsValue(mSimulatedPrevType))
					mSimulatedStackDepth += 1;
				mSimulatedPrevType = TOKEN_FUNC;
			}
		}

		if (mSimulatedStackDepth > mMaxStackDepth)
		{
			mMaxStackDepth = mSimulatedStackDepth;
		}
	}

	int           mMaxStackDepth;
	int           mSimulatedStackDepth;

	// Emit SSE binary operation with XMM register
	int emitBOP(TokenType opType, bool isReverse, RegXMM const& dst, RegXMM const& src)
	{
		switch (opType)
		{
		case BOP_ADD:
			Asm::addsd(dst, src);
			return 1;
		case BOP_MUL:
			Asm::mulsd(dst, src);
			return 1;
		case BOP_SUB:
			if (isReverse)
			{
				// dst = src - dst
				// Use xmm1 as scratch
				Asm::movsd(xmm1, dst);  // temp = dst
				Asm::movsd(dst, src);    // dst = src
				Asm::subsd(dst, xmm1);  // dst = src - temp
				return 3;
			}
			else
			{
				Asm::subsd(dst, src);
				return 1;
			}
		case BOP_DIV:
			if (isReverse)
			{
				// dst = src / dst
				Asm::movsd(xmm1, dst);
				Asm::movsd(dst, src);
				Asm::divsd(dst, xmm1);
				return 3;
			}
			else
			{
				Asm::divsd(dst, src);
				return 1;
			}
		case BOP_COMMA:
			Asm::movsd(dst, src);
			return 1;
		}
		return 0;
	}

	// Emit SSE binary operation with memory operand
	template< class ...Args >
	int emitBOPMem(TokenType opType, bool isReverse, RegXMM const& dst, Args&& ...args)
	{
		switch (opType)
		{
		case BOP_ADD:
			Asm::addsd(dst, qword_ptr(std::forward<Args>(args)...));
			return 1;
		case BOP_MUL:
			Asm::mulsd(dst, qword_ptr(std::forward<Args>(args)...));
			return 1;
		case BOP_SUB:
			if (isReverse)
			{
				// Use xmm1 as scratch
				Asm::movsd(xmm1, dst);
				Asm::movsd(dst, qword_ptr(std::forward<Args>(args)...));
				Asm::subsd(dst, xmm1);
				return 3;
			}
			else
			{
				Asm::subsd(dst, qword_ptr(std::forward<Args>(args)...));
				return 1;
			}
		case BOP_DIV:
			if (isReverse)
			{
				Asm::movsd(xmm1, dst);
				Asm::movsd(dst, qword_ptr(std::forward<Args>(args)...));
				Asm::divsd(dst, xmm1);
				return 3;
			}
			else
			{
				Asm::divsd(dst, qword_ptr(std::forward<Args>(args)...));
				return 1;
			}
		case BOP_COMMA:
			Asm::movsd(dst, qword_ptr(std::forward<Args>(args)...));
			return 1;
		}
		return 0;
	}

	template< class ...Args >
	int emitBOPWithLayout(TokenType opType, bool isReverse, ValueLayout layout, RegXMM const& dst, Args&& ...args)
	{
		if (IsPointer(layout))
		{
			Asm::mov(rax, qword_ptr(std::forward<Args>(args)...));
			return 1 + emitBOPWithBaseLayout(opType, isReverse, ToBase(layout), dst, rax);
		}

		return emitBOPWithBaseLayout(opType, isReverse, layout, dst, std::forward<Args>(args)...);
	}

	template< class ...Args >
	int emitBOPWithBaseLayout(TokenType opType, bool isReverse, ValueLayout layout, RegXMM const& dst, Args&& ...args)
	{
		switch (layout)
		{
		case ValueLayout::Double:
			return emitBOPMem(opType, isReverse, dst, std::forward<Args>(args)...);
		case ValueLayout::Float:
			// Load float, convert to double, then operate
			// Use xmm0 as scratch
			Asm::movss(xmm0, dword_ptr(std::forward<Args>(args)...));
			Asm::cvtss2sd(xmm0, xmm0);
			return 2 + emitBOP(opType, isReverse, dst, xmm0);
		case ValueLayout::Int32:
			// Load int, convert to double
			// Use xmm0 as scratch
			Asm::cvtsi2sd(xmm0, dword_ptr(std::forward<Args>(args)...));
			return 1 + emitBOP(opType, isReverse, dst, xmm0);
		default:
			NEVER_REACH("emitBOPWithBaseLayout miss case");
		}
		return 0;
	}

	template< class ...Args >
	int emitLoadValueWithLayout(ValueLayout layout, RegXMM const& dst, Args&& ...args)
	{
		if (IsPointer(layout))
		{
			Asm::mov(rax, qword_ptr(std::forward<Args>(args)...));
			return 1 + emitLoadValueWithBaseLayout(ToBase(layout), dst, rax);
		}

		return emitLoadValueWithBaseLayout(layout, dst, std::forward<Args>(args)...);
	}

	template< class ...Args >
	int emitLoadValueWithBaseLayout(ValueLayout layout, RegXMM const& dst, Args&& ...args)
	{
		switch (layout)
		{
		case ValueLayout::Double:
			Asm::movsd(dst, qword_ptr(std::forward<Args>(args)...));
			return 1;
		case ValueLayout::Float:
			Asm::movss(dst, dword_ptr(std::forward<Args>(args)...));
			Asm::cvtss2sd(dst, dst);
			return 2;
		case ValueLayout::Int32:
			Asm::cvtsi2sd(dst, dword_ptr(std::forward<Args>(args)...));
			return 1;
		default:
			NEVER_REACH("emitLoadValueWithBaseLayout miss case");
		}
		return 0;
	}

	template< class ...Args >
	int emitStoreValueWithLayout(ValueLayout layout, RegXMM const& src, Args&& ...args)
	{
		if (IsPointer(layout))
		{
			Asm::mov(rax, qword_ptr(std::forward<Args>(args)...));
			return 1 + emitStoreValueWithBaseLayout(ToBase(layout), src, rax);
		}
		return emitStoreValueWithBaseLayout(layout, src, std::forward<Args>(args)...);
	}

	template< class ...Args >
	int emitStoreValueWithBaseLayout(ValueLayout layout, RegXMM const& src, Args&& ...args)
	{
		switch (layout)
		{
		case ValueLayout::Double:
			Asm::movsd(qword_ptr(std::forward<Args>(args)...), src);
			return 1;
		case ValueLayout::Float:
			// Use xmm0 as scratch
			Asm::cvtsd2ss(xmm0, src);
			Asm::movss(dword_ptr(std::forward<Args>(args)...), xmm0);
			return 2;
		case ValueLayout::Int32:
			Asm::cvttsd2si(eax, src);
			Asm::mov(dword_ptr(std::forward<Args>(args)...), eax);
			return 2;
		default:
			NEVER_REACH("emitStoreValueWithBaseLayout miss case");
		}
		return 0;
	}

	int emitLoadValue(ValueInfo const& info, RegXMM const& dst)
	{
		switch (info.type)
		{
		case VALUE_VARIABLE:
			{
				Asm::movabs(rax, (uint64)info.var->ptr);
				return 1 + emitLoadValueWithBaseLayout(info.var->layout, dst, rax);
			}
		case VALUE_INPUT:
			{
				StackValue& inputValue = mInputStack[info.input.index];
				return emitLoadValueWithLayout(inputValue.layout, dst, rbp, inputValue.offset);
			}
		case VALUE_CONST:
			{
				StackValue& constValue = mConstStack[info.idxCV];
				return emitLoadValueWithLayout(constValue.layout, dst, &mConstLabel, constValue.offset);
			}
		case VALUE_EXPR_TEMP:
			{
				// Load temporary expression value from stack
				int offset = GetTempStackOffset(info.idxTemp);
				Asm::movsd(dst, qword_ptr(rbp, offset));
				return 1;
			}
		}
		return 0;
	}

	int findXMMWithValue(ValueInfo const& value)
	{
		for (int i = 0; i < (int)mXMMStack.size(); ++i)
		{
			if (value.type == mXMMStack[i].type)
			{
				switch (value.type)
				{
				case VALUE_CONST:
					if (value.idxCV == mXMMStack[i].idxCV)
						return mXMMStack[i].idxXMM;
					break;
				case VALUE_VARIABLE:
					if (value.var == mXMMStack[i].var)
						return mXMMStack[i].idxXMM;
					break;
				case VALUE_INPUT:
					if (value.input.index == mXMMStack[i].input.index)
						return mXMMStack[i].idxXMM;
					break;
				case VALUE_EXPR_TEMP:
					if (value.idxTemp == mXMMStack[i].idxTemp)
						return mXMMStack[i].idxXMM;
					break;
				}
			}
		}
		return INDEX_NONE;
	}

public:
	int mNumTemps = 0;

	int GetTempStackOffset(int index)
	{
		// XMM saves end at: -16 - (MaxXMMStackSize * 16) = -176
		return -176 - 8 - index * 8;
	}

	int GetTempStackOffsetSIMD(int index)
	{
		// SIMD temps need 16-byte alignment and 16-byte size
		// Start after XMM saves (-176)
		return -176 - 16 - index * 16;
	}
protected:
	int32 mStackReserve;
	int32 mStackReservePos;
};


class SSECodeGeneratorX64 : public TCodeGenerator<SSECodeGeneratorX64>
	, public SSECodeGeneratorX64Base
{
	using BaseClass = SSECodeGeneratorX64Base;
public:
	using TokenType = SSECodeGeneratorX64Base::TokenType;

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mData->clear();
		mNumXMMUsed = 0;
		mNumInstruction = 0;

		mPrevValue.type = TOKEN_NONE;
		mPrevValue.var = nullptr;

		Asm::clearLink();

		BaseClass::codeInit(numInput, inputLayouts);

		// Initialize XMM stack
		mXMMStack.clear();
	}

	void postLoadCode()
	{
		// Function prologue for x64
		Asm::push(rbp);
		Asm::mov(rbp, rsp);

		// Calculate precise stack size
		int tempSize = mNumTemps * 8;
		if (tempSize % 16 != 0) tempSize += 8; // Maintain 16-byte alignment
		int finalStackSize = 32 + 16 * MaxXMMStackSize + tempSize + 32;

		mStackReserve = finalStackSize;
		mStackReservePos = mData->getCodeLength();
		Asm::sub(rsp, imm32(mStackReserve));
		mNumInstruction += 3;

		// Calculate precise input reserve count for Shadow Space storage
		mReservedInputCount = 0;
		for (int i = 0; i < (int)std::min(mInputUsed.size(), (size_t)4); ++i)
		{
			if (mInputUsed[i])
				mReservedInputCount = i + 1;
		}

		// Only reserve registers for the actual expression stack depth.
		// Inputs will be loaded from Shadow Space (rbp+10h, etc.) on demand.
		mMaxXMMStackUsage = mMaxStackDepth;

		// Save non-volatile XMM registers (xmm6 - xmm15) based on actual stack depth
		mXMMSaveCodeStart = mData->getCodeLength();
		mXMMSaveOffsets.resize(MaxXMMStackSize);
		mXMMSaveInstrSizes.resize(MaxXMMStackSize);

		for (int i = 0; i < MaxXMMStackSize; ++i)
		{
			int start = mData->getCodeLength();
			Asm::movsd(qword_ptr(rbp, -16 - (i * 16)), stackXMM(i));
			mXMMSaveOffsets[i] = start;
			mXMMSaveInstrSizes[i] = mData->getCodeLength() - start;
		}
		mXMMSaveCodeEnd = mData->getCodeLength();

		// Save first 4 floating-point args to shadow space only (if used)
		for (int i = 0; i < mReservedInputCount; ++i)
		{
			if (mInputUsed[i])
			{
				// We only store use-index inputs to shadow space. 
				// The expression generator (emitLoadValue) already knows how to read from here.
				Asm::movsd(qword_ptr(rbp, int8(mInputStack[i].offset)), xmm(i));
			}
		}

		mNumInstruction += 4 + mReservedInputCount;
	}

	void codeConstValue(ConstValueInfo const& val)
	{
		checkAndLoadPrevValue();

		int idx = addConstValue(val);

		mPrevValue.type = VALUE_CONST;
		mPrevValue.idxCV = idx;
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeVar(VariableInfo const& varInfo)
	{
		checkAndLoadPrevValue();

		mPrevValue.type = VALUE_VARIABLE;
		mPrevValue.var = &varInfo;
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeInput(InputInfo const& input)
	{
		checkAndLoadPrevValue();

		mPrevValue.type = VALUE_INPUT;
		mPrevValue.input = input;
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeFunction(FuncInfo const& info)
	{
		checkAndLoadPrevValue();

		int numParam = info.getArgNum();

		// In Windows x64, first 4 float params go in xmm0-xmm3
		// Additional params on stack
		// We need to set up the call properly

		// Calculate how many live values remain after popping params
		int numLive = (int)mXMMStack.size() - numParam;
		if (numLive < 0) numLive = 0;

		// Check stack overflow
		if (mReservedInputCount + numLive + 1 >= MaxXMMStackSize)
		{
			throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Expression too complex (stack overflow)");
		}

		// Move parameters to xmm0-xmm3 (from non-volatile stackXMM registers)
		for (int i = 0; i < numParam && i < 4; ++i)
		{
			int srcStackIdx = numLive + i;
			if (srcStackIdx < (int)mXMMStack.size())
			{
				int phyIdx = mXMMStack[srcStackIdx].idxXMM;
				Asm::movsd(xmm(i), stackXMM(phyIdx));
				++mNumInstruction;
			}
		}

		// Handle params > 4 (push to stack)
		for (int i = 4; i < numParam; ++i)
		{
			int srcStackIdx = numLive + i;
			if (srcStackIdx < (int)mXMMStack.size())
			{
				int phyIdx = mXMMStack[srcStackIdx].idxXMM;
				int stackOffset = 32 + (i - 4) * 8;
				Asm::movsd(qword_ptr(rsp, int8(stackOffset)), stackXMM(phyIdx));
				++mNumInstruction;
			}
		}

		// Call the function
		Asm::movabs(rax, (uint64)info.funcPtr);
		Asm::call(rax);
		mNumInstruction += 2;

		// Pop the parameters from our stack
		for (int i = 0; i < numParam && !mXMMStack.empty(); ++i)
		{
			mXMMStack.pop_back();
		}

		// Result is in xmm0 (volatile), save it to a non-volatile stackXMM register
		int resultIdx = mReservedInputCount + numLive;
		Asm::movsd(stackXMM(resultIdx), xmm0);
		++mNumInstruction;

		// Update stack usage
		int newStackTop = resultIdx + 1;
		if (newStackTop > mMaxXMMStackUsage) mMaxXMMStackUsage = newStackTop;

		// Push result to our stack with correct physical register index
		mXMMStack.emplace_back();
		mXMMStack.back().type = TOKEN_FUNC;
		mXMMStack.back().idxXMM = resultIdx;

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	static void* GetFuncAddress(int id)
	{
		switch (id)
		{
		case EFuncSymbol::Exp: return (void*)static_cast<double(*)(double)>(&std::exp);
		case EFuncSymbol::Ln:  return (void*)static_cast<double(*)(double)>(&std::log);
		case EFuncSymbol::Sin: return (void*)static_cast<double(*)(double)>(&std::sin);
		case EFuncSymbol::Cos: return (void*)static_cast<double(*)(double)>(&std::cos);
		case EFuncSymbol::Tan: return (void*)static_cast<double(*)(double)>(&std::tan);
		case EFuncSymbol::Cot: return (void*)static_cast<double(*)(double)>(&std::tan); // 1/tan
		case EFuncSymbol::Sec: return (void*)static_cast<double(*)(double)>(&std::cos); // 1/cos
		case EFuncSymbol::Csc: return (void*)static_cast<double(*)(double)>(&std::sin); // 1/sin
		case EFuncSymbol::Sqrt: return (void*)static_cast<double(*)(double)>(&std::sqrt);
		}
		return nullptr;
	}


	void codeFunction(FuncSymbolInfo const& info)
	{
		void* funcAddr = GetFuncAddress(info.id);
		if (!funcAddr)
			throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Unknown function symbol");

		int numParam = info.numArgs;

		// 1. Save live registers? NO!
		// Our values are in stackXMM (xmm6-xmm15), which are non-volatile.
		// The called function is required to preserve them.

		int numLive = (int)mXMMStack.size() - numParam;
		if (mReservedInputCount + numLive + 1 >= MaxXMMStackSize) // +1 for result
		{
			throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Expression too complex (stack overflow)");
		}

		// 2. Move arguments to parameter registers (xmm0...xmm3)
		// Arguments are at mXMMStack[numLive] ... mXMMStack[end]
		for (int i = 0; i < numParam; ++i)
		{
			int srcStackIdx = numLive + i;
			int phyIdx = mXMMStack[srcStackIdx].idxXMM;
			Asm::movsd(xmm(i), stackXMM(phyIdx));
			++mNumInstruction;
		}

		// 3. Call the function
		Asm::movabs(rax, (uint64)funcAddr);
		Asm::call(rax);
		mNumInstruction += 2;

		// 4. Handle Inverse Trig functions (Cot, Sec, Csc)
		if (info.id == EFuncSymbol::Cot || info.id == EFuncSymbol::Sec || info.id == EFuncSymbol::Csc)
		{
			// result = 1.0 / result
			// Load 1.0
			ConstValueInfo oneVal(1.0);
			int idx = addConstValue(oneVal);
			StackValue& constValue = mConstStack[idx];

			// Use xmm1 as scratch (result is in xmm0)
			Asm::movsd(xmm0, xmm1); // result = xmm1
			mNumInstruction += 3;
		}

		// 5. Place result (xmm0) onto the stack
		int resultIdx = numLive;
		Asm::movsd(stackXMM(resultIdx), xmm(0));
		++mNumInstruction;

		// Update Stack Usage
		int newStackTop = resultIdx + 1;
		if (newStackTop > mMaxXMMStackUsage) mMaxXMMStackUsage = newStackTop;

		// Update XMMStack state
		for (int i = 0; i < numParam; ++i)
		{
			mXMMStack.pop_back();
		}

		mXMMStack.emplace_back();
		mXMMStack.back().type = TOKEN_FUNC;
		mXMMStack.back().idxXMM = resultIdx;

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeBinaryOp(TokenType opType, bool isReverse)
	{
		if (opType == BOP_ASSIGN)
		{
			switch (mPrevValue.type)
			{
			case VALUE_INPUT:
				{
					StackValue& inputValue = mInputStack[mPrevValue.input.index];
					if (!IsPointer(inputValue.layout))
					{
						throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Input value layout is not pointer when assign");
					}
					// Store from the top of XMM stack
					if (!mXMMStack.empty())
					{
						int srcXMM = (int)mXMMStack.size() - 1;
						mNumInstruction += emitStoreValueWithLayout(inputValue.layout, stackXMM(srcXMM), rbp, inputValue.offset);
					}
				}
				break;
			case VALUE_VARIABLE:
				{
					if (!mXMMStack.empty())
					{
						int srcXMM = (int)mXMMStack.size() - 1;
						Asm::movabs(rax, (uint64)mPrevValue.var->ptr);
						mNumInstruction += 1 + emitStoreValueWithBaseLayout(mPrevValue.var->layout, stackXMM(srcXMM), rax);
					}
				}
				break;
			}
		}
		else
		{
			int dstXMM = INDEX_NONE;

			if (IsValue(mPrevValue.type) && mPrevValue.idxXMM == INDEX_NONE)
			{
				// Case 1: Right operand is in mPrevValue (not on stack yet)
				// Left operand MUST be on mXMMStack top.
				if (mXMMStack.empty())
				{
					// Should not happen with valid expression parsing
					return;
				}
				dstXMM = mXMMStack.back().idxXMM;

				switch (mPrevValue.type)
				{
				case VALUE_VARIABLE:
					{
						Asm::movabs(rax, (uint64)mPrevValue.var->ptr);
						mNumInstruction += 1 + emitBOPWithBaseLayout(opType, isReverse, mPrevValue.var->layout, stackXMM(dstXMM), rax);
					}
					break;
				case VALUE_INPUT:
					{
						StackValue& inputValue = mInputStack[mPrevValue.input.index];
						mNumInstruction += emitBOPWithLayout(opType, isReverse, inputValue.layout, stackXMM(dstXMM), rbp, inputValue.offset);
					}
					break;
				case VALUE_CONST:
					{
						StackValue& constValue = mConstStack[mPrevValue.idxCV];
						mNumInstruction += emitBOPWithLayout(opType, isReverse, constValue.layout, stackXMM(dstXMM), &mConstLabel, constValue.offset);
					}
					break;
				case VALUE_EXPR_TEMP:
					{
						int offset = GetTempStackOffset(mPrevValue.idxTemp);
						mNumInstruction += emitBOPMem(opType, isReverse, stackXMM(dstXMM), rbp, offset);
					}
					break;
				}
			}
			else
			{
				// Case 2: Right operand is already on stack or in a register cached from elsewhere (e.g. x*x)
				int srcXMM = INDEX_NONE;
				if (mPrevValue.idxXMM != INDEX_NONE)
				{
					srcXMM = mPrevValue.idxXMM;
					// Note: If cached, we don't pop from stack
				}
				else if (!mXMMStack.empty())
				{
					srcXMM = mXMMStack.back().idxXMM;
					mXMMStack.pop_back();
				}

				if (srcXMM != INDEX_NONE && !mXMMStack.empty())
				{
					dstXMM = mXMMStack.back().idxXMM;
					mNumInstruction += emitBOP(opType, isReverse, stackXMM(dstXMM), stackXMM(srcXMM));
				}
			}
		}

		mPrevValue.type = TOKEN_BINARY_OP;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeUnaryOp(TokenType type)
	{
		checkAndLoadPrevValue();

		if (type == UOP_MINS)
		{
			// Negate: xorpd with sign bit mask or use subsd from 0
			int xmmIdx = mXMMStack.empty() ? mReservedInputCount : mXMMStack.back().idxXMM;
			// Use xmm1 as scratch
			Asm::xorpd(xmm1, xmm1);           // zero
			Asm::subsd(xmm1, stackXMM(xmmIdx));     // 0 - value
			Asm::movsd(stackXMM(xmmIdx), xmm1);     // store back
			mNumInstruction += 3;
		}

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeEnd()
	{
		if (IsValue(mPrevValue.type))
		{
			checkAndLoadPrevValue();
		}

		// Move result to xmm0 if not already there
		// The result is at the top of the stack
		if (!mXMMStack.empty())
		{
			// stackXMM(0) is xmm6. xmm0 is volatile.
			// Function return value must be in xmm0.
			// We need to use the physical register index stored in idxXMM
			int resultXMMIdx = mXMMStack.back().idxXMM;
			Asm::movsd(xmm0, stackXMM(resultXMMIdx));
			++mNumInstruction;
		}

		// Restore used non-volatile XMM registers (xmm6+)
		// We used registers 0 to mMaxXMMStackUsage-1
		int numRestore = mMaxXMMStackUsage;
		if (numRestore > (int)MaxXMMStackSize)
			numRestore = (int)MaxXMMStackSize;

		for (int i = 0; i < numRestore; ++i)
		{
			Asm::movsd(stackXMM(i), qword_ptr(rbp, -16 - (i * 16)));
			++mNumInstruction;
		}

		auto removeCode = [this](int start, int size) {
			if (size > 0)
			{
				int codeAfter = mData->getCodeLength() - (start + size);

				if (codeAfter > 0)
				{
					memmove(mData->mCode + start, mData->mCode + start + size, codeAfter);
				}
				this->adjustLabelLinks(start + size, -size);
				mData->mCodeEnd -= size;
			}
		};

		// Optimize: Remove unused XMM save instructions from the prologue
		// We only remove the SAVE instructions that were pre-emitted for registers beyond actualUsed.
		int actualUsed = mMaxXMMStackUsage;
		int preEstimated = MaxXMMStackSize;
		if (actualUsed < preEstimated)
		{
			int numToRemove = preEstimated - actualUsed;
			if (numToRemove > 0)
			{
				// Remove excess SAVE instructions from the prologue in reverse order.
				// No need to remove RESTORE instructions because we only emitted numRestore of them above.
				for (int i = 0; i < numToRemove; ++i)
				{
					int idx = (preEstimated - 1) - i;
					removeCode(mXMMSaveOffsets[idx], mXMMSaveInstrSizes[idx]);
					mNumInstruction -= 1;
				}
			}
		}


		// Function epilogue
		Asm::mov(rsp, rbp);
		Asm::pop(rbp);
		Asm::ret();
		mNumInstruction += 3;

		// Emit constant data
		Asm::bind(&mConstLabel);

		if (!mConstStorage.empty())
		{
			mData->pushCode(&mConstStorage[0], mConstStorage.size());
		}

		Asm::reloc(mData->mCode);
#if _DEBUG
		mData->mNumInstruction = mNumInstruction;
#endif
	}


	void codeExprTemp(ExprTempInfo const& tempInfo)
	{
		checkAndLoadPrevValue();

		mPrevValue.type = VALUE_EXPR_TEMP;
		mPrevValue.idxTemp = tempInfo.tempIndex;
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeStoreTemp(int16 tempIndex)
	{
		// Store the current top of stack (result of last op) to temp slot
		// Do NOT pop it, as it is still needed as the result of the current expression node

		if (mXMMStack.empty())
			return; // Error?

		// Current result is at the top
		int regIdx = mXMMStack.back().idxXMM;

		// Store to stack memory
		int offset = GetTempStackOffset(tempIndex);
		Asm::movsd(qword_ptr(rbp, offset), stackXMM(regIdx));
		++mNumInstruction;

		if (tempIndex >= mNumTemps)
			mNumTemps = tempIndex + 1;
	}

protected:
	void checkAndLoadPrevValue()
	{
		if (!ExprParse::IsValue(mPrevValue.type))
			return;

		// Allocate next XMM register
		int xmmIdx = (int)mXMMStack.size();
		RegXMM dstReg = stackXMM(xmmIdx);

		// Load the value into XMM register
		mNumInstruction += emitLoadValue(mPrevValue, dstReg);

		// Push to stack
		mXMMStack.push_back(mPrevValue);
		mXMMStack.back().idxXMM = xmmIdx;

		updateStackUsage();
	}



};

//=============================================================================
// SSESIMDCodeGeneratorX64 - SIMD version (Processing 4 floats)
//=============================================================================
class SSESIMDCodeGeneratorX64 : public TCodeGenerator<SSESIMDCodeGeneratorX64>
	, public SSECodeGeneratorX64Base
{
	using BaseClass = SSECodeGeneratorX64Base;
public:
	using TokenType = SSECodeGeneratorX64Base::TokenType;

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mData->clear();
		mNumXMMUsed = 0;
		mNumInstruction = 0;
		mPrevValue.type = TOKEN_NONE;
		mXMMStack.clear();
		Asm::clearLink();

		mInputStack.resize(numInput);
		for (int i = 0; i < numInput; ++i)
		{
			mInputStack[i].layout = inputLayouts[i];
			if (i < 4)
				mInputStack[i].offset = 16 + i * 8;
			else
				mInputStack[i].offset = 16 + 32 + (i - 4) * 8;
		}

		mConstStorage.clear();
		mConstStack.clear();
		mNumTemps = 0;
		mSimulatedStackDepth = 0;
		mMaxStackDepth = 0;
	}

	void postLoadCode()
	{
		// Function prologue for x64
		Asm::push(rbp);
		Asm::mov(rbp, rsp);

		int tempSize = mNumTemps * 16;
		int finalStackSize = 32 + 16 * MaxXMMStackSize + tempSize + 64;

		mStackReserve = finalStackSize;
		mStackReservePos = mData->getCodeLength();
		Asm::sub(rsp, imm32(mStackReserve));
		mNumInstruction += 3;

		// Calculate precise input reserve count for Shadow Space storage
		mReservedInputCount = 0;
		for (int i = 0; i < (int)std::min(mInputUsed.size(), (size_t)4); ++i)
		{
			if (mInputUsed[i])
				mReservedInputCount = i + 1;
		}

		// Only reserve registers for the actual expression stack depth.
		// Inputs will be loaded from Shadow Space (rbp+10h, etc.) on demand.
		mMaxXMMStackUsage = mMaxStackDepth;

		// SIMD: Save non-volatile XMM registers using movups (128-bit)
		mXMMSaveCodeStart = mData->getCodeLength();
		mXMMSaveOffsets.resize(MaxXMMStackSize);

		int numSave = std::max(0, mMaxXMMStackUsage);
		if (numSave > MaxXMMStackSize)
			numSave = MaxXMMStackSize;

		for (int i = 0; i < numSave; ++i)
		{
			mXMMSaveOffsets[i] = mData->getCodeLength();
			Asm::movups(xmmword_ptr(rbp, -16 - (i * 16)), stackXMM(i));
		}
		mXMMSaveCodeEnd = mData->getCodeLength();

		// SIMD: Save first 4 floating-point args to shadow space only (if used)
		for (int i = 0; i < mReservedInputCount; ++i)
		{
			if (mInputUsed[i])
			{
				Asm::movups(xmmword_ptr(rbp, int8(mInputStack[i].offset)), xmm(i));
			}
		}

		mNumInstruction += 4 + mReservedInputCount;
	}

	void codeConstValue(ConstValueInfo const& val)
	{
		checkAndLoadPrevValue();
		mPrevValue.type = VALUE_CONST;
		mPrevValue.idxCV = addConstValue(val);
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeVar(VariableInfo const& varInfo)
	{
		checkAndLoadPrevValue();
		mPrevValue.type = VALUE_VARIABLE;
		mPrevValue.var = &varInfo;
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeInput(InputInfo const& input)
	{
		checkAndLoadPrevValue();
		mPrevValue.type = VALUE_INPUT;
		mPrevValue.input = input;
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeExprTemp(ExprTempInfo const& tempInfo)
	{
		checkAndLoadPrevValue();
		mPrevValue.type = VALUE_EXPR_TEMP;
		mPrevValue.idxTemp = tempInfo.tempIndex;
		mPrevValue.idxXMM = findXMMWithValue(mPrevValue);
	}

	void codeStoreTemp(int16 tempIndex)
	{
		if (mXMMStack.empty()) return;

		int regIdx = mXMMStack.back().idxXMM;
		int offset = GetTempStackOffsetSIMD(tempIndex);
		Asm::movups(xmmword_ptr(rbp, offset), stackXMM(regIdx));
		mNumInstruction++;

		if (tempIndex >= mNumTemps)
			mNumTemps = tempIndex + 1;
	}

	// Internal SIMD wrappers to get addresses
	static __m128 __cdecl SIMD_Sin(__m128 v) { return SIMD::sin(v); }
	static __m128 __cdecl SIMD_Cos(__m128 v) { return SIMD::cos(v); }
	static __m128 __cdecl SIMD_Tan(__m128 v) { return SIMD::tan(v); }
	static __m128 __cdecl SIMD_Exp(__m128 v) { return SIMD::exp(v); }
	static __m128 __cdecl SIMD_Log(__m128 v) { return SIMD::log(v); }

	static void* GetSIMDFuncAddress(int id)
	{
		switch (id)
		{
		case EFuncSymbol::Exp: return (void*)&SIMD_Exp;
		case EFuncSymbol::Ln:  return (void*)&SIMD_Log;
		case EFuncSymbol::Sin: return (void*)&SIMD_Sin;
		case EFuncSymbol::Cos: return (void*)&SIMD_Cos;
		case EFuncSymbol::Tan: return (void*)&SIMD_Tan;
		}
		return nullptr;
	}

	void codeFunction(FuncInfo const& info)
	{
		checkAndLoadPrevValue();
		int numParam = info.getArgNum();
		int numLive = (int)mXMMStack.size() - numParam;

		// We need to call this scalar function 4 times (for each SIMD lane)
		// 1. Prepare result accumulator in xmm15 (we'll assemble results here)
		Asm::xorps(xmm15, xmm15);

		for (int lane = 0; lane < 4; ++lane)
		{
			// a. Extract parameters for the current lane and put them in XMM0...XMM3
			for (int i = 0; i < numParam && i < 4; ++i)
			{
				int srcPhyIdx = mXMMStack[numLive + i].idxXMM;
				RegXMM srcReg = stackXMM(srcPhyIdx);

				// Move the target lane value to the lowest position (Scalar pos)
				if (lane == 0) {
					Asm::movss(xmm(i), srcReg);
				}
				else {
					// Use shufps or rotate to bring lane i to index 0
					Asm::movaps(xmm(i), srcReg);
					Asm::shufps(xmm(i), xmm(i), uint8(lane));
				}
				mNumInstruction += 2;
			}

			// b. Call the scalar function (Result in XMM0)
			Asm::movabs(rax, (uint64)info.funcPtr);
			Asm::call(rax);
			mNumInstruction += 2;

			// c. Insert the result into the corresponding lane of our assembly register (xmm15)
			if (lane == 0) {
				Asm::movss(xmm15, xmm0);
			}
			else {
				// Use insertps to put xmm0[0] into xmm15[lane]
				// Lane bits are (lane << 4)
				Asm::insertps(xmm15, xmm0, uint8(lane << 4));
				mNumInstruction++;
			}
		}

		// 3. Pop parameters from stack
		for (int i = 0; i < numParam; ++i) mXMMStack.pop_back();

		// 4. Save the assembled 4-lane result back to stack
		int resultIdx = mReservedInputCount + numLive;
		Asm::movaps(stackXMM(resultIdx), xmm15);
		mXMMStack.emplace_back();
		mXMMStack.back().type = TOKEN_FUNC;
		mXMMStack.back().idxXMM = resultIdx;
		mNumInstruction++;

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeFunction(FuncSymbolInfo const& info)
	{
		checkAndLoadPrevValue();
		if (info.id == EFuncSymbol::Sqrt)
		{
			// Direct SIMD instruction for SQRT
			int xmmIdx = mXMMStack.back().idxXMM;
			Asm::sqrtps(stackXMM(xmmIdx), stackXMM(xmmIdx));
			mNumInstruction++;
			return;
		}

		void* funcAddr = GetSIMDFuncAddress(info.id);
		if (!funcAddr) throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "SIMD function not supported");

		int numParam = info.numArgs;
		int numLive = (int)mXMMStack.size() - numParam;

		// Map arguments to parameter registers (XMM0...)
		for (int i = 0; i < numParam; ++i)
		{
			Asm::movups(xmm(i), stackXMM(mXMMStack[numLive + i].idxXMM));
			mNumInstruction++;
		}

		Asm::movabs(rax, (uint64)funcAddr);
		Asm::call(rax);
		mNumInstruction += 2;

		// Cleanup stack
		for (int i = 0; i < numParam; ++i) mXMMStack.pop_back();

		// Save result
		int resultIdx = numLive;
		Asm::movups(stackXMM(resultIdx), xmm0);
		mXMMStack.emplace_back();
		mXMMStack.back().type = TOKEN_FUNC;
		mXMMStack.back().idxXMM = resultIdx;
		mNumInstruction++;

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeBinaryOp(TokenType opType, bool isReverse)
	{
		if (opType == BOP_ASSIGN)
		{
			switch (mPrevValue.type)
			{
			case VALUE_INPUT:
				{
					StackValue& inputValue = mInputStack[mPrevValue.input.index];
					if (!IsPointer(inputValue.layout))
						throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Input value layout is not pointer when assign");
					if (!mXMMStack.empty()) {
						int srcPhyIdx = mXMMStack.back().idxXMM;
						Asm::movups(xmmword_ptr(rbp, inputValue.offset), stackXMM(srcPhyIdx));
						mNumInstruction++;
					}
				}
				break;
			case VALUE_VARIABLE:
				{
					if (!mXMMStack.empty()) {
						int srcPhyIdx = mXMMStack.back().idxXMM;
						Asm::movabs(rax, (uint64)mPrevValue.var->ptr);
						Asm::movups(xmmword_ptr(rax, 0), stackXMM(srcPhyIdx));
						mNumInstruction += 2;
					}
				}
				break;
			}
		}
		else
		{
			// Determine destination (left operand)
			int dstXMMIdx;
			if (IsValue(mPrevValue.type) && mPrevValue.idxXMM == INDEX_NONE) {
				dstXMMIdx = mXMMStack.empty() ? mReservedInputCount : mXMMStack.back().idxXMM;
			}
			else {
				dstXMMIdx = (mXMMStack.size() >= 2) ? mXMMStack[mXMMStack.size() - 2].idxXMM : mReservedInputCount;
			}
			RegXMM dst = stackXMM(dstXMMIdx);

			auto emitSIMDBOP = [&](RegXMM src) {
				switch (opType) {
				case BOP_ADD: Asm::addps(dst, src); break;
				case BOP_MUL: Asm::mulps(dst, src); break;
				case BOP_SUB:
					if (isReverse) { Asm::movaps(xmm1, dst); Asm::movaps(dst, src); Asm::subps(dst, xmm1); }
					else Asm::subps(dst, src);
					break;
				case BOP_DIV:
					if (isReverse) { Asm::movaps(xmm1, dst); Asm::movaps(dst, src); Asm::divps(dst, xmm1); }
					else Asm::divps(dst, src);
					break;
				}
				mNumInstruction++;
			};

			if (mPrevValue.idxXMM != INDEX_NONE)
			{
				emitSIMDBOP(stackXMM(mPrevValue.idxXMM));
				mXMMStack.pop_back();
			}
			else if (IsValue(mPrevValue.type))
			{
				emitLoadValueSIMD(mPrevValue, xmm1);
				emitSIMDBOP(xmm1);
			}
			else
			{
				// Both are results on stack
				int srcPhyIdx = mXMMStack.back().idxXMM;
				emitSIMDBOP(stackXMM(srcPhyIdx));
				mXMMStack.pop_back();
			}
		}

		mPrevValue.type = TOKEN_BINARY_OP;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeUnaryOp(TokenType type)
	{
		checkAndLoadPrevValue();
		if (type == UOP_MINS)
		{
			// Negate all 4 lanes: xorps with 0x80000000
			int xmmIdx = mXMMStack.back().idxXMM;
			RegXMM reg = stackXMM(xmmIdx);

			// Use a constant -0.0f (which is 0x80000000 bit mask)
			ConstValueInfo maskVal(-0.0f);
			int idx = addConstValue(maskVal);
			StackValue& constValue = mConstStack[idx];

			// 1. Load mask [v, ?, ?, ?]
			Asm::movss(xmm1, dword_ptr(&mConstLabel, constValue.offset));
			// 2. Broadcast to [v, v, v, v]
			Asm::shufps(xmm1, xmm1, 0);
			// 3. XOR to negate
			Asm::xorps(reg, xmm1);
			mNumInstruction += 4;
		}

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeEnd()
	{
		if (IsValue(mPrevValue.type)) checkAndLoadPrevValue();
		if (!mXMMStack.empty())
		{
			int resultXMMIdx = mXMMStack.back().idxXMM;
			Asm::movups(xmm0, stackXMM(resultXMMIdx));
			mNumInstruction++;
		}

		// Standard Epilogue: Restore used non-volatile XMM registers (xmm6+)
		int numRestore = mMaxXMMStackUsage;
		if (numRestore > (int)MaxXMMStackSize)
			numRestore = (int)MaxXMMStackSize;

		for (int i = 0; i < numRestore; ++i) {
			Asm::movups(stackXMM(i), xmmword_ptr(rbp, -16 - (i * 16)));
			mNumInstruction++;
		}

		auto removeCode = [this](int start, int size) {
			if (size > 0)
			{
				int codeAfter = mData->getCodeLength() - (start + size);
				if (codeAfter > 0)
				{
					memmove(mData->mCode + start, mData->mCode + start + size, codeAfter);
				}
				this->adjustLabelLinks(start + size, -size);
				mData->mCodeEnd -= size;
			}
		};

		// Optimize: Remove unused XMM save instructions from the prologue
		int actualUsed = mMaxXMMStackUsage;
		int preEstimated = MaxXMMStackSize;
		if (actualUsed < preEstimated)
		{
			int numToRemove = preEstimated - actualUsed;
			if (numToRemove > 0)
			{
				for (int i = 0; i < numToRemove; ++i)
				{
					int idx = (preEstimated - 1) - i;
					removeCode(mXMMSaveOffsets[idx], mXMMSaveInstrSizes[idx]);
					mNumInstruction -= 1;
				}
			}
		}


		Asm::mov(rsp, rbp); Asm::pop(rbp); Asm::ret();
		mNumInstruction += 3;

		Asm::bind(&mConstLabel);
		if (!mConstStorage.empty()) mData->pushCode(&mConstStorage[0], (int)mConstStorage.size());
		Asm::reloc(mData->mCode);
#if _DEBUG
		mData->mNumInstruction = mNumInstruction;
#endif
	}

	// Override loading to keep values as float and broadcast them
	int emitLoadValueSIMD(ValueInfo const& info, RegXMM const& dst)
	{
		switch (info.type)
		{
		case VALUE_VARIABLE:
			Asm::movabs(rax, (uint64)info.var->ptr);
			Asm::movss(dst, dword_ptr(rax, 0));
			Asm::shufps(dst, dst, 0);
			return 3;
		case VALUE_INPUT:
			{
				StackValue& inputValue = mInputStack[info.input.index];
				Asm::movss(dst, dword_ptr(rbp, inputValue.offset));
				Asm::shufps(dst, dst, 0);
				return 3;
			}
		case VALUE_CONST:
			{
				StackValue& constValue = mConstStack[info.idxCV];
				Asm::movss(dst, dword_ptr(&mConstLabel, constValue.offset));
				Asm::shufps(dst, dst, 0);
				return 3;
			}
		case VALUE_EXPR_TEMP:
			{
				// Load temporary expression value from stack (SIMD: 128-bit)
				int offset = GetTempStackOffsetSIMD(info.idxTemp);
				Asm::movups(dst, xmmword_ptr(rbp, offset));
				return 1;
			}
		}
		return 0;
	}

	void checkAndLoadPrevValue()
	{
		if (!ExprParse::IsValue(mPrevValue.type)) return;
		int xmmIdx = mReservedInputCount + (int)mXMMStack.size();
		RegXMM dstReg = stackXMM(xmmIdx);

		int count = emitLoadValueSIMD(mPrevValue, dstReg);

		mXMMStack.push_back(mPrevValue);
		mXMMStack.back().idxXMM = xmmIdx;
		updateStackUsage();
	}
};

#endif // TARGET_PLATFORM_64BITS
#endif
