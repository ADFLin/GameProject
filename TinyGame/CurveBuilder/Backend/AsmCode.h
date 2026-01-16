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
	TArray< RealType > mConstTable;

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
	TArray< int > mRegStack;
	TArray< ValueInfo > mStackValues;

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
		:mbIsSIMD(false)
	{
		mNumInstruction = 0;
	}

	bool mbIsSIMD;

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


	int32 mReservedInputCount = 0;

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


		// Initialize XMM stack
		mXMMStack.clear();
		mRegAllocator.reset(numInput);
		mCurPushIndex = 0;
	}


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
				Asm::movsd(xmm5, dst);  // temp = dst
				Asm::movsd(dst, src);    // dst = src
				Asm::subsd(dst, xmm5);  // dst = src - temp
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
				Asm::movsd(xmm5, dst);
				Asm::movsd(dst, src);
				Asm::divsd(dst, xmm5);
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
				Asm::movsd(xmm5, dst);
				Asm::movsd(dst, qword_ptr(std::forward<Args>(args)...));
				Asm::subsd(dst, xmm5);
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
				Asm::movsd(xmm5, dst);
				Asm::movsd(dst, qword_ptr(std::forward<Args>(args)...));
				Asm::divsd(dst, xmm5);
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


	static int GetTempStackOffset(int index)
	{
		// XMM saves end at: -16 - (MaxXMMStackSize * 16) = -176
		return -176 - 8 - index * 8;
	}

	static int GetTempStackOffsetSIMD(int index)
	{
		// SIMD temps need 16-byte alignment and 16-byte size
		// Start after XMM saves (-176)
		return -176 - 16 - index * 16;
	}

	virtual int emitLoadValue(ValueInfo const& info, RegXMM const& dst)
	{
		if (info.idxXMM != INDEX_NONE)
		{
			if (xmm(info.idxXMM).index() != dst.index())
			{
				if (mbIsSIMD)
					Asm::movups(dst, xmm(info.idxXMM));
				else
					Asm::movsd(dst, xmm(info.idxXMM));
				return 1;
			}
			return 0;
		}
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



	struct RegAllocator : public ExprParse
	{
		static int const ScratchReg = 5; // xmm5
		TArray<int> volatileFree;
		TArray<int> nonVolatileFree;
		uint32      usedMask = 0;

		// Simulation State
		struct AllocEvent
		{
			uint32 index : 24;
			uint32 bAlloc : 1;
			int32  refIndex : 7; // -1 if new alloc, else index in current simulated stack to reuse
		};
		TArray<AllocEvent> mAllocEvents;

		TArray<bool>       mPushCrossesCall;
		TArray<int>        mPushStack; 
		TArray<Unit>       mSimStackValue;
		Unit               mSimPrevUnit;
		TokenType          mSimulatedPrevType;
		TArray<bool>       mInputUsed;
		int                mNumTemps = 0;
		int                mCurPushIndex = 0;

		void reset(int numInput)
		{
			// Simulation Reset
			mAllocEvents.clear();
			mPushCrossesCall.clear();
			mPushStack.clear();
			mSimStackValue.clear();
			mSimPrevUnit = Unit();
			mSimulatedPrevType = TOKEN_NONE;
			mInputUsed.clear();
			mInputUsed.resize(numInput, false);
			mNumTemps = 0;
			mCurPushIndex = 0;
		}

		int findValueInSimStack(ExprParse::Unit const& code)
		{
			for (int i = 0; i < (int)mSimStackValue.size(); ++i)
			{
				if (code.type == mSimStackValue[i].type)
				{
					switch (code.type)
					{
					case VALUE_CONST: if (code.constValue == mSimStackValue[i].constValue) return i; break;
					case VALUE_VARIABLE: if (code.variable.ptr == mSimStackValue[i].variable.ptr) return i; break;
					case VALUE_INPUT: if (code.input.index == mSimStackValue[i].input.index) return i; break;
					case VALUE_EXPR_TEMP: if (code.exprTemp.tempIndex == mSimStackValue[i].exprTemp.tempIndex) return i; break;
					}
				}
			}
			return -1;
		}

		void pushSimValue(ExprParse::Unit const& code)
		{
			int refIdx = findValueInSimStack(code);
			mPushStack.push_back(mCurPushIndex);
			mSimStackValue.push_back(code);
			mAllocEvents.push_back({ (uint32)mCurPushIndex, 1, (int8)refIdx });
			if (mPushCrossesCall.size() <= mCurPushIndex) mPushCrossesCall.resize(mCurPushIndex + 1);
			mPushCrossesCall[mCurPushIndex] = false;
			mCurPushIndex++;
		}

		void popSimStack(int count)
		{
			for (int i = 0; i < count; ++i)
			{
				if (!mPushStack.empty())
				{
					mAllocEvents.push_back({ (uint32)mPushStack.back(), 0, -1 });
					mPushStack.pop_back();
					mSimStackValue.pop_back();
				}
			}
		}

		static int const RegSlotBase = 128;

		void resetRegisters()
		{
			volatileFree.clear();
			for (int i = 4; i >= 1; --i) volatileFree.push_back(i);

			nonVolatileFree.clear();
			for (int i = 15; i >= 6; --i) nonVolatileFree.push_back(i);

			usedMask = 0;
		}

		void preLoadCode(ExprParse::Unit const& code)
		{
			if (IsValue(code.type))
			{
				if (IsValue(mSimulatedPrevType))
				{
					pushSimValue(mSimPrevUnit);
				}
				mSimPrevUnit = code;
				mSimulatedPrevType = code.type;
			}
			else if (IsFunction(code.type))
			{
				int numArgs = (code.type == FUNC_DEF) ? code.symbol->func.getArgNum() : code.funcSymbol.numArgs;
				if (IsValue(mSimulatedPrevType))
				{
					pushSimValue(mSimPrevUnit);
				}

				// Inform simulation that remaining items on stack will cross this call.
				// Arguments do not cross the call because they are consumed (popped) by it.
				int numRemaining = (int)mPushStack.size() - numArgs;
				for (int i = 0; i < numRemaining; ++i)
				{
					int idx = mPushStack[i];
					if (idx < mPushCrossesCall.size()) mPushCrossesCall[idx] = true;
				}

				if (numArgs > 0)
				{
					popSimStack(numArgs);
				}

				// Function Result Push (matches codeFunction increment)
				mPushStack.push_back(mCurPushIndex);
				mSimStackValue.push_back(Unit(code.type)); 
				mAllocEvents.push_back({ (uint32)mCurPushIndex, 1, -1 });
				if (mPushCrossesCall.size() <= mCurPushIndex) mPushCrossesCall.resize(mCurPushIndex + 1);
				mPushCrossesCall[mCurPushIndex] = false;
				mCurPushIndex++;

				mSimPrevUnit = Unit();
				mSimulatedPrevType = TOKEN_FUNC;
			}
			else if (IsBinaryOperator(code.type))
			{
				if (code.type == BOP_ASSIGN)
				{
					// No stack change
				}
				else
				{
					if (!IsValue(mSimulatedPrevType))
					{
						// Case 2: Right operand is on stack
						popSimStack(1);
					}
					// Case 1 & 2: Result stays in left operand stack slot
				}

				mSimPrevUnit = Unit();
				mSimulatedPrevType = TOKEN_BINARY_OP;
			}
			else if (IsUnaryOperator(code.type))
			{
				if (IsValue(mSimulatedPrevType))
				{
					pushSimValue(mSimPrevUnit);
				}
				
				mSimPrevUnit = Unit();
				mSimulatedPrevType = TOKEN_UNARY_OP;
			}

			// Property tracking
			if (code.type == VALUE_EXPR_TEMP)
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
		}

		int alloc(bool preferVolatile)
		{
			int reg = -1;
			if (preferVolatile && !volatileFree.empty()) { reg = volatileFree.back(); volatileFree.pop_back(); }
			else if (!nonVolatileFree.empty()) { reg = nonVolatileFree.back(); nonVolatileFree.pop_back(); }
			else if (!volatileFree.empty()) { reg = volatileFree.back(); volatileFree.pop_back(); }
			if (reg != -1) usedMask |= (1 << reg);
			return reg;
		}

		void free(int reg)
		{
			if (reg >= 1 && reg <= 4) volatileFree.push_back(reg);
			else if (reg >= 6 && reg <= 15) nonVolatileFree.push_back(reg);
		}

		// Emit SSE binary operation with XMM register

		void generateAllocPlan(TArray<int>& outPlan)
		{
			outPlan.clear();
			if (mAllocEvents.empty())
				return;

			resetRegisters();
			int maxPushIdx = 0;
			for (auto const& ev : mAllocEvents) if ((int)ev.index > maxPushIdx) maxPushIdx = ev.index;
			outPlan.resize(maxPushIdx + 1, -1);

			TArray<uint32> activePushes; 
			for (auto const& ev : mAllocEvents)
			{
				if (ev.bAlloc)
				{
					int regIdx = -1;
					if (ev.refIndex != -1)
					{
						uint32 refEventIdx = activePushes[ev.refIndex];
						regIdx = outPlan[refEventIdx];
					}
					else
					{
						bool bCrossesCall = (ev.index < mPushCrossesCall.size()) ? mPushCrossesCall[ev.index] : true;
						regIdx = alloc(!bCrossesCall);
					}

					// If exhausted, assign to an anonymous stack slot
					if (regIdx == -1)
					{
						regIdx = RegSlotBase + mNumTemps++;
					}

					outPlan[ev.index] = regIdx;
					activePushes.push_back(ev.index);
				}
				else
				{
					for (int i = (int)activePushes.size() - 1; i >= 0; --i)
					{
						if (activePushes[i] == ev.index)
						{
							int reg = outPlan[ev.index];
							activePushes.erase(activePushes.begin() + i);

							if (reg < RegSlotBase)
							{
								bool isStillUsed = false;
								for (uint32 otherIdx : activePushes) { if (outPlan[otherIdx] == reg) { isStillUsed = true; break; } }
								if (!isStillUsed) free(reg);
							}
							break;
						}
					}
				}
			}
		}
	};
	
	RegAllocator mRegAllocator;
	
	TArray<int>  mAllocPlan;
	int mCurPushIndex = 0;

	void preLoadCode(ExprParse::Unit const& code)
	{
		if (code.type == VALUE_CONST)
		{
			addConstValue(code.constValue);
		}
		mRegAllocator.preLoadCode(code);
	}

	// checkAndLoadPrevValue replacement that uses mAllocPlan
	void checkAndLoadPrevValue()
	{
		if (!ExprParse::IsValue(mPrevValue.type))
			return;

		int xmmIdx = -1;
		if (mCurPushIndex < mAllocPlan.size())
			xmmIdx = mAllocPlan[mCurPushIndex];
		
		mCurPushIndex++;

		if (xmmIdx == -1) throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Register allocation failed (Plan missing)");

		if (xmmIdx >= RegAllocator::RegSlotBase)
		{
			// Spill to memory slot
			int slotIdx = xmmIdx - RegAllocator::RegSlotBase;
			int offset = mbIsSIMD ? GetTempStackOffsetSIMD(slotIdx) : GetTempStackOffset(slotIdx);
			
			// If already in register, just move to memory. If not, use ScratchReg as window.
			int currentReg = findXMMWithValue(mPrevValue);
			if (currentReg != INDEX_NONE)
			{
				if (mbIsSIMD) Asm::movups(xmmword_ptr(rbp, offset), xmm(currentReg));
				else Asm::movsd(qword_ptr(rbp, offset), xmm(currentReg));
				mNumInstruction++;
			}
			else
			{
				mNumInstruction += emitLoadValue(mPrevValue, xmm(RegAllocator::ScratchReg));
				if (mbIsSIMD) Asm::movups(xmmword_ptr(rbp, offset), xmm(RegAllocator::ScratchReg));
				else Asm::movsd(qword_ptr(rbp, offset), xmm(RegAllocator::ScratchReg));
				mNumInstruction++;
			}
		}
		else
		{
			// Load to physical register
			RegXMM dstReg = xmm(xmmIdx);
			int currentReg = findXMMWithValue(mPrevValue);
			CHECK(currentReg == INDEX_NONE || currentReg == xmmIdx);
			mNumInstruction += emitLoadValue(mPrevValue, dstReg);
		}

		mXMMStack.push_back(mPrevValue);
		mXMMStack.back().idxXMM = xmmIdx;
	}
};


