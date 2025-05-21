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
		FORCEINLINE RealType eval() const { return mEvalCode.evalT<RealType>(); }
		FORCEINLINE RealType eval(RealType p0) const { return mEvalCode.evalT<RealType>(p0); }
		FORCEINLINE RealType eval(RealType p0, RealType p1) const { return mEvalCode.evalT<RealType>(p0,p1); }
		FORCEINLINE RealType eval(RealType p0, RealType p1, RealType p2) const { return mEvalCode.evalT<RealType>(p0,p1,p2); }


		FORCEINLINE FloatVector eval(FloatVector const& p0) const { return mEvalCode.evalT<FloatVector>(p0); }
		FORCEINLINE FloatVector eval(FloatVector const& p0, FloatVector const& p1) const { return mEvalCode.evalT<FloatVector>(p0, p1); }
		FORCEINLINE FloatVector eval(FloatVector const& p0, FloatVector const& p1, FloatVector const& p2) const { return mEvalCode.evalT<FloatVector>(p0, p1, p2); }

	private:
		bool           mIsParsed;
		std::string    mStrExpr;
		ExecutableCode mEvalCode;
		friend class FunctionParser;
	};

}//namespace CB


#endif  //Expression_H




