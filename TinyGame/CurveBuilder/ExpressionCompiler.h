#ifndef FPUCompiler_H
#define FPUCompiler_H

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
class FPUCodeGeneratorV0;
class FPUCodeGeneratorV1;

#if ENABLE_BYTE_CODE
#include "ExprByteCode.h"
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
		using EvalFunc = RT(*)(Args...);
		return reinterpret_cast<EvalFunc>(&mCode[0])(args...);
#elif ENABLE_BYTE_CODE
		TByteCodeExecutor<RT> executor;
		return executor.execute(mByteCodeData, args...);
#else
		return FExpressUtils::template EvalutePosfixCodes<RT>(mCodes, args...);
#endif
	}

#if ENABLE_BYTE_CODE && EBC_USE_VALUE_BUFFER
	template< typename T >
	void initValueBuffer(T buffer[], int numInput)
	{
		std::copy(mByteCodeData.constValues.begin(), mByteCodeData.constValues.end(), buffer + numInput);

		T* pValue = buffer + numInput + mByteCodeData.constValues.size();
		RealType** pVar = mByteCodeData.vars.data();
		for (int i = mByteCodeData.vars.size(); i ; --i)
		{
			*pValue = **pVar;
			++pValue;
			++pVar;
		}
	}

	template< typename RT >
	FORCEINLINE RT evalT(TArrayView< RT > valueBuffer) const
	{
		TByteCodeExecutor<RT> executor;
		return executor.execute(mByteCodeData, valueBuffer);
	}
#endif
	void   printCode();
	void   clearCode();
	int    getCodeLength() const 
	{ 
#if ENABLE_FPU_CODE
		return int(mCodeEnd - mCode);
#elif ENABLE_BYTE_CODE
		return mByteCodeData.codes.size();
#else
		return mCodes.size();
#endif
	}

protected:

#if ENABLE_FPU_CODE
	void pushCode(uint8 byte);
	void pushCode(uint8 byte1,uint8 byte2);
	void pushCode(uint8 byte1,uint8 byte2,uint8 byte3 );
	void pushCode(uint8 const* data, int size);

	void pushCode(uint32 val){  pushCodeT( val ); }
	void pushCode(uint64 val) { pushCodeT(val); }
	void pushCode(void* ptr){  pushCodeT( ptr ); }
	void pushCode(double value){ pushCodeT( value ); }
	
	void setCode( unsigned pos , void* ptr );
	void setCode( unsigned pos , uint8 byte );

	template < class T >
	void pushCodeT( T value )
	{
		checkCodeSize( sizeof( T ) );
		*reinterpret_cast< T* >( mCodeEnd ) = value;
		mCodeEnd += sizeof( T );
	}
	void   pushCodeInternal(uint8 byte);
	__declspec(noinline)  void  checkCodeSize( int freeSize );
	uint8*  mCode;
	uint8*  mCodeEnd;
	int     mMaxCodeSize;
#if _DEBUG
	int     mNumInstruction;
#endif
	friend class AsmCodeGenerator;
	friend class FPUCodeGeneratorV0;
	friend class FPUCodeGeneratorV1;
	friend class ExpressionCompiler;

#else

public:
#if ENABLE_BYTE_CODE
	ExprByteCodeExecData mByteCodeData;
#else
	TArray<ExprParse::Unit> mCodes;
#endif

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




#endif //FPUCompiler_H