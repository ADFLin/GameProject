#ifndef ExpressionCore_h__
#define ExpressionCore_h__

#include "Template/ArrayView.h"
#include "DataStructure/Array.h"
#include "Meta/Concept.h"

#include <string>
#include <cassert>

#include "Math/SIMD.h"

#define SIMD_USE_AVX 0
#if SIMD_USE_AVX
using FloatVector = SIMD::TFloatVector<8>;
#else
using FloatVector = SIMD::TFloatVector<4>;
#endif

using RealType = float;

enum EExprErrorCode
{
	eExprError,
	eAllocFailed,
	eGenerateCodeFailed,
};

class ExprParseException : public std::exception
{
public:
	ExprParseException(EExprErrorCode code, char const* what)
		:std::exception(what)
		, errorCode(code) {}
	EExprErrorCode errorCode;
};

enum class ValueLayout
{
	Int32 = 0,
	Double,
	Float,

	PointerStart,

	Int32Ptr = PointerStart,
	DoublePtr,
	FloatPtr,

	Real = sizeof(RealType) == sizeof(double) ? Double : Float,
	RealPtr = sizeof(RealType) == sizeof(double) ? DoublePtr : FloatPtr,
};

inline bool IsPointer(ValueLayout layout) { return layout >= ValueLayout::PointerStart; }
inline ValueLayout ToPointer(ValueLayout layout) { assert(!IsPointer(layout)); return ValueLayout(int(layout) + int(ValueLayout::PointerStart)); }
inline ValueLayout ToBase(ValueLayout layout) { assert(IsPointer(layout)); return ValueLayout(int(layout) - int(ValueLayout::PointerStart)); }

inline uint32 GetLayoutSize(ValueLayout layout)
{
	switch (layout)
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
	if constexpr (Meta::IsSameType< T, double >::Value) return ValueLayout::Double;
	if constexpr (Meta::IsSameType< T, double* >::Value) return ValueLayout::DoublePtr;
	if constexpr (Meta::IsSameType< T, float >::Value) return ValueLayout::Float;
	if constexpr (Meta::IsSameType< T, float* >::Value) return ValueLayout::FloatPtr;
	if constexpr (Meta::IsSameType< T, int32 >::Value) return ValueLayout::Int32;
	if constexpr (Meta::IsSameType< T, int32* >::Value) return ValueLayout::Int32Ptr;
	static_assert("No ValueLayout");
	return ValueLayout::Int32;
}

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

enum class EFuncCall
{
	Cdecl,
	Thiscall,
	Clrcall,
	Stdcall,
	Fastcall,
};

struct FuncSignatureInfo
{
	TArrayView< ValueLayout const > argTypes;
	ValueLayout returnType;
	EFuncCall   call;

	FuncSignatureInfo(ValueLayout inRTLayout, ValueLayout const* inArgs, int numArgs)
		:argTypes(inArgs, numArgs)
		, returnType(inRTLayout)
		, call(EFuncCall::Cdecl)
	{
	}
};

template< class T > struct TGetFuncSignature {};
template< class RT >
struct TGetFuncSignature< RT(*)() >
{
	static FuncSignatureInfo const& Result()
	{
		static FuncSignatureInfo sResult{ GetValueLayout<RT>(), nullptr , 0 };
		return sResult;
	}
};
template< class RT, class ...Args >
struct TGetFuncSignature< RT(*)(Args...) >
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
	FuncInfo(T func) : funcPtr((void*)func), signature(&TGetFuncSignature<T>::Result()) {}

	int         getArgNum() const { return (int)signature->argTypes.size(); }
	ValueLayout getArgType(int idx) { return signature->argTypes[idx]; }
	ValueLayout getReturnType() { return signature->returnType; }

	bool isSameLayout(FuncInfo const& rhs) const { return signature == rhs.signature; }
	bool operator == (FuncInfo const& rhs) const { return (funcPtr == rhs.funcPtr); }
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

	ConstValueInfo() {}
	ConstValueInfo(double value) :layout(ValueLayout::Double), asDouble(value) {}
	ConstValueInfo(float  value) :layout(ValueLayout::Float), asFloat(value) {}
	ConstValueInfo(int32  value) :layout(ValueLayout::Int32), asInt32(value) {}

	bool operator == (ConstValueInfo const& rhs) const
	{
		if (layout != rhs.layout) return false;
		switch (layout)
		{
		case ValueLayout::Int32:  return asInt32 == rhs.asInt32;
		case ValueLayout::Double: return asDouble == rhs.asDouble;
		case ValueLayout::Float:  return asFloat == rhs.asFloat;
		}
		return true;
	}

	bool isSameValue(void* data) const
	{
		switch (layout)
		{
		case ValueLayout::Int32:  return *(int32*)data == asInt32;
		case ValueLayout::Double: return *(double*)data == asDouble;
		case ValueLayout::Float:  return *(float*)data == asFloat;
		}
		return false;
	}

	void assignTo(void* data) const
	{
		switch (layout)
		{
		case ValueLayout::Int32:  *(int32*)data = asInt32; break;
		case ValueLayout::Double: *(double*)data = asDouble; break;
		case ValueLayout::Float:  *(float*)data = asFloat; break;
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
		switch (layout)
		{
		case ValueLayout::Int32:  *(int32*)data = *(int32*)ptr; break;
		case ValueLayout::Double: *(double*)data = *(double*)ptr; break;
		case ValueLayout::Float:  *(float*)data = *(float*)ptr; break;
		}
	}
};

