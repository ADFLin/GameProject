#ifndef ExpressionEvaluator_h__
#define ExpressionEvaluator_h__

#include "ExpressionTree.h"

class ExprEvaluatorBase : public ExprParse
{
public:
	using Unit = ExprParse::Unit;
	ExprEvaluatorBase() : mInputs(nullptr, 0) {}

	void pushStack(Unit const& unit) { mValueStack.push_back(unit); }
	void pushStack(RealType val) { mValueStack.push_back(Unit::ConstValue(val)); }

	RealType popStack()
	{
		RealType result = getValue(mValueStack.back());
		mValueStack.pop_back();
		return result;
	}

	RealType getValue(Unit const& valueCode)
	{
		switch (valueCode.type)
		{
		case VALUE_CONST:
			CHECK(valueCode.constValue.layout == ValueLayout::Real);
			return valueCode.constValue.asReal;
		case VALUE_VARIABLE:
			CHECK(valueCode.variable.layout == ValueLayout::Real);
			return *((RealType*)valueCode.variable.ptr);
		case VALUE_INPUT:
			return mInputs[valueCode.input.index];
		case VALUE_EXPR_TEMP:
			return mTempValues[valueCode.exprTemp.tempIndex];
		}
		NEVER_REACH("");
		return 0;
	}

	void visitValue(Unit const& unit) { pushStack(unit); }
	void visitOp(Unit const& unit);

	void exec(Unit const& unit);
	void execFuncCall(FuncInfo const& funcInfo);
	void execFuncCall(FuncSymbolInfo const& funcSymbol);

	TArrayView<RealType const> mInputs;
	TArray<RealType, TFixedAllocator<16> > mTempValues;
	TArray<Unit, TFixedAllocator<32> > mValueStack;
};

class ExprTreeEvaluator : public ExprEvaluatorBase
{
public:
	template < typename ...Ts >
	RealType eval(ExpressionTreeData& tree, Ts... input)
	{
		RealType inputs[] = { (RealType)input... };
		mInputs = inputs;
		TExprTreeVisitOp op(tree, *this);
		op.execute();
		return popStack();
	}
};

#endif // ExpressionEvaluator_h__
