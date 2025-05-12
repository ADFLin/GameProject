#ifndef ExpressionParser_H
#define ExpressionParser_H

#include "Template\ArrayView.h"
#include "DataStructure\Array.h"

#include <string>
#include <unordered_map>
#include <cmath>
#include <cassert>

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


template< typename T >
constexpr ValueLayout GetValueLayout()
{
	if constexpr (Meta::IsSameType< T, double >::Value)
	{
		return ValueLayout::Double;
	}
	if constexpr (Meta::IsSameType< T, double* >::Value)
	{
		return ValueLayout::DoublePtr;
	}
	if constexpr (Meta::IsSameType< T, float >::Value)
	{
		return ValueLayout::Float;
	}
	if constexpr (Meta::IsSameType< T, float* >::Value)
	{
		return ValueLayout::FloatPtr;
	}
	if constexpr (Meta::IsSameType< T, int32 >::Value)
	{
		return ValueLayout::Int32;
	}
	if constexpr (Meta::IsSameType< T, int32* >::Value)
	{
		return ValueLayout::Int32Ptr;
	}

	static_assert("No ValueLayout");
	return ValueLayout::Int32;
}

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
		static FuncSignatureInfo sResult{ GetValueLayout<RT>(), nullptr , 0 };
		return sResult;
	}
};

template< class RT, class ...Args >
struct TGetFuncSignature< RT (*)(Args...) >
{
	static FuncSignatureInfo const& Result()
	{
		static ValueLayout const sArgs[] = { GetValueLayout<Args>()... };
		static FuncSignatureInfo sResult{ GetValueLayout<RT>() , sArgs , sizeof...(Args) };
		return sResult;
	}
};


struct FuncSymbolInfo
{
	int32 id;
	int32 numArgs;
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

	int         getArgNum() const { return (int)signature->argTypes.size(); }
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

class ExprOutputContext;
class ExprParse
{
public:
	static unsigned const TOKEN_MASK        = 0xff0000;
	static unsigned const SYMBOL_FLAG_BIT   = 0x800000;
	static unsigned const PRECEDENCE_MASK   = 0x001f00;
	static unsigned const REVERSE_BIT       = 0x002000;
	static unsigned const ASSOC_LR_BIT      = 0x004000;
	static unsigned const EXCHANGE_BIT      = 0x008000;

	#define PRECEDENCE(ORDER) ((ORDER) << 8)

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

		BOP_COMMA        = 0x000001 | PRECEDENCE(0) | TOKEN_BINARY_OP ,
		BOP_ASSIGN       = 0x000002 | PRECEDENCE(1) | TOKEN_BINARY_OP | ASSOC_LR_BIT ,
		BOP_BIG          = 0x000003 | PRECEDENCE(2) | TOKEN_BINARY_OP ,
		BOP_BIGEQU       = 0x000004 | PRECEDENCE(2) | TOKEN_BINARY_OP ,
		BOP_SML          = 0x000005 | PRECEDENCE(2) | TOKEN_BINARY_OP ,
		BOP_SMLEQU       = 0x000006 | PRECEDENCE(2) | TOKEN_BINARY_OP ,
		BOP_EQU          = 0x000007 | PRECEDENCE(2) | TOKEN_BINARY_OP ,
		BOP_NEQU         = 0x000008 | PRECEDENCE(2) | TOKEN_BINARY_OP ,
		BOP_ADD          = 0x000009 | PRECEDENCE(3) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_SUB          = 0x00000a | PRECEDENCE(3) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_MUL          = 0x00000b | PRECEDENCE(6) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_DIV          = 0x00000c | PRECEDENCE(6) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT ,
		BOP_POW          = 0x00000d | PRECEDENCE(8) | TOKEN_BINARY_OP ,

		UOP_MINS         = 0x000001 | PRECEDENCE(9) | TOKEN_UNARY_OP | ASSOC_LR_BIT ,
		UOP_PLUS         = 0x000002 | PRECEDENCE(9) | TOKEN_UNARY_OP | ASSOC_LR_BIT ,
		UOP_INC_PRE      = 0x000003 | PRECEDENCE(9) | TOKEN_UNARY_OP ,
		UOP_DEC_PRE      = 0x000004 | PRECEDENCE(9) | TOKEN_UNARY_OP ,