struct InputInfo { uint8 index; };
struct ExprTempInfo { int32 tempIndex; };

struct SymbolEntry;

class ExprParse
{
public:
	static unsigned const TOKEN_MASK = 0xff0000;
	static unsigned const SYMBOL_FLAG_BIT = 0x800000;
	static unsigned const PRECEDENCE_MASK = 0x001f00;
	static unsigned const REVERSE_BIT = 0x002000;
	static unsigned const ASSOC_LR_BIT = 0x004000;
	static unsigned const EXCHANGE_BIT = 0x008000;

#define PRECEDENCE(ORDER) ((ORDER) << 8)
	enum TokenType
	{
		TOKEN_TYPE_ERROR = -1,
		TOKEN_NONE = 0x000000,
		TOKEN_LBAR = 0x010000,
		TOKEN_RBAR = 0x020000,
		TOKEN_BINARY_OP = 0x030000,

		TOKEN_FUNC = 0x040000,
		TOKEN_VALUE = 0x050000,
		TOKEN_UNARY_OP = 0x060000,

		IDT_SEPARETOR = 0x070001,

		BOP_COMMA = 0x000001 | PRECEDENCE(0) | TOKEN_BINARY_OP,
		BOP_ASSIGN = 0x000002 | PRECEDENCE(1) | TOKEN_BINARY_OP | ASSOC_LR_BIT,
		BOP_BIG = 0x000003 | PRECEDENCE(2) | TOKEN_BINARY_OP,
		BOP_BIGEQU = 0x000004 | PRECEDENCE(2) | TOKEN_BINARY_OP,
		BOP_SML = 0x000005 | PRECEDENCE(2) | TOKEN_BINARY_OP,
		BOP_SMLEQU = 0x000006 | PRECEDENCE(2) | TOKEN_BINARY_OP,
		BOP_EQU = 0x000007 | PRECEDENCE(2) | TOKEN_BINARY_OP,
		BOP_NEQU = 0x000008 | PRECEDENCE(2) | TOKEN_BINARY_OP,
		BOP_ADD = 0x000009 | PRECEDENCE(3) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT,
		BOP_SUB = 0x00000a | PRECEDENCE(3) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT,
		BOP_MUL = 0x00000b | PRECEDENCE(6) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT,
		BOP_DIV = 0x00000c | PRECEDENCE(6) | TOKEN_BINARY_OP | REVERSE_BIT | EXCHANGE_BIT,
		BOP_POW = 0x00000d | PRECEDENCE(8) | TOKEN_BINARY_OP,

		UOP_MINS = 0x000001 | PRECEDENCE(9) | TOKEN_UNARY_OP | ASSOC_LR_BIT,
		UOP_PLUS = 0x000002 | PRECEDENCE(9) | TOKEN_UNARY_OP | ASSOC_LR_BIT,
		UOP_INC_PRE = 0x000003 | PRECEDENCE(9) | TOKEN_UNARY_OP,
		UOP_DEC_PRE = 0x000004 | PRECEDENCE(9) | TOKEN_UNARY_OP,

