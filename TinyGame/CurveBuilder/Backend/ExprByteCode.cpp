#include "ExprByteCode.h"

#include "Misc/DuffDevice.h"
#include "Meta/IndexList.h"


constexpr int GetOpCmdSizeConstexpr(EExprByteCode::Type op)
{
	switch (op)
	{
	case EExprByteCode::Input:
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::Const:
	case EExprByteCode::Variable:
#endif
	case EExprByteCode::Temp:
	case EExprByteCode::Store:
#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2
	case EExprByteCode::IAdd:
	case EExprByteCode::ISub:
	case EExprByteCode::ISubR:
	case EExprByteCode::IMul:
	case EExprByteCode::IDiv:
	case EExprByteCode::IDivR:

#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CAdd:
	case EExprByteCode::CSub:
	case EExprByteCode::CSubR:
	case EExprByteCode::CMul:
	case EExprByteCode::CDiv:
	case EExprByteCode::CDivR:
	case EExprByteCode::VAdd:
	case EExprByteCode::VSub:
	case EExprByteCode::VSubR:
	case EExprByteCode::VMul:
	case EExprByteCode::VDiv:
	case EExprByteCode::VDivR:
#endif

#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2

#if EBC_USE_VALUE_BUFFER
	case EExprByteCode::ISMul:
	case EExprByteCode::ISMulAdd:
	case EExprByteCode::ISMulSub:
	case EExprByteCode::ISMulMul:
	case EExprByteCode::ISMulDiv:
		return 2;

#if EBC_MAX_OP_CMD_SZIE >= 3
	case EExprByteCode::IIAdd:
	case EExprByteCode::IISub:
	case EExprByteCode::IIMul:
	case EExprByteCode::IIDiv:
		return 3;
#endif
#endif


	case EExprByteCode::IMulAdd:
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CMulAdd:
	case EExprByteCode::VMulAdd:
#endif
	case EExprByteCode::IMulSub:
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CMulSub:
	case EExprByteCode::VMulSub:
#endif
	case EExprByteCode::IAddMul:
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CAddMul:
	case EExprByteCode::VAddMul:
#endif

#endif

#endif
	case EExprByteCode::FuncCall0:
	case EExprByteCode::FuncCall1:
	case EExprByteCode::FuncCall2:
	case EExprByteCode::FuncCall3:
	case EExprByteCode::FuncCall4:
	case EExprByteCode::FuncCall5:
		return 2;
	}

	return 1;
}



static char const* GetOpString(uint8 opCode)
{
	switch (opCode)
	{
	case EExprByteCode::Input: return "I";
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::Const: return "C";
	case EExprByteCode::Variable:  return "V";
	case EExprByteCode::Temp: return "Temp";
#endif

	case EExprByteCode::Store: return "Store";

	case EExprByteCode::Add: return "+";
	case EExprByteCode::Sub: return "-";
	case EExprByteCode::SubR: return "-r";
	case EExprByteCode::Mul: return "*";
	case EExprByteCode::Div: return "/";
	case EExprByteCode::DivR: return "/r";
	case EExprByteCode::Mins: return "Mins";
#if EBC_USE_COMPOSITIVE_CODE_LEVEL

	case EExprByteCode::IAdd: return "I+";
	case EExprByteCode::ISub: return "I-";
	case EExprByteCode::ISubR:return "I-r";
	case EExprByteCode::IMul: return "I*";
	case EExprByteCode::IDiv: return "I/";
	case EExprByteCode::IDivR:return "I/r";

#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CAdd: return "C+";
	case EExprByteCode::CSub: return "C-";
	case EExprByteCode::CSubR:return "C-r";
	case EExprByteCode::CMul: return "C*";
	case EExprByteCode::CDiv: return "C/";
	case EExprByteCode::CDivR:return "C/r";
	case EExprByteCode::VAdd: return "V+";
	case EExprByteCode::VSub:return "V-";
	case EExprByteCode::VSubR:return "V-r";
	case EExprByteCode::VMul:return "V*";
	case EExprByteCode::VDiv:return "V/";
	case EExprByteCode::VDivR:return "V/r";
#endif

	case EExprByteCode::SMul:return "S*";

#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2
	case EExprByteCode::SMulAdd:return "S*+";
	case EExprByteCode::SMulSub:return "S*-";
	case EExprByteCode::SMulMul:return "S**";
	case EExprByteCode::SMulDiv:return "S*/";

#if EBC_USE_VALUE_BUFFER
	case EExprByteCode::ISMul:return "IS*";
	case EExprByteCode::ISMulAdd:return "IS*+";
	case EExprByteCode::ISMulSub:return "IS*-";
	case EExprByteCode::ISMulMul:return "IS**";
	case EExprByteCode::ISMulDiv:return "IS*/";
#if EBC_MAX_OP_CMD_SZIE >= 3
	case EExprByteCode::IIAdd:return "II+";
	case EExprByteCode::IISub:return "II-";
	case EExprByteCode::IIMul:return "II*";
	case EExprByteCode::IIDiv:return "II/";
#endif

#endif
	case EExprByteCode::AddMul: return "+*";
	case EExprByteCode::IAddMul: return "I+*";
	case EExprByteCode::MulAdd: return "*+";
	case EExprByteCode::IMulAdd: return "I*+";
	case EExprByteCode::MulSub: return "*-";
	case EExprByteCode::IMulSub: return "I*-";

#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CAddMul: return "C+*";
	case EExprByteCode::CMulAdd: return "C*+";
	case EExprByteCode::CMulSub: return "C*-";
	case EExprByteCode::VAddMul: return "V+*";
	case EExprByteCode::VMulAdd: return "V*+";
	case EExprByteCode::VMulSub: return "V*-";
#endif

#endif

#endif
	case EExprByteCode::FuncSymbol + EFuncSymbol::Exp:  return "exp";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Ln:   return "ln";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Sin:  return "sin";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Cos:  return "cos";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Tan:  return "tan";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Cot:  return "cot";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Sec:  return "sec";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Csc:  return "csc";
	case EExprByteCode::FuncSymbol + EFuncSymbol::Sqrt: return "sqrt";

	case EExprByteCode::FuncCall0: return "call0";
	case EExprByteCode::FuncCall1: return "call1";
	case EExprByteCode::FuncCall2: return "call2";
	case EExprByteCode::FuncCall3: return "call3";
	case EExprByteCode::FuncCall4: return "call4";
	case EExprByteCode::FuncCall5: return "call5";
	}

	return "?";
}