		FUNC_DEF         = 0x000001 | PRECEDENCE(10) | TOKEN_FUNC ,
		FUNC_SYMBOL      = 0x000002 | PRECEDENCE(10) | TOKEN_FUNC,

		VALUE_CONST      = 0x000001 | TOKEN_VALUE ,
		VALUE_VARIABLE   = 0x000002 | TOKEN_VALUE ,
		VALUE_INPUT      = 0x000003 | TOKEN_VALUE ,
	};

	#undef PRECEDENCE
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

		Unit(FuncSymbolInfo funcSymbol)
			:type(FUNC_SYMBOL), funcSymbol(funcSymbol){}

		Unit(InputInfo val)
			:type(VALUE_INPUT), input(val) {}
		Unit(ConstValueInfo val)
			:type(VALUE_CONST), constValue(val) {}
		Unit(VariableInfo val)
			:type(VALUE_VARIABLE), variable(val) {}

		Unit(TokenType type, SymbolEntry const* symbol)
			:type(type), symbol(symbol) {}

		template< typename T >
		static Unit ConstValue(T value)
		{
			return Unit(ConstValueInfo{ value });
		}


		TokenType    type;
		union
		{
			SymbolEntry const* symbol;
			bool            isReverse;
			ConstValueInfo  constValue;
			VariableInfo    variable;
			FuncSymbolInfo  funcSymbol;
			InputInfo       input;
		};
	};

	
	using UnitCodes = TArray<Unit>;
	static void Print(ExprOutputContext& context, UnitCodes const& codes, bool haveBracket);


	enum
	{
		CN_LEFT  = 0,
		CN_RIGHT = 1,
	};
	static int IsLeaf(int idxNode) { return idxNode < 0; }
	static int LEAF_UNIT_INDEX( int idxNode ){ return -( idxNode + 1 ); }

	struct Node
	{
		int  indexOp;
		int  children[2];

		int   getLeaf( int idxChild ) const
		{ 
			CHECK(IsLeaf(children[idxChild]));
			return LEAF_UNIT_INDEX( children[ idxChild ] ); 
		}
	};
	using NodeVec = TArray< Node >;

	template< class TCodeGenerator >
	static void  Process(TCodeGenerator& generator, Unit const& unit)
	{
		switch (unit.type)
		{
		case ExprParse::VALUE_CONST:
			generator.codeConstValue(unit.constValue);
			break;
		case ExprParse::VALUE_VARIABLE:
			generator.codeVar(unit.variable);
			break;
		case ExprParse::VALUE_INPUT:
			generator.codeInput(unit.input);
			break;
		case ExprParse::FUNC_DEF:
			generator.codeFunction(unit.symbol->func);
			break;
		case ExprParse::FUNC_SYMBOL:
			generator.codeFunction(unit.funcSymbol);
		default:
			switch (unit.type & TOKEN_MASK)
			{
			case TOKEN_UNARY_OP:
				generator.codeUnaryOp(unit.type);
				break;
			case TOKEN_BINARY_OP:
				generator.codeBinaryOp(unit.type, unit.isReverse);
				break;
			}
		}
	}


	static bool IsValueEqual(Unit const& a, Unit const& b);
};


class ExprOutputContext : ExprParse
{
public:
	ExprOutputContext(SymbolTable const& table);
	ExprOutputContext(SymbolTable const& table, std::ostream& stream)
		:table(table)
		,mStream(stream)
	{
	}


	char funcArgSeparetor = ',';
	std::ostream& mStream;
	SymbolTable const& table;

	void output(char c);
	void output(char const* str);
	void output(Unit const& unit);
	void outputSpace(int num);
	void outputEOL();
};


#define DEFAULT_FUNC_SYMBOL_LIST(op)\
		op(Exp, "exp", 1)\
		op(Ln, "ln", 1)\
		op(Sin, "sin", 1)\
		op(Cos, "cos", 1)\
		op(Tan, "tan", 1)\
		op(Cot, "cot", 1)\
		op(Sec, "sec", 1)\
		op(Csc, "csc", 1)\
		op(Sqrt, "sqrt", 1)

