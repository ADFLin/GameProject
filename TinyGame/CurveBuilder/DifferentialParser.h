#ifndef DifferentialParser_H
#define DifferentialParser_H

#include "FunctionParser.h"


class DifferentialParser : public ExprParse
{
public:
    DifferentialParser(ExpressionTreeData& evalData)
		:mEvalData(evalData)
	{
	
	
	}
	~DifferentialParser()
	{


	}


	void parse()
	{
		mOutputData.clear();
		
		if (!mEvalData.nodes.empty())
		{
			Node node;
			node.children[CN_RIGHT] = INDEX_NONE;
			node.indexOp = INDEX_NONE;
			mOutputData.nodes.push_back(node);
			Index index = evalExpr(mEvalData.nodes[0].children[CN_LEFT]);
			mOutputData.nodes[0].children[CN_LEFT] = emitIdentityIfNeed(index).value;
		}
	}
	ExpressionTreeData mEvalData;
	ExpressionTreeData mOutputData;
	static constexpr int IDENTITY_INDEX = MaxInt32;



#define DEFAULT_FUNC_SYMBOL_LIST(op)\
		op(Exp, "exp", 1)\
		op(Ln, "ln", 1)\
		op(Sin, "sin", 1)\
		op(Cos, "cos", 1)\
		op(Tan, "tan", 1)\
		op(Cot, "cot", 1)\
		op(Sec, "sec", 1)\
		op(Csc, "csc", 1)

	enum EFuncSymbol
	{
	#define ENUM_OP(SYMBOL, NAME, NUM_ARG) SYMBOL,
		DEFAULT_FUNC_SYMBOL_LIST(ENUM_OP)
	#undef ENUM_OP
	};

	static FuncSymbolInfo const& GetFuncSymbolEntry(EFuncSymbol symbol)
	{
	#define FUNC_INFO_OP(SYMBOL, NAME, NUM_ARG) FuncSymbolInfo{EFuncSymbol::SYMBOL , NUM_ARG},
		static FuncSymbolInfo StaticMap[] =
		{
			DEFAULT_FUNC_SYMBOL_LIST(FUNC_INFO_OP)
		};
	#undef FUNC_INFO_OP
		return StaticMap[symbol];
	}

	static void DefineFuncSymbol(SymbolTable& table)
	{
#define DEFINE_FUNC_OP(SYMBOL, NAME, NUM_ARG) table.defineFuncSymbol(NAME, EFuncSymbol::SYMBOL, NUM_ARG);
		DEFAULT_FUNC_SYMBOL_LIST(DEFINE_FUNC_OP)
#undef DEFINE_FUNC_OP
	}

	struct Index
	{
		Index(int value, bool bOutput)
			:value(value), bOutput(bOutput) {}

		bool operator == (int v) const { return value == v; }
		bool operator != (int v) const { return value != v; }

		static Index Identity(){ return Index { IDENTITY_INDEX, false }; }
		static Index Zero() { return Index{ 0, false }; }
		bool bOutput;
		int  value;
	};



