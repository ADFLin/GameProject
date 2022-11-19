#ifndef ExpressionParser_H
#define ExpressionParser_H

#include "Template\ArrayView.h"

#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <cassert>



using std::string;

class CodeTemplate;
class SymbolTable;
struct SymbolEntry;


using RealType = double;

enum EFundamentalType
{


};

struct TypeLayout
{

	bool bPointer;

};
enum class ValueLayout
{
	Int32  = 0,
	Double,
	Float,

	PointerStart ,

	Int32Ptr = PointerStart,
	DoublePtr,
	FloatPtr,

	Real = sizeof(RealType) == sizeof(double) ? Double : Float,
	RealPtr = sizeof(RealType) == sizeof(double) ? DoublePtr : FloatPtr,
};

inline bool IsPointer(ValueLayout layout)
{
	return layout >= ValueLayout::PointerStart;
}

inline ValueLayout ToPointer(ValueLayout layout)
{
	assert(!IsPointer(layout));
	return  ValueLayout(int(layout) + int(ValueLayout::PointerStart) );
}

inline ValueLayout ToBase(ValueLayout layout)
{
	assert(IsPointer(layout));
	return  ValueLayout(int(layout) - int(ValueLayout::PointerStart) );
}

inline uint32 GetLayoutSize(ValueLayout layout)
{
	switch( layout )
	{
	case ValueLayout::Int32: return sizeof(int32);
	case ValueLayout::Double: return sizeof(double);
	case ValueLayout::Float: return sizeof(float);
	case ValueLayout::DoublePtr:
	case ValueLayout::Int32Ptr:
	case ValueLayout::FloatPtr:
		return sizeof(void*);
	default:
		break;
	}
	return 0;
}

template< class T >
struct TTypeToValueLayout {};
template<>
struct TTypeToValueLayout< double >
{
	static const ValueLayout Result = ValueLayout::Double;
};
template<>
struct TTypeToValueLayout< float >
{
	static const ValueLayout Result = ValueLayout::Float;
};
template<>
struct TTypeToValueLayout< int32 >
{
	static const ValueLayout Result = ValueLayout::Int32;
};

template<>
struct TTypeToValueLayout< double* >
{
	static const ValueLayout Result = ValueLayout::DoublePtr;
};
template<>
struct TTypeToValueLayout< float* >
{
	static const ValueLayout Result = ValueLayout::FloatPtr;
};
template<>
struct TTypeToValueLayout< int32* >
{
	static const ValueLayout Result = ValueLayout::Int32Ptr;
};


enum class EFuncCall
{
	Cdecl,
	Thiscall,
	Clrcall ,
	Stdcall ,
	Fastcall,
};

struct FuncSignatureInfo
{
	TArrayView< ValueLayout const > argTypes;
	ValueLayout returnType;
	EFuncCall   call;

	FuncSignatureInfo(ValueLayout inRTLayout, ValueLayout const* inArgs, int numArgs)
		:argTypes(inArgs, numArgs)
		,returnType(inRTLayout)
		,call( EFuncCall::Cdecl )
	{
	}
};

template< class T >
struct TGetFuncSignature
{

};

template< class RT >
struct TGetFuncSignature< RT (*)() >
{
	static FuncSignatureInfo const& Result()
	{
		static FuncSignatureInfo sResult{ TTypeToValueLayout<RT>::Result , nullptr , 0 };
		return sResult;
	}
};

template< class RT, class ...Args >
struct TGetFuncSignature< RT (*)(Args...) >
{
	static FuncSignatureInfo const& Result()
	{
		static ValueLayout const sArgs[] = { TTypeToValueLayout<Args>::Result... };
		static FuncSignatureInfo sResult{ TTypeToValueLayout<RT>::Result , sArgs , sizeof...(Args) };
		return sResult;
	}
};


struct FuncInfo
{
	void* funcPtr;
	FuncSignatureInfo const* signature;
	template< class T >
	FuncInfo(T func)
		:funcPtr(func)
		,signature( &TGetFuncSignature<T>::Result() )
	{

	}


	int         getArgNum() const { return signature->argTypes.size(); }
	ValueLayout getArgType(int idx) { return signature->argTypes[idx]; }
	ValueLayout getReturnType() { return signature->returnType; }

	bool isSameLayout(FuncInfo const& rhs) const
	{
		return signature == rhs.signature;
	}

	bool operator == ( FuncInfo const& rhs ) const
	{
		return (funcPtr == rhs.funcPtr);
	}

};

