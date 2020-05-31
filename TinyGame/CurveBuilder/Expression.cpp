#include "Expression.h"

namespace CB
{

	Expression::Expression(string const& ExprStr /*= ""*/)
		:mStrExpr(ExprStr)
		, mIsParsed(false)
	{

	}

	Expression::Expression(Expression const& rhs)
		:mStrExpr(rhs.mStrExpr)
		, mIsParsed(rhs.mIsParsed)
		, mEvalCode()
	{
		mIsParsed = false;
	}

	Expression::~Expression()
	{

	}


	Expression& Expression::operator=(const Expression& expr)
	{
		mStrExpr = expr.mStrExpr;
		mEvalCode.clearCode();
		mIsParsed = false;
		return *this;
	}

}//namespace CB
