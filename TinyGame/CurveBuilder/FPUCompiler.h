#ifndef FPUCompiler_H
#define FPUCompiler_H

#include "ExpressionParser.h"
#include "Core/IntegerType.h"

class FPUCompiler;
class FPUCodeTemplate;


class FPUCodeData
{
public:
	explicit FPUCodeData( int size = 0 );
	~FPUCodeData();
	double eval() const;
	double eval(double p0) const;
	double eval(double p0, double p1) const;
	double eval(double p0, double p1, double p2) const;

	void   printCode();
	void   clearCode();
	int    getCodeLength() const { return mCodeEnd - mCode; }
	int    getInputNum() const { return mNumInput; }

protected:
	void pushCode(uint8 byte);
	void pushCode(uint8 byte1,uint8 byte2);
	void pushCode(uint8 byte1,uint8 byte2,uint8 byte3 );

	void pushCode( uint32 val){  pushCodeT( val ); }
	void pushCode( void* ptr ){  pushCodeT( ptr ); }
	void pushCode( double value ){ pushCodeT( value ); }
	
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

	int     mNumInput;
	uint8*  mCode;
	uint8*  mCodeEnd;
	int     mMaxCodeSize;
#if _DEBUG
	int     mNumInstruction;
#endif
	friend class FPUCodeTemplate;
	friend class FPUCompiler;
};

class FPUCompiler
{
public:
	FPUCompiler();
	bool compile( char const* expr , SymbolTable const& table , FPUCodeData& data , int numInput = 0);
	void enableOpimization(bool enable = true){	mOptimization = enable;	}

	bool isUsingVar( char const* varName )
	{
		return mResult.isUsingVar( varName );
	}
protected:
	ParseResult  mResult;
	bool         mOptimization;
};




#endif //FPUCompiler_H