int GetOpCmdSize(EExprByteCode::Type op)
{
	struct Result
	{
		constexpr Result()
			:values{ 1 }
		{
			for (uint32 i = 0; i < EExprByteCode::COUNT; ++i)
			{
				values[i] = GetOpCmdSizeConstexpr(EExprByteCode::Type(i));
			}
		}

		int  values[EExprByteCode::COUNT];
	};
	static constexpr Result StaticResult;
	return StaticResult.values[op];
}

int GetNextOpCmdOffset(EExprByteCode::Type op)
{
#if EBC_USE_FIXED_OP_CMD_SIZE
	return EBC_USE_FIXED_OP_CMD_SIZE;
#else
	return GetOpCmdSize(op);
#endif
}

#if EBC_USE_COMPOSITIVE_CODE_LEVEL

constexpr EExprByteCode::Type tryMergeOpConstexpr(EExprByteCode::Type leftOp, EExprByteCode::Type rightOp)
{

#define MERGE_OP(L, R , M)\
		if (leftOp == EExprByteCode::L && rightOp == EExprByteCode::R ) return EExprByteCode::M;

#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2

#if EBC_USE_VALUE_BUFFER
	MERGE_OP(Input, Add, IAdd)
	MERGE_OP(Input, Sub, ISub)
	MERGE_OP(Input, SubR, ISubR)
	MERGE_OP(Input, Mul, IMul)
	MERGE_OP(Input, Div, IDiv)
	MERGE_OP(Input, DivR, IDivR)
	MERGE_OP(Input, SMul, ISMul)
	MERGE_OP(ISMul, Add, ISMulAdd)
	MERGE_OP(ISMul, Sub, ISMulSub)
	MERGE_OP(ISMul, Mul, ISMulMul)
	MERGE_OP(ISMul, Div, ISMulDiv)

#if EBC_MAX_OP_CMD_SZIE >= 3
	MERGE_OP(Input, IAdd, IIAdd)
	MERGE_OP(Input, ISub, IISub)
	MERGE_OP(Input, IMul, IIMul)
	MERGE_OP(Input, IDiv, IIDiv)
#endif
#endif
	MERGE_OP(Add, Mul, AddMul)
	MERGE_OP(Mul, Add,  MulAdd)
	MERGE_OP(Mul, Sub,  MulSub)
	MERGE_OP(IMul, Add, IMulAdd)
	MERGE_OP(IMul, Sub, IMulSub)
	MERGE_OP(IAdd, Mul, IAddMul)
	MERGE_OP(SMul, Add, SMulAdd)
	MERGE_OP(SMul, Sub, SMulSub)
	MERGE_OP(SMul, Mul, SMulMul)
	MERGE_OP(SMul, Div, SMulDiv)
#endif
#if !EBC_USE_VALUE_BUFFER
	MERGE_OP(CMul, Add, CMulAdd)
	MERGE_OP(CMul, Sub, CMulSub)
	MERGE_OP(CAdd, Mul, CAddMul)
	MERGE_OP(VMul, Add, VMulAdd)
	MERGE_OP(VMul, Sub, VMulSub)
	MERGE_OP(VAdd, Mul, VAddMul)
#endif

#undef MERGE_OP

	return EExprByteCode::None;
}


template< typename T >
constexpr void InversePairingFunction(T z, T& outA, T& outB)
{
	T w = std::floor(std::sqrt(8 * z + 1) - 1 / 2);
	T t = w * (w + 1) / 2;
	outB = z - t;
	outA = w - outB;
}

