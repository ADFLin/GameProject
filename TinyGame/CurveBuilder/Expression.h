#ifndef Expression_H
#define Expression_H

#include "ExpressionCompiler.h"

namespace CB
{
	class Expression
	{
	public:
		Expression(std::string const& ExprStr = "");
		Expression(Expression const& rhs);
		Expression& operator=(const Expression& expr);

		~Expression();

		bool  isParsed() { return mIsParsed; }
		void  setExprString(std::string const& ExprStr)
		{
			mStrExpr = ExprStr;
			mIsParsed = false;
		}
		ExecutableCode&  getEvalData() { return mEvalCode; }
		std::string const& getExprString() const { return mStrExpr; }
		FORCEINLINE double eval() const { return mEvalCode.evalT<double>(); }
		FORCEINLINE double eval(double p0) const { return mEvalCode.evalT<double>(p0); }
		FORCEINLINE double eval(double p0, double p1) const { return mEvalCode.evalT<double>(p0,p1); }
		FORCEINLINE double eval(double p0, double p1, double p2) const { return mEvalCode.evalT<double>(p0,p1,p2); }

	private:
		bool           mIsParsed;
		std::string    mStrExpr;
		ExecutableCode mEvalCode;
		friend class FunctionParser;
	};

}//namespace CB


#endif  //Expression_H




