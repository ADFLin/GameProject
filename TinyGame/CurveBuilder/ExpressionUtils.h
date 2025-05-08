#ifndef ExpressionUtils_h__
#define ExpressionUtils_h__

#include "Template/ArrayView.h"
#include "ExpressionParser.h"

class FExpressUtils
{
public:

	static bool Parse();
	static std::string Differentiate(char const* exprssion, char const* x);

	template< typename RT>
	static RT EvalutePosfixCodes(TArrayView<ExprParse::Unit const> codes)
	{
		ExprEvaluatorBase evaluator;
		DoEvalate(evaluator, codes);
		return RT(evaluator.popStack());
	}

	template< typename RT, class ...Args >
	static RT EvalutePosfixCodes(TArrayView<ExprParse::Unit const> codes, Args ...args)
	{
		ExprEvaluatorBase evaluator;
		void* inputs[] = { &args... };
		evaluator.mInputs = inputs;
		DoEvalate(evaluator, codes);
		return RT(evaluator.popStack());
	}

	static void DoEvalate(ExprEvaluatorBase &evaluator, TArrayView<ExprParse::Unit const> codes);

};

#endif // ExpressionUtils_h__