constexpr EExprByteCode::Type tryMergeOpConstexpr(uint32 value)
{
	uint32 a = value % EExprByteCode::COUNT;
	uint32 b = value / EExprByteCode::COUNT;
	//InversePairingFunction(pairValue, a, b);
	return tryMergeOpConstexpr(EExprByteCode::Type(a), EExprByteCode::Type(b));
}

EExprByteCode::Type tryMergeOp(EExprByteCode::Type leftOp, EExprByteCode::Type rightOp)
{
	struct Result
	{
		constexpr Result()
			:values{ EExprByteCode::None }
		{
			for (uint32 i = 0; i < EExprByteCode::COUNT * EExprByteCode::COUNT; ++i)
			{
				values[i] = tryMergeOpConstexpr(i);
			}
		}

		EExprByteCode::Type  values[EExprByteCode::COUNT * EExprByteCode::COUNT];
	};
	static constexpr Result StaticResult;
	return StaticResult.values[ leftOp + EExprByteCode::COUNT * rightOp];
}


int tryMergeOpCmd(uint8* pCodeL, uint8* pCodeR, uint8* pMergeCode)
{
	auto opMerged = tryMergeOp(EExprByteCode::Type(*pCodeL), EExprByteCode::Type(*pCodeR));
	if (opMerged == EExprByteCode::None)
		return 0;

	pMergeCode[0] = opMerged;
	switch (opMerged)
	{
#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2
	case EExprByteCode::AddMul:
	case EExprByteCode::MulAdd:
	case EExprByteCode::MulSub:  
		return 1;
	case EExprByteCode::IMulAdd: 
	case EExprByteCode::IMulSub: 
	case EExprByteCode::IAddMul:
		pMergeCode[1] = pCodeL[1]; 
		return 2;
	case EExprByteCode::SMulAdd:
	case EExprByteCode::SMulSub:
	case EExprByteCode::SMulMul:
	case EExprByteCode::SMulDiv:
		return 1;
#if EBC_USE_VALUE_BUFFER
	case EExprByteCode::IAdd:
	case EExprByteCode::ISub:
	case EExprByteCode::ISubR:
	case EExprByteCode::IMul:
	case EExprByteCode::IDiv:
	case EExprByteCode::IDivR:
	case EExprByteCode::ISMul:
	case EExprByteCode::ISMulAdd:
	case EExprByteCode::ISMulSub:
	case EExprByteCode::ISMulMul:
	case EExprByteCode::ISMulDiv:
		pMergeCode[1] = pCodeL[1];
		return 2;

#if EBC_MAX_OP_CMD_SZIE >= 3
	case EExprByteCode::IIAdd:
	case EExprByteCode::IISub:
	case EExprByteCode::IIMul:
	case EExprByteCode::IIDiv:
		pMergeCode[1] = pCodeL[1];
		pMergeCode[2] = pCodeR[1];
		return 3;
#endif

#endif

#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CMulAdd:
	case EExprByteCode::CMulSub:
	case EExprByteCode::VMulAdd:
	case EExprByteCode::VMulSub:
	case EExprByteCode::CAddMul:
	case EExprByteCode::VAddMul:
		pMergeCode[1] = pCodeL[1];
		return 2;
#endif

#endif
	default:
		break;
	}

}

uint8* GetPrevOpCmd(uint8* pCode, uint8* pCodeStart)
{
	CHECK(pCode > pCodeStart);
#if EBC_USE_FIXED_OP_CMD_SIZE
	return pCode - EBC_USE_FIXED_OP_CMD_SIZE;
#else
	int len = pCode - pCodeStart;
	uint8* pCodeCur = pCodeStart;
	do
	{
		int num = GetOpCmdSize(EExprByteCode::Type(*pCodeCur));
		len -= num;
		if (len == 0)
			break;

		pCodeCur += num;
	}
	while (len > 0);
	return pCodeCur;
#endif
}

