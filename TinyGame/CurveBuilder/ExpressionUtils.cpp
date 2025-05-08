#include "ExpressionUtils.h"
#include "DifferentialEvaluator.h"

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


int constexpr BlockSize = 8;

#define DUFF_DEVICE( DIM , OP )\
	{\
		int blockCount = (DIM + BlockSize - 1) / BlockSize;\
		switch (DIM % BlockSize)\
		{\
		case 0: do { OP;\
		case 7: OP;\
		case 6: OP;\
		case 5: OP;\
		case 4: OP;\
		case 3: OP;\
		case 2: OP;\
		case 1: OP;\
			} while (--blockCount > 0);\
		}\
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

#if 1
	int index = 0;
	int numCodes = codes.size();
#define OP evaluator.exec(codes[index]); ++index;
	DUFF_DEVICE(codes.size(), OP);
#undef OP

#else
	for (auto const& code : codes)
	{
		evaluator.exec(code);
	}
#endif
#endif
}

