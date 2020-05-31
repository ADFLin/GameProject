#include "FunctionParser.h"
#include "Expression.h"

#include <cmath>

namespace CB
{

	static RealType sVarDummy;

	void ReplaceAllSubString(std::string& str, std::string const& from, std::string const& to)
	{
		size_t posStart = 0;
		while( (posStart = str.find(from, posStart)) != std::string::npos )
		{
			str.replace(posStart, from.length(), to);
			posStart += to.length();
		}
	}

	bool FunctionParser::checkValid(char const* expr)
	{
		try
		{
			ExpressionParser parser;
			if (!parser.parse(expr, mSymbolDefine))
				return false;
		}
		catch( ExprParseException&  )
		{
			return false;
		}
		return true;
	}

	bool FunctionParser::parse(Expression& expr , int numInput , ValueLayout inputLayouts[] )
	{
		expr.mIsParsed = compile(expr.getExprString().c_str(), mSymbolDefine, expr.getEvalData(), numInput , inputLayouts);
		return expr.mIsParsed;
	}

	FunctionParser::FunctionParser()
	{
		SymbolTable& table = getSymbolDefine();

		table.defineVarInput("x", 0);
		table.defineVarInput("y", 1);
		table.defineVarInput("u", 0);
		table.defineVarInput("v", 1);

		table.defineFunc("sin", static_cast< RealType (*)(RealType) >(sin));
		table.defineFunc("cos", static_cast< RealType(*)(RealType) >(cos));
		table.defineFunc("tan", static_cast< RealType(*)(RealType) >(tan));
		table.defineFunc("exp", static_cast< RealType(*)(RealType) >(exp));
		table.defineFunc("ln", static_cast< RealType(*)(RealType) >(log));
		table.defineFunc("log", static_cast< RealType(*)(RealType) >(log10));
		table.defineFunc("sqrt", static_cast< RealType(*)(RealType) >(sqrt));
		table.defineFunc("pow", static_cast< RealType(*)(RealType, RealType) >(pow));
	}

}//namespace CB