int ComposteCodes(TArray<uint8>& inoutCodes)
{
	int numCmd = 0;

	uint8* pCodeStart = inoutCodes.data();
	uint8* pCodeEnd = pCodeStart + inoutCodes.size();
	uint8* pCodeL = pCodeStart;
	int nextCodeOffset = GetNextOpCmdOffset(EExprByteCode::Type(*pCodeL));
	uint8* pCodeR = pCodeStart + nextCodeOffset;

	uint8* pCodeWrite = pCodeStart;
	auto WriteCode = [&](uint8* pCode, int num)
	{
		CHECK(num > 0);
		FMemory::Move(pCodeWrite, pCode, num);
		pCodeWrite += num;
	};


	while (pCodeR < pCodeEnd)
	{
		uint8 codeMerged[EBC_MAX_OP_CMD_SZIE];
		int num = tryMergeOpCmd(pCodeL, pCodeR, codeMerged);

		if (num)
		{
			while (pCodeWrite != pCodeStart)
			{
				uint8* pCodePrev = GetPrevOpCmd(pCodeWrite, pCodeStart);
				uint8 codeMergedPrev[EBC_MAX_OP_CMD_SZIE];
				int numPrev = tryMergeOpCmd(pCodePrev, codeMerged, codeMergedPrev);
				if (numPrev == 0)
					break;

				std::copy_n(codeMergedPrev, numPrev, codeMerged);
				pCodeWrite = pCodePrev;
				num = numPrev;
			}

#if EBC_USE_FIXED_OP_CMD_SIZE
			for (int i = num; i < EBC_USE_FIXED_OP_CMD_SIZE; ++i)
				codeMerged[i] = 0;
			num = EBC_USE_FIXED_OP_CMD_SIZE;
#endif

			pCodeL = pCodeWrite;
			WriteCode(codeMerged, num);
			pCodeWrite -= num;

			nextCodeOffset = num;
			pCodeR += GetNextOpCmdOffset(EExprByteCode::Type(*pCodeR));
		}
		else
		{
			++numCmd;

			if (pCodeWrite != pCodeL)
			{
				WriteCode(pCodeL, nextCodeOffset);
			}
			else
			{
				pCodeWrite += nextCodeOffset;
			}

			pCodeL = pCodeR;
			nextCodeOffset = GetNextOpCmdOffset(EExprByteCode::Type(*pCodeL));
			pCodeR += nextCodeOffset;
		}
	}

	if (pCodeWrite != pCodeL + nextCodeOffset)
	{
		++numCmd;
		WriteCode(pCodeL, nextCodeOffset);
	}

	if (pCodeWrite != pCodeEnd)
	{
		inoutCodes.resize(pCodeWrite - inoutCodes.data());
	}

	return numCmd;
}

#endif


void ExprByteCodeCompiler::codeInit(int numInput, ValueLayout inputLayouts[])
{
	mStacks.clear();

	mNumCmd = 0;
	mMaxTempIndex = -1;
#if EBC_USE_VALUE_BUFFER
	mOutput.numInput = numInput;
#endif
}


void ExprByteCodeCompiler::codeEnd()
{
	//LogMsg("Num Cmd = %d", mNumCmd);

#if EBC_USE_VALUE_BUFFER
	mOutput.numTemp = mMaxTempIndex + 1;

	int tempOffset = mOutput.numInput + mOutput.constValues.size() + mOutput.vars.size();
	// Patch Temp indices and convert Temp -> Input for optimization
	int pos = 0;
	int len = mOutput.codes.size();
	uint8* pCodes = mOutput.codes.data();

	while (pos < len)
	{
		EExprByteCode::Type op = (EExprByteCode::Type)pCodes[pos];
		if (op == EExprByteCode::Temp)
		{
			pCodes[pos] = EExprByteCode::Input; // Convert to Input
			pCodes[pos + 1] += tempOffset; // Shift index
		}
		else if (op == EExprByteCode::Store)
		{
			pCodes[pos + 1] += tempOffset; // Shift index
		}
		// Advance
		pos += GetNextOpCmdOffset(op);
	}
#endif

#if EBC_USE_COMPOSITIVE_CODE_LEVEL
	mNumCmd = ComposteCodes(mOutput.codes);
#endif

	//LogMsg("Num Cmd = %d", mNumCmd);

#if 1
	std::string debugMeg;
	for (int i = 0; i < mOutput.codes.size(); i += GetNextOpCmdOffset(EExprByteCode::Type(mOutput.codes[i])))
	{
		debugMeg += "[";
		debugMeg += GetOpString(mOutput.codes[i]);
		debugMeg += "]";
	}
	LogMsg("Cmd = %s", debugMeg.c_str());
#endif

}

void ExprByteCodeCompiler::codeConstValue(ConstValueInfo const& value)
{
	CHECK(value.layout == ValueLayout::Real);
	int index = mOutput.constValues.findIndex(value.asReal);

#if EBC_USE_VALUE_BUFFER
	CHECK(index != INDEX_NONE);
	index += mOutput.numInput;
	CHECK(index < 255);
	pushValue(EExprByteCode::Input, index);
#else
	if (index == INDEX_NONE)
	{
		index = mOutput.constValues.size();
		mOutput.constValues.push_back(value.asReal);
	}
	pushValue(EExprByteCode::Const, index);
#endif
}

void ExprByteCodeCompiler::codeVar(VariableInfo const& varInfo)
{
	CHECK(varInfo.layout == ValueLayout::Real);
#if EBC_USE_VALUE_BUFFER
	int index = mOutput.vars.findIndex((RealType*)varInfo.ptr);
	if (index == INDEX_NONE)
	{
		index = mOutput.vars.size();
		mOutput.vars.push_back((RealType*)varInfo.ptr);
	}
	index += mOutput.numInput + mOutput.constValues.size();
	CHECK(index < 255);
	pushValue(EExprByteCode::Input, index);
#else
	int index = mOutput.pointers.findIndex(varInfo.ptr);
	if (index == INDEX_NONE)
	{
		index = mOutput.pointers.size();
		mOutput.pointers.push_back(varInfo.ptr);
	}

	CHECK(index < 255);
	pushValue(EExprByteCode::Variable, index);
#endif
}

