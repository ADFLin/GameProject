#include "ExpressionEvaluator.h"
#include "LogSystem.h"
#include <cmath>

void ExprEvaluatorBase::visitOp(Unit const& unit)
{
	switch (unit.type)
	{
	case FUNC_DEF:    execFuncCall(unit.symbol->func); break;
	case FUNC_SYMBOL: execFuncCall(unit.funcSymbol); break;
	case BOP_ADD:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		pushStack(lhs + rhs);
	}
	break;
	case BOP_SUB:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		if (unit.isReverse) pushStack(rhs - lhs); else pushStack(lhs - rhs);
	}
	break;
	case BOP_MUL:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		pushStack(lhs * rhs);
	}
	break;
	case BOP_DIV:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		if (unit.isReverse) pushStack(rhs / lhs); else pushStack(lhs / rhs);
	}
	break;
	case UOP_MINS:
	{
		RealType lhs = popStack();
		pushStack(-lhs);
	}
	break;
	default:
#if _DEBUG
		LogWarning(0, "Unknow Code %d", unit.type);
#endif
		break;
	}

	if (unit.storeIndex != -1)
	{
		if (unit.storeIndex >= (int)mTempValues.size())
			mTempValues.resize(unit.storeIndex + 1);
		mTempValues[unit.storeIndex] = getValue(mValueStack.back());
	}
}

void ExprEvaluatorBase::exec(Unit const& unit)
{
	switch (unit.type)
	{
	case VALUE_CONST:
	case VALUE_INPUT:
	case VALUE_VARIABLE:
	case VALUE_EXPR_TEMP:
		pushStack(unit);
		break;
	case FUNC_DEF:    execFuncCall(unit.symbol->func); break;
	case FUNC_SYMBOL: execFuncCall(unit.funcSymbol); break;
	case BOP_ADD:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		pushStack(lhs + rhs);
	}
	break;
	case BOP_SUB:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		if (unit.isReverse) pushStack(rhs - lhs); else pushStack(lhs - rhs);
	}
	break;
	case BOP_MUL:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		pushStack(lhs * rhs);
	}
	break;
	case BOP_DIV:
	{
		RealType rhs = popStack();
		RealType lhs = popStack();
		if (unit.isReverse) pushStack(rhs / lhs); else pushStack(lhs / rhs);
	}
	break;
	case UOP_MINS:
	{
		RealType lhs = popStack();
		pushStack(-lhs);
	}
	break;
	default:
#if _DEBUG
		LogWarning(0, "Unknow Code %d", unit.type);
#endif
		break;
	}

	if (unit.storeIndex != -1)
	{
		if (unit.storeIndex >= (int)mTempValues.size())
			mTempValues.resize(unit.storeIndex + 1);
		mTempValues[unit.storeIndex] = getValue(mValueStack.back());
	}
}

void ExprEvaluatorBase::execFuncCall(FuncInfo const& funcInfo)
{
	RealType params[5];
	int   numParam = funcInfo.getArgNum();
	void* funcPtr = funcInfo.funcPtr;

	 RealType value = 0;
	switch (numParam)
	{
	case 0: value = (*static_cast<FuncType0>(funcPtr))(); break;
	case 1:
		params[0] = popStack();
		value = (*static_cast<FuncType1>(funcPtr))(params[0]);
		break;
	case 2:
		params[0] = popStack();
		params[1] = popStack();
		value = (*static_cast<FuncType2>(funcPtr))(params[1], params[0]);
		break;
	case 3:
		params[0] = popStack();
		params[1] = popStack();
		params[2] = popStack();
		value = (*static_cast<FuncType3>(funcPtr))(params[2], params[1], params[0]);
		break;
	case 4:
		params[0] = popStack();
		params[1] = popStack();
		params[2] = popStack();
		params[3] = popStack();
		value = (*static_cast<FuncType4>(funcPtr))(params[3], params[2], params[1], params[0]);
		break;
	case 5:
		params[0] = popStack();
		params[1] = popStack();
		params[2] = popStack();
		params[3] = popStack();
		params[4] = popStack();
		value = (*static_cast<FuncType5>(funcPtr))(params[4], params[3], params[2], params[1], params[0]);
		break;
	}
	pushStack(value);
}

void ExprEvaluatorBase::execFuncCall(FuncSymbolInfo const& funcSymbol)
{
	switch (funcSymbol.id)
	{
	case EFuncSymbol::Exp:  pushStack(std::exp(popStack())); break;
	case EFuncSymbol::Ln:   pushStack(std::log(popStack())); break;
	case EFuncSymbol::Sin:  pushStack(std::sin(popStack())); break;
	case EFuncSymbol::Cos:  pushStack(std::cos(popStack())); break;
	case EFuncSymbol::Tan:  pushStack(std::tan(popStack())); break;
	case EFuncSymbol::Cot:  pushStack(1.0 / std::tan(popStack())); break;
	case EFuncSymbol::Sec:  pushStack(1.0 / std::cos(popStack())); break;
	case EFuncSymbol::Csc:  pushStack(1.0 / std::sin(popStack())); break;
	case EFuncSymbol::Sqrt: pushStack(std::sqrt(popStack())); break;
	}
}
