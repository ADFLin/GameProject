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
		, mEvalData()
	{
		mIsParsed = false;
	}

	Expression::~Expression()
	{

	}


	Expression& Expression::operator=(const Expression& expr)
	{
		mStrExpr = expr.mStrExpr;
		mEvalData.clearCode();
		mIsParsed = false;
		return *this;
	}

}//namespace CB