	Index evalExpr(int index)
	{
		if (IsLeaf(index))
		{
			Unit const& unit = mEvalData.codes[LEAF_UNIT_INDEX(index)];
			CHECK(unit.type == VALUE_CONST || unit.type == VALUE_VARIABLE || unit.type == VALUE_INPUT);

			switch (unit.type)
			{
			case VALUE_CONST:
				break;
			case VALUE_INPUT:
				if (unit.symbol->input.index == 0)
					return Index::Identity();
				break;
			case VALUE_VARIABLE:
				break;
			}
			return Index::Zero();
		}

		Node& node = mEvalData.nodes[index];
		Unit const& op = mEvalData.codes[node.indexOp];

		if (ExprParse::IsFunction(op.type))
		{
			CHECK(op.funcSymbol.numArgs == 1);
			int indexArg = node.children[CN_LEFT];

			Index indexArgEval = evalExpr(indexArg);
			if (indexArgEval != 0)
			{
				Index indexFuncEval = Index::Zero();
				switch (op.funcSymbol.id)
				{
				case EFuncSymbol::Exp:
					indexFuncEval = emitFuncSymbol(EFuncSymbol::Exp, emitExpr(indexArg));
					break;
				case EFuncSymbol::Ln:
					indexFuncEval = emitDiv(Index::Identity(), emitExpr(indexArg));
					break;
				case EFuncSymbol::Sin:
					indexFuncEval = emitFuncSymbol(EFuncSymbol::Cos, emitExpr(indexArg));
					break;
				case EFuncSymbol::Cos:
					indexFuncEval = emitMins(emitFuncSymbol(EFuncSymbol::Sin, emitExpr(indexArg)));
					break;
				case EFuncSymbol::Tan:
					indexFuncEval = emitPow(emitFuncSymbol(EFuncSymbol::Sec, emitExpr(indexArg)),emitConst(2));
					break;
				case EFuncSymbol::Cot:
					indexFuncEval = emitMins(emitPow(emitFuncSymbol(EFuncSymbol::Sec, emitExpr(indexArg)), emitConst(2)));
					break;
				case EFuncSymbol::Sec:
					indexFuncEval = emitMul(emitFuncSymbol(EFuncSymbol::Sec, emitExpr(indexArg)), emitFuncSymbol(EFuncSymbol::Tan, emitExpr(indexArg)));
					break;
				case EFuncSymbol::Csc:
					indexFuncEval = emitMins(emitMul(emitFuncSymbol(EFuncSymbol::Csc, emitExpr(indexArg)), emitFuncSymbol(EFuncSymbol::Cot, emitExpr(indexArg))));
					break;
				}

				return emitMul(indexFuncEval , indexArgEval);
			}
		}
		else if (ExprParse::IsBinaryOperator(op.type))
		{
			int indexLeft = node.children[0];
			int indexRight = node.children[1];
			if (op.isReverse)
				std::swap(indexLeft, indexRight);

			Index indexEvalL = evalExpr(indexLeft);
			Index indexEvalR = evalExpr(indexRight);

			switch (op.type)
			{
			case BOP_ADD:
				return emitAdd(indexEvalL, indexEvalR);
			case BOP_SUB:
				return emitSub(indexEvalL, indexEvalR);
			case BOP_MUL:
				{
					//(d(L*R) = dL*R + L*dR
					return emitAdd(
						(indexEvalR == 0) ? Index::Zero() : emitMul(emitExpr(indexLeft), indexEvalR), 
						(indexEvalL == 0) ? Index::Zero() : emitMul(indexEvalL, emitExpr(indexRight))
					);
				}
			case BOP_DIV:
				{
					if (indexEvalR == 0)
					{
						// dL / R
						return (indexEvalL == 0) ? Index::Zero() : emitDiv(indexEvalL, emitExpr(indexRight));
					}
					//(d(L/R) = (dL*R - L*dR) / R^2
					return emitDiv(
						emitSub(
							(indexEvalR == 0) ? Index::Zero() : emitMul(emitExpr(indexLeft), indexEvalR),
							(indexEvalL == 0) ? Index::Zero() : emitMul(indexEvalL, emitExpr(indexRight))
						),
						emitPow(emitExpr(indexRight), emitConst(2))
					);
				}
			case BOP_POW:
				{
					if (indexEvalR == 0)
					{
						// R * L^(R - 1) * [dL]
						return emitMul(
							emitMul(
								emitExpr(indexRight),
								emitPow(emitExpr(indexLeft), emitSub(emitExpr(indexRight), Index::Identity()))
							),
							indexEvalL
						);
					}

					//d(L^R) = L^R * [In(L)*dR + R/L* dL]
					return emitMul(
						emitPow(emitExpr(indexLeft), emitExpr(indexRight)),
						emitAdd(
							(indexEvalR == 0) ? Index::Zero() : emitMul(
								emitFuncSymbol(EFuncSymbol::Ln, emitExpr(indexLeft)),
								indexEvalR
							),
							(indexEvalL == 0) ? Index::Zero() : emitMul(
								emitDiv(emitExpr(indexRight), emitExpr(indexLeft)),
								indexEvalL
							)
						)
					);
				}
			default:
				break;
			}
		}
		else
		{
			Index indexEval = evalExpr(node.children[0]);

			if (indexEval != 0)
			{
				switch (op.type)
				{
				case UOP_PLUS:
					return indexEval;
				case UOP_MINS:
					return emitUop(UOP_MINS, indexEval);
				}
			}
		}
		return Index::Zero();
	}

	Index emitMins(Index index)
	{
		if (index.value == 0)
			return Index::Zero();

		return emitUop(UOP_MINS, index);
	}

	Index emitFuncSymbol(EFuncSymbol symbol, Index indexArg)
	{
		auto entry = GetFuncSymbolEntry(symbol);

		int indexFuncOp = mOutputData.codes.size();
		mOutputData.codes.push_back(Unit{ entry });

		int indexFuncNode = mOutputData.nodes.size();
		Node node;
		node.indexOp = indexFuncOp;
		node.children[0] = emitIfNeed(indexArg);
		node.children[1] = 0;
		mOutputData.nodes.push_back(node);

		return Index{ indexFuncNode , true };
	}

	Index emitFuncSymbol(EFuncSymbol symbol, Index indexArgs[] , int numArg )
	{
		auto entry = GetFuncSymbolEntry(symbol);

		int indexFuncOp = mOutputData.codes.size();
		mOutputData.codes.push_back(Unit{ entry });

		int indexFuncNode = mOutputData.nodes.size();
		Node node;
		node.indexOp = indexFuncOp;
		node.children[0] = 0;
		node.children[1] = 0;
		mOutputData.nodes.push_back(node);



		for (int i = 0; i < numArg; ++i)
		{




		}

		return Index{ indexFuncNode , true };
	}

