#include "ExpressionCompiler.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "ProfileSystem.h"


ExpressionCompiler::ExpressionCompiler()
{
	mOptimization = true;
}

bool ExpressionCompiler::compile( char const* expr , SymbolTable const& table , ExecutableCode& data , int numInput , ValueLayout inputLayouts[] )
{
	try
	{
		ExpressionParser parser;
		if (!parser.parse(expr, table, mResult))
			return false;

#if 1
		if (mOptimization)
			mResult.optimize();
#endif


		{
			TIME_SCOPE("Generate Code");
#if ENABLE_FPU_CODE
			FPUCodeGeneratorV0 generator;
			generator.setCodeData(&data.mData);
			mResult.generateCode(generator, numInput, inputLayouts);
#elif ENABLE_BYTE_CODE
			ExprByteCodeCompiler generator(data.mData);
			mResult.generateCode(generator, numInput, inputLayouts);
#else
			data.mCodes = mResult.genratePosifxCodes();
#endif
		}
		return true;
	}
	catch ( ExprParseException& e)
	{
		LogMsg("Compile error : (%d) : %s", e.errorCode, e.what());
		return false;
	}
	catch ( std::exception& )
	{
		return false;
	}
}


ExecutableCode::ExecutableCode( int size )
{

}


ExecutableCode::~ExecutableCode()
{

}

void ExecutableCode::clearCode()
{
#if ENABLE_FPU_CODE
	mData.clear();
#elif ENABLE_BYTE_CODE
	mData.clear();
#else
	mCodes.clear();
#endif
}

