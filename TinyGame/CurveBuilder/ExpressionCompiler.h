#ifndef FPUCompiler_H
#define FPUCompiler_H

#include "ExpressionParser.h"
#include "Core/IntegerType.h"

#include "Math/SIMD.h"
#define SIMD_USE_AVX 0
#if SIMD_USE_AVX
using FloatVector = SIMD::TFloatVector<8>;
#else
using FloatVector = SIMD::TFloatVector<4>;
#endif


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

class ExecutableCode;

template<typename TValue>
class TByteCodeExecutor
{
public:
	TValue doExecute(ExecutableCode const& code);
	int  execCode(uint8 const* pCode, TValue*& pValueStack, TValue& topValue);

	ExecutableCode const* mCode;
	TArrayView<TValue const> mInputs;
};

class ByteCodeExecutor : public TByteCodeExecutor<RealType>
{
public:

	template< typename RT>
	RT execute(ExecutableCode const& code)
	{
		return RT(doExecute(code));
	}

	template< typename RT, class ...Args >
	RT execute(ExecutableCode const& code, Args ...args)
	{
		RealType inputs[] = { args... };
		mInputs = inputs;
		return RT(doExecute(code));
	}
};

class ByteCodeExecutorSIMD : public TByteCodeExecutor<FloatVector>
{
public:
	template< class ...Args >
	FloatVector execute(ExecutableCode const& code, Args ...args)
	{
		FloatVector inputs[] = { args... };
		mInputs = inputs;
		return doExecute(code);
	}
};
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

	template< class RT , class ...Args >
	FORCEINLINE RT evalT(Args ...args) const
	{
		if constexpr (std::is_same_v< RT, FloatVector >)
		{
#if ENABLE_BYTE_CODE
			ByteCodeExecutorSIMD executor;
			return executor.execute(*this, args...);
#else
			return 0.0f;
#endif
		}
		else
		{
#if ENABLE_FPU_CODE
			using EvalFunc = RT(*)(Args...);
			return reinterpret_cast<EvalFunc>(&mCode[0])(args...);
#elif ENABLE_BYTE_CODE
			ByteCodeExecutor executor;
			return executor.execute<RT>(*this, args...);
#else
			return FExpressUtils::template EvalutePosfixCodes<RT>(mCodes, args...);
#endif
		}
	}

	void   printCode();
	void   clearCode();
	int    getCodeLength() const 
	{ 
#if ENABLE_FPU_CODE
		return int(mCodeEnd - mCode); 
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
	TArray<uint8>    mCodes;
	TArray<void*>    mPointers;
	TArray<RealType> mConstValues;
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