struct ConstValueInfo
{
	ValueLayout layout;
	union 
	{
		double   asDouble;
		float    asFloat;
		int32    asInt32;
		RealType asReal;
	};

	ConstValueInfo(){}
	ConstValueInfo(double value):layout(ValueLayout::Double), asDouble(value){}
	ConstValueInfo(float  value):layout(ValueLayout::Float), asFloat(value) {}
	ConstValueInfo(int32  value):layout(ValueLayout::Int32), asInt32(value) {}

	bool operator == (ConstValueInfo const& rhs ) const
	{
		if( layout != rhs.layout )
			return false;

		switch( layout )
		{
		case ValueLayout::Int32:  return asInt32 == rhs.asInt32;
		case ValueLayout::Double: return asDouble == rhs.asDouble;
		case ValueLayout::Float:  return asFloat == rhs.asFloat;
		}

		return true;
	}

	bool isSameValue (void* data) const
	{
		switch( layout )
		{
		case ValueLayout::Int32:  return *(int32*)data == asInt32;
		case ValueLayout::Double: return *(double*)data == asDouble;
		case ValueLayout::Float:  return *(float*)data == asFloat;
		}
		return false;
	}

	void assignTo(void* data) const
	{
		switch( layout )
		{
		case ValueLayout::Int32:  *(int32*)data = asInt32;
		case ValueLayout::Double: *(double*)data = asDouble;
		case ValueLayout::Float:  *(float*)data = asFloat;
		}
	}
};

struct VariableInfo
{
	ValueLayout layout;
	void* ptr;

	VariableInfo(double* pValue) :layout(ValueLayout::Double), ptr(pValue) {}
	VariableInfo(float*  pValue) :layout(ValueLayout::Float), ptr(pValue) {}
	VariableInfo(int32*  pValue) :layout(ValueLayout::Int32), ptr(pValue) {}

	void assignTo(void* data) const
	{
		switch( layout )
		{
		case ValueLayout::Int32:  *(int32*)data = *(int32*)ptr;
		case ValueLayout::Double: *(double*)data = *(double*)ptr;
		case ValueLayout::Float:  *(float*)data = *(float*)ptr;
		}
	}
};

struct InputInfo
{
	uint8       index;
};


using FuncType0 = RealType (__cdecl *)();
using FuncType1 = RealType (__cdecl *)(RealType);
using FuncType2 = RealType (__cdecl *)(RealType,RealType);
using FuncType3 = RealType (__cdecl *)(RealType,RealType,RealType);
using FuncType4 = RealType (__cdecl *)(RealType,RealType,RealType,RealType);
using FuncType5 = RealType (__cdecl *)(RealType,RealType,RealType,RealType,RealType);

class ExprParse
{
public:
	static unsigned const TOKEN_MASK        = 0xff0000;
	static unsigned const SYMBOL_FLAG_BIT   = 0x800000;
	static unsigned const PRECEDENCE_MASK   = 0x001f00;
	static unsigned const REVERSE_BIT       = 0x002000;
	static unsigned const ASSOC_LR_BIT      = 0x004000;
	static unsigned const EXCHANGE_BIT      = 0x008000;

	enum TokenType
	{
		TOKEN_TYPE_ERROR = -1,
		TOKEN_NONE       = 0x000000 ,
		TOKEN_LBAR       = 0x010000 ,
		TOKEN_RBAR       = 0x020000 ,
		TOKEN_BINARY_OP  = 0x030000 ,

		TOKEN_FUNC       = 0x040000 ,
		TOKEN_VALUE      = 0x050000 ,
		TOKEN_UNARY_OP   = 0x060000 ,

		IDT_SEPARETOR    = 0x070001 ,

		BOP_COMMA        = 0x000001 | TOKEN_BINARY_OP ,
		BOP_ASSIGN       = 0x000102 | TOKEN_BINARY_OP | ASSOC_LR_BIT ,
		BOP_BIG          = 0x000203 | TOKEN_BINARY_OP ,
		BOP_BIGEQU       = 0x000204 | TOKEN_BINARY_OP ,
		BOP_SML          = 0x000205 | TOKEN_BINARY_OP ,
		BOP_SMLEQU       = 0x000206 | TOKEN_BINARY_OP ,
		BOP_EQU          = 0x000207 | TOKEN_BINARY_OP ,
		BOP_NEQU         = 0x000208 | TOKEN_BINARY_OP ,
		BOP_ADD          = 0x000309 | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_SUB          = 0x00030a | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_MUL          = 0x00060b | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_DIV          = 0x00060c | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_POW          = 0x00080d | TOKEN_BINARY_OP ,

