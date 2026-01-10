#include "ExpressionParser.h"

#include "StringParse.h"
#include "ProfileSystem.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

void ExprParse::Print(ExprOutputContext& context, UnitCodes const& codes, bool haveBracket)
{
	if (!haveBracket)
	{
		for (auto const& unit : codes)
		{
			context.output(unit);
		}
	}
	else
	{
		for (auto const& unit : codes)
		{
			context.output("[");
			if (IsBinaryOperator(unit.type) && unit.isReverse)
				context.output("re");
			context.output(unit);

			if (unit.storeIndex != INDEX_NONE)
			{
				context.output("(s)");
			}
			context.output("]");
		}
	}
	context.output('\n');
}


bool ExprParse::IsValueEqual(Unit const& a, Unit const& b)
{
	CHECK(IsValue(a.type) && IsValue(b.type));
	if (a.type == b.type)
	{
		switch (a.type)
		{
		case VALUE_CONST:
			return a.constValue == b.constValue;
		case VALUE_VARIABLE:
			return a.variable.ptr == b.variable.ptr;
		case VALUE_INPUT:
			return a.input.index == b.input.index;
		case VALUE_EXPR_TEMP:
			return a.exprTemp.tempIndex == b.exprTemp.tempIndex;
		}
	}
	return false;
}

bool ExpressionParser::analyzeTokenUnit(char const* expr, SymbolTable const& table, ExprParse::UnitCodes& infixCode)
{
	infixCode.clear();

	if (*expr == 0)
		return false;

	Tokenizer tok(expr, " \t\n", "()+-*/^<>=!,");


	ExprParse::TokenType typePrev = ExprParse::TOKEN_NONE;
	StringView token;
	Tokenizer::TokenType tokenType;
	for (tokenType = tok.take(token); tokenType != EStringToken::None; tokenType = tok.take(token))
	{
		ExprParse::TokenType type = ExprParse::TOKEN_TYPE_ERROR;

		if (token.size() == 1)
		{
			switch (token.data()[0])
			{
			case '+':
				{
					if (tok.nextChar() == '+')
					{
						type = ExprParse::UOP_INC_PRE;
						tok.offset(1);
					}
					else
					{
						if (ExprParse::IsValue(typePrev) || typePrev == ExprParse::TOKEN_RBAR)
							type = ExprParse::BOP_ADD;
						else
							type = ExprParse::UOP_PLUS;
					}
				}
				break;
			case '-':
				{
					if (tok.nextChar() == '-')
					{
						type = ExprParse::UOP_DEC_PRE;
						tok.offset(1);
					}
					else
					{
						if (ExprParse::IsValue(typePrev) || typePrev == ExprParse::TOKEN_RBAR)
							type = ExprParse::BOP_SUB;
						else
							type = ExprParse::UOP_MINS;
					}
				}
				break;
			case '*': type = ExprParse::BOP_MUL; break;
			case '/': type = ExprParse::BOP_DIV; break;
			case '^': type = ExprParse::BOP_POW; break;
			case ',': type = ExprParse::BOP_COMMA; break;
			case '(': type = ExprParse::TOKEN_LBAR; break;
			case ')': type = ExprParse::TOKEN_RBAR; break;
			case '>':
				{
					if (tok.nextChar() == '=')
					{
						type = ExprParse::BOP_BIGEQU;
						tok.offset(1);
					}
					else
					{
						type = ExprParse::BOP_BIG;
					}
				}
				break;
			case '<':
				{
					if (tok.nextChar() == '=')
					{
						type = ExprParse::BOP_SMLEQU;
						tok.offset(1);
					}
					else
					{
						type = ExprParse::BOP_SML;
					}
				}
				break;
			case '=':
				{
					if (tok.nextChar() == '=')
					{
						type = ExprParse::BOP_EQU; tok.offset(1);
					}
					else if (typePrev == ExprParse::VALUE_VARIABLE || (typePrev == ExprParse::VALUE_INPUT))
					{
						type = ExprParse::BOP_ASSIGN;
					}
					else
					{
						return false;
					}
				}
				break;
			case '!':
				{
					if (tok.nextChar() == '=')
					{
						type = ExprParse::BOP_NEQU;
						tok.offset(1);
					}
					else
					{
						mErrorMsg = "No = after !";
						return false;
					}
				}
				break;
			}
		}

		if (type != ExprParse::TOKEN_TYPE_ERROR)
		{
			infixCode.push_back(ExprParse::Unit(type));
		}
		else if (isdigit(token[0]) || token[0] == '.')
		{
			double val = atof(token.toCString());
			infixCode.push_back(ExprParse::Unit::ConstValue((RealType)val));
			type = ExprParse::VALUE_CONST;
		}
		else
		{
			SymbolEntry const* symbol = table.findSymbol(token.toCString());

			if (symbol)
			{
				switch (symbol->type)
				{
				case SymbolEntry::eFunction:
					{
						type = ExprParse::FUNC_DEF;
						infixCode.push_back(ExprParse::Unit(type, symbol));
					}
					break;
				case SymbolEntry::eFunctionSymbol:
					{
						type = ExprParse::FUNC_SYMBOL;
						infixCode.push_back(ExprParse::Unit(symbol->funcSymbol));
					}
					break;
				case SymbolEntry::eConstValue:
					{
						type = ExprParse::VALUE_CONST;
						infixCode.push_back(ExprParse::Unit(symbol->constValue));
					}
					break;
				case SymbolEntry::eVariable:
					{
						type = ExprParse::VALUE_VARIABLE;
						infixCode.push_back(ExprParse::Unit(symbol->varValue));
					}
					break;
				case SymbolEntry::eInputVar:
					{
						type = ExprParse::VALUE_INPUT;
						infixCode.push_back(ExprParse::Unit(symbol->input));
					}
					break;
				}
			}
			else
			{
				mErrorMsg = "undefine symbol : ";
				mErrorMsg += token.toCString();
				return false;
			}
		}

		if (type == ExprParse::TOKEN_TYPE_ERROR)
		{
			mErrorMsg = "unknown token";
			return false;
		}

		typePrev = type;
	}

	return true;
}

