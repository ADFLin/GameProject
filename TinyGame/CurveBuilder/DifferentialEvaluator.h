#ifndef DifferentialEvaluator_h__
#define DifferentialEvaluator_h__

#include "ExpressionParser.h"

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
			[&](Node& node)
			{
				int indexNodeLeft = outputTree_R<bOutput>(nodeCopy.children[CN_LEFT]);
				int indexNodeRight = outputTree_R<bOutput>(nodeCopy.children[CN_RIGHT]);

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
		if (index.bOutput)
			return index.value;

		if (index.value == 0)
		{
			return outputConst(0);
		}
		if (index.value == IDENTITY_INDEX)
		{
			return outputConst(1.0);
		}

		Unit const& unit = mEvalData->getLeafCode(index.value);
		return mOutputData->addLeafCode(unit);
	}

	int outputConst(RealType value)
	{
		return mOutputData->addLeafCode(Unit::ConstValue(value));
	}

	Unit getValue(Index index)
	{
		if (index == 0)
		{
			return Unit::ConstValue(RealType(0));
		}
		if (index == IDENTITY_INDEX)
		{
			return Unit::ConstValue(RealType(1));
		}
		return index.bOutput ? mOutputData->getLeafCode(index.value) : mEvalData->getLeafCode(index.value);
	}


	bool isValue(Index index)
	{
		if (index == 0 || index == IDENTITY_INDEX)
			return true;

		return ExprParse::IsLeaf(index.value);
	}

	bool isEqualValue(Index indexLeft, Index indexRight);
};


#endif // DifferentialEvaluator_h__
