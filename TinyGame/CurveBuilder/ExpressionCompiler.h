#pragma once
#ifndef ExpressionCompiler_H_536C0F45_91A2_4182_B5BF_1BBCB21BE035
#define ExpressionCompiler_H_536C0F45_91A2_4182_B5BF_1BBCB21BE035

#include "ExpressionParser.h"
#include "Core/IntegerType.h"

#define ENABLE_FPU_CODE 0

#if ENABLE_FPU_CODE
#else
#define ENABLE_BYTE_CODE 1
#include "ExpressionUtils.h"
#endif

#ifndef ENABLE_BYTE_CODE
#define ENABLE_BYTE_CODE 0
#endif

class ExpressionCompiler;

#if ENABLE_FPU_CODE
#include "Backend/FPUCode.h"
#elif ENABLE_BYTE_CODE
#include "Backend/ExprByteCode.h"
#endif

class ExecutableCode
{
public:
	explicit ExecutableCode( int size = 0 );
	~ExecutableCode();

#if ENABLE_FPU_CODE
	static bool constexpr IsSupportSIMD = false;
#elif ENABLE_BYTE_CODE
	static bool constexpr IsSupportSIMD = true;
#else
	static bool constexpr IsSupportSIMD = false;
#endif

	template< typename RT , typename ...TArgs >
	FORCEINLINE RT evalT(TArgs ...args) const
	{
#if ENABLE_FPU_CODE
		using EvalFunc = RT(*)(TArgs...);
		return reinterpret_cast<EvalFunc>(&mData.mCode[0])(args...);
#elif ENABLE_BYTE_CODE
		TByteCodeExecutor<RT> executor;
		return executor.execute(mData, args...);
#else
		return FExpressUtils::template EvalutePosfixCodes<RT>(mCodes, args...);
#endif
	}

#if ENABLE_BYTE_CODE && EBC_USE_VALUE_BUFFER
	template< typename T >
	void initValueBuffer(T buffer[])
	{
		mData.initValueBuffer(buffer);
	}

	template< typename RT >
	FORCEINLINE RT evalT(TArrayView< RT > valueBuffer) const
	{
		TByteCodeExecutor<RT> executor;
		return executor.execute(mData, valueBuffer);
	}
#endif

	void   clearCode();
	int    getCodeLength() const 
	{ 
#if ENABLE_FPU_CODE
		return mData.getCodeLength();
#elif ENABLE_BYTE_CODE
		return mData.codes.size();
#else
		return mCodes.size();
#endif
	}

protected:

#if ENABLE_FPU_CODE
	FPUCodeData mData;
#elif ENABLE_BYTE_CODE
public:
	ExprByteCodeExecData mData;
#else
public:
	TArray<ExprParse::Unit> mCodes;
#endif
};

class ExpressionCompiler
{
public:
	ExpressionCompiler();
	bool compile( char const* expr , SymbolTable const& table , ExecutableCode& data , int numInput = 0 , ValueLayout inputLayouts[] = nullptr);
	void enableOpimization(bool enable = true){	mOptimization = enable;	}
	bool isUsingVar(char const* varName) const
	{
		return mResult.isUsingVar(varName);
	}
	bool isUsingInput(char const* varName) const
	{
		return mResult.isUsingInput(varName);
	}
protected:
	ParseResult  mResult;
	bool         mOptimization;
};

#endif // ExpressionCompiler_H_536C0F45_91A2_4182_B5BF_1BBCB21BE035