#ifndef DifferentialEvaluator_h__
#define DifferentialEvaluator_h__

#include "FunctionParser.h"


class DifferentialEvaluator : public ExprParse
{
public:
    DifferentialEvaluator()
	{
	
	
	}

	~DifferentialEvaluator()
	{


	}


	void eval(ExpressionTreeData const& evalData , ExpressionTreeData& outputData);

	ExpressionTreeData const* mEvalData;
	ExpressionTreeData* mOutputData;
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



	Index evalExpr(int index);

	Index emitMins(Index index);

	Index emitFuncSymbol(EFuncSymbol symbol, Index indexArg);

	Index emitFuncSymbol(EFuncSymbol symbol, Index indexArgs[] , int numArg )
	{
		auto entry = GetFuncSymbolEntry(symbol);

		int indexFuncOp = mOutputData->codes.size();
		mOutputData->codes.push_back(Unit{ entry });

		int indexFuncNode = mOutputData->nodes.size();
		Node node;
		node.indexOp = indexFuncOp;
		node.children[CN_LEFT] = 0;
		node.children[CN_RIGHT] = 0;
		mOutputData->nodes.push_back(node);



		for (int i = 0; i < numArg; ++i)
		{




		}

		return Index{ indexFuncNode , true };
	}

	Index emitConst(RealType value);

	Index emitAdd(Index indexLeft, Index indexRight);
	Index emitSub(Index indexLeft, Index indexRight);
	Index emitMul(Index indexLeft, Index indexRight);
	Index emitDiv(Index indexLeft, Index indexRight);
	Index emitPow(Index indexLeft, Index indexRight);

	Index emitUop(TokenType op, Index index);
	Index emitBop(TokenType op, Index indexLeft, Index indexRight);

	Index emitExpr(int index)
	{
		CHECK(index != IDENTITY_INDEX);
		if (index == 0)
			return Index::Zero();

		return emitExprInternal<false>(index);
	}


	Index emitExprConditional(int index, bool bCondition)
	{
		CHECK(index != 0);
		if (!bCondition)
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
		ExpressionTreeData const& readData = bOutput ? *mOutputData : *mEvalData;
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

			return Index{ mOutputData->addLeafCode(unit) , true };
		}

		return Index{ outputNodeAndChildren<bOutput>(index), true };
	}

	template< bool bOutput >
	int outputNodeAndChildren(int index)
	{
		ExpressionTreeData const& readData = bOutput ? *mOutputData : *mEvalData;

		Node const& nodeCopy = readData.nodes[index];
		Unit const& opCopy = readData.codes[nodeCopy.indexOp];

		return mOutputData->addOpCode(opCopy, 
			[&](int indexNode)
			{
				int indexNodeLeft = outputTree_R<bOutput>(nodeCopy.children[CN_LEFT]);
				int indexNodeRight = outputTree_R<bOutput>(nodeCopy.children[CN_RIGHT]);

				Node& node = mOutputData->nodes[indexNode];
				node.children[CN_LEFT] = indexNodeLeft;
				node.children[CN_RIGHT] = indexNodeRight;
			}
		);
	}

	template< bool bOutput >
	int outputTree_R(int index)
	{
		if (index == 0)
			return 0;
		
		if (IsLeaf(index))
		{
			ExpressionTreeData const& readData = bOutput ? *mOutputData : *mEvalData;
			Unit const& unit = readData.getLeafCode(index);

			return mOutputData->addLeafCode(unit);
		}

		return outputNodeAndChildren<bOutput>(index);
	}

	int outputIfNeed(Index index)
	{
		CHECK(index.value != 0);
		if (index.bOutput)
			return index.value;

		if (index.value == IDENTITY_INDEX)
		{
			return outputConst(1.0);
		}

		Unit const& unit = mEvalData->getLeafCode(index.value);
		return mOutputData->addLeafCode(unit);
	}

	int outputConst(RealType value)
	{
		return mOutputData->addLeafCode(Unit{ VALUE_CONST, ConstValueInfo(value) });
	}

	static Unit GetZeroValue()
	{
		return Unit(VALUE_CONST, ConstValueInfo{ RealType(0) });
	}
	static Unit GetIdentityValue()
	{
		return Unit(VALUE_CONST, ConstValueInfo{ RealType(1) });
	}

	Unit getValue(Index index)
	{
		if (index == 0)
		{
			return GetZeroValue();
		}
		if (index == IDENTITY_INDEX)
		{
			return GetIdentityValue();
		}
		return index.bOutput ? mOutputData->getLeafCode(index.value) : mEvalData->getLeafCode(index.value);
	}


	bool isValue(Index index)
	{
		if (index == 0 || index == IDENTITY_INDEX)
			return true;

		return ExprParse::IsLeaf(index.value);
	}

	bool isEqualValue(Index indexLeft, Index indexRight)
	{
		if (isValue(indexLeft) != isValue(indexRight))
			return false;

		if (isValue(indexLeft))
		{
			Unit valueL = getValue(indexLeft);
			Unit valueR = getValue(indexRight);
			return IsValueEqual(valueL, valueR);
		}

		return false;
	}
};


#endif // DifferentialEvaluator_h__
