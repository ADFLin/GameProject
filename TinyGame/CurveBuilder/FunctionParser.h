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
		DefineTable& getDefineTable() { return mDefineTable; }
		bool         checkVaild(char const* expr);
		bool         parse(Expression& expr , int numInput = 0);

	private:
		DefineTable  mDefineTable;
	};
}//namespace CB



#endif //FunctionParser_H