enum EFuncSymbol
{
#define ENUM_OP(SYMBOL, NAME, NUM_ARG) SYMBOL,
	DEFAULT_FUNC_SYMBOL_LIST(ENUM_OP)
#undef ENUM_OP
	COUNT,
};


struct SymbolEntry
{
	enum Type
	{
		eFunction,
		eFunctionSymbol,
		eConstValue,
		eVariable,
		eInputVar,
	};
	Type type;

	union
	{
		InputInfo      input;
		FuncInfo       func;
		FuncSymbolInfo funcSymbol;
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

	SymbolEntry(FuncSymbolInfo value)
		:type(eFunctionSymbol)
		, funcSymbol(value)
	{
	}
};

class SymbolTable
{
public:
	// can redefine
	template< class T >
	void            defineConst(char const* name, T val) { mNameToEntryMap[name] = ConstValueInfo{ val }; }
	template< class T >
	void            defineVar(char const* name, T* varPtr) { mNameToEntryMap[name] = VariableInfo{ varPtr }; }
	template< class T>
	void            defineFunc(char const* name, T func) { mNameToEntryMap[name] = FuncInfo{ func }; }

	void            defineFuncSymbol(char const* name, int32 symbolId, int32 numParams = 1) { mNameToEntryMap[name] = FuncSymbolInfo{ symbolId, numParams }; }

	void            defineVarInput(char const* name, uint8 inputIndex) { mNameToEntryMap[name] = InputInfo{ inputIndex }; }
	
	ConstValueInfo const* findConst(char const* name) const;
	FuncInfo const*       findFunc(char const* name) const;
	VariableInfo const*   findVar(char const* name) const;
	InputInfo const*      findInput(char const* name) const;

	char const*     getFuncName(FuncInfo const& info) const;
	char const*     getFuncName(FuncSymbolInfo const& info) const;
	char const*     getVarName(void* varPtr) const;
	char const*     getInputName(int index) const;

	bool            isFuncDefined(char const* name) const{  return isDefinedInternal(name , SymbolEntry::eFunction ); }
	bool            isVarDefined(char const* name) const{ return isDefinedInternal(name, SymbolEntry::eVariable ); }
	bool            isConstDefined(char const* name) const{ return isDefinedInternal(name, SymbolEntry::eConstValue); }

	int             getVarTable( char const* varStr[], double varVal[] ) const;


