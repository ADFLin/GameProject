#ifndef ExprByteCode_h__
#define ExprByteCode_h__

#include "../ExpressionParser.h"


#define EBC_USE_VALUE_BUFFER 1
#define EBC_MAX_OP_CMD_SZIE 3
#define EBC_USE_FIXED_OP_CMD_SIZE EBC_MAX_OP_CMD_SZIE
//#define EBC_USE_FIXED_OP_CMD_SIZE 0
#define EBC_USE_COMPOSITIVE_CODE_LEVEL 2
#define EBC_USE_LAZY_STACK_PUSH 1
#define EBC_MAX_FUNC_ARG_NUM 5

namespace EExprByteCode
{
	enum Type : uint8
	{
		None,

		Input,
#if !EBC_USE_VALUE_BUFFER
		Const,
		Variable,
#endif

#if EBC_USE_COMPOSITIVE_CODE_LEVEL

		Add,
		IAdd,
#if !EBC_USE_VALUE_BUFFER
		CAdd,
		VAdd,
#endif

		Sub,
		ISub,
#if !EBC_USE_VALUE_BUFFER
		CSub,
		VSub,
#endif

		SubR,
		ISubR,
#if !EBC_USE_VALUE_BUFFER
		CSubR,
		VSubR,
#endif

		Mul,
		IMul,
#if !EBC_USE_VALUE_BUFFER
		CMul,
		VMul,
#endif

		Div,
		IDiv,
#if !EBC_USE_VALUE_BUFFER
		CDiv,
		VDiv,
#endif

		DivR,
		IDivR,
#if !EBC_USE_VALUE_BUFFER
		CDivR,
		VDivR,
#endif


		SMul,
#if EBC_USE_COMPOSITIVE_CODE_LEVEL >= 2
		SMulAdd,
		SMulSub,
		SMulMul,
		SMulDiv,

#if EBC_USE_VALUE_BUFFER
		ISMul,
		ISMulAdd,
		ISMulSub,
		ISMulMul,
		ISMulDiv,

#if EBC_MAX_OP_CMD_SZIE >= 3
		IIAdd,
		IISub,
		IIMul,
		IIDiv,
#endif
#endif

		MulAdd,
		IMulAdd,
#if !EBC_USE_VALUE_BUFFER
		CMulAdd,
		VMulAdd,
#endif

		AddMul,
		IAddMul,
#if !EBC_USE_VALUE_BUFFER
		CAddMul,
		VAddMul,
#endif

		MulSub,
		IMulSub,
#if !EBC_USE_VALUE_BUFFER
		CMulSub,
		VMulSub,
#endif


#endif

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

		FuncSymbolEndPlusOne,
		FuncCall0 = FuncSymbolEndPlusOne,
		FuncCall1,
		FuncCall2,
		FuncCall3,
		FuncCall4,
		FuncCall5,

		COUNT,
	};
}



struct ExprByteCodeExecData
{

	TArray<uint8>     codes;
	TArray<void*>     pointers;
	TArray<RealType>  constValues;

#if EBC_USE_VALUE_BUFFER
	int               numInput;
	TArray<RealType*> vars;

	template< typename T >
	void initValueBuffer(T buffer[])
	{
		std::copy(constValues.begin(), constValues.end(), buffer + numInput);

		T* pValue = buffer + numInput + constValues.size();
		RealType** pVar = vars.data();
		for (int i = vars.size(); i; --i)
		{
			*pValue = **pVar;
			++pValue;
			++pVar;
		}
	}
#endif

	void clear()
	{
		codes.clear();
		pointers.clear();
		constValues.clear();
#if EBC_USE_VALUE_BUFFER
		vars.clear();
		numInput = 0;
#endif
	}
};

struct ExprByteCodeCompiler : public TCodeGenerator<ExprByteCodeCompiler>
{
	ExprByteCodeExecData& mOutput;
	int mNumCmd = 0;

	ExprByteCodeCompiler(ExprByteCodeExecData& output)
		:mOutput(output)
	{

	}
	using TokenType = ExprParse::TokenType;

#if EBC_USE_VALUE_BUFFER
	TArray<int> mVarIndexPosList;
#endif
	void codeInit(int numInput, ValueLayout inputLayouts[]);

	void codeEnd();

	void preLoadConst(ConstValueInfo const& value)
	{
		int index = mOutput.constValues.addUnique(value.asReal);
	}

	void codeConstValue(ConstValueInfo const& value);
	void codeVar(VariableInfo const& varInfo);
	void codeInput(InputInfo const& input);
	void codeFunction(FuncInfo const& info);
	void codeFunction(FuncSymbolInfo const& info);
	void codeBinaryOp(TokenType type, bool isReverse);
	void codeUnaryOp(TokenType type);

#if EBC_USE_LAZY_STACK_PUSH
	void visitSeparetor()
	{
		auto leftValue = mStacks.back();
		if (leftValue.byteCode != EExprByteCode::None)
		{
			outputCmd(leftValue.byteCode, leftValue.index);
		}

		mStacks.pop_back();
	}
	void pushResultValue()
	{
		mStacks.push_back({ EExprByteCode::None, 0 });
	}

	struct StackData
	{
		EExprByteCode::Type byteCode;
		uint8 index;
	};
	TArray< StackData > mStacks;

#endif

	void pushValue(EExprByteCode::Type byteCode, uint8 index)
	{
#if EBC_USE_LAZY_STACK_PUSH
		mStacks.push_back({ byteCode, index });
#else
		outputCmd(byteCode, index);
#endif
	}

	void outputCmd(EExprByteCode::Type a)
	{
		++mNumCmd;
		mOutput.codes.push_back(a);
#if EBC_USE_FIXED_OP_CMD_SIZE >= 2
		mOutput.codes.push_back(0);
#endif
#if EBC_USE_FIXED_OP_CMD_SIZE >= 3
		mOutput.codes.push_back(0);
#endif
	}
	void outputCmd(EExprByteCode::Type a, uint8 b)
	{
		++mNumCmd;
		mOutput.codes.push_back(a);
		mOutput.codes.push_back(b);
#if EBC_USE_FIXED_OP_CMD_SIZE >= 3
		mOutput.codes.push_back(0);
#endif
	}
	void outputCmd(EExprByteCode::Type a, uint8 b, uint c)
	{
		++mNumCmd;
		mOutput.codes.push_back(a);
		mOutput.codes.push_back(b);
		mOutput.codes.push_back(c);
	}

};

template<typename TValue>
class TByteCodeExecutor
{
public:

	TValue execute(ExprByteCodeExecData const& execData)
	{
		return doExecute(execData);
	}

	template< class ...TArgs >
	TValue execute(ExprByteCodeExecData const& execData, TArgs ...args)
	{
		TValue inputs[] = { args... };
		mInputs = inputs;
		return doExecute(execData);
	}

	TValue execute(ExprByteCodeExecData const& execData, TArrayView<TValue const> valueBuffers)
	{
		mInputs = valueBuffers;
		return doExecute(execData);
	}

private:
	TValue doExecute(ExprByteCodeExecData const& execData);
	int  execCode(uint8 const* pCode, TValue*& pValueStack, TValue& topValue);

	ExprByteCodeExecData const* mExecData;
	TArrayView<TValue const> mInputs;
};

#endif // ExprByteCode_h__
