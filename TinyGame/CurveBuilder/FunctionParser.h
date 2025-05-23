#ifndef FunctionParser_H
#define FunctionParser_H

#include "ExpressionCompiler.h"

namespace CB
{
	class Expression;

	class IFunctionParser
	{
	public:
		virtual ~IFunctionParser() = default;
		virtual bool checkValid(char const* expr) = 0;
		virtual bool compile(Expression& expr, int numInput = 0, ValueLayout inputLayouts[] = nullptr) = 0;
		virtual bool isUsingVar(char const* varName) const = 0;
		virtual bool isUsingInput(char const* varName) const = 0;
	};

	class FunctionParser : public IFunctionParser
	{
	public:
		FunctionParser();
		SymbolTable& getSymbolDefine() { return mSymbolDefine; }
		bool         checkValid(char const* expr);
		bool         compile(Expression& expr, int numInput, ValueLayout inputLayouts[] = nullptr);
		bool         parse(Expression& expr, int numInput, ValueLayout inputLayouts[], ParseResult& parseResult);
		bool		 isUsingVar(char const* varName) const
		{
			return mCompiler.isUsingVar(varName);
		}
		bool		isUsingInput(char const* varName) const
		{
			return mCompiler.isUsingInput(varName);
		}

	private:
		ExpressionCompiler  mCompiler;
		SymbolTable  mSymbolDefine;
	};
}//namespace CB



#endif //FunctionParser_H