	SymbolEntry const* findSymbol(char const* name) const
	{
		HashString key;
		if (!HashString::Find(name, key))
			return nullptr;

		auto iter = mNameToEntryMap.find(key);
		if( iter != mNameToEntryMap.end() )
			return &iter->second;
			
		return nullptr;
	}
	SymbolEntry const* findSymbol(char const* name, SymbolEntry::Type type) const
	{
		HashString key;
		if (!HashString::Find(name, key))
			return nullptr;

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
	std::unordered_map< HashString, SymbolEntry > mNameToEntryMap;

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



struct ExpressionTreeData : public ExprParse
{
	NodeVec      nodes;
	UnitCodes    codes;


	template< typename TFunc >
	int  addOpCode(Unit const& code, TFunc Func)
	{
		int indexCode = codes.size();
		codes.push_back(code);

		int indexNode = nodes.size();
		Node node;
		node.indexOp = indexCode;
		nodes.push_back(node);

		Func(indexNode);
		return indexNode;
	}

	int  addLeafCode(Unit const& code)
	{
		CHECK(IsValue(code.type));
		int indexOutput = codes.size();
		codes.push_back(code);
		return -(indexOutput + 1);
	}

	Unit const& getLeafCode(int index) const
	{
		CHECK(IsLeaf(index));
		return codes[LEAF_UNIT_INDEX(index)];
	}

	void clear()
	{
		nodes.clear();
		codes.clear();
	}

	void printExpression(SymbolTable const& table);
	std::string getExpressionText(SymbolTable const& table);

	void output_R(class TreeExprOutputContext& context, Node const& parent, int idxNode);
};



template< typename TVisitor >
class ExprTreeVisitOp : ExprParse
{
public:
	ExprTreeVisitOp(ExpressionTreeData& tree, TVisitor& visitor)
		:mTree(tree), mVisitor(visitor) {}



	void execute()
	{
		if (!mTree.nodes.empty())
		{
			Node& root = mTree.nodes[0];
			execute_R(root.children[CN_LEFT]);
		}
	}

	void execute_R(int idxNode)
	{
		if (idxNode == 0)
			return;

		if (idxNode < 0)
		{
			mVisitor.visitValue(mTree.codes[LEAF_UNIT_INDEX(idxNode)]);
			return;
		}

		Node& node = mTree.nodes[idxNode];
		Unit& unit = mTree.codes[node.indexOp];
		if (unit.type == BOP_ASSIGN)
		{
			execute_R(node.children[CN_RIGHT]);
			execute_R(node.children[CN_LEFT]);
			mVisitor.visitOp(unit);
		}
		else
		{
			execute_R(node.children[CN_LEFT]);
			execute_R(node.children[CN_RIGHT]);
			if (unit.type != IDT_SEPARETOR)
			{
				mVisitor.visitOp(unit);
			}
		}
	}

	ExpressionTreeData& mTree;
	TVisitor& mVisitor;
};

#define USE_FIXED_VALUE 0

class ExprEvaluatorBase : ExprParse
{
public:
	void visitValue(Unit const& unit)
	{
	#if USE_FIXED_VALUE
		mValueStack.push_back(getValue(unit));
	#else
		mValueStack.push_back(unit);
	#endif
	}

	void visitOp(Unit const& unit)
	{
		switch (unit.type)
		{
		case TOKEN_FUNC:
			if (unit.type == FUNC_DEF)
			{
				FuncInfo const& funcInfo = unit.symbol->func;
				RealType params[5];
				int   numParam = funcInfo.getArgNum();
				void* funcPtr = funcInfo.funcPtr;

				RealType value;
				switch (numParam)
				{
				case 0:
					value = (*static_cast<FuncType0>(funcPtr))();
					break;
				case 1:
					params[0] = popStack();
					value = (*static_cast<FuncType1>(funcPtr))(params[0]);
					break;
				case 2:
					params[0] = popStack();
					params[1] = popStack();
					value = (*static_cast<FuncType2>(funcPtr))(params[1], params[0]);
					break;
				case 3:
					params[0] = popStack();
					params[1] = popStack();
					params[2] = popStack();
					value = (*static_cast<FuncType3>(funcPtr))(params[2], params[1], params[0]);
					break;
				case 4:
					params[0] = popStack();
					params[1] = popStack();
					params[2] = popStack();
					params[3] = popStack();
					value = (*static_cast<FuncType4>(funcPtr))(params[3], params[2], params[1], params[0]);
					break;
				case 5:
					params[0] = popStack();
					params[1] = popStack();
					params[2] = popStack();
					params[3] = popStack();
					params[4] = popStack();
					value = (*static_cast<FuncType5>(funcPtr))(params[4], params[3], params[2], params[1], params[0]);
					break;
				}
				pushStack(value);
			}
			else
			{
				switch (unit.funcSymbol.id)
				{
				case EFuncSymbol::Exp:  pushStack(exp(popStack())); break;
				case EFuncSymbol::Ln:   pushStack(log(popStack())); break;
				case EFuncSymbol::Sin:  pushStack(sin(popStack())); break;
				case EFuncSymbol::Cos:  pushStack(cos(popStack())); break;
				case EFuncSymbol::Tan:  pushStack(tan(popStack())); break;
				case EFuncSymbol::Cot:  pushStack(1.0 / tan(popStack())); break;
				case EFuncSymbol::Sec:  pushStack(1.0 / cos(popStack())); break;
				case EFuncSymbol::Csc:  pushStack(1.0 / sin(popStack())); break;
				case EFuncSymbol::Sqrt: pushStack(sqrt(popStack())); break;
				}
			}
			break;
		case TOKEN_UNARY_OP:
			{
				RealType lhs = popStack();
				switch (unit.type)
				{
				case UOP_MINS: pushStack(-lhs); break;
				}
			}
			break;
		case TOKEN_BINARY_OP:
		{
			RealType rhs = popStack();
			RealType lhs = popStack();

			switch (unit.type)
			{
			case BOP_ADD: pushStack(lhs + rhs); break;
			case BOP_SUB: if (unit.isReverse) pushStack(rhs - lhs); else pushStack(lhs - rhs); break;
			case BOP_MUL: pushStack(lhs * rhs); break;
			case BOP_DIV: if (unit.isReverse) pushStack(rhs / lhs); else pushStack(lhs / rhs); break;
			}
		}
		break;
		}
	}

	void exec(Unit const& unit)
	{
		switch (unit.type)
		{
		case VALUE_CONST:
		case VALUE_INPUT:
		case VALUE_VARIABLE:
			visitValue(unit);
			break;
		case FUNC_DEF:
			{
				FuncInfo const& funcInfo = unit.symbol->func;
				RealType params[5];
				int   numParam = funcInfo.getArgNum();
				void* funcPtr = funcInfo.funcPtr;

				RealType value;
				switch (numParam)
				{
				case 0:
					value = (*static_cast<FuncType0>(funcPtr))();
					break;
				case 1:
					params[0] = popStack();
					value = (*static_cast<FuncType1>(funcPtr))(params[0]);
					break;
				case 2:
					params[0] = popStack();
					params[1] = popStack();
					value = (*static_cast<FuncType2>(funcPtr))(params[1], params[0]);
					break;
				case 3:
					params[0] = popStack();
					params[1] = popStack();
					params[2] = popStack();
					value = (*static_cast<FuncType3>(funcPtr))(params[2], params[1], params[0]);
					break;
				case 4:
					params[0] = popStack();
					params[1] = popStack();
					params[2] = popStack();
					params[3] = popStack();
					value = (*static_cast<FuncType4>(funcPtr))(params[3], params[2], params[1], params[0]);
					break;
				case 5:
					params[0] = popStack();
					params[1] = popStack();
					params[2] = popStack();
					params[3] = popStack();
					params[4] = popStack();
					value = (*static_cast<FuncType5>(funcPtr))(params[4], params[3], params[2], params[1], params[0]);
					break;
				}
				pushStack(value);
			}
			break;
		case BOP_ADD: 
			{
				RealType rhs = popStack();
				RealType lhs = popStack();
				pushStack(lhs + rhs);
			}
			break;
		case BOP_SUB: 
			{
				RealType rhs = popStack();
				RealType lhs = popStack();
				if (unit.isReverse) pushStack(rhs - lhs); else pushStack(lhs - rhs); 
			}
			break;
		case BOP_MUL: 
			{
				RealType rhs = popStack();
				RealType lhs = popStack();
				pushStack(lhs * rhs); 
			}
			break;
		case BOP_DIV: 
			{
				RealType rhs = popStack();
				RealType lhs = popStack();
				if (unit.isReverse) pushStack(rhs / lhs); else pushStack(lhs / rhs);
			}
			break;
		case UOP_MINS: 
			{
				RealType lhs = popStack();
				pushStack(-lhs); break;
			}
			break;
		}
	}

	void pushStack(RealType value)
	{
#if USE_FIXED_VALUE
		mValueStack.push_back(value);
#else
		mValueStack.push_back(Unit::ConstValue(value));
#endif
	}

	RealType popStack()
	{
		CHECK(mValueStack.empty() == false);

#if USE_FIXED_VALUE
		RealType result = mValueStack.back();
		mValueStack.pop_back();
#else
		RealType result = getValue(mValueStack.back());
		mValueStack.pop_back();
#endif
		return result;
	}

	RealType getValue(Unit const& valueCode)
	{
		switch (valueCode.type)
		{
		case VALUE_CONST:
			CHECK(valueCode.constValue.layout == ValueLayout::Real);
			return valueCode.constValue.asReal;
		case VALUE_VARIABLE:
			CHECK(valueCode.variable.layout == ValueLayout::Real);
			return *((RealType*)valueCode.variable.ptr);
		case VALUE_INPUT:
			return *((RealType*)mInputs[valueCode.input.index]);
		}
		NEVER_REACH("");
		return 0;
	}

	TArrayView<void*> mInputs;
#if USE_FIXED_VALUE
	TArray<RealType, TInlineAllocator<32> > mValueStack;
#else
	TArray<Unit, TInlineAllocator<32> > mValueStack;
#endif
};

class ExprTreeEvaluator : public ExprEvaluatorBase
{
public:
	template < typename ...Ts >
	RealType eval(ExpressionTreeData& tree, Ts... input)
	{
		void* inputs[] = { &input... };
		mInputs = inputs;
		ExprTreeVisitOp op(tree, *this);
		op.execute();
		return popStack();
	}
};

class ExprTreeBuilder : public ExprParse
{
	using Unit = ExprParse::Unit;
public:

	void build( ExpressionTreeData& treeData ) /*throw ParseException */;
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

	void  printTree_R(ExprOutputContext& context, int idxNode, int depth);
	int   build_R(int idxStart, int idxEnd, bool bFuncDef);
	void  convertPostfixCode_R( UnitCodes& codes , int idxNode );
	ErrorCode   checkTreeError_R( int idxNode );
	int   optimizeNodeOrder_R( int idxNode );

	bool  canExchangeNode( int idxNode , TokenType type )
	{
		CHECK( idxNode < 0 );
		CHECK( CanExchange( type ) );
		Unit const& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];

		if ( !IsBinaryOperator( unit.type ) )
			return false;
		if ( PrecedeceOrder( type ) != PrecedeceOrder( unit.type ) )
			return false;

		return true;
	}

	unsigned haveConstValueChild( int idxNode )
	{
		CHECK( idxNode > 0 );
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
		Unit const& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];
		if ( unit.type != VALUE_CONST )
			return false;
		return true;
	}