void ExprByteCodeCompiler::codeInput(InputInfo const& input)
{
	pushValue(EExprByteCode::Input, input.index);
}

void ExprByteCodeCompiler::codeFunction(FuncInfo const& info)
{
	if (info.getArgNum() > EBC_MAX_FUNC_ARG_NUM)
	{
		LogError("Func Arg greater than EBC_MAX_FUNC_ARG_NUM. please change value");
	}
	int index = mOutput.pointers.findIndex(info.funcPtr);
	if (index == INDEX_NONE)
	{
		index = mOutput.pointers.size();
		mOutput.pointers.push_back(info.funcPtr);
	}
	CHECK(index < 255);

#if EBC_USE_LAZY_STACK_PUSH
	if (info.getArgNum() > 0)
	{
		auto argValue = mStacks.back();
		if (argValue.byteCode != EExprByteCode::None)
		{
			outputCmd(argValue.byteCode, argValue.index);
		}
		mStacks.pop_back();
		outputCmd(EExprByteCode::Type(EExprByteCode::FuncCall0 + info.getArgNum()), index);
	}
	else
	{
		outputCmd(EExprByteCode::FuncCall0, index);
	}

	pushResultValue();
#else
	outputCmd(EExprByteCode::Type(EExprByteCode::FuncCall0 + info.getArgNum()), index);
#endif
}

void ExprByteCodeCompiler::codeFunction(FuncSymbolInfo const& info)
{
#if EBC_USE_LAZY_STACK_PUSH
	auto leftValue = mStacks.back();
	if (leftValue.byteCode != EExprByteCode::None)
	{
		outputCmd(leftValue.byteCode, leftValue.index);
	}

	mStacks.pop_back();
#endif
	outputCmd(EExprByteCode::Type(EExprByteCode::FuncSymbol + info.id));

#if EBC_USE_LAZY_STACK_PUSH
	pushResultValue();
#endif
}

void ExprByteCodeCompiler::codeBinaryOp(TokenType type, bool isReverse)
{
#if EBC_USE_LAZY_STACK_PUSH
	auto rightValue = mStacks.back();
	auto leftValue = mStacks[mStacks.size() - 2];
	if (leftValue.byteCode != EExprByteCode::None)
	{
		outputCmd(leftValue.byteCode, leftValue.index);
	}
#endif

#if EBC_USE_COMPOSITIVE_CODE_LEVEL
	EExprByteCode::Type opCode;
	switch (type)
	{
	case BOP_ADD: opCode = EExprByteCode::Add; break;
	case BOP_SUB: opCode = isReverse ? EExprByteCode::SubR : EExprByteCode::Sub; break;
	case BOP_MUL: opCode = EExprByteCode::Mul; break;
	case BOP_DIV: opCode = isReverse ? EExprByteCode::DivR : EExprByteCode::Div; break;
	}

	if (leftValue.byteCode == rightValue.byteCode && leftValue.index == rightValue.index &&
		(type == BOP_MUL) && (rightValue.byteCode != EExprByteCode::None))
	{
		outputCmd(EExprByteCode::SMul);
	}
	else if (rightValue.byteCode == EExprByteCode::None)
	{
		outputCmd(opCode);
	}
	else
	{
		outputCmd(rightValue.byteCode, rightValue.index);
		outputCmd(opCode);
	}
#else
#if EBC_USE_LAZY_STACK_PUSH
	outputCmd(rightValue.byteCode, rightValue.index);
#endif
	switch (type)
	{
	case BOP_ADD: outputCmd(EExprByteCode::Add); break;
	case BOP_SUB: outputCmd(isReverse ? EExprByteCode::SubR : EExprByteCode::Sub); break;
	case BOP_MUL: outputCmd(EExprByteCode::Mul); break;
	case BOP_DIV: outputCmd(isReverse ? EExprByteCode::DivR : EExprByteCode::Div); break;
	}

#endif

#if EBC_USE_LAZY_STACK_PUSH
	mStacks.pop_back();
	mStacks.pop_back();
	pushResultValue();
#endif
}

void ExprByteCodeCompiler::codeUnaryOp(TokenType type)
{
#if EBC_USE_LAZY_STACK_PUSH
	auto leftValue = mStacks.back();
	if (leftValue.byteCode != EExprByteCode::None)
	{
		outputCmd(leftValue.byteCode, leftValue.index);
	}

	mStacks.pop_back();
#endif
	switch (type)
	{
	case UOP_MINS: outputCmd(EExprByteCode::Mins); break;
	}
#if EBC_USE_LAZY_STACK_PUSH
	pushResultValue();
#endif
}

