#include "ExpressionUtils.h"
#include "DifferentialEvaluator.h"
#include "Misc/DuffDevice.h"

std::string FExpressUtils::Differentiate(char const* exprssion, char const* x)
{
	SymbolTable table;
	DifferentialEvaluator::DefineFuncSymbol(table);
	table.defineVarInput(x, 0);

	ExpressionTreeData exprData;
	ExpressionParser parser;
	if (!parser.parse(exprssion, table, exprData))
		return "";

	DifferentialEvaluator evaluator;
	ExpressionTreeData exprDataDiff;
	evaluator.eval(exprData, exprDataDiff);
	return exprDataDiff.getExpressionText(table);
}



void FExpressUtils::DoEvalate(ExprEvaluatorBase &evaluator, TArrayView<ExprParse::Unit const> codes)
{
#if 0
	for (auto const& code : codes)
	{
		if (ExprParse::IsValue(code.type))
		{
			evaluator.visitValue(code);
		}
		else
		{
			evaluator.visitOp(code);
		}
	}
#else

#if 0
	int index = 0;
	int numCodes = codes.size();
#define OP evaluator.exec(codes[index]); ++index;
	DUFF_DEVICE_8(codes.size(), OP);
#undef OP

#else
	for (auto const& code : codes)
	{
		evaluator.exec(code);
	}
#endif
#endif
}