	TArray< int > mIdxOpNext;
	Node*  mTreeNodes;
	int    mNumNodes;
	Unit*  mExprCodes;
};

class ParseResult : public ExprParse
{
public:
	using Unit = ExprParse::Unit;

	bool   isUsingVar(char const* name) const;
	bool   isUsingInput(char const* name) const;
	void   optimize();

	template< class TCodeGenerator >
	void   generateCode(TCodeGenerator& generator, int numInput, ValueLayout inputLayouts[]);


	UnitCodes const& getInfixCodes() const { return mTreeData.codes; }
	UnitCodes const& getPostfixCodes() const { return mPFCodes; }

private:

	bool optimizeZeroValue(int index);
	bool optimizeConstValue(int index);
	bool optimizeVarOrder(int index);
	bool optimizeOperatorOrder(int index);
	bool optimizeValueOrder(int index);

	SymbolTable const* mSymbolDefine = nullptr;
	ExpressionTreeData mTreeData;
	UnitCodes    mPFCodes;

	friend class ExpressionParser;
public:

	void MoveElement(int start,int end,int move)
	{
		for ( int i = start; i < end; ++i )
			mPFCodes[i+move] = mPFCodes[i];
	}
	void printInfixCodes() { ExprOutputContext constext(*mSymbolDefine);  ExprParse::Print(constext, mTreeData.codes, false); }
	void printPostfixCodes(){ ExprOutputContext constext(*mSymbolDefine); ExprParse::Print(constext, mPFCodes, true); }

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
	bool parse(char const* expr, SymbolTable const& table);
	bool parse(char const* expr, SymbolTable const& table, ParseResult& result);
	bool parse(char const* expr, SymbolTable const& table, ExpressionTreeData& outTreeData);
	std::string const& getErrorMsg(){ return mErrorMsg; }

protected:

	bool analyzeTokenUnit( char const* expr , SymbolTable const& table , UnitCodes& infixCode );
	void convertCode( UnitCodes& infixCode , UnitCodes& postfixCode );

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