class SSECodeGeneratorX64 : public TCodeGenerator<SSECodeGeneratorX64>
	                      , public SSECodeGeneratorX64Base
{
	using BaseClass = SSECodeGeneratorX64Base;
public:
	using TokenType = SSECodeGeneratorX64Base::TokenType;

	void codeInit(int numInput, ValueLayout inputLayouts[])
	{
		mbIsSIMD = false;
		mData->clear();
		mNumXMMUsed = 0;
		mNumInstruction = 0;

		mPrevValue.type = TOKEN_NONE;
		mPrevValue.var = nullptr;

		Asm::clearLink();

		BaseClass::codeInit(numInput, inputLayouts);
	}
	
	void postLoadCode()
	{
		// Function prologue for x64
		Asm::push(rbp);
		Asm::mov(rbp, rsp);

		if (IsValue(mRegAllocator.mSimulatedPrevType))
		{
			mRegAllocator.pushSimValue(mRegAllocator.mSimPrevUnit);
		}

		// Calculate precise stack size
		int tempSize = mRegAllocator.mNumTemps * 8;
		if (tempSize % 16 != 0) tempSize += 8; // Maintain 16-byte alignment
		int finalStackSize = 32 + 16 * MaxXMMStackSize + tempSize + 32;

		Asm::sub(rsp, imm32(finalStackSize));
		mNumInstruction += 3;

		// Calculate precise input reserve count for Shadow Space storage
		mReservedInputCount = 0;
		for (int i = 0; i < (int)std::min(mRegAllocator.mInputUsed.size(), (size_t)4); ++i)
		{
			if (mRegAllocator.mInputUsed[i])
				mReservedInputCount = i + 1;
		}

		// Generate the allocation plan based on collected stack info
		mRegAllocator.generateAllocPlan(mAllocPlan);
		mCurPushIndex = 0;

		// Save non-volatile XMM registers (xmm6 - xmm15)
		// We save all of them here based on the simulation plan
		for (int i = 0; i < MaxXMMStackSize; ++i)
		{
			int reg = 6 + i;

			// Save only if used in simulation
			if (mRegAllocator.usedMask & (1 << reg))
			{
				Asm::movsd(qword_ptr(rbp, -16 - (i * 16)), xmm(reg));
			}
		}

		// Save first 4 floating-point args to shadow space only (if used)
		for (int i = 0; i < mReservedInputCount; ++i)
		{
			if (mRegAllocator.mInputUsed[i])
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
		if (mReservedInputCount + numLive + 1 >= MaxXMMStackSize + 4)
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
				if (phyIdx >= RegAllocator::RegSlotBase)
				{
					int offset = GetTempStackOffset(phyIdx - RegAllocator::RegSlotBase);
					Asm::movsd(xmm(i), qword_ptr(rbp, offset));
				}
				else
				{
					Asm::movsd(xmm(i), xmm(phyIdx));
				}
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
				if (phyIdx >= RegAllocator::RegSlotBase)
				{
					// Use xmm5 as scratch if spilled
					int offset = GetTempStackOffset(phyIdx - RegAllocator::RegSlotBase);
					Asm::movsd(xmm5, qword_ptr(rbp, offset));
					Asm::movsd(qword_ptr(rsp, int8(stackOffset)), xmm5);
					mNumInstruction++;
				}
				else
				{
					Asm::movsd(qword_ptr(rsp, int8(stackOffset)), xmm(phyIdx));
				}
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
		
		int resultReg = -1;
		if (mCurPushIndex < mAllocPlan.size())
			resultReg = mAllocPlan[mCurPushIndex];

		mCurPushIndex++; // Consume one push ID for the result

		if (resultReg >= RegAllocator::RegSlotBase)
		{
			int offset = GetTempStackOffset(resultReg - RegAllocator::RegSlotBase);
			Asm::movsd(qword_ptr(rbp, offset), xmm0);
		}
		else
		{
			Asm::movsd(xmm(resultReg), xmm0);
		}
		++mNumInstruction;

		// Push result to our stack with correct physical register index
		mXMMStack.emplace_back();
		mXMMStack.back().type = TOKEN_FUNC;
		mXMMStack.back().idxXMM = resultReg;

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
		if (mReservedInputCount + numLive + 1 >= MaxXMMStackSize + 4) // +1 for result
		{
			throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Expression too complex (stack overflow)");
		}

		// 2. Move arguments to parameter registers (xmm0...xmm3)
		// Arguments are at mXMMStack[numLive] ... mXMMStack[end]
		for (int i = 0; i < numParam; ++i)
		{
			int srcStackIdx = numLive + i;
			int phyIdx = mXMMStack[srcStackIdx].idxXMM;
			if (phyIdx >= RegAllocator::RegSlotBase)
			{
				int offset = GetTempStackOffset(phyIdx - RegAllocator::RegSlotBase);
				Asm::movsd(xmm(i), qword_ptr(rbp, offset));
			}
			else
			{
				Asm::movsd(xmm(i), xmm(phyIdx));
			}
			++mNumInstruction;
		}

		// 3. Call the function
		Asm::movabs(rax, (uint64)funcAddr);
		Asm::call(rax);
		mNumInstruction += 2;

		// 4. Handle Inverse Trig functions (Cot, Sec, Csc)
		if (info.id == EFuncSymbol::Cot || info.id == EFuncSymbol::Sec || info.id == EFuncSymbol::Csc)
		{
			// result (xmm0) = 1.0 / result (xmm0)
			// Load 1.0 into xmm1
			ValueInfo vi;
			vi.type = VALUE_CONST;
			vi.idxCV = addConstValue(ConstValueInfo(1.0));
			mNumInstruction += emitLoadValue(vi, xmm1);
			Asm::divsd(xmm1, xmm0);
			Asm::movsd(xmm0, xmm1);
			mNumInstruction += 2;
		}

		int resultReg = -1;
		if (mCurPushIndex < mAllocPlan.size())
			resultReg = mAllocPlan[mCurPushIndex];

		mCurPushIndex++;

		if (resultReg >= RegAllocator::RegSlotBase)
		{
			int offset = GetTempStackOffset(resultReg - RegAllocator::RegSlotBase);
			Asm::movsd(qword_ptr(rbp, offset), xmm0);
		}
		else
		{
			Asm::movsd(xmm(resultReg), xmm0);
		}
		++mNumInstruction;

		// Update XMMStack state
		for (int i = 0; i < numParam; ++i)
		{
			mXMMStack.pop_back();
		}

		mXMMStack.emplace_back();
		mXMMStack.back().type = TOKEN_FUNC;
		mXMMStack.back().idxXMM = resultReg;

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
						int stackXMM = mXMMStack[(int)mXMMStack.size() - 1].idxXMM;
						mNumInstruction += emitStoreValueWithLayout(inputValue.layout, xmm(stackXMM), rbp, inputValue.offset);
					}
				}
				break;
			case VALUE_VARIABLE:
				{
					if (!mXMMStack.empty())
					{
						int stackXMM = mXMMStack[(int)mXMMStack.size() - 1].idxXMM;
						Asm::movabs(rax, (uint64)mPrevValue.var->ptr);
						mNumInstruction += 1 + emitStoreValueWithBaseLayout(mPrevValue.var->layout, xmm(stackXMM), rax);
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
						mNumInstruction += 1 + emitBOPWithBaseLayout(opType, isReverse, mPrevValue.var->layout, xmm(dstXMM), rax);
					}
					break;
				case VALUE_INPUT:
					{
						StackValue& inputValue = mInputStack[mPrevValue.input.index];
						mNumInstruction += emitBOPWithLayout(opType, isReverse, inputValue.layout, xmm(dstXMM), rbp, inputValue.offset);
					}
					break;
				case VALUE_CONST:
					{
						StackValue& constValue = mConstStack[mPrevValue.idxCV];
						mNumInstruction += emitBOPWithLayout(opType, isReverse, constValue.layout, xmm(dstXMM), &mConstLabel, constValue.offset);
					}
					break;
				case VALUE_EXPR_TEMP:
					{
						int offset = GetTempStackOffset(mPrevValue.idxTemp);
						mNumInstruction += emitBOPMem(opType, isReverse, xmm(dstXMM), rbp, offset);
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
					if (dstXMM >= RegAllocator::RegSlotBase)
					{
						// Reload destination to scratch, compute, then spill back
						int offset = GetTempStackOffset(dstXMM - RegAllocator::RegSlotBase);
						Asm::movsd(xmm(RegAllocator::ScratchReg), qword_ptr(rbp, offset));

						if (srcXMM >= RegAllocator::RegSlotBase)
						{
							int srcOffset = GetTempStackOffset(srcXMM - RegAllocator::RegSlotBase);
							mNumInstruction += emitBOPMem(opType, isReverse, xmm(RegAllocator::ScratchReg), rbp, srcOffset);
						}
						else
						{
							mNumInstruction += emitBOP(opType, isReverse, xmm(RegAllocator::ScratchReg), xmm(srcXMM));
						}

						Asm::movsd(qword_ptr(rbp, offset), xmm(RegAllocator::ScratchReg));
						mNumInstruction += 2;
					}
					else
					{
						if (srcXMM >= RegAllocator::RegSlotBase)
						{
							int srcOffset = GetTempStackOffset(srcXMM - RegAllocator::RegSlotBase);
							mNumInstruction += emitBOPMem(opType, isReverse, xmm(dstXMM), rbp, srcOffset);
						}
						else
						{
							mNumInstruction += emitBOP(opType, isReverse, xmm(dstXMM), xmm(srcXMM));
						}
					}
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
			
			if (xmmIdx >= RegAllocator::RegSlotBase)
			{
				int offset = GetTempStackOffset(xmmIdx - RegAllocator::RegSlotBase);
				Asm::movsd(xmm(RegAllocator::ScratchReg), qword_ptr(rbp, offset));
				
				Asm::xorpd(xmm1, xmm1);           // zero
				Asm::subsd(xmm1, xmm(RegAllocator::ScratchReg));     // 0 - value
				
				Asm::movsd(qword_ptr(rbp, offset), xmm1);
				mNumInstruction += 4;
			}
			else
			{
				// Use xmm5 as scratch
				Asm::xorpd(xmm5, xmm5);           // zero
				Asm::subsd(xmm5, xmm(xmmIdx));     // 0 - value
				Asm::movsd(xmm(xmmIdx), xmm5);     // store back
				mNumInstruction += 3;
			}
		}

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeEnd()
	{
		checkAndLoadPrevValue();

		// Move result to xmm0 if not already there
		// The result is at the top of the stack
		if (!mXMMStack.empty())
		{
			// stackXMM(0) is xmm6. xmm0 is volatile.
			// Function return value must be in xmm0.
			// We need to use the physical register index stored in idxXMM
			int resultXMMIdx = mXMMStack.back().idxXMM;
			if (resultXMMIdx >= RegAllocator::RegSlotBase)
			{
				int offset = GetTempStackOffset(resultXMMIdx - RegAllocator::RegSlotBase);
				Asm::movsd(xmm0, qword_ptr(rbp, offset));
			}
			else
			{
				Asm::movsd(xmm0, xmm(resultXMMIdx));
			}
			++mNumInstruction;
		}

		// Restore used non-volatile XMM registers (xmm6+)
		for (int i = 0; i < MaxXMMStackSize; ++i)
		{
			int reg = 6 + i;
			if (mRegAllocator.usedMask & (1 << reg))
			{
				Asm::movsd(xmm(reg), qword_ptr(rbp, -16 - (i * 16)));
				++mNumInstruction;
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
		Asm::movsd(qword_ptr(rbp, offset), xmm(regIdx));
		++mNumInstruction;

	}

protected:




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
		mbIsSIMD = true;
		mData->clear();
		mNumXMMUsed = 0;
		mNumInstruction = 0;
		mPrevValue.type = TOKEN_NONE;
		mPrevValue.var = nullptr;
		Asm::clearLink();

		BaseClass::codeInit(numInput, inputLayouts);
	}

	void postLoadCode()
	{
		// Function prologue for x64
		Asm::push(rbp);
		Asm::mov(rbp, rsp);

		if (IsValue(mRegAllocator.mSimulatedPrevType))
		{
			mRegAllocator.pushSimValue(mRegAllocator.mSimPrevUnit);
		}

		// Calculate precise stack size. SIMD needs more space for temps? 
		// Actually xmmword is 16 bytes. tempIndex counts slots. 
		// If tempIndex is used for SIMD, each slot is 16 bytes.
		// GetTempStackOffsetSIMD confirms index*16.
		int tempSize = mRegAllocator.mNumTemps * 16;
		int finalStackSize = 32 + 16 * MaxXMMStackSize + tempSize + 64;

		Asm::sub(rsp, imm32(finalStackSize));
		mNumInstruction += 3;

		// Calculate precise input reserve count for Shadow Space storage
		mReservedInputCount = 0;
		for (int i = 0; i < (int)std::min(mRegAllocator.mInputUsed.size(), (size_t)4); ++i)
		{
			if (mRegAllocator.mInputUsed[i])
				mReservedInputCount = i + 1;
		}

		// Generate the allocation plan based on collected stack info
		mRegAllocator.generateAllocPlan(mAllocPlan);
		mCurPushIndex = 0;

		// Save non-volatile XMM registers (xmm6 - xmm15)
		// We save all of them here based on the simulation plan
		for (int i = 0; i < MaxXMMStackSize; ++i)
		{
			int reg = 6 + i;

			// Save only if used in simulation
			if (mRegAllocator.usedMask & (1 << reg))
			{
				Asm::movups(xmmword_ptr(rbp, -16 - (i * 16)), xmm(reg));
			}
		}

		// SIMD: Save first 4 scalar args to shadow space (if used)
		// Even in SIMD mode, JIT inputs are usually scalar unless specifically handled.
		for (int i = 0; i < mReservedInputCount; ++i)
		{
			if (mRegAllocator.mInputUsed[i])
			{
				// Use movss or movsd to avoid overwriting adjacent shadow slots (8 bytes each)
				// Assuming float/double inputs.
				if (mInputStack[i].layout == ValueLayout::Double || mInputStack[i].layout == ValueLayout::DoublePtr)
					Asm::movsd(qword_ptr(rbp, int8(mInputStack[i].offset)), xmm(i));
				else
					Asm::movss(dword_ptr(rbp, int8(mInputStack[i].offset)), xmm(i));
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
		Asm::movups(xmmword_ptr(rbp, offset), xmm(regIdx));
		mNumInstruction++;

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

		// Use the plan's assigned result register as the accumulator
		// Note: The plan assumes arguments are popped (freed) before result is pushed (allocated).
		// So resultReg MIGHT reuse an argument register.
		int accRegIdx = -1;
		if (mCurPushIndex < mAllocPlan.size())
			accRegIdx = mAllocPlan[mCurPushIndex];
		mCurPushIndex++;

		if (accRegIdx == -1) throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "Register allocation failed");


		// Check for conflict: Does accRegIdx alias any live input argument?
		bool hasConflict = false;
		for (int i = 0; i < numParam; ++i)
		{
			if (mXMMStack[numLive + i].idxXMM == accRegIdx)
			{
				hasConflict = true;
				break;
			}
		}

		// If conflict, we must save the input value elsewhere because accReg will be overwritten
		if (hasConflict)
		{
			Asm::sub(rsp, imm32(16));
			if (accRegIdx >= RegAllocator::RegSlotBase)
			{
				int offset = GetTempStackOffsetSIMD(accRegIdx - RegAllocator::RegSlotBase);
				Asm::movups(xmm0, xmmword_ptr(rbp, offset));
				Asm::movups(xmmword_ptr(rsp, 0), xmm0);
			}
			else
			{
				Asm::movups(xmmword_ptr(rsp, 0), xmm(accRegIdx));
			}
			mNumInstruction += 2;
		}

		// Use ScratchReg as accumulator if result is spilled
		int physicalAccIdx = (accRegIdx >= RegAllocator::RegSlotBase) ? RegAllocator::ScratchReg : accRegIdx;
		RegXMM accReg = xmm(physicalAccIdx);
		Asm::xorps(accReg, accReg);

		for (int lane = 0; lane < 4; ++lane)
		{
			for (int i = 0; i < numParam && i < 4; ++i)
			{
				RegXMM srcReg = xmm(0); // Initialize
				int srcPhyIdx = mXMMStack[numLive + i].idxXMM;

				if (hasConflict && srcPhyIdx == accRegIdx)
				{
					// Load from spill
					Asm::movups(xmm0, xmmword_ptr(rsp, 0)); 
					srcReg = xmm0;
					mNumInstruction++;
				}
				else if (srcPhyIdx >= RegAllocator::RegSlotBase)
				{
					// Load from spilled virtual register slot
					int offset = GetTempStackOffsetSIMD(srcPhyIdx - RegAllocator::RegSlotBase);
					Asm::movups(xmm(RegAllocator::ScratchReg), xmmword_ptr(rbp, offset)); 
					srcReg = xmm(RegAllocator::ScratchReg);
					mNumInstruction++;
				}
				else
				{
					srcReg = xmm(srcPhyIdx);
				}
				

				if (lane == 0) {
					Asm::movss(xmm(i), srcReg);
				}
				else {
					Asm::movaps(xmm(i), srcReg);
					Asm::shufps(xmm(i), xmm(i), uint8(lane));
				}
				mNumInstruction += 2;
			}

			// Call Scalar Function
			// Since we iterate lanes, we call the scalar function implementation
			Asm::movabs(rax, (uint64)info.funcPtr);
			Asm::call(rax);
			mNumInstruction += 2;

			// Insert result (xmm0) into accumulator
			// 0x00 -> lane 0, 0x10 -> lane 1, 0x20 -> lane 2, 0x30 -> lane 3
			uint8 mask = (lane << 4); 
			Asm::insertps(accReg, xmm0, mask);
			mNumInstruction++;
		}

		if (hasConflict)
		{
			Asm::add(rsp, imm32(16));
			mNumInstruction++;
		}

		// If result was spilled, move from scratch register to stack slot
		if (accRegIdx >= RegAllocator::RegSlotBase)
		{
			int offset = GetTempStackOffsetSIMD(accRegIdx - RegAllocator::RegSlotBase);
			Asm::movups(xmmword_ptr(rbp, offset), accReg);
			mNumInstruction++;
		}
		for (int i = 0; i < numParam; ++i)
		{
			mXMMStack.pop_back();
		}

		mXMMStack.emplace_back();
		mXMMStack.back().type = TOKEN_FUNC;
		mXMMStack.back().idxXMM = accRegIdx;

		mPrevValue.type = TOKEN_FUNC;
		mPrevValue.idxXMM = INDEX_NONE;
	}

	void codeFunction(FuncSymbolInfo const& info)
	{
		checkAndLoadPrevValue();
		if (info.id == EFuncSymbol::Sqrt)
		{
			int xmmIdx = mXMMStack.back().idxXMM;
			Asm::sqrtps(xmm(xmmIdx), xmm(xmmIdx));
			mNumInstruction++;
			return;
		}

		void* funcAddr = GetSIMDFuncAddress(info.id);
		if (!funcAddr) throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "SIMD function not supported");

		int numParam = info.numArgs;
		int numLive = (int)mXMMStack.size() - numParam;

		for (int i = 0; i < numParam; ++i)
		{
			if (i >= 4) throw ExprParseException(EExprErrorCode::eGenerateCodeFailed, "SIMD function calls with > 4 parameters not supported in JIT");
			
			int srcPhyIdx = mXMMStack[numLive + i].idxXMM;
			if (srcPhyIdx >= RegAllocator::RegSlotBase)
			{
				int offset = GetTempStackOffsetSIMD(srcPhyIdx - RegAllocator::RegSlotBase);
				Asm::movups(xmm(i), xmmword_ptr(rbp, offset));
			}
			else
			{
				Asm::movups(xmm(i), xmm(srcPhyIdx));
			}
			mNumInstruction++;
		}

		Asm::movabs(rax, (uint64)funcAddr);
		Asm::call(rax);
		mNumInstruction += 2;

		for (int i = 0; i < numParam; ++i) {
			mXMMStack.pop_back();
		}

		int resultIdx = -1;
		if (mCurPushIndex < mAllocPlan.size())
			resultIdx = mAllocPlan[mCurPushIndex];
		mCurPushIndex++;

		if (resultIdx >= RegAllocator::RegSlotBase)
		{
			int offset = GetTempStackOffsetSIMD(resultIdx - RegAllocator::RegSlotBase);
			Asm::movups(xmmword_ptr(rbp, offset), xmm0);
		}
		else
		{
			Asm::movups(xmm(resultIdx), xmm0);
		}
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
						Asm::movups(xmmword_ptr(rbp, inputValue.offset), xmm(srcPhyIdx));
						mNumInstruction++;
					}
				}
				break;
			case VALUE_VARIABLE:
				{
					if (!mXMMStack.empty()) {
						int srcPhyIdx = mXMMStack.back().idxXMM;
						Asm::movabs(rax, (uint64)mPrevValue.var->ptr);
						Asm::movups(xmmword_ptr(rax, 0), xmm(srcPhyIdx));
						mNumInstruction += 2;
					}
				}
				break;
			}
		}
		else
		{
			// Determine destination (left operand)
			int dstXMMIdx = INDEX_NONE;
			if (IsValue(mPrevValue.type) && mPrevValue.idxXMM == INDEX_NONE) {
				dstXMMIdx = mXMMStack.empty() ? mReservedInputCount : mXMMStack.back().idxXMM;
			}
			else {
				// Right operand is on stack (or cached reg), so Dst is below it.
				// If Right is on Stack, Dst is Back-1.
				// If Right is Cached Reg (idxXMM != NONE), Dst is Back.
				if (mPrevValue.idxXMM != INDEX_NONE)
					dstXMMIdx = mXMMStack.back().idxXMM;
				else
					dstXMMIdx = (mXMMStack.size() >= 2) ? mXMMStack[mXMMStack.size() - 2].idxXMM : mReservedInputCount;
			}
			RegXMM dst = xmm(dstXMMIdx);

			auto emitSIMDBOP = [&](auto const& src) {
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
				int srcPhyIdx = mPrevValue.idxXMM;
				if (srcPhyIdx >= RegAllocator::RegSlotBase)
				{
					int offset = GetTempStackOffsetSIMD(srcPhyIdx - RegAllocator::RegSlotBase);
					emitSIMDBOP(xmmword_ptr(rbp, offset));
				}
				else
				{
					emitSIMDBOP(xmm(srcPhyIdx));
				}
			}
			else if (IsValue(mPrevValue.type))
			{
				emitLoadValue(mPrevValue, xmm1);
				emitSIMDBOP(xmm1);
			}
			else
			{
				// Both are results on stack
				int srcPhyIdx = mXMMStack.back().idxXMM;
				if (srcPhyIdx >= RegAllocator::RegSlotBase)
				{
					int offset = GetTempStackOffsetSIMD(srcPhyIdx - RegAllocator::RegSlotBase);
					emitSIMDBOP(xmmword_ptr(rbp, offset));
				}
				else
				{
					emitSIMDBOP(xmm(srcPhyIdx));
				}

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
			RegXMM reg = xmm(xmmIdx);

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
		checkAndLoadPrevValue();
		
		if (!mXMMStack.empty())
		{
			int resultXMMIdx = mXMMStack.back().idxXMM;
			Asm::movups(xmm0, xmm(resultXMMIdx));
			// Note: result must be in xmm0. stackXMM(i) assumption is broken if we use RegAllocator indices.
			// mXMMStack now stores PHYSICAL indices.
			mNumInstruction++;
		}

		// Standard Epilogue: Restore used non-volatile XMM registers (xmm6+)
		for (int i = 0; i < MaxXMMStackSize; ++i)
		{
			int reg = 6 + i;
			if (mRegAllocator.usedMask & (1 << reg))
			{
				Asm::movups(xmm(reg), xmmword_ptr(rbp, -16 - (i * 16)));
				mNumInstruction++;
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
	int emitLoadValue(ValueInfo const& info, RegXMM const& dst) override
	{
		if (info.idxXMM != INDEX_NONE)
		{
			if (xmm(info.idxXMM).index() != dst.index())
			{
				Asm::movups(dst, xmm(info.idxXMM));
				return 1;
			}
			return 0;
		}

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
};

#endif // TARGET_PLATFORM_64BITS
#endif
