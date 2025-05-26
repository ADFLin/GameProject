#ifndef ExprByteCode_h__
#define ExprByteCode_h__

#include "ExpressionParser.h"

#include "Math/SIMD.h"

#define SIMD_USE_AVX 0
#if SIMD_USE_AVX
using FloatVector = SIMD::TFloatVector<8>;
#else
using FloatVector = SIMD::TFloatVector<4>;
#endif

#define EBC_USE_VALUE_BUFFER 0

#define EBC_USE_FIXED_SIZE 2
#define EBC_USE_COMPOSITIVE_CODE 1
#define EBC_USE_COMPOSITIVE_OP_CODE 1
#define EBC_MAX_FUNC_ARG_NUM 0


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

#if EBC_USE_COMPOSITIVE_CODE

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
#if EBC_USE_COMPOSITIVE_OP_CODE
		SMulAdd,
		SMulSub,
		SMulMul,
		SMulDiv,

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
	};
}


struct ExprByteCodeExecData
{
	TArray<uint8>     codes;
	TArray<void*>     pointers;
	TArray<RealType>  constValues;
#if EBC_USE_VALUE_BUFFER
	TArray<RealType*> vars;
#endif

	void clear()
	{
		codes.clear();
		pointers.clear();
		constValues.clear();
#if EBC_USE_VALUE_BUFFER
		vars.clear();
#endif
	}
};

struct ExprByteCodeCompiler : public TCodeGenerator<ExprByteCodeCompiler>, public ExprParse
{
	ExprByteCodeExecData& mOutput;
	int mNumCmd = 0;

	ExprByteCodeCompiler(ExprByteCodeExecData& output)
		:mOutput(output)
	{

	}
	using TokenType = ParseResult::TokenType;


#if EBC_USE_VALUE_BUFFER
	int mNumInput = 0;
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

#endif

	void codeBinaryOp(TokenType type, bool isReverse);

	void codeUnaryOp(TokenType type);


#if EBC_USE_COMPOSITIVE_OP_CODE
	int   mNumDeferredOutCodes;
	uint8 mDeferredOutputCodes[3];

	void outputCmdDeferred(EExprByteCode::Type a)
	{
		checkOuputDeferredCmd();
		mNumDeferredOutCodes = 1;
		mDeferredOutputCodes[0] = a;
	}
	void outputCmdDeferred(EExprByteCode::Type a, uint8 b)
	{
		checkOuputDeferredCmd();
		mNumDeferredOutCodes = 2;
		mDeferredOutputCodes[0] = a;
		mDeferredOutputCodes[1] = b;
	}
	void outputCmdDeferred(EExprByteCode::Type a, uint8 b, uint c)
	{
		checkOuputDeferredCmd();
		mNumDeferredOutCodes = 3;
		mDeferredOutputCodes[0] = a;
		mDeferredOutputCodes[1] = b;
		mDeferredOutputCodes[2] = c;
	}

	void checkOuputDeferredCmd()
	{
		if (mNumDeferredOutCodes == 0)
			return;

		ouputDeferredCmdChecked();
	}

	void ouputDeferredCmdChecked()
	{
		CHECK(mNumDeferredOutCodes > 0);
		switch (mNumDeferredOutCodes)
		{
		case 1: outputCmdActual((EExprByteCode::Type)mDeferredOutputCodes[0]); break;
		case 2: outputCmdActual((EExprByteCode::Type)mDeferredOutputCodes[0], mDeferredOutputCodes[1]); break;
		case 3: outputCmdActual((EExprByteCode::Type)mDeferredOutputCodes[0], mDeferredOutputCodes[1], mDeferredOutputCodes[2]); break;
		}
		mNumDeferredOutCodes = 0;
	}
#else
	void outputCmdDeferred(EExprByteCode::Type a)
	{
		outputCmdActual(a);
	}
	void outputCmdDeferred(EExprByteCode::Type a, uint8 b)
	{
		outputCmdActual(a, b);
	}
	void outputCmdDeferred(EExprByteCode::Type a, uint8 b, uint c)
	{
		outputCmdActual(a, b, c);
	}

	void checkOuputDeferredCmd()
	{
	}
#endif
	void visitSeparetor()
	{
#if EBC_USE_COMPOSITIVE_CODE
		auto leftValue = mStacks.back();
		if (leftValue.byteCode != EExprByteCode::None)
		{
			checkOuputDeferredCmd();
			outputCmd(leftValue.byteCode, leftValue.index);
		}

		mStacks.pop_back();
#endif
	}

	void pushResultValue()
	{
		mStacks.push_back({ EExprByteCode::None, 0 });
	}

	void pushValue(EExprByteCode::Type byteCode, uint8 index)
	{
#if EBC_USE_COMPOSITIVE_CODE
		mStacks.push_back({ byteCode, index });
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


	void outputCmd(EExprByteCode::Type a)
	{
		checkOuputDeferredCmd();
		outputCmdActual(a);
	}
	void outputCmd(EExprByteCode::Type a, uint8 b)
	{
		checkOuputDeferredCmd();
		outputCmdActual(a, b);
	}
	void outputCmd(EExprByteCode::Type a, uint8 b, uint c)
	{
		checkOuputDeferredCmd();
		outputCmdActual(a, b, c);
	}

	void outputCmdActual(EExprByteCode::Type a)
	{
		++mNumCmd;
		mOutput.codes.push_back(a);
#if EBC_USE_FIXED_SIZE >= 2
		mOutput.codes.push_back(0);
#endif
#if EBC_USE_FIXED_SIZE >= 3
		mOutput.codes.push_back(0);
#endif
	}

	void outputCmdActual(EExprByteCode::Type a, uint8 b)
	{
		++mNumCmd;
		mOutput.codes.push_back(a);
		mOutput.codes.push_back(b);
#if EBC_USE_FIXED_SIZE >= 3
		mOutput.codes.push_back(0);
#endif
	}

	void outputCmdActual(EExprByteCode::Type a, uint8 b, uint c)
	{
		++mNumCmd;
		mOutput.codes.push_back(a);
		mOutput.codes.push_back(b);
		mOutput.codes.push_back(c);
	}


	static char const* GetOpString(uint8 opCode)
	{
		switch (opCode)
		{
		case EExprByteCode::Input: return "I";
#if !EBC_USE_VALUE_BUFFER
		case EExprByteCode::Const: return "C";
		case EExprByteCode::Variable:  return "V";
#endif

		case EExprByteCode::Add: return "+";
		case EExprByteCode::Sub: return "-";
		case EExprByteCode::SubR: return "-r";
		case EExprByteCode::Mul: return "*";
		case EExprByteCode::Div: return "/";
		case EExprByteCode::DivR: return "/r";
		case EExprByteCode::Mins: return "Mins";
#if EBC_USE_COMPOSITIVE_CODE

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

#if EBC_USE_COMPOSITIVE_OP_CODE
		case EExprByteCode::SMulAdd:return "S*+";
		case EExprByteCode::SMulSub:return "S*-";
		case EExprByteCode::SMulMul:return "S**";
		case EExprByteCode::SMulDiv:return "S*/";

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
};

template<typename TValue>
class TByteCodeExecutor
{
public:
	TValue doExecute(ExprByteCodeExecData const& execData);
	int  execCode(uint8 const* pCode, TValue*& pValueStack, TValue& topValue);

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

	ExprByteCodeExecData const* mExecData;
	TArrayView<TValue const> mInputs;
};

#endif // ExprByteCode_h__
