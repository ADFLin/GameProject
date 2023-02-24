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
		expr.mIsParsed = mCompiler.compile(expr.getExprString().c_str(), mSymbolDefine, expr.getEvalData(), numInput , inputLayouts);
		return expr.mIsParsed;
	}

	FunctionParser::FunctionParser()
	{
		mSymbolDefine.defineVarInput("x", 0);
		mSymbolDefine.defineVarInput("y", 1);
		mSymbolDefine.defineVarInput("u", 0);
		mSymbolDefine.defineVarInput("v", 1);

		mSymbolDefine.defineFunc("sin", static_cast< RealType (*)(RealType) >(sin));
		mSymbolDefine.defineFunc("cos", static_cast< RealType(*)(RealType) >(cos));
		mSymbolDefine.defineFunc("tan", static_cast< RealType(*)(RealType) >(tan));
		mSymbolDefine.defineFunc("exp", static_cast< RealType(*)(RealType) >(exp));
		mSymbolDefine.defineFunc("ln", static_cast< RealType(*)(RealType) >(log));
		mSymbolDefine.defineFunc("log", static_cast< RealType(*)(RealType) >(log10));
		mSymbolDefine.defineFunc("sqrt", static_cast< RealType(*)(RealType) >(sqrt));
		mSymbolDefine.defineFunc("pow", static_cast< RealType(*)(RealType, RealType) >(pow));
	}

}//namespace CB