#include "ExprByteCode.h"

#include "Meta/IndexList.h"
#include "Misc/DuffDevice.h"


constexpr int GetOpCmdSizeConstexpr(EExprByteCode::Type op)
{
	switch (op)
	{
	case EExprByteCode::Input:
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::Const:
	case EExprByteCode::Variable:
#endif
#if EBC_USE_COMPOSITIVE_CODE
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

#if EBC_USE_COMPOSITIVE_OP_CODE

#if EBC_USE_VALUE_BUFFER
	case EExprByteCode::CSMulAdd:
	case EExprByteCode::CSMulSub:
	case EExprByteCode::CSMulMul:
	case EExprByteCode::CSMulDiv:
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

template< size_t ...Is >
TArrayView< int const > MakeOpCmdSizeMap(TIndexList<Is...>)
{
	static constexpr int Result[] = { GetOpCmdSizeConstexpr(EExprByteCode::Type(Is))... };
	return TArrayView< int const >(Result);
}


int GetOpCmdSize(EExprByteCode::Type op)
{
	static TArrayView< int const > StaticSizeMap = MakeOpCmdSizeMap(TIndexRange<0, EExprByteCode::COUNT - 1>());
	return StaticSizeMap[op];
}

int GetNextOpCmdOffset(EExprByteCode::Type op)
{
#if EBC_USE_FIXED_SIZE
	return EBC_USE_FIXED_SIZE;
#else
	return GetOpCmdSize(op);
#endif
}

#if EBC_USE_COMPOSITIVE_OP_CODE

EExprByteCode::Type tryMergeOp(EExprByteCode::Type leftOp, EExprByteCode::Type rightOp)
{
	switch (leftOp)
	{
	case EExprByteCode::Add:
		switch (rightOp)
		{
		case EExprByteCode::Mul:  return EExprByteCode::AddMul;
		}
		break;
	case EExprByteCode::Sub:
		break;
	case EExprByteCode::Mul:
		switch (rightOp)
		{
		case EExprByteCode::Add:  return EExprByteCode::MulAdd;
		case EExprByteCode::Sub:  return EExprByteCode::MulSub;
		}
		break;
	case EExprByteCode::Div:
		break;
	case EExprByteCode::IMul:
		switch (rightOp)
		{
		case EExprByteCode::Add:  return EExprByteCode::IMulAdd;
		case EExprByteCode::Sub:  return EExprByteCode::IMulSub;
		}
		break;
	case EExprByteCode::IAdd:
		switch (rightOp)
		{
		case EExprByteCode::Mul:  return EExprByteCode::IAddMul;
		}
		break;
	case EExprByteCode::SMul:
		switch (rightOp)
		{
		case EExprByteCode::Add:  return EExprByteCode::SMulAdd;
		case EExprByteCode::Sub:  return EExprByteCode::SMulSub;
		case EExprByteCode::Mul:  return EExprByteCode::SMulMul;
		case EExprByteCode::Div:  return EExprByteCode::SMulDiv;
		}
		break;
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::CMul:
		switch (rightOp)
		{
		case EExprByteCode::Add:  return EExprByteCode::CMulAdd;
		case EExprByteCode::Sub:  return EExprByteCode::CMulSub;
		}
		break;
	case EExprByteCode::VMul:
		switch (rightOp)
		{
		case EExprByteCode::Add:  return EExprByteCode::VMulAdd;
		case EExprByteCode::Sub:  return EExprByteCode::VMulSub;
		}
		break;
	case EExprByteCode::CAdd:
		switch (rightOp)
		{
		case EExprByteCode::Mul:  return EExprByteCode::CAddMul;
		}
		break;
	case EExprByteCode::VAdd:
		switch (rightOp)
		{
		case EExprByteCode::Mul:  return EExprByteCode::VAddMul;
		}
		break;
#endif
	default:
		break;
	}

	return EExprByteCode::None;
}


int tryMergeOpCmd(uint8* pCodeL, uint8* pCodeR, uint8* pMergeCode)
{
	auto opMerged = tryMergeOp(EExprByteCode::Type(*pCodeL), EExprByteCode::Type(*pCodeR));
	if (opMerged == EExprByteCode::None)
		return 0;

	pMergeCode[0] = opMerged;
	switch (opMerged)
	{
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
	default:
		break;
	}

}
#endif


void ExprByteCodeCompiler::codeInit(int numInput, ValueLayout inputLayouts[])
{
	mStacks.clear();

	mNumCmd = 0;
#if EBC_USE_VALUE_BUFFER
	mNumInput = numInput;
#endif
}


void ExprByteCodeCompiler::codeEnd()
{
	LogMsg("Num Cmd = %d", mNumCmd);

#if EBC_USE_COMPOSITIVE_OP_CODE

	uint8* pCodeWrite = mOutput.codes.data();
	uint8* pCodeEnd = mOutput.codes.data() + mOutput.codes.size();

	uint8* pCodeL = mOutput.codes.data();
	int nextCodeOffset = GetNextOpCmdOffset(EExprByteCode::Type(*pCodeL));
	uint8* pCodeR = mOutput.codes.data() + nextCodeOffset;


	auto WriteCode = [&](uint8* pCode, int num)
	{
		CHECK(num > 0);
		do { *(pCodeWrite++) = *(pCode++); --num; } while (num);
	};

	while( pCodeR < pCodeEnd)
	{
		uint8 codeMerged[8];
		int num = tryMergeOpCmd(pCodeL, pCodeR, codeMerged);

		if (num)
		{
#if EBC_USE_FIXED_SIZE
			for (int i = num; i < EBC_USE_FIXED_SIZE; ++i)
				codeMerged[i] = 0;
			num = EBC_USE_FIXED_SIZE;
#endif
			pCodeL = pCodeWrite;
			WriteCode(codeMerged, num);
			pCodeWrite -= num;

			nextCodeOffset = num;
			pCodeR += GetNextOpCmdOffset(EExprByteCode::Type(*pCodeR));
		}
		else
		{
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

	if (pCodeWrite != pCodeL)
	{
		WriteCode(pCodeL, nextCodeOffset);
	}
	else
	{
		pCodeWrite += nextCodeOffset;
	}

	if ( pCodeWrite != pCodeEnd)
	{
		mOutput.codes.resize(pCodeWrite - mOutput.codes.data());
	}
#endif


#if EBC_USE_FIXED_SIZE
	std::string debugMeg;
	for (int i = 0; i < mOutput.codes.size(); i += EBC_USE_FIXED_SIZE)
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
	index += mNumInput;
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
	index += mNumInput + mOutput.constValues.size();
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

#if EBC_USE_COMPOSITIVE_CODE
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
#if EBC_USE_COMPOSITIVE_CODE
	auto leftValue = mStacks.back();
	if (leftValue.byteCode != EExprByteCode::None)
	{
		outputCmd(leftValue.byteCode, leftValue.index);
	}

	mStacks.pop_back();
#endif
	outputCmd(EExprByteCode::Type(EExprByteCode::FuncSymbol + info.id));

#if EBC_USE_COMPOSITIVE_CODE
	pushResultValue();
#endif
}

void ExprByteCodeCompiler::codeBinaryOp(TokenType type, bool isReverse)
{
#if EBC_USE_COMPOSITIVE_CODE
	auto rightValue = mStacks.back();
	auto leftValue = mStacks[mStacks.size() - 2];
	if (leftValue.byteCode != EExprByteCode::None)
	{
		outputCmd(leftValue.byteCode, leftValue.index);
	}

	EExprByteCode::Type opCode;
	switch (type)
	{
	case BOP_ADD: opCode = EExprByteCode::Add; break;
	case BOP_SUB: opCode = isReverse ? EExprByteCode::SubR : EExprByteCode::Sub; break;
	case BOP_MUL: opCode = EExprByteCode::Mul; break;
	case BOP_DIV: opCode = isReverse ? EExprByteCode::DivR : EExprByteCode::Div; break;
	}

	if (rightValue.byteCode == EExprByteCode::None)
	{
		outputCmd(opCode);
	}
	else
	{
		if (leftValue.byteCode == rightValue.byteCode && leftValue.index == rightValue.index &&
			(type == BOP_MUL))
		{
			outputCmd(EExprByteCode::SMul);
		}
		else
		{
			outputCmd(EExprByteCode::Type(opCode + rightValue.byteCode), rightValue.index);
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
	pushResultValue();
#endif
}

void ExprByteCodeCompiler::codeUnaryOp(TokenType type)
{
#if EBC_USE_COMPOSITIVE_CODE
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
#if EBC_USE_COMPOSITIVE_CODE
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
	TValue  topValue = 0.0;
#if 1
	switch (*pCode)
	{
	case EExprByteCode::Input:
		topValue = GET_INPUT(pCode[1]);
		break;
#if !EBC_USE_VALUE_BUFFER
	case EExprByteCode::Const:
		topValue = GET_CONST(pCode[1]);
		break;
	case EExprByteCode::Variable:
		topValue = GET_VAR(pCode[1]);
		break;
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
#if EBC_USE_FIXED_SIZE
	pCode += EBC_USE_FIXED_SIZE;
#else
	pCode += 2;
#endif
#endif
#define EXECUTE_CODE() execCode(pCode, stackValue, topValue)

#if EBC_USE_FIXED_SIZE
#if 0
	while (pCode < pCodeEnd)
	{
		EXECUTE_CODE();
		pCode += EBC_USE_FIXED_SIZE;
	}
#else

	int numOp = (pCodeEnd - pCode) / EBC_USE_FIXED_SIZE;
#define EXECUTE_OP  EXECUTE_CODE(); pCode += EBC_USE_FIXED_SIZE
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
FORCEINLINE TValue Invoke(RT(*func)(Args...), TValue args[])
{
	return InvokeInternal(func, args, TIndexRange<0, sizeof...(Args)>());
}

template< typename TValue, typename RT, typename ...Args, size_t ...Is>
FORCEINLINE TValue InvokeInternal(RT(*func)(Args...), TValue args[], TIndexList<Is...>)
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
#endif
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

#if EBC_USE_COMPOSITIVE_OP_CODE
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
		TValue params[1];
		params[0] = topValue;
		topValue = Invoke(static_cast<FuncType1>(funcPtr), params);
	}
	return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 2
	case EExprByteCode::FuncCall2:
	{
		void* funcPtr = mExecData->pointers[pCode[1]];
		TValue params[2];
		params[1] = topValue;
		params[0] = popStack();
		topValue = Invoke(static_cast<FuncType2>(funcPtr), params);
	}
	return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 3
	case EExprByteCode::FuncCall3:
	{
		void* funcPtr = mExecData->pointers[pCode[1]];
		TValue params[3];
		params[2] = topValue;
		params[1] = popStack();
		params[0] = popStack();
		topValue = Invoke(static_cast<FuncType3>(funcPtr), params);
	}
	return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 4
	case EExprByteCode::FuncCall4:
	{
		void* funcPtr = mExecData->pointers[pCode[1]];
		TValue params[4];
		params[3] = topValue;
		params[2] = popStack();
		params[1] = popStack();
		params[0] = popStack();
		topValue = Invoke(static_cast<FuncType4>(funcPtr), params);
	}
	return 2;
#endif
#if EBC_MAX_FUNC_ARG_NUM >= 5
	case EExprByteCode::FuncCall5:
	{
		void* funcPtr = mExecData->pointers[pCode[1]];
		TValue params[5];
		params[4] = topValue;
		params[3] = popStack();
		params[2] = popStack();
		params[1] = popStack();
		params[0] = popStack();
		topValue = Invoke(static_cast<FuncType5>(funcPtr), params);
	}
	return 2;
#endif
	}
	return 1;
}

template RealType TByteCodeExecutor<RealType>::doExecute(ExprByteCodeExecData const& code);
template FloatVector TByteCodeExecutor<FloatVector>::doExecute(ExprByteCodeExecData const& code);

