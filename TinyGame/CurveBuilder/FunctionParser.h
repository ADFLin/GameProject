#ifndef FunctionParser_H
#define FunctionParser_H

#include "FPUCompiler.h"

namespace CB
{
	class Expression;

	class FunctionParser : public FPUCompiler
	{
	public:
		FunctionParser();
		SymbolTable& getSymbolDefine() { return mSymbolDefine; }
		bool         checkValid(char const* expr);
		bool         parse(Expression& expr , int numInput = 0 , ValueLayout inputLayouts[] = nullptr);

	private:
		SymbolTable  mSymbolDefine;
	};
}//namespace CB



#endif //FunctionParser_H