		UOP_MINS         = 0x000901 | TOKEN_UNARY_OP | ASSOC_LR_BIT ,
		UOP_PLUS         = 0x000902 | TOKEN_UNARY_OP | ASSOC_LR_BIT ,
		UOP_INC_PRE      = 0x000903 | TOKEN_UNARY_OP ,
		UOP_DEC_PRE      = 0x000904 | TOKEN_UNARY_OP ,

		FUNC_DEF         = 0x000a01 | TOKEN_FUNC ,

		VALUE_CONST      = 0x000001 | TOKEN_VALUE ,
		VALUE_VARIABLE   = 0x000002 | TOKEN_VALUE ,
		VALUE_INPUT      = 0x000003 | TOKEN_VALUE ,
	};

	inline static int  PrecedeceOrder( TokenType token ){ return token & PRECEDENCE_MASK; }
	inline static bool IsFunction(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_FUNC; }
	inline static bool IsBinaryOperator(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_BINARY_OP; }
	inline static bool IsValue(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_VALUE; }
	inline static bool IsUnaryOperator(TokenType token){ return ( token & TOKEN_MASK ) == TOKEN_UNARY_OP; }
	inline static bool IsOperator(TokenType token){ return IsUnaryOperator(token) || IsBinaryOperator(token); }

	inline static bool CanReverse( TokenType token ){ assert( IsBinaryOperator( token ) ); return !!( token & REVERSE_BIT ); }
	inline static bool CanExchange( TokenType token ){ assert( IsBinaryOperator( token ) ); return !!( token & EXCHANGE_BIT ); }
	inline static bool IsAssocLR( TokenType token ){ assert( IsOperator( token ) ); return !!( token & ASSOC_LR_BIT ); }
	
	struct Unit
	{
		Unit(){}
		Unit(TokenType type)
			:type(type),isReverse(false){}
		Unit(TokenType type, ConstValueInfo val)
			:type(type),constValue(val){}
		Unit(TokenType type,SymbolEntry const* symbol)
			:type(type), symbol(symbol){}


		TokenType    type;
		union
		{
			SymbolEntry const* symbol;
			bool            isReverse;
			ConstValueInfo  constValue;
		};
	};

	
	using UnitCodes = std::vector<Unit>;
	static void print( Unit const& unit , SymbolTable const& table );
	static void print( UnitCodes const& codes , SymbolTable const& table , bool haveBracket );


	enum
	{
		CN_LEFT  = 0,
		CN_RIGHT = 1,
	};

	static int LEAF_UNIT_INDEX( int idxNode ){ return -( idxNode + 1 ); }

	struct Node
	{
		Unit*    opUnit;
		int      parent;
		intptr_t children[2];

		int   getLeaf( int idxChild )
		{ 
			assert( children[ idxChild ] < 0 );
			return LEAF_UNIT_INDEX( children[ idxChild ] ); 
		}
	};
	using NodeVec = std::vector< Node >;
};


struct SymbolEntry
{
	enum Type
	{
		eFunction,
		eConstValue,
		eVariable,
		eInputVar,
	};
	Type type;

	union
	{
		InputInfo      input;
		FuncInfo       func;
		ConstValueInfo constValue;
		VariableInfo   varValue;
	};

	SymbolEntry() {}
	SymbolEntry(FuncInfo const& funInfo)
		:type(eFunction)
		,func(funInfo)
	{
	}

	SymbolEntry(ConstValueInfo value)
		:type(eConstValue)
		,constValue(value)
	{
	}
	SymbolEntry(VariableInfo value)
		:type(eVariable)
		,varValue(value)
	{

	}

	SymbolEntry(InputInfo value)
		:type(eInputVar)
		,input(value)
	{
	}
};

class SymbolTable
{
public:
	// can redefine
	template< class T >
	void            defineConst(char const* name, T val) { mNameToEntryMap[name] = ConstValueInfo(val); }
	template< class T >
	void            defineVar( char const* name , T* varPtr){  mNameToEntryMap[name] = VariableInfo(varPtr); }
	template< class T>
	void            defineFunc(char const* name, T func ) { mNameToEntryMap[name] = FuncInfo(func); }

	void            defineVarInput(char const* name, uint8 inputIndex) { mNameToEntryMap[name] = InputInfo{ inputIndex }; }
	
	ConstValueInfo const* findConst(char const* name) const;
	FuncInfo const*       findFunc(char const* name) const;
	VariableInfo const*   findVar(char const* name) const;
	InputInfo const*      findInput(char const* name) const;