	Index emitConst(RealType value)
	{
		if (value == RealType(1))
		{
			return Index::Identity();
		}

		int indexCode = mOutputData.codes.size();
		mOutputData.codes.push_back(Unit{ VALUE_CONST, ConstValueInfo(value)});
		return Index { -(indexCode + 1) , true };
	}

	Index emitExpr(int index)
	{
		CHECK(index != IDENTITY_INDEX);
		if (index == 0)
			return Index::Zero();

		return emitExprInternal<false>(index);
	}


	Index emitExprConditional(int index, bool bNeedEmit)
	{
		if (!bNeedEmit || index == 0)
			return Index::Zero();

		return emitExprInternal<false>(index);
	}

	Index emitExpr(Index index)
	{
		if (index == 0)
			return Index::Zero();

		if (index == IDENTITY_INDEX)
			return Index::Identity();

		if (index.bOutput)
			return emitExprInternal<true>(index.value);


		return emitExprInternal<false>(index.value);
	}

	template< bool bOutput >
	Index emitExprInternal(int index)
	{
		ExpressionTreeData const& readData = bOutput ? mOutputData : mEvalData;
		if (IsLeaf(index))
		{
			Unit const& unit = readData.codes[LEAF_UNIT_INDEX(index)];

			if (unit.type == VALUE_CONST)
			{
				bool bEqualOne = false;
				switch (unit.constValue.layout)
				{
				case ValueLayout::Int32:  bEqualOne = (unit.constValue.asInt32 == 1);
				case ValueLayout::Double: bEqualOne = (unit.constValue.asDouble == 1.0);
				case ValueLayout::Float:  bEqualOne = (unit.constValue.asFloat == 1.0f);
				}
				if (bEqualOne)
					return Index::Identity();
			}

			int indexOutput = mOutputData.codes.size();
			mOutputData.codes.push_back(unit);
			return Index{ -(indexOutput + 1) , true };
		}

		return Index{ emitNodeAndChildren<bOutput>(index), true };
	}

	template< bool bOutput >
	int emitNodeAndChildren(int index)
	{
		ExpressionTreeData const& readData = bOutput ? mOutputData : mEvalData;

		Node const& nodeCopy = readData.nodes[index];
		Unit const& opCopy = readData.codes[nodeCopy.indexOp];


		int indexOp = mOutputData.codes.size();
		mOutputData.codes.push_back(opCopy);

		int indexNode = mOutputData.nodes.size();
		mOutputData.nodes.push_back(nodeCopy);

		mOutputData.nodes[indexNode].indexOp = indexOp;
		mOutputData.nodes[indexNode].children[0] = emitTree_R<bOutput>(nodeCopy.children[0]);
		mOutputData.nodes[indexNode].children[1] = emitTree_R<bOutput>(nodeCopy.children[1]);
		return indexNode;
	}

	template< bool bOutput >
	int emitTree_R(int index)
	{
		if (index == 0)
			return 0;
		
		if (IsLeaf(index))
		{
			ExpressionTreeData const& readData = bOutput ? mOutputData : mEvalData;
			Unit const& unit = readData.codes[LEAF_UNIT_INDEX(index)];
			int indexOutput = mOutputData.codes.size();
			mOutputData.codes.push_back(unit);
			return -(indexOutput + 1);
		}

		return emitNodeAndChildren<bOutput>(index);
	}

	int emitIfNeed(Index index)
	{
		CHECK(index.value != 0);
		if (index.bOutput)
			return index.value;

		if (index.value == IDENTITY_INDEX)
		{
			return emitConst(1.0).value;
		}

		Unit const& unit = mEvalData.codes[LEAF_UNIT_INDEX(index.value)];
		int indexOutput = mOutputData.codes.size();
		mOutputData.codes.push_back(unit);
		return -(indexOutput + 1);
	}

	Index emitUop(TokenType op, Index index)
	{
		Unit code;
		code.type = op;
		code.isReverse = false;

		int indexCode = mOutputData.codes.size();
		mOutputData.codes.push_back(code);

		Node node;
		node.indexOp = indexCode;
		node.children[CN_LEFT] = emitIfNeed(index);
		node.children[CN_RIGHT] = 0;

		int indexNode = mOutputData.nodes.size();
		mOutputData.nodes.push_back(node);

		return Index{ indexNode, true };
	}