#define GET_CONST(CODE) (mExecData->constValues[CODE])
#define GET_INPUT(CODE) (mInputs[CODE])
#define GET_VAR(CODE)	(*(RealType*)mExecData->pointers[CODE])

template<typename TValue>
TValue TByteCodeExecutor<TValue>::doExecute(ExprByteCodeExecData const& execData)
{
	mExecData = &execData;

	int index = 0;
	int numCodes = execData.codes.size();
	uint8 const* pCode = execData.codes.data();
	uint8 const* pCodeEnd = pCode + numCodes;
	TValue  stackValues[32];
	TValue* stackValue = stackValues;
#if !EBC_USE_VALUE_BUFFER
	TValue  tempValues[256];
#else
	TValue* tempValues = const_cast<TValue*>(mInputs.data());
#endif
	TValue  topValue = 0.0;
#if 1
	switch (*pCode)
	{
	case EExprByteCode::Input:
		topValue = GET_INPUT(pCode[1]);
		break;
	case EExprByteCode::Temp:
		topValue = tempValues[pCode[1]];
		break;
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::Const:
		topValue = GET_CONST(pCode[1]);
		break;
	case EExprByteCode::Variable:
		topValue = GET_VAR(pCode[1]);
		break;
#endif

#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2
#if EBC_USE_VALUE_BUFFER

#if EBC_MAX_OP_CMD_SZIE >= 3
	case EExprByteCode::IIAdd:
		{
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs + rhs;
		}
		break;
	case EExprByteCode::IISub:
		{
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs - rhs;
		}
		break;
	case EExprByteCode::IIMul:
		{
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs * rhs;
		}
		break;
	case EExprByteCode::IIDiv:
		{
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs / rhs;
		}
		break;
#endif

	case EExprByteCode::ISMul:
		{
			auto rhs = GET_INPUT(pCode[1]);
			topValue = rhs * rhs;
		}
		break;
#endif
#endif

	case EExprByteCode::FuncCall0:
	{
		void* funcPtr = mExecData->pointers[pCode[1]];
		topValue = (*static_cast<FuncType0>(funcPtr))();
	}
	break;
	default:
		return 0.0f;
	}
#if EBC_USE_FIXED_OP_CMD_SIZE
	pCode += EBC_USE_FIXED_OP_CMD_SIZE;
#else
	pCode += GetNextOpCmdOffset(EExprByteCode::Type(*pCode));
#endif
#endif
#define EXECUTE_CODE() execCode(pCode, stackValue, topValue, tempValues)

#if EBC_USE_FIXED_OP_CMD_SIZE
#if 0
	while (pCode < pCodeEnd)
	{
		EXECUTE_CODE();
		pCode += EBC_USE_FIXED_SIZE;
#else

	int numOp = (pCodeEnd - pCode) / EBC_USE_FIXED_OP_CMD_SIZE;
#define EXECUTE_OP  EXECUTE_CODE(); pCode += EBC_USE_FIXED_OP_CMD_SIZE
	DUFF_DEVICE_4(numOp, EXECUTE_OP);
#undef  EXECUTE_OP

#endif
#else
	while (pCode < pCodeEnd)
	{
		pCode += EXECUTE_CODE();
	}
#endif
	return topValue;
}

template< typename TValue, typename RT, typename ...Args>
FORCEINLINE TValue Invoke(RT(*func)(Args...), TValue* args)
{
	return InvokeInternal(func, args, TIndexRange<0, sizeof...(Args)>());
}

template< typename TValue, typename RT, typename ...Args, size_t ...Is>
FORCEINLINE TValue InvokeInternal(RT(*func)(Args...), TValue* args, TIndexList<Is...>)
{
	if constexpr (std::is_same_v<TValue, FloatVector> && !std::is_same_v<TValue, RT>)
	{
		TValue result;
		result[0] = (*func)(args[Is][0]...);
		result[1] = (*func)(args[Is][1]...);
		result[2] = (*func)(args[Is][2]...);
		result[3] = (*func)(args[Is][3]...);
		if constexpr (FloatVector::Size > 4)
		{
			result[4] = (*func)(args[Is][4]...);
			result[5] = (*func)(args[Is][5]...);
			result[6] = (*func)(args[Is][6]...);
			result[7] = (*func)(args[Is][7]...);
		}
		return result;
	}
	else
	{
		return (*func)(args[Is]...);
	}
}

template<typename TValue>
FORCEINLINE int TByteCodeExecutor<TValue>::execCode(uint8 const* __restrict pCode, TValue* __restrict & pValueStack, TValue& topValue, TValue* __restrict tempValues)
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
	case EExprByteCode::Input:
		pushStack(topValue);
		topValue = GET_INPUT(pCode[1]);
		return 2;
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::Const:
		pushStack(topValue);
		topValue = GET_CONST(pCode[1]);
		return 2;
	case EExprByteCode::Variable:
		pushStack(topValue);
		topValue = GET_VAR(pCode[1]);
		return 2;
	case EExprByteCode::Temp:
		pushStack(topValue);
		topValue = tempValues[pCode[1]];
		return 2;
#endif
	case EExprByteCode::Store:
		tempValues[pCode[1]] = topValue;
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

#if EBC_USE_COMPOSITIVE_CODE_LEVEL

	case EExprByteCode::IAdd:
		{
			auto const& rhs = GET_INPUT(pCode[1]);
			topValue = topValue + rhs;
		}
		return 2;
	case EExprByteCode::ISub:
		{
			auto const& rhs = GET_INPUT(pCode[1]);
			topValue = topValue - rhs;
		}
		return 2;
	case EExprByteCode::ISubR:
		{
			auto const& rhs = GET_INPUT(pCode[1]);
			topValue = rhs - topValue;
		}
		return 2;
	case EExprByteCode::IMul:
		{
			auto const& rhs = GET_INPUT(pCode[1]);
			topValue = topValue * rhs;
		}
		return 2;
	case EExprByteCode::IDiv:
		{
			auto const& rhs = GET_INPUT(pCode[1]);
			topValue = topValue / rhs;
		}
		return 2;
	case EExprByteCode::IDivR:
		{
			auto const& rhs = GET_INPUT(pCode[1]);
			topValue = rhs / topValue;
		}
		return 2;

#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CAdd:
		{
			auto rhs = GET_CONST(pCode[1]);
			topValue = topValue + rhs;
		}
		return 2;
	case EExprByteCode::CSub:
		{
			auto rhs = GET_CONST(pCode[1]);
			topValue = topValue - rhs;
		}
		return 2;
	case EExprByteCode::CSubR:
		{
			auto rhs = GET_CONST(pCode[1]);
			topValue = rhs - topValue;
		}
		return 2;
	case EExprByteCode::CMul:
		{
			auto rhs = GET_CONST(pCode[1]);
			topValue = topValue * rhs;
		}
		return 2;
	case EExprByteCode::CDiv:
		{
			auto rhs = GET_CONST(pCode[1]);
			topValue = topValue / rhs;
		}
		return 2;
	case EExprByteCode::CDivR:
		{
			auto rhs = GET_CONST(pCode[1]);
			topValue = rhs / topValue;
		}
		return 2;
	case EExprByteCode::VAdd:
		{
			auto rhs = GET_VAR(pCode[1]);
			topValue = topValue + rhs;
		}
		return 2;
	case EExprByteCode::VSub:
		{
			auto rhs = GET_VAR(pCode[1]);
			topValue = topValue - rhs;
		}
		return 2;
	case EExprByteCode::VSubR:
		{
			auto rhs = GET_VAR(pCode[1]);
			topValue = rhs - topValue;
		}
		return 2;
	case EExprByteCode::VMul:
		{
			auto rhs = GET_VAR(pCode[1]);
			topValue = topValue * rhs;
		}
		return 2;
	case EExprByteCode::VDiv:
		{
			auto rhs = GET_VAR(pCode[1]);
			topValue = topValue / rhs;
		}
		return 2;
	case EExprByteCode::VDivR:
		{
			auto rhs = GET_VAR(pCode[1]);
			topValue = rhs / topValue;
		}
		return 2;
#endif

	case EExprByteCode::SMul:
		{
			topValue = topValue * topValue;
		}
		return 1;
#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2
	case EExprByteCode::SMulAdd:
		{
			auto lhs = popStack();
			topValue = lhs + topValue * topValue;
		}
		return 1;
	case EExprByteCode::SMulSub:
		{
			auto lhs = popStack();
			topValue = lhs - topValue * topValue;
		}
		return 1;
	case EExprByteCode::SMulMul:
		{
			auto lhs = popStack();
			topValue = lhs * (topValue * topValue);
		}
		return 1;
	case EExprByteCode::SMulDiv:
		{
			auto lhs = popStack();
			topValue = lhs / (topValue * topValue);
		}
		return 1;
#if EBC_USE_VALUE_BUFFER
	case EExprByteCode::ISMul:
		{
			pushStack(topValue);
			auto rhs = GET_INPUT(pCode[1]);
			topValue = rhs * rhs;
		}
		return 2;
	case EExprByteCode::ISMulAdd:
		{
			auto rhs = GET_INPUT(pCode[1]);
			topValue = topValue + rhs * rhs;
		}
		return 2;
	case EExprByteCode::ISMulSub:
		{
			auto rhs = GET_INPUT(pCode[1]);
			topValue = topValue - rhs * rhs;
		}
		return 2;
	case EExprByteCode::ISMulMul:
		{
			auto rhs = GET_INPUT(pCode[1]);
			topValue = topValue * (rhs * rhs);
		}
		return 2;
	case EExprByteCode::ISMulDiv:
		{
			auto rhs = GET_INPUT(pCode[1]);
			topValue = topValue / (rhs * rhs);
		}
		return 2;
#endif
	case EExprByteCode::MulAdd:
		{
			auto rhs = popStack();
			auto lhs = popStack();
			topValue = lhs + topValue * rhs;
		}
		return 1;
	case EExprByteCode::IMulAdd:
		{
			auto rhs = GET_INPUT(pCode[1]);
			auto lhs = popStack();
			topValue = lhs + topValue * rhs;
		}
		return 2;
#if EBC_MAX_OP_CMD_SZIE >= 3
	case EExprByteCode::IIAdd:
		{
			pushStack(topValue);
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs + rhs;
		}
		return 3;
	case EExprByteCode::IISub:
		{
			pushStack(topValue);
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs - rhs;
		}
		return 3;
	case EExprByteCode::IIMul:
		{
			pushStack(topValue);
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs * rhs;
		}
		return 3;
	case EExprByteCode::IIDiv:
		{
			pushStack(topValue);
			auto lhs = GET_INPUT(pCode[1]);
			auto rhs = GET_INPUT(pCode[2]);
			topValue = lhs / rhs;
		}
		return 3;
#endif

#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CMulAdd:
		{
			auto rhs = GET_CONST(pCode[1]);
			auto lhs = popStack();
			topValue = lhs + topValue * rhs;
		}
		return 2;
	case EExprByteCode::VMulAdd:
		{
			auto rhs = GET_VAR(pCode[1]);
			auto lhs = popStack();
			topValue = lhs + topValue * rhs;
		}
		return 2;
#endif
	case EExprByteCode::MulSub:
		{
			auto rhs = popStack();
			auto lhs = popStack();
			topValue = lhs - topValue * rhs;
		}
		return 1;
	case EExprByteCode::IMulSub:
		{
			auto rhs = GET_INPUT(pCode[1]);
			auto lhs = popStack();
			topValue = lhs - topValue * rhs;
		}
		return 2;
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CMulSub:
		{
			auto rhs = GET_CONST(pCode[1]);
			auto lhs = popStack();
			topValue = lhs - topValue * rhs;
		}
		return 2;
	case EExprByteCode::VMulSub:
		{
			auto rhs = GET_VAR(pCode[1]);
			auto lhs = popStack();
			topValue = lhs - topValue * rhs;
		}
		return 2;
#endif
	case EExprByteCode::AddMul:
		{
			auto rhs = popStack();
			auto lhs = popStack();
			topValue = lhs * (topValue + rhs);
		}
		return 1;
	case EExprByteCode::IAddMul:
		{
			auto rhs = GET_INPUT(pCode[1]);
			auto lhs = popStack();
			topValue = lhs * (topValue + rhs);
		}
		return 2;
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CAddMul:
		{
			auto rhs = GET_CONST(pCode[1]);
			auto lhs = popStack();
			topValue = lhs * (topValue + rhs);
		}
		return 2;
	case EExprByteCode::VAddMul:
		{
			auto rhs = GET_VAR(pCode[1]);
			auto lhs = popStack();
			topValue = lhs * (topValue + rhs);
		}
		return 2;
#endif

#endif

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

#if EBC_MAX_FUNC_ARG_NUM >= 1 
	case EExprByteCode::FuncCall0:
		{
			pushStack(topValue);
			void* funcPtr = mExecData->pointers[pCode[1]];
			topValue = (*static_cast<FuncType0>(funcPtr))();
		}
		return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 1 
	case EExprByteCode::FuncCall1:
		{
			void* funcPtr = mExecData->pointers[pCode[1]];
			*pValueStack = topValue;
			topValue = Invoke(static_cast<FuncType1>(funcPtr), pValueStack);
		}
		return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 2
	case EExprByteCode::FuncCall2:
		{
			void* funcPtr = mExecData->pointers[pCode[1]];

			*pValueStack = topValue;
			topValue = Invoke(static_cast<FuncType2>(funcPtr), pValueStack);
			pValueStack -= 1;
		}
		return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 3
	case EExprByteCode::FuncCall3:
		{
			void* funcPtr = mExecData->pointers[pCode[1]];
			*pValueStack = topValue;
			topValue = Invoke(static_cast<FuncType3>(funcPtr), pValueStack);
			pValueStack -= 2;
		}
		return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 4
	case EExprByteCode::FuncCall4:
		{
			void* funcPtr = mExecData->pointers[pCode[1]];
			*pValueStack = topValue;
			topValue = Invoke(static_cast<FuncType4>(funcPtr), pValueStack);
			pValueStack -= 3;
		}
		return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 5
	case EExprByteCode::FuncCall5:
		{
			void* funcPtr = mExecData->pointers[pCode[1]];
			*pValueStack = topValue;
			topValue = Invoke(static_cast<FuncType5>(funcPtr), pValueStack);
			pValueStack -= 4;
		}
		return 2;
#endif
		}
	return 1;
}

template RealType TByteCodeExecutor<RealType>::doExecute(ExprByteCodeExecData const& code);
template FloatVector TByteCodeExecutor<FloatVector>::doExecute(ExprByteCodeExecData const& code);
template double TByteCodeExecutor<double>::doExecute(ExprByteCodeExecData const& code);