	char const*     getFuncName( FuncInfo const& info ) const;
	char const*     getVarName( void* var ) const;
	char const*     getInputName(int index) const;

	bool            isFuncDefined(char const* name) const{  return isDefinedInternal( name , SymbolEntry::eFunction ); }
	bool            isVarDefined(char const* name) const{ return isDefinedInternal(name, SymbolEntry::eVariable ); }
	bool            isConstDefined(char const* name) const{ return isDefinedInternal(name, SymbolEntry::eConstValue); }

	int             getVarTable( char const* varStr[], double varVal[] ) const;


	SymbolEntry const* findSymbol(char const* name) const
	{
		auto iter = mNameToEntryMap.find(name);
		if( iter != mNameToEntryMap.end() )
			return &iter->second;
			
		return nullptr;
	}
	SymbolEntry const* findSymbol(char const* name, SymbolEntry::Type type) const
	{
		auto iter = mNameToEntryMap.find(name);
		if( iter != mNameToEntryMap.end() )
		{
			if( iter->second.type == type )
				return &iter->second;
		}
		
		return nullptr;
	}
protected:


	bool  isDefinedInternal(char const* name, SymbolEntry::Type type) const
	{
		return findSymbol(name, type) != nullptr;
	}
	std::map< HashString, SymbolEntry > mNameToEntryMap;

};


enum EExprErrorCode
{
	eExprError  ,
	eAllocFailed ,
	eGenerateCodeFailed ,
};

class ExprParseException : public std::exception
{
public:
	ExprParseException( EExprErrorCode code , char const* what )
		:std::exception( what )
		,errorCode( code ){}
	EExprErrorCode errorCode;
};

class ExprTreeBuilder : public ExprParse
{
	using Unit = ExprParse::Unit;
public:

	void build( NodeVec& nodes , Unit* exprCode , int numUnit ) /*throw ParseException */;
	void convertPostfixCode( UnitCodes& codes );
	enum ErrorCode
	{
		TREE_NO_ERROR = 0 ,
		TREE_OP_NO_CHILD ,
		TREE_ASSIGN_LEFT_NOT_VAR ,
		TREE_FUN_PARAM_NUM_NO_MATCH ,
	};
	ErrorCode checkTreeError();

	void  optimizeNodeOrder();
	bool  optimizeNodeConstValue(int idxNode );
	bool  optimizeNodeBOpOrder( int idxNode );

	void  printTree( SymbolTable const& table );
private:

	Node& getNode( int idx ){ return mTreeNodes[ idx ]; }
	void  printSpace( int num );
	void  printTree_R( int idxNode , int depth );
	int   buildTree_R( int idxParent , int idxStart , int idxEnd , bool funDef );
	void  convertPostfixCode_R( UnitCodes& codes , int idxNode );
	ErrorCode   checkTreeError_R( int idxNode );
	int   optimizeNodeOrder_R( int idxNode );

	bool  canExchangeNode( int idxNode , TokenType type )
	{
		assert( idxNode < 0 );
		assert( CanExchange( type ) );
		Unit& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];

		if ( !IsBinaryOperator( unit.type ) )
			return false;
		if ( PrecedeceOrder( type ) != PrecedeceOrder( unit.type ) )
			return false;

		return true;
	}

	unsigned haveConstValueChild( int idxNode )
	{
		assert( idxNode > 0 );
		Node& node = mTreeNodes[ idxNode ];
		int result = 0;
		if ( isConstValueNode( node.children[ CN_LEFT ] ) )
			result |= ( 1 << CN_LEFT );
		if ( isConstValueNode( node.children[ CN_RIGHT ] ) )
			result |= ( 1 << CN_RIGHT );
		return result;
	}

	bool  isConstValueNode( int idxNode )
	{
		if ( idxNode >= 0 )
			return false;
		Unit& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];
		if ( unit.type != VALUE_CONST )
			return false;
		return true;
	}

	std::vector< int > mIdxOpNext;
	SymbolTable const* mTable;
	Node*    mTreeNodes;
	int      mNumNodes;
	Unit*    mExprCodes;
};

class ParseResult : public ExprParse
{
public:
	using Unit = ExprParse::Unit;

	bool   isUsingVar( char const* name ) const;
	bool   isUsingInput(char const* name) const;
	void   optimize();

