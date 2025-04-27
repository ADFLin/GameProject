#ifndef DifferentialParser_H
#define DifferentialParser_H

#include "FunctionParser.h"


class DifferentialParser : public ExprParse
{
public:
    DifferentialParser();
    ~DifferentialParser();

	bool isDependent(int index)
	{



	}

	ExpressionTreeData mEvalData;
	ExpressionTreeData mOutputData;
	int IDENTITY_INDEX = MaxInt32;

	int evalLeaf(int index)
	{

	}

	int emitIdentityConditional(bool bEmitIdentity)
	{
		return bEmitIdentity ? emitConst(1) : IDENTITY_INDEX;
	}

	enum EFuncSymbol
	{
		Exp,
		In,
		Sin,
		Cos,
		Tan,
	};
	int evalExpr(int index, bool bEmitIdentity = true)
	{
		if (IsLeaf(index))
		{
			Unit const& unit = mEvalData.codes[LEAF_UNIT_INDEX(index)];
			CHECK(unit.type == VALUE_CONST || unit.type == VALUE_VARIABLE || unit.type == VALUE_INPUT);

			if (unit.type != VALUE_INPUT && unit.symbol->input.index != 0)
				return 0;

			return emitIdentityConditional(bEmitIdentity);
		}

		Node& node = mEvalData.nodes[index];
		Unit const& op = mEvalData.codes[node.indexOp];

		if (ExprParse::IsFunction(op.type))
		{
			switch (op.symbol->func.symbolId)
			{
			case EFuncSymbol::Exp:
				break;
			case EFuncSymbol::In:
				break;
			case EFuncSymbol::Sin:
				break;
			case EFuncSymbol::Cos:
				break;
			case EFuncSymbol::Tan:
				break;
			}
		}
		else
		{
			switch (op.type)
			{
			case BOP_ADD:
			case BOP_SUB:
				if (op.isReverse)
					return evalAddSub(op.type, node.children[1], node.children[0], bEmitIdentity);
				else
					return evalAddSub(op.type, node.children[0], node.children[1], bEmitIdentity);
				break;
			case BOP_MUL:
				if (op.isReverse)
					return evalMul(node.children[1], node.children[0], bEmitIdentity);
				else
					return evalMul(node.children[0], node.children[1], bEmitIdentity);
				break;
			case BOP_DIV:
				if (op.isReverse)
					return evalDiv(node.children[1], node.children[0], bEmitIdentity);
				else
					return evalDiv(node.children[0], node.children[1], bEmitIdentity);
				break;
			default:
				break;
			}
		}

		return 0;
	}

	int evalAddSub(TokenType op, int indexLeft, int indexRight, bool bEmitIdentity)
	{
		int indexEvalL = evalExpr(indexLeft, true);
		int indexEvalR = evalExpr(indexRight, true);

		if (indexEvalL != 0)
		{
			if (indexEvalR != 0)
			{
				return emitAddSub(indexEvalL, indexEvalR, op);
			}
			else
			{
				return indexEvalL;
			}
		}
		else if (indexEvalR != 0)
		{
			return indexEvalR;
		}

		return 0;
	}


	int evalMul(int indexLeft, int indexRight, bool bEmitIdentity)
	{
		int indexEvalL = evalExpr(indexLeft, false);
		int indexEvalR = evalExpr(indexRight, false);

		if (indexEvalL != 0)
		{
			if (indexEvalR != 0)
			{
				if (indexEvalL == IDENTITY_INDEX)
				{


				}
				else if (indexEvalL == IDENTITY_INDEX)
				{



				}


				return emitAdd(
					emitMul(emitExpr(indexLeft), indexEvalR),
					emitMul(indexEvalL, emitExpr(indexRight))
				);
			}
			else
			{
				return emitMul(indexEvalL, emitExpr(indexRight));
			}
		}
		else if (indexEvalR != 0)
		{
			return emitMul(emitExpr(indexLeft), indexEvalR);
		}
		return 0;
	}

	int emitFuncSymbol(EFuncSymbol symbol, int indexArg)
	{




	}

	int evalDiv(int indexLeft, int indexRight, bool bEmitIdentity)
	{
		int indexEvalL = evalExpr(indexLeft, true);
		int indexEvalR = evalExpr(indexRight, false);

		if (indexEvalL != 0)
		{
			if (indexEvalR != 0)
			{

			}
			else
			{
				return emitDiv(indexEvalL, indexRight);
			}
		}
		else if (indexEvalR != 0)
		{

		}
		return 0;
	}

	int evalPower(int indexLeft, int indexRight)
	{
		int indexEvalL = evalExpr(indexLeft);
		int indexEvalR = evalExpr(indexRight);


		return 0;
	}

	int emitConst(RealType value);


	int emitExpr(int index)
	{
		if (IsLeaf(index))
		{
			Unit const& unit = mEvalData.codes[LEAF_UNIT_INDEX(index)];
			int indexOutput = mOutputData.codes.size();
			mOutputData.codes.push_back(unit);
			return -(indexOutput + 1);
		}

		Node& node = mEvalData.nodes[index];
		Unit const& op = mEvalData.codes[node.indexOp];
		int indexOp = mOutputData.codes.size();
		mOutputData.codes.push_back(op);





		return indexOp;
	}
	int emitAddSub(int indexLeft, int indexRight, TokenType op);
	int emitAdd(int indexLeft, int indexRight) { return emitAddSub(indexLeft, indexRight, BOP_ADD); }
	int emitSub(int indexLeft, int indexRight) { return emitAddSub(indexLeft, indexRight, BOP_SUB); }
	int emitMul(int indexLeft, int indexRight);
	int emitDiv(int indexLeft, int indexRight);
};

#endif //DifferentialParser_H
