#include "DifferentialEvaluator.h"


void DifferentialEvaluator::eval(ExpressionTreeData const& evalData, ExpressionTreeData& outputData)
{
	mEvalData = &evalData;
	mOutputData = &outputData;

	mOutputData->clear();
	if (!mEvalData->nodes.empty())
	{
		Node node;
		node.indexOp = INDEX_NONE;
		node.children[CN_RIGHT] = 0;
		mOutputData->nodes.push_back(node);
		Index index = evalExpr(mEvalData->nodes[0].children[CN_LEFT]);
		mOutputData->nodes[0].children[CN_LEFT] = outputIfNeed(index);
	}
}

DifferentialEvaluator::Index DifferentialEvaluator::evalExpr(int index)
{
	if (IsLeaf(index))
	{
		Unit const& unit = mEvalData->codes[LEAF_UNIT_INDEX(index)];
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

	Node const& node = mEvalData->nodes[index];
	Unit const& op = mEvalData->codes[node.indexOp];

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
				indexFuncEval = emitPow(emitFuncSymbol(EFuncSymbol::Sec, emitExpr(indexArg)), emitConst(2));
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

			return emitMul(indexFuncEval, indexArgEval);
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
			//(d(L*R) = L*dR + dL*R 
			return emitAdd(
				(indexEvalL == 0) ? Index::Zero() : emitMul(indexEvalL, emitExpr(indexRight)),
				(indexEvalR == 0) ? Index::Zero() : emitMul(emitExpr(indexLeft), indexEvalR)
			);
		}
		case BOP_DIV:
		{
			if (indexEvalR == 0)
			{
				// dL / R
				return (indexEvalL == 0) ? Index::Zero() : emitDiv(indexEvalL, emitExpr(indexRight));
			}
			//(d(L/R) = dL/R - L*dR/R^2
			return emitSub(
				indexEvalL == 0 ? Index::Zero() : emitDiv(indexEvalL, emitExpr(indexRight)),
				indexEvalR == 0 ? Index::Zero() : emitDiv(
					emitMul(emitExpr(indexLeft), indexEvalR),
					emitPow(emitExpr(indexRight), emitConst(2))
				)
			);
		}
		case BOP_POW:
		{
			if (indexEvalR == 0)
			{
				// R * L^(R - 1) * [dL]
				return emitMul
				(
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
				emitAdd
				(
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
				return emitMins(indexEval);
			}
		}
	}
	return Index::Zero();
}

DifferentialEvaluator::Index DifferentialEvaluator::emitMins(Index index)
{
	if (index.value == 0)
		return Index::Zero();

	return emitUop(UOP_MINS, index);
}

DifferentialEvaluator::Index DifferentialEvaluator::emitFuncSymbol(EFuncSymbol symbol, Index indexArg)
{
	auto entry = GetFuncSymbolEntry(symbol);
	int indexFunc = mOutputData->addOpCode(Unit{ entry },
		[&](int indexNode)
		{
			int indexNodeLeft = outputIfNeed(indexArg);

			Node& node = mOutputData->nodes[indexNode];
			node.children[CN_LEFT] = indexNodeLeft;
			node.children[CN_RIGHT] = 0;
		}
	);
	return Index{ indexFunc , true };
}

DifferentialEvaluator::Index DifferentialEvaluator::emitConst(RealType value)
{
	if (value == RealType(0))
	{
		return Index::Zero();
	}
	if (value == RealType(1))
	{
		return Index::Identity();
	}
	return Index{ outputConst(value) , true };
}

DifferentialEvaluator::Index DifferentialEvaluator::emitAdd(Index indexLeft, Index indexRight)
{
	if (indexLeft != 0)
	{
		if (indexRight != 0) //L+R
		{
#if 0
			if (isEqualValue(indexLeft, indexRight))
				return emitMul(emitConst(2), indexLeft);
#endif

			return emitBop(BOP_ADD, indexLeft, indexRight);
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

DifferentialEvaluator::Index DifferentialEvaluator::emitSub(Index indexLeft, Index indexRight)
{
	if (indexLeft != 0)
	{
		if (indexRight != 0) //L-R
		{
			if (indexLeft == IDENTITY_INDEX && indexRight == IDENTITY_INDEX)
				return Index::Zero();

			if (isEqualValue(indexLeft, indexRight))
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
		return emitUop(UOP_MINS, indexRight);
	}

	return Index::Zero();
}

DifferentialEvaluator::Index DifferentialEvaluator::emitMul(Index indexLeft, Index indexRight)
{
	if (indexLeft != 0 && indexRight != 0)
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

DifferentialEvaluator::Index DifferentialEvaluator::emitDiv(Index indexLeft, Index indexRight)
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

DifferentialEvaluator::Index DifferentialEvaluator::emitPow(Index indexLeft, Index indexRight)
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

DifferentialEvaluator::Index DifferentialEvaluator::emitBop(TokenType op, Index indexLeft, Index indexRight)
{
	CHECK(IsBinaryOperator(op));

	if (isValue(indexLeft) && isValue(indexRight))
	{
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
				resultValue = pow(leftValue.constValue.asReal, rightValue.constValue.asReal);
				break;
			default:
				NEVER_REACH("");
			}
			return emitConst(resultValue);
		}
	}

	Unit code;
	code.type = op;
	code.isReverse = false;

	int indexNode = mOutputData->addOpCode(code,
		[&](int indexNode)
		{
			int indexNodeLeft = outputIfNeed(indexLeft);
			int indexNodeRight = outputIfNeed(indexRight);

			Node& node = mOutputData->nodes[indexNode];
			node.children[CN_LEFT] = indexNodeLeft;
			node.children[CN_RIGHT] = indexNodeRight;
		}
	);
	return Index{ indexNode, true };
}

DifferentialEvaluator::Index DifferentialEvaluator::emitUop(TokenType op, Index index)
{
	CHECK(IsUnaryOperator(op));
	Unit code;
	code.type = op;
	code.isReverse = false;
	int indexNode = mOutputData->addOpCode(code,
		[&](int indexNode)
		{
			int indexNodeLeft = outputIfNeed(index);

			Node& node = mOutputData->nodes[indexNode];
			node.children[CN_LEFT] = indexNodeLeft;
			node.children[CN_RIGHT] = 0;
		}
	);
	return Index{ indexNode, true };
}