	template< class TCodeGenerator >
	void   generateCode(TCodeGenerator& generator, int numInput, ValueLayout inputLayouts[]);
	template< class TCodeGenerator >
	static void  Process(TCodeGenerator& generator, Unit const& unit)
	{
		switch (unit.type)
		{
		case ExprParse::VALUE_CONST:
			generator.codeConstValue(unit.constValue);
			break;
		case ExprParse::VALUE_VARIABLE:
			generator.codeVar(unit.symbol->varValue);
			break;
		case ExprParse::VALUE_INPUT:
			generator.codeInput(unit.symbol->input);
			break;
		default:
			switch (unit.type & TOKEN_MASK)
			{
			case TOKEN_FUNC:
				generator.codeFunction(unit.symbol->func);
				break;
			case TOKEN_UNARY_OP:
				generator.codeUnaryOp(unit.type);
				break;
			case TOKEN_BINARY_OP:
				generator.codeBinaryOp(unit.type, unit.isReverse);
				break;
			}
		}
	}

	UnitCodes const& getInfixCodes() const { return mIFCodes; }
	UnitCodes const& getPostfixCodes() const { return mPFCodes; }

private:

	bool optimizeZeroValue(int index);
	bool optimizeConstValue(int index);
	bool optimizeVarOrder(int index);
	bool optimizeOperatorOrder(int index);
	bool optimizeValueOrder(int index);

	SymbolTable const* mSymbolDefine = nullptr;
	NodeVec      mTreeNodes;
	UnitCodes    mIFCodes;
	UnitCodes    mPFCodes;


	template< typename TCodeGenerator >
	void visitTree(TCodeGenerator& geneartor)
	{
		if (!mTreeNodes.empty())
		{
			Node& root = mTreeNodes[0];
			visitTree_R(geneartor, root.children[CN_LEFT]);
		}
	}

	template< typename TCodeGenerator >
	void visitTree_R(TCodeGenerator& geneartor, int idxNode)
	{
		if (idxNode < 0)
		{
			Process(geneartor, mIFCodes[LEAF_UNIT_INDEX(idxNode)]);
			return;
		}
		else if (idxNode == 0)
			return;

		Node& node = mTreeNodes[idxNode];
		Unit& unit = *node.opUnit;
		if (unit.type == BOP_ASSIGN)
		{
			visitTree_R(geneartor, node.children[CN_RIGHT]);
			visitTree_R(geneartor, node.children[CN_LEFT]);
			Process(geneartor, unit);
		}
		else
		{
			visitTree_R(geneartor, node.children[CN_LEFT]);
			visitTree_R(geneartor, node.children[CN_RIGHT]);
			if (unit.type != IDT_SEPARETOR)
			{
				Process(geneartor, unit);
			}
		}
	}

	friend class ExpressionParser;
public:

	void MoveElement(int start,int end,int move)
	{
		for ( int i = start; i < end; ++i )
			mPFCodes[i+move] = mPFCodes[i];
	}
	void printInfixCodes(){ ExprParse::print( mIFCodes , *mSymbolDefine ,  false ); }
	void printPostfixCodes(){ ExprParse::print( mPFCodes , *mSymbolDefine ,  true ); }

};

class TCodeGenerator
{
public:
	using TokenType = ParseResult::TokenType;
	void codeInit(int numInput, ValueLayout inputLayouts[]);
	void codeConstValue(ConstValueInfo const&val);
	void codeVar( VariableInfo const& varInfo);
	void codeInput( InputInfo const& input );
	void codeFunction(FuncInfo const& info);
	void codeBinaryOp(TokenType type,bool isReverse);
	void codeUnaryOp(TokenType type);
	void codeEnd();
};


class ExpressionParser : public ExprParse
{
public:

	enum
	{

	};

	// test Compile String has used Var(name)
	bool parse( char const* expr , SymbolTable const& table );
	bool parse( char const* expr , SymbolTable const& table , ParseResult& result );

	std::string const& getErrorMsg(){ return mErrorMsg; }

protected:

	bool analyzeTokenUnit( char const* expr , SymbolTable const& table , UnitCodes& infixCode );
	void convertCode( UnitCodes& infixCode , UnitCodes& postfixCode );
	bool testTokenValid(int bToken,int cToken);

	std::string  mErrorMsg;
};



template< class TCodeGenerator >
void ParseResult::generateCode( TCodeGenerator& generator , int numInput, ValueLayout inputLayouts[] )
{
	generator.codeInit(numInput , inputLayouts);
	for ( Unit const& unit : mPFCodes )
	{
		Process(generator, unit);
	}
	generator.codeEnd();
}


#endif //ExpressionParser_H
