#ifndef ExpressionTree_h__
#define ExpressionTree_h__

#include "ExpressionCore.h"
#include "SymbolTable.h"
#include <ostream>

struct ExpressionTreeData : public ExprParse
{
	NodeVec      nodes;
	UnitCodes    codes;

	template< typename TFunc >
	int  addOpCode(Unit const& code, TFunc Func)
	{
		int indexCode = (int)codes.size();
		codes.push_back(code);

		int indexNode = (int)nodes.size();
		Node node;
		node.indexOp = indexCode;
		nodes.push_back(node);

		Func(nodes[indexNode]);
		return indexNode;
	}

	int  addLeafCode(Unit const& code)
	{
		CHECK(IsValue(code.type));
		int indexOutput = (int)codes.size();
		codes.push_back(code);
		return -(indexOutput + 1);
	}

	Unit const& getLeafCode(int index) const
	{
		CHECK(IsLeaf(index));
		return codes[LEAF_UNIT_INDEX(index)];
	}

	void clear() { nodes.clear(); codes.clear(); }

	void printExpression(SymbolTable const& table);
	std::string getExpressionText(SymbolTable const& table);
	void output_R(class TreeExprOutputContext& context, Node const& parent, int idxNode);
};

template< typename TVisitor, bool bEmitFunction = false >
class TExprTreeVisitOp : ExprParse
{
public:
	TExprTreeVisitOp(ExpressionTreeData const& tree, TVisitor& visitor)
		:mTree(tree), mVisitor(visitor) {}

	void execute()
	{
		if (!mTree.nodes.empty())
		{
			Node const& root = mTree.nodes[0];
			execute_R(root.children[CN_LEFT]);
		}
	}

	void execute_R(int idxNode)
	{
		if (idxNode == 0) return;
		if (idxNode < 0)
		{
			mVisitor.visitValue(mTree.codes[LEAF_UNIT_INDEX(idxNode)]);
			return;
		}

		Node const& node = mTree.nodes[idxNode];
		Unit const& unit = mTree.codes[node.indexOp];
		if (unit.type == BOP_ASSIGN)
		{
			execute_R(node.children[CN_RIGHT]);
			execute_R(node.children[CN_LEFT]);
			mVisitor.visitOp(unit);
		}
		else
		{
			if constexpr (bEmitFunction) { if (unit.type == FUNC_DEF) mVisitor.visitFuncStart(unit); }
			execute_R(node.children[CN_LEFT]);
			if constexpr (bEmitFunction) { if (unit.type == IDT_SEPARETOR) mVisitor.visitSeparetor(); }
			execute_R(node.children[CN_RIGHT]);
			if (unit.type != IDT_SEPARETOR) mVisitor.visitOp(unit);
			if constexpr (bEmitFunction) { if (unit.type == FUNC_DEF) mVisitor.visitFuncEnd(unit); }
		}
	}

	ExpressionTreeData const& mTree;
	TVisitor& mVisitor;
};

class ExprOutputContext : ExprParse
{
public:
	ExprOutputContext(SymbolTable const& table);
	ExprOutputContext(SymbolTable const& table, std::ostream& stream) :table(table), mStream(stream) {}

	char funcArgSeparetor = ',';
	std::ostream& mStream;
	SymbolTable const& table;

	void output(char c) { mStream << c; }
	void output(char const* str) { mStream << str; }
	void output(Unit const& unit);
	void outputSpace(int num);
	void outputEOL() { mStream << "\n"; }
};

class ExprTreeBuilder : public ExprParse
{
	using Unit = ExprParse::Unit;
public:

	void build(ExpressionTreeData& treeData);
	enum ErrorCode
	{
		TREE_NO_ERROR = 0,
		TREE_OP_NO_CHILD,
		TREE_ASSIGN_LEFT_NOT_VAR,
		TREE_FUN_PARAM_NUM_NO_MATCH,
	};
	ErrorCode checkTreeError();

	void  printTree(SymbolTable const& table);
private:

	Node& getNode(int idx) { return mTreeNodes[idx]; }

	void  printTree_R(class ExprOutputContext& context, int idxNode, int depth);
	int   build_R(int idxStart, int idxEnd, bool bFuncDef);
	ErrorCode   checkTreeError_R(int idxNode);

	bool  canExchangeNode(int idxNode, TokenType type)
	{
		CHECK(IsLeaf(idxNode));
		CHECK(CanExchange(type));
		Unit const& unit = mExprCodes[LEAF_UNIT_INDEX(idxNode)];

		if (!IsBinaryOperator(unit.type))
			return false;
		if (PrecedeceOrder(type) != PrecedeceOrder(unit.type))
			return false;

		return true;
	}

	TArray< int > mIdxOpNext;
	Node*  mTreeNodes;
	int    mNumNodes;
	Unit*  mExprCodes;
};

#endif // ExpressionTree_h__
