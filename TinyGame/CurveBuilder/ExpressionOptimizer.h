#ifndef ExpressionOptimizer_h__
#define ExpressionOptimizer_h__

#include "ExpressionTree.h"
#include <unordered_map>
#include <vector>

class ExprTreeOptimizer : public ExprParse
{
	using Unit = ExprParse::Unit;
public:
	bool optimize(ExpressionTreeData& treeData);

	bool  optimizeNodeOrder();
	bool  optimizeNodeConstValue(int idxNode);
	bool  optimizeNodeBOpOrder(int idxNode);

	// CSE - Common Subexpression Elimination
	bool  eliminateCommonSubexpressions(ExpressionTreeData& treeData);

private:
	Node& getNode(int idx) { return mTreeNodes[idx]; }

	// Optimization passes
	int   optimizeNodeOrder_R(int idxNode, bool* outChanged = nullptr);
	int   optimizeConstantFolding_R(int idxNode, bool* outChanged = nullptr);
	int   optimizeAlgebraicSimplification_R(int idxNode, bool* outChanged = nullptr);
	bool  optimizeStrengthReduction_R(int idxNode);
	bool  optimizeNodeBOpOrder_R(int idxNode);

	// Helper functions
	bool  canExchangeNode(int idxNode, TokenType type)
	{
		CHECK(IsLeaf(idxNode));
		CHECK(CanExchange(type));
		Unit const& unit = mExprCodes[LEAF_UNIT_INDEX(idxNode)];
		return IsBinaryOperator(unit.type) && PrecedeceOrder(type) == PrecedeceOrder(unit.type);
	}

	unsigned haveConstValueChild(int idxNode)
	{
		CHECK(idxNode > 0);
		Node& node = mTreeNodes[idxNode];
		int result = 0;
		if (isConstValueNode(node.children[CN_LEFT])) result |= (1 << CN_LEFT);
		if (isConstValueNode(node.children[CN_RIGHT])) result |= (1 << CN_RIGHT);
		return result;
	}

	bool  isConstValueNode(int idxNode)
	{
		if (idxNode >= 0) return false;
		return mExprCodes[LEAF_UNIT_INDEX(idxNode)].type == VALUE_CONST;
	}

	bool isConstZero(int idxNode) { return isConstValueNode(idxNode) && mExprCodes[LEAF_UNIT_INDEX(idxNode)].constValue.asReal == 0.0f; }
	bool isConstOne(int idxNode) { return isConstValueNode(idxNode) && mExprCodes[LEAF_UNIT_INDEX(idxNode)].constValue.asReal == 1.0f; }
	bool isConstValue(int idxNode, RealType value) { return isConstValueNode(idxNode) && mExprCodes[LEAF_UNIT_INDEX(idxNode)].constValue.asReal == value; }

	bool areNodesEqual(int idxA, int idxB)
	{
		if (idxA == idxB) return true;
		bool isLeafA = IsLeaf(idxA);
		bool isLeafB = IsLeaf(idxB);
		if (isLeafA != isLeafB) return false;
		if (isLeafA) return IsValueEqual(mExprCodes[LEAF_UNIT_INDEX(idxA)], mExprCodes[LEAF_UNIT_INDEX(idxB)]);
		Node const& nodeA = mTreeNodes[idxA];
		Node const& nodeB = mTreeNodes[idxB];
		Unit const& unitA = mExprCodes[nodeA.indexOp];
		Unit const& unitB = mExprCodes[nodeB.indexOp];
		if (unitA.type != unitB.type) return false;
		// For functions, also compare the function identity
		if (unitA.type == FUNC_DEF)
		{
			if (unitA.symbol != unitB.symbol) return false;
		}
		else if (unitA.type == FUNC_SYMBOL)
		{
			if (unitA.funcSymbol.id != unitB.funcSymbol.id) return false;
		}
		return areNodesEqual(nodeA.children[CN_LEFT], nodeB.children[CN_LEFT]) && areNodesEqual(nodeA.children[CN_RIGHT], nodeB.children[CN_RIGHT]);
	}

	int replaceNodeWithChild(int idxNode, int childIndex) { return mTreeNodes[idxNode].children[childIndex]; }
	int convertNodeToConstant(int idxNode, RealType value)
	{
		Node& node = mTreeNodes[idxNode];
		mExprCodes[node.indexOp] = Unit::ConstValue(value);
		return -(node.indexOp + 1);
	}

	// CSE helper methods
	size_t hashSubtree(int idxNode);
	bool   areSubtreesIdentical(int idxA, int idxB);
	void   collectSubtreeHashes_R(int idxNode, std::unordered_map<size_t, std::vector<int>>& hashToNodes);
	int    eliminateCommonSubexpressions_R(int idxNode, std::unordered_map<int, int> const& nodesToReplace);

	Node*  mTreeNodes;
	int    mNumNodes;
	Unit*  mExprCodes;
};

class PostfixCodeOptimizer : public ExprParse
{
public:
	PostfixCodeOptimizer(UnitCodes& codes, SymbolTable const& table)
		:mPFCodes(codes), mTable(table) {}

	void optimize();
	bool optimizeZeroValue(int index);
	bool optimizeConstValue(int index);
	bool optimizeVarOrder(int index);
	bool optimizeOperatorOrder(int index);
	bool optimizeValueOrder(int index);

	void MoveElement(int start, int end, int move) { for (int i = start; i < end; ++i) mPFCodes[i + move] = mPFCodes[i]; }
	void printCodes() { ExprOutputContext constext(mTable); ExprParse::Print(constext, mPFCodes, true); }

	UnitCodes&    mPFCodes;
	SymbolTable const& mTable;
};

#endif // ExpressionOptimizer_h__