void ExpressionParser::convertCode( UnitCodes& infixCode , UnitCodes& postfixCode )
{
	UnitCodes stack;
	stack.emplace_back(ExprParse::TOKEN_LBAR);
	infixCode.emplace_back(ExprParse::TOKEN_RBAR);
	
	postfixCode.clear();

	for ( UnitCodes::iterator iter = infixCode.begin()
		  ;iter!= infixCode.end(); ++iter )
	{
		Unit& elem = *iter;
		if ( ExprParse::IsValue( elem.type ) )
		{
			postfixCode.push_back(*iter);
		}
		else if ( elem.type == ExprParse::TOKEN_LBAR )
		{
			stack.push_back(*iter);
		}
		else if ( ( elem.type == ExprParse::BOP_COMMA ) || 
			      ( elem.type == ExprParse::TOKEN_RBAR ) )
		{
			for(;;)
			{
				const Unit& tElem = stack.back();
				if ( (tElem.type != ExprParse::TOKEN_LBAR) &&
					 (tElem.type != ExprParse::BOP_COMMA  )  )
				{
					postfixCode.push_back(tElem);
					stack.pop_back();
				}
				else
				{
					stack.pop_back();
					break;
				}
			}
			if ( elem.type == ExprParse::BOP_COMMA  )
				stack.push_back(*iter);
		}
		else if ( ExprParse::IsFunction( elem.type ) )
		{
			stack.push_back(*iter);
		}
		else if (  ExprParse::IsOperator(elem.type)  )
		{

			for(;;)
			{
				if ( stack.empty() ) break;
				const Unit& tElem = stack.back();
				if ( (tElem.type == ExprParse::TOKEN_LBAR) ||
					 (tElem.type == ExprParse::BOP_COMMA )  )
					break;

				if ( ExprParse::PrecedeceOrder( tElem.type ) >=
					 ExprParse::PrecedeceOrder( elem.type  ) )
				{
					postfixCode.push_back(tElem);
					stack.pop_back();
				}
				else break;
			}

			stack.push_back(elem);
			if ( elem.type == ExprParse::BOP_ASSIGN )
			{
				Unit& tElem = postfixCode.back();
				stack.push_back(tElem);
				postfixCode.pop_back();
			}
		}

		//printPostfixArray();

	}
	infixCode.pop_back();
}

bool ExpressionParser::parse(char const* expr, SymbolTable const& table)
{
	ParseResult result;
	if (!analyzeTokenUnit(expr, table, result.mTreeData.codes))
		return false;

	ExprTreeBuilder builder;
	try
	{
		builder.build(result.mTreeData);
	}
	catch (ExprParseException& e)
	{
		mErrorMsg = e.what();
		return false;
	}

	return true;
}

bool ExpressionParser::parse(char const* expr, SymbolTable const& table, ParseResult& result)
{

	result.mSymbolDefine = &table;
	{
		TIME_SCOPE("Analyze Token");
		if (!analyzeTokenUnit(expr, table, result.mTreeData.codes))
			return false;
	}

#if _DEBUG
	result.printInfixCodes();
#endif
	ExprTreeBuilder builder;
	try
	{

		{
			TIME_SCOPE("Build Expr Tree");
			builder.build(result.mTreeData);
		}

#if _DEBUG
		builder.printTree(table);
#endif

		{
			TIME_SCOPE("Check Expr Error");
			auto error = builder.checkTreeError();
			if (error != ExprTreeBuilder::TREE_NO_ERROR)
			{
				return false;
			}
		}
	}
	catch (ExprParseException& e)
	{
		mErrorMsg = e.what();
		return false;
	}

#if _DEBUG
	builder.printTree( table );
#endif


#if _DEBUG || true
	result.printPostfixCodes();
#endif
	return true;
}

bool ExpressionParser::parse(char const* expr, SymbolTable const& table, ExpressionTreeData& outTreeData)
{
	if (!analyzeTokenUnit(expr, table, outTreeData.codes))
		return false;

	try
	{
		ExprTreeBuilder builder;
		builder.build(outTreeData);
	}
	catch (ExprParseException& e)
	{
		mErrorMsg = e.what();
		return false;
	}

	return true;
}

bool ParseResult::isUsingVar(char const* name) const
{
	if (mSymbolDefine)
	{
		SymbolEntry const* symbol = mSymbolDefine->findSymbol(name);
		if (symbol && symbol->type == SymbolEntry::eVariable)
		{
			for (auto const& unit : mTreeData.codes)
			{
				if (unit.type == ExprParse::VALUE_VARIABLE && unit.variable.ptr == symbol->varValue.ptr)
					return true;
			}
		}
	}
	return false;
}

bool ParseResult::isUsingInput(char const* name) const
{
	if (mSymbolDefine)
	{
		SymbolEntry const* symbol = mSymbolDefine->findSymbol(name);
		if (symbol && symbol->type == SymbolEntry::eInputVar)
		{
			for (auto const& unit : mTreeData.codes)
			{
				if (unit.type == ExprParse::VALUE_INPUT && unit.input.index == symbol->input.index)
					return true;
			}
		}
	}
	return false;
}

ExprParse::UnitCodes ParseResult::genratePosifxCodes() const
{
	struct Visitor
	{
		void visitValue(ExprParse::Unit const& unit) { codes.push_back(unit); }
		void visitOp(ExprParse::Unit const& unit) { codes.push_back(unit); }
		ExprParse::UnitCodes codes;
	};

	Visitor visitor;
	TExprTreeVisitOp<Visitor> op(mTreeData, visitor);
	op.execute();
	return visitor.codes;
}