	Index emitBop(TokenType op, Index indexLeft, Index indexRight)
	{
		Unit code;
		code.type = op;
		code.isReverse = false;

		auto leftValue = getValue(indexLeft);
		auto rightValue = getValue(indexRight);

		if (leftValue.type == VALUE_CONST && rightValue.type == VALUE_CONST)
		{
			RealType resultValue = 0.0;
			switch (op)
			{
			case ExprParse::BOP_ADD:
				resultValue = leftValue.constValue.asReal + rightValue.constValue.asReal;
				break;
			case ExprParse::BOP_SUB:
				resultValue = leftValue.constValue.asReal - rightValue.constValue.asReal;
				break;
			case ExprParse::BOP_MUL:
				resultValue = leftValue.constValue.asReal * rightValue.constValue.asReal;
				break;
			case ExprParse::BOP_DIV:
				resultValue = leftValue.constValue.asReal / rightValue.constValue.asReal;
				break;
			case ExprParse::BOP_POW:
				resultValue = pow(leftValue.constValue.asReal,rightValue.constValue.asReal);
				break;
			default:
				NEVER_REACH("");
			}

			return emitConst(resultValue);
		}

		int indexCode = mOutputData.codes.size();
		mOutputData.codes.push_back(code);

		Node node;
		node.indexOp = indexCode;
		node.children[CN_LEFT] = emitIfNeed(indexLeft);
		node.children[CN_RIGHT] = emitIfNeed(indexRight);

		int indexNode = mOutputData.nodes.size();
		mOutputData.nodes.push_back(node);

		return Index{ indexNode, true };
	}

	Index emitIdentityIfNeed(Index index)
	{
		if (index == IDENTITY_INDEX)
		{
			return emitConst(1.0);
		}
		return index;
	}
	
	Index emitAdd(Index indexLeft, Index indexRight)
	{
		if (indexLeft != 0)
		{
			if (indexRight != 0) //L+R
			{
				return emitBop(BOP_ADD, indexLeft , indexRight);
			}
			else //L
			{
				return indexLeft;
			}
		}
		else if (indexRight != 0) //R
		{
			return indexRight;
		}

		return Index::Zero();
	}
	
	Index emitSub(Index indexLeft, Index indexRight)
	{
		if (indexLeft != 0)
		{
			if (indexRight != 0) //L-R
			{
				if (indexLeft == IDENTITY_INDEX && indexRight == IDENTITY_INDEX)
					return Index::Zero();

				return emitBop(BOP_SUB, indexLeft, indexRight);
			}
			else //L
			{
				return indexLeft;
			}
		}
		else if (indexRight != 0) //R
		{
			return indexRight;
		}

		return Index::Zero();
	}

	Unit getValue(Index index)
	{
		if (index == IDENTITY_INDEX)
		{
			return Unit(VALUE_CONST, ConstValueInfo{RealType(1.0)});
		}
		int indexCode = LEAF_UNIT_INDEX(index.value);
		return index.bOutput ? mOutputData.codes[indexCode] : mEvalData.codes[indexCode];
	}

	bool isEqualValue(Index indexLeft, Index indexRight)
	{
		CHECK(indexLeft != 0 && indexLeft != IDENTITY_INDEX);
		CHECK(indexRight != 0 && indexRight != IDENTITY_INDEX);

		if (ExprParse::IsLeaf(indexLeft.value) != ExprParse::IsLeaf(indexRight.value))
			return false;

		if (ExprParse::IsLeaf(indexLeft.value))
		{
			Unit valueL = getValue(indexLeft);
			Unit valueR = getValue(indexRight);
			return IsValueEqual(valueL, valueR);
		}

		return false;
	}

	Index emitMul(Index indexLeft, Index indexRight)
	{
		if (indexLeft != 0  && indexRight != 0)
		{
			if (indexLeft != IDENTITY_INDEX)
			{
				if (indexRight != IDENTITY_INDEX) // L*R
				{
					return emitBop(BOP_MUL, indexLeft, indexRight);
				}
				else // L
				{
					return indexLeft;
				}
			}
			else // R
			{
				return indexRight;
			}
		}

		return Index::Zero();
	}

	Index emitDiv(Index indexLeft, Index indexRight)
	{
		if (indexLeft != 0)
		{
			if (indexRight != IDENTITY_INDEX)
			{
				if (isEqualValue(indexLeft, indexRight))
				{
					return Index::Identity();
				}

				return emitBop(BOP_DIV, indexLeft, indexRight);
			}
			else
			{
				return indexLeft;
			}
		}

		return Index::Zero();
	}

	Index emitPow(Index indexLeft, Index indexRight)
	{
		if (indexRight == 0)
		{
			return Index::Identity();
		}

		if (indexLeft != 0)
		{
			if (indexRight != IDENTITY_INDEX)
			{
				return emitBop(BOP_POW, indexLeft, indexRight);
			}
			else
			{
				return indexLeft;
			}
		}
		return Index::Zero();
	}
};

#endif //DifferentialParser_H