		FUNC_DEF = 0x000001 | PRECEDENCE(10) | TOKEN_FUNC,
		FUNC_SYMBOL = 0x000002 | PRECEDENCE(10) | TOKEN_FUNC,

		VALUE_CONST = 0x000001 | TOKEN_VALUE,
		VALUE_VARIABLE = 0x000002 | TOKEN_VALUE,
		VALUE_INPUT = 0x000003 | TOKEN_VALUE,
		VALUE_EXPR_TEMP = 0x000004 | TOKEN_VALUE,
	};
#undef PRECEDENCE

	inline static int  PrecedeceOrder(TokenType token) { return token & PRECEDENCE_MASK; }
	inline static bool IsFunction(TokenType token) { return (token & TOKEN_MASK) == TOKEN_FUNC; }
	inline static bool IsBinaryOperator(TokenType token) { return (token & TOKEN_MASK) == TOKEN_BINARY_OP; }
	inline static bool IsValue(TokenType token) { return (token & TOKEN_MASK) == TOKEN_VALUE; }
	inline static bool IsUnaryOperator(TokenType token) { return (token & TOKEN_MASK) == TOKEN_UNARY_OP; }
	inline static bool IsOperator(TokenType token) { return IsUnaryOperator(token) || IsBinaryOperator(token); }

	inline static bool CanReverse(TokenType token) { assert(IsBinaryOperator(token)); return !!(token & REVERSE_BIT); }
	inline static bool CanExchange(TokenType token) { assert(IsBinaryOperator(token)); return !!(token & EXCHANGE_BIT); }
	inline static bool IsAssocLR(TokenType token) { assert(IsOperator(token)); return !!(token & ASSOC_LR_BIT); }

	struct Unit
	{
		Unit() : storeIndex(-1) {}
		Unit(TokenType type) :type(type), isReverse(false), storeIndex(-1) {}
		Unit(FuncSymbolInfo funcSymbol) :type(FUNC_SYMBOL), funcSymbol(funcSymbol), storeIndex(-1) {}
		Unit(InputInfo val) :type(VALUE_INPUT), input(val), storeIndex(-1) {}
		Unit(ConstValueInfo val) :type(VALUE_CONST), constValue(val), storeIndex(-1) {}
		Unit(VariableInfo val) :type(VALUE_VARIABLE), variable(val), storeIndex(-1) {}
		Unit(ExprTempInfo val) :type(VALUE_EXPR_TEMP), exprTemp(val), storeIndex(-1) {}
		Unit(TokenType type, SymbolEntry const* symbol) :type(type), symbol(symbol), storeIndex(-1) {}

		template< typename T >
		static Unit ConstValue(T value) { return Unit(ConstValueInfo{ value }); }
		static Unit ExprTemp(int32 tempIndex) { return Unit(ExprTempInfo{ tempIndex }); }

		TokenType    type;
		int16        storeIndex = -1;
		union
		{
			SymbolEntry const* symbol;
			bool            isReverse;
			ConstValueInfo  constValue;
			VariableInfo    variable;
			FuncSymbolInfo  funcSymbol;
			InputInfo       input;
			ExprTempInfo    exprTemp;
		};
	};

	using UnitCodes = TArray<Unit>;
	static void Print(class ExprOutputContext& context, UnitCodes const& codes, bool haveBracket);
	static bool IsValueEqual(Unit const& a, Unit const& b);

	enum { CN_LEFT = 0, CN_RIGHT = 1 };
	static int IsLeaf(int idxNode) { return idxNode < 0; }
	static int LEAF_UNIT_INDEX(int idxNode) { return -(idxNode + 1); }

	struct Node
	{
		int  indexOp;
		int  children[2];

		int   getLeaf(int idxChild) const
		{
			CHECK(IsLeaf(children[idxChild]));
			return LEAF_UNIT_INDEX(children[idxChild]);
		}
	};
	using NodeVec = TArray< Node >;
};

using FuncType0 = RealType(__cdecl *)();
using FuncType1 = RealType(__cdecl *)(RealType);
using FuncType2 = RealType(__cdecl *)(RealType, RealType);
using FuncType3 = RealType(__cdecl *)(RealType, RealType, RealType);
using FuncType4 = RealType(__cdecl *)(RealType, RealType, RealType, RealType);
using FuncType5 = RealType(__cdecl *)(RealType, RealType, RealType, RealType, RealType);


#endif // ExpressionCore_h__
