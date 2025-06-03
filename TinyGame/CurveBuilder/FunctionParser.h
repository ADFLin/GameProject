#ifndef FunctionParser_H
#define FunctionParser_H

#include "ExpressionParser.h"

namespace CB
{
	class Expression;

	class IFunctionParser
	{
	public:
		virtual ~IFunctionParser() = default;
		virtual bool checkValid(char const* expr) = 0;
		virtual bool compile(Expression& expr, int numInput, ValueLayout inputLayouts[], ParseResult& parseResult) = 0;
	};

	class FunctionParser : public IFunctionParser
	{
	public:
		FunctionParser();
		SymbolTable& getSymbolDefine() { return mSymbolDefine; }
		bool         checkValid(char const* expr);
		bool         compile(Expression& expr, int numInput, ValueLayout inputLayouts[], ParseResult& parseResult);
		bool         parse(Expression& expr, int numInput, ValueLayout inputLayouts[], ParseResult& parseResult);

	private:
		SymbolTable  mSymbolDefine;
	};
}//namespace CB



#endif //FunctionParser_H