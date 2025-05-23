#include "Expression.h"

namespace CB
{

	Expression::Expression(std::string const& ExprStr /*= ""*/)
		:mStrExpr(ExprStr)
		,mIsParsed(false)
	{

	}

	Expression::Expression(Expression const& rhs)
		:mStrExpr(rhs.mStrExpr)
		, mIsParsed(rhs.mIsParsed)
	{
		mEvalResource = nullptr;
		mIsParsed = false;
	}

	Expression::~Expression()
	{
		delete mEvalResource;
	}

	Expression& Expression::operator=(const Expression& expr)
	{
		mStrExpr = expr.mStrExpr;

		delete mEvalResource;
		mEvalResource = nullptr;
		mIsParsed = false;
		return *this;
	}

}//namespace CB
