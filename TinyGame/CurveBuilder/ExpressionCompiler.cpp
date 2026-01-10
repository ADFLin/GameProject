#include "ExpressionCompiler.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include <algorithm>

#include "ProfileSystem.h"
#include "LogSystem.h"


ExpressionCompiler::ExpressionCompiler()
{
	mOptimization = true;
	mbGenerateSIMD = false;
	mTargetType = ECodeExecType::Asm;
}

bool ExpressionCompiler::compile( char const* expr, SymbolTable const& table, ParseResult&  parseResult, ExecutableCode& data , int numInput , ValueLayout inputLayouts[] )
{
	try
	{
		ExpressionParser parser;
		if (!parser.parse(expr, table, parseResult))
			return false;

#if 1
		if (mOptimization)
		{
			// Log before optimization
			LogDevMsg(0, "=== Before Optimization ===");
			parseResult.printPostfixCodes();
			
			if (parseResult.optimize())
			{
				// Log after optimization
				LogDevMsg(0, "=== After Optimization ===");
				parseResult.printPostfixCodes();
			}
		}
#endif


		{
			TIME_SCOPE("Generate Code");

			switch (mTargetType)
			{
#if ENABLE_ASM_CODE
			case ECodeExecType::Asm:
			{
				if (mbGenerateSIMD)
				{
#if TARGET_PLATFORM_64BITS
					SSESIMDCodeGeneratorX64 generator;
					generator.setCodeData(&data.initAsm());
					parseResult.generateCode(generator, numInput, inputLayouts);
#else
					return false;
#endif
				}
				else
				{
#if TARGET_PLATFORM_64BITS
					SSECodeGeneratorX64 generator;
#else
					FPUCodeGeneratorV0 generator;
#endif
					generator.setCodeData(&data.initAsm());
					parseResult.generateCode(generator, numInput, inputLayouts);
				}
			}
			break;
#endif

#if ENABLE_BYTE_CODE
			case ECodeExecType::ByteCode:
			{
				ExprByteCodeCompiler generator(data.initByteCode());
				parseResult.generateCode(generator, numInput, inputLayouts);
			}
			break;
#endif

#if ENABLE_INTERP_CODE
			case ECodeExecType::Interp:
			{
				data.initInterp() = parseResult.genratePosifxCodes();
			}
			break;
#endif

			default:
				return false;
			}

			data.setSIMD(mbGenerateSIMD || mTargetType == ECodeExecType::ByteCode);
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
