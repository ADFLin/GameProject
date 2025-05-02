#include "ExpressionUtils.h"
#include "DifferentialEvaluator.h"

std::string FExpressUtils::Differentiate(char const* exprssion, char const* x)
{
	SymbolTable table;
	DifferentialEvaluator::DefineFuncSymbol(table);
	table.defineVarInput(x, 0);

	ExpressionTreeData exprData;
	ExpressionParser parser;
	parser.parse(exprssion, table, exprData);

	DifferentialEvaluator evaluator;
	ExpressionTreeData exprDataDiff;
	evaluator.eval(exprData, exprDataDiff);
	return exprDataDiff.getExpressionText(table);
}

