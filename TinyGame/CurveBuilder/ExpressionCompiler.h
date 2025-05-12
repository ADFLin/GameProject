#ifndef FPUCompiler_H
#define FPUCompiler_H

#include "ExpressionParser.h"
#include "Core/IntegerType.h"

#define ENABLE_FPU_CODE 0

#if ENABLE_FPU_CODE
#else
#define ENALBE_BYTE_CODE 1
#include "ExpressionUtils.h"
#endif


class ExpressionCompiler;
class FPUCodeGeneratorV0;
class FPUCodeGeneratorV1;

#if ENALBE_BYTE_CODE

class ExecutableCode;
class ByteCodeExecutor
{
public:

	template< typename RT>
	RT execute(ExecutableCode const& code)
	{
		ExprEvaluatorBase evaluator;
		return RT(doExecute(code));
	}

	template< typename RT, class ...Args >
	RT execute(ExecutableCode const& code, Args ...args)
	{
		ExprEvaluatorBase evaluator;
		void* inputs[] = { &args... };
		mInputs = inputs;
		return RT(doExecute(code));
	}

	RealType doExecute(ExecutableCode const& code);

#define USE_STACK_INPUT 2

#if USE_STACK_INPUT == 1
	int  execCode(uint8 const* pCode, RealType*& pValueStack);
#elif USE_STACK_INPUT == 2
	int  execCode(uint8 const* pCode, RealType*& pValueStack, RealType& topValue);
#else
	void pushStack(RealType value)
	{
		mValueStack.push_back(value);
	}

	RealType popStack()
	{
		CHECK(mValueStack.empty() == false);
		RealType result = mValueStack.back();
		mValueStack.pop_back();
		return result;
	}
	int  execCode(uint8 const* pCode);
	TArray<RealType, TInlineAllocator<32> > mValueStack;
#endif
	ExecutableCode const* mCode;
	TArrayView<void*> mInputs;
};
#endif

class ExecutableCode
{
public:
	explicit ExecutableCode( int size = 0 );
	~ExecutableCode();

	template< class RT , class ...Args >
	FORCEINLINE RT evalT(Args ...args) const
	{
#if ENABLE_FPU_CODE
		using EvalFunc = RT (*)(Args...);
		return reinterpret_cast<EvalFunc>(&mCode[0])(args...);
#elif ENALBE_BYTE_CODE
		ByteCodeExecutor executor;
		return executor.execute<RT>(*this, args...);
#else
		return FExpressUtils::template EvalutePosfixCodes<RT>(mCodes, args...);
#endif
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
	#if ENALBE_BYTE_CODE
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