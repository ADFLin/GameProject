#ifndef Expression_H
#define Expression_H

#include "FPUCompiler.h"

namespace CB
{
	class Expression
	{
	public:
		Expression(string const& ExprStr = "");
		Expression(Expression const& rhs);
		Expression& operator=(const Expression& expr);

		~Expression();

		bool  isParsed() { return mIsParsed; }
		void  setExprString(string const& ExprStr)
		{
			mStrExpr = ExprStr;
			mIsParsed = false;
		}
		ExecutableCode&  getEvalData() { return mEvalData; }
		string const& getExprString() const { return mStrExpr; }
		double eval() const { return mEvalData.evalT<double>(); }
		double eval(double p0) const { return mEvalData.evalT<double>(p0); }
		double eval(double p0, double p1) const { return mEvalData.evalT<double>(p0,p1); }
		double eval(double p0, double p1, double p2) const { return mEvalData.evalT<double>(p0,p1,p2); }

	private:
		bool          mIsParsed;
		string        mStrExpr;
		ExecutableCode   mEvalData;
		friend class FunctionParser;
	};

}//namespace CB


#endif  //Expression_H




