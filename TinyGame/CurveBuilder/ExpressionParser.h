#ifndef ExpressionParser_H
#define ExpressionParser_H

#include "ExpressionOptimizer.h"
#include "ExpressionEvaluator.h"

class CodeTemplate;

class ParseResult : public ExprParse
{
public:
	using Unit = ExprParse::Unit;

	bool   isUsingVar(char const* name) const;
	bool   isUsingInput(char const* name) const;

	template< class TCodeGenerator >
	void   generateCode(TCodeGenerator& generator, int numInput, ValueLayout inputLayouts[]);

	ExprParse::UnitCodes genratePosifxCodes() const;


	bool optimize()
	{
		ExprTreeOptimizer optimizer;
		return optimizer.optimize(mTreeData);
	}

private:

	SymbolTable const* mSymbolDefine = nullptr;
	ExpressionTreeData mTreeData;

	friend class ExpressionParser;
public:

	void printInfixCodes()
	{
		ExprOutputContext constext(*mSymbolDefine);
		ExprParse::Print(constext, mTreeData.codes, false);
	}

	void printPostfixCodes()
	{
		ExprOutputContext constext(*mSymbolDefine);
		ExprParse::Print(constext, genratePosifxCodes(), true);
	}

};

class ExpressionParser : public ExprParse
{
public:
	bool parse(char const* expr, SymbolTable const& table);
	bool parse(char const* expr, SymbolTable const& table, ParseResult& result);
	bool parse(char const* expr, SymbolTable const& table, ExpressionTreeData& outTreeData);
	std::string const& getErrorMsg() { return mErrorMsg; }

protected:

	bool analyzeTokenUnit(char const* expr, SymbolTable const& table, ExprParse::UnitCodes& infixCode);
	void convertCode(ExprParse::UnitCodes& infixCode, ExprParse::UnitCodes& postfixCode);

	std::string  mErrorMsg;
};


template< typename T >
class TCodeGenerator : public ExprParse
{
public:
	using TokenType = ExprParse::TokenType;

	T* _this() { return static_cast<T*>(this); }
	//Override
	void codeInit(int numInput, ValueLayout inputLayouts[]);
	void codeConstValue(ConstValueInfo const&val);
	void codeVar(VariableInfo const& varInfo);
	void codeInput(InputInfo const& input);
	void codeExprTemp(ExprTempInfo const& tempInfo);
	void codeStoreTemp(int16 tempIndex);
	void codeFunction(FuncInfo const& info);
	void codeFunction(FuncSymbolInfo const& info);
	void codeBinaryOp(TokenType type, bool isReverse);
	void codeUnaryOp(TokenType type);
	void codeEnd();

	void visitFuncStart(ExprParse::Unit const& unit) {}
	void visitFuncEnd(ExprParse::Unit const& unit) {}
	void visitSeparetor() {}
	void postLoadCode() {}

	void visitValue(ExprParse::Unit const& unit)
	{
		switch (unit.type)
		{
		case ExprParse::VALUE_CONST:
			_this()->codeConstValue(unit.constValue);
			break;
		case ExprParse::VALUE_VARIABLE:
			_this()->codeVar(unit.variable);
			break;
		case ExprParse::VALUE_INPUT:
			_this()->codeInput(unit.input);
			break;
		case ExprParse::VALUE_EXPR_TEMP:
			_this()->codeExprTemp(unit.exprTemp);
			break;
		}
	}
	void visitOp(ExprParse::Unit const& unit)
	{
		switch (unit.type)
		{
		case ExprParse::FUNC_DEF:
			_this()->codeFunction(unit.symbol->func);
			break;
		case ExprParse::FUNC_SYMBOL:
			_this()->codeFunction(unit.funcSymbol);
			break;
		default:
			switch (unit.type & ExprParse::TOKEN_MASK)
			{
			case ExprParse::TOKEN_UNARY_OP:
				_this()->codeUnaryOp(unit.type);
				break;
			case ExprParse::TOKEN_BINARY_OP:
				_this()->codeBinaryOp(unit.type, unit.isReverse);
				break;
			}
			break;
		}

		if (unit.storeIndex != -1)
		{
			_this()->codeStoreTemp(unit.storeIndex);
		}
	}
};

struct CPreLoadCodeCallable
{
	template< typename T>
	static auto Requires(T& t, ExprParse::Unit& code) -> decltype
	(
		t.preLoadCode(code)
	);
};

struct CPostLoadCodeCallable
{
	template< typename T>
	static auto Requires(T& t) -> decltype
	(
		t.postLoadCode()
	);
};

// Helper visitor for preloading codes in postfix order
template< class TCodeGenerator >
struct TPreLoadVisitor : ExprParse
{
	TCodeGenerator& generator;
	TPreLoadVisitor(TCodeGenerator& gen) : generator(gen) {}

	void visitValue(ExprParse::Unit const& unit)
	{
		generator.preLoadCode(unit);
	}
	void visitOp(ExprParse::Unit const& unit)
	{
		generator.preLoadCode(unit);
	}
};

template< class TCodeGenerator >
void ParseResult::generateCode(TCodeGenerator& generator, int numInput, ValueLayout inputLayouts[])
{
	generator.codeInit(numInput, inputLayouts);

	if constexpr (TCheckConcept< CPreLoadCodeCallable, TCodeGenerator >::Value)
	{
		// Use tree traversal (postfix order) for accurate stack depth simulation
		TPreLoadVisitor<TCodeGenerator> preloadVisitor(generator);
		TExprTreeVisitOp<TPreLoadVisitor<TCodeGenerator>> preloadOp(mTreeData, preloadVisitor);
		preloadOp.execute();
	}

	if constexpr (TCheckConcept< CPostLoadCodeCallable, TCodeGenerator >::Value)
	{
		generator.postLoadCode();
	}

	TExprTreeVisitOp<TCodeGenerator, true> op(mTreeData, generator);
	op.execute();
	generator.codeEnd();
}

#endif //ExpressionParser_H
