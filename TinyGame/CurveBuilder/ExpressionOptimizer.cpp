#include "ExpressionOptimizer.h"
#include "SymbolTable.h"
#include "LogSystem.h"

#include "Core/TypeHash.h"

#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>

void PostfixCodeOptimizer::optimize()
{
#ifdef _DEBUG
	printCodes();
#endif
	int bIndex = 0;
	for (int index = 0; index < (int)mPFCodes.size(); ++index)
	{
		Unit& elem = mPFCodes[index];
		if (optimizeZeroValue(index))	index = 0;
		else if (optimizeConstValue(index)) index = 0;
		else if (optimizeValueOrder(index) ||
			optimizeOperatorOrder(index) ||
			optimizeVarOrder(index))
			index = 0;
		else bIndex = index;
	}
}

bool PostfixCodeOptimizer::optimizeValueOrder(int index)
{
	if (!(index < (int)mPFCodes.size()))
		return false;

	bool hadChange = false;

	Unit& elem = mPFCodes[index];
	if (ExprParse::IsValue(elem.type))
	{
		int num = 0;
		int opIndex = index;
		// Find Eval Fun/Operator Index 
		do {
			++opIndex;
			if (!(opIndex < (int)mPFCodes.size()))
			{
				opIndex = index;
				break;
			}
			Unit& elem2 = mPFCodes[opIndex];

			if (ExprParse::IsValue(elem2.type)) ++num;
			else if (ExprParse::IsBinaryOperator(elem2.type))
			{
				--num;
			}
			else if (ExprParse::IsFunction(elem2.type))
			{
				num -= (elem2.symbol->func.getArgNum() - 1);
			}

		} while (num > 0);

		if (opIndex - index > 2 && ExprParse::IsBinaryOperator(mPFCodes[opIndex].type))
		{
			Unit temp = mPFCodes[index];

			MoveElement(index + 1, opIndex, -1);

			mPFCodes[opIndex - 1] = temp;
			mPFCodes[opIndex].isReverse = true;

			hadChange = true;
#ifdef _DEBUG
			std::cout << "optimizeValueOrder :" << std::endl;
			printCodes();
#endif
			optimizeValueOrder(index);
		}
	}

	return hadChange;
}

bool PostfixCodeOptimizer::optimizeOperatorOrder(int index)
{
	if (!(index < (int)mPFCodes.size()))
		return false;

	Unit& elem = mPFCodes[index];
	if (!((elem.type == ExprParse::BOP_ADD || elem.type == ExprParse::BOP_SUB ||
		elem.type == ExprParse::BOP_MUL || elem.type == ExprParse::BOP_DIV) &&
		mPFCodes[index - 1].type == ExprParse::VALUE_CONST))
		return false;

	bool hadChange = false;
	int bIndex = 0;

	bool isNegive = false;
	if ((elem.type == ExprParse::BOP_SUB || elem.type == ExprParse::BOP_DIV))
		isNegive = true;

	int moveIndex = index;
	for (int i = index + 1; i < (int)mPFCodes.size(); ++i)
	{
		Unit& testElem = mPFCodes[i];
		if (ExprParse::IsFunction(testElem.type)) break;
		if (ExprParse::IsBinaryOperator(testElem.type))
		{
			if (ExprParse::PrecedeceOrder(elem.type) == ExprParse::PrecedeceOrder(testElem.type) &&
				mPFCodes[i - 1].type == ExprParse::VALUE_CONST)
			{
				if (isNegive)
				{
					ExprParse::TokenType type = mPFCodes[i].type;
					switch (mPFCodes[i].type)
					{
					case ExprParse::BOP_ADD: type = ExprParse::BOP_SUB; break;
					case ExprParse::BOP_SUB: type = ExprParse::BOP_ADD; break;
					case ExprParse::BOP_MUL: type = ExprParse::BOP_DIV; break;
					case ExprParse::BOP_DIV: type = ExprParse::BOP_MUL; break;
					}
					mPFCodes[i].type = type;

				}
				moveIndex = i;
			}
			else break;
		}
	}

	if (moveIndex != index)
	{
		Unit temp = mPFCodes[index];
		MoveElement(index + 1, moveIndex + 1, -1);
		mPFCodes[moveIndex] = temp;
#ifdef _DEBUG
		std::cout << "optimizeOperatorOrder :" << std::endl;
		printCodes();
#endif
		optimizeConstValue(index + 1);
		hadChange = true;
	}
	return hadChange;
}

bool PostfixCodeOptimizer::optimizeVarOrder(int index)
{
	if (!(index + 1 < (int)mPFCodes.size()))
		return false;

	if (mPFCodes[index].type != ExprParse::VALUE_VARIABLE ||
		!ExprParse::IsBinaryOperator(mPFCodes[index + 1].type))
		return false;

	Unit& elem = mPFCodes[index + 1];
	int moveIndex = index;

	for (int i = index - 2; i >= 0; i -= 2)
	{
		Unit& testElem = mPFCodes[i + 1];
		if (mPFCodes[i].type == ExprParse::VALUE_CONST &&
			ExprParse::IsBinaryOperator(mPFCodes[i + 1].type))
		{
			if (ExprParse::PrecedeceOrder(mPFCodes[index + 1].type) ==
				ExprParse::PrecedeceOrder(mPFCodes[i + 1].type))
			{
				moveIndex = i;
			}
			else break;
		}
		else break;
	}
	if (moveIndex != index)
	{
		Unit temp0 = mPFCodes[index];
		Unit temp1 = mPFCodes[index + 1];

		MoveElement(moveIndex, index, 2);
		mPFCodes[moveIndex] = temp0;
		mPFCodes[moveIndex + 1] = temp1;

#ifdef _DEBUG
		std::cout << "optimizeVarOrder :" << std::endl;
		printCodes();
#endif
		return true;
	}

	return false;
}

bool PostfixCodeOptimizer::optimizeConstValue(int index)
{
	if (!(index < (int)mPFCodes.size()))
		return false;

	bool hadChange = false;
	Unit& elem = mPFCodes[index];
	if (ExprParse::IsBinaryOperator(elem.type))
	{
		Unit& elemL = mPFCodes[index - 2];
		Unit& elemR = mPFCodes[index - 1];
		if (elemL.type == ExprParse::VALUE_CONST &&
			elemR.type == ExprParse::VALUE_CONST)
		{
			assert(elemL.constValue.layout == ValueLayout::Real && elemR.constValue.layout == ValueLayout::Real);
			bool done = false;
			RealType val = 0;

			switch (elem.type)
			{
			case ExprParse::BOP_ADD:
				val = elemL.constValue.asReal + elemR.constValue.asReal;
				done = true;
				break;
			case ExprParse::BOP_MUL:
				val = elemL.constValue.asReal * elemR.constValue.asReal;
				done = true;
				break;
			case ExprParse::BOP_SUB:
				if (elem.isReverse)
					val = elemR.constValue.asReal - elemL.constValue.asReal;
				else
					val = elemL.constValue.asReal - elemR.constValue.asReal;
				done = true;
				break;
			case ExprParse::BOP_DIV:
				if (elem.isReverse)
					val = elemR.constValue.asReal / elemL.constValue.asReal;
				else
					val = elemL.constValue.asReal / elemR.constValue.asReal;
				done = true;
				break;
			}
			if (done)
			{
				elem = Unit::ConstValue(val);

				MoveElement(index, (int)mPFCodes.size(), -2);
				mPFCodes.pop_back();
				mPFCodes.pop_back();
#ifdef _DEBUG
				std::cout << "optimizeConstValue :" << std::endl;
				printCodes();
#endif
				optimizeConstValue(index - 2);
				hadChange = true;
			}

		}
	}
	else if (ExprParse::IsUnaryOperator(elem.type))
	{
		Unit& elemL = mPFCodes[index - 1];

		if (elemL.type == ExprParse::VALUE_CONST)
		{
			bool done = false;
			RealType val = 0;

			switch (elem.type)
			{
			case ExprParse::UOP_MINS:
				val = -elemL.constValue.asDouble;
				done = true;
				break;
			}

			if (done)
			{
				elem = Unit::ConstValue(val);

				MoveElement(index, (int)mPFCodes.size(), -1);
				mPFCodes.pop_back();
#ifdef _DEBUG
				std::cout << "optimizeConstValue :" << std::endl;
				printCodes();
#endif
				optimizeConstValue(index - 1);
				hadChange = true;
			}
		}
	}
	else if (ExprParse::IsFunction(elem.type))
	{
		if (elem.type == ExprParse::FUNC_SYMBOL)
			return false;

		RealType val[5];
		int num = elem.symbol->func.getArgNum();
		void* funPtr = elem.symbol->func.funcPtr;
		bool testOk = true;
		for (int j = 0; j < num; ++j)
		{
			if (mPFCodes[index - j - 1].type != ExprParse::VALUE_CONST)
			{
				testOk = false;
				break;
			}
			val[j] = mPFCodes[index - j - 1].constValue.asReal;
		}
		if (testOk)
		{
			switch (num)
			{
			case 0:
				val[0] = (*(FuncType0)funPtr)();
				break;
			case 1:
				val[0] = (*(FuncType1)funPtr)(val[0]);
				break;
			case 2:
				val[0] = (*(FuncType2)funPtr)(val[1], val[0]);
				break;
			case 3:
				val[0] = (*(FuncType3)funPtr)(val[2], val[1], val[0]);
				break;
			case 4:
				val[0] = (*(FuncType4)funPtr)(val[3], val[2], val[1], val[0]);
				break;
			case 5:
				val[0] = (*(FuncType5)funPtr)(val[4], val[3], val[2], val[1], val[0]);
				break;
			}
			elem = Unit::ConstValue(val[0]);
			for (int n = index - num; n < (int)mPFCodes.size() - num; ++n)
				mPFCodes[n] = mPFCodes[n + num];

			for (int j = 0; j < num; ++j)
				mPFCodes.pop_back();
#ifdef _DEBUG
			std::cout << "optimizeConstValue :" << std::endl;
			printCodes();
#endif
			optimizeConstValue(index - num);
			hadChange = true;
		}
	}

	return  hadChange;
}

bool PostfixCodeOptimizer::optimizeZeroValue(int index)
{
	if (!(index < (int)mPFCodes.size()))
		return false;

	bool hadChange = false;
	Unit& elem = mPFCodes[index];
	if (elem.type == ExprParse::BOP_ADD || elem.type == ExprParse::BOP_SUB)
	{
		if (mPFCodes[index - 1].type == ExprParse::VALUE_CONST &&
			mPFCodes[index - 1].constValue.asReal == 0)
		{
			MoveElement(index + 1, (int)mPFCodes.size(), -2);
			mPFCodes.pop_back();
			mPFCodes.pop_back();
			hadChange = true;
#ifdef _DEBUG
			std::cout << "optimizeZeroValue :" << std::endl;
			printCodes();
#endif
		}
	}
	return hadChange;
}

bool ExprTreeOptimizer::optimize(ExpressionTreeData& treeData)
{
	mTreeNodes = treeData.nodes.data();
	mNumNodes = (int)treeData.nodes.size();
	mExprCodes = treeData.codes.data();

	if (mNumNodes == 0)
		return false;

	Node& root = mTreeNodes[0];
	if (root.children[ExprParse::CN_LEFT] <= 0)
		return false;

	// Helper lambda to generate postfix codes and log them
	auto logPostfix = [&treeData](char const* passName) {
		struct Visitor {
			void visitValue(ExprParse::Unit const& unit) { codes.push_back(unit); }
			void visitOp(ExprParse::Unit const& unit) { codes.push_back(unit); }
			ExprParse::UnitCodes codes;
		};
		Visitor visitor;
		TExprTreeVisitOp<Visitor> op(treeData, visitor);
		op.execute();
		
		std::stringstream ss;
		for (auto const& unit : visitor.codes)
		{
			ss << "[";
			if (IsBinaryOperator(unit.type) && unit.isReverse) ss << "re";
			switch (unit.type)
			{
			case VALUE_CONST: ss << unit.constValue.asReal; break;
			case VALUE_VARIABLE: ss << "var"; break;
			case VALUE_INPUT: ss << "in" << unit.input.index; break;
			case VALUE_EXPR_TEMP: ss << "Temp" << unit.exprTemp.tempIndex; break;
			case BOP_ADD: ss << "+"; break;
			case BOP_SUB: ss << "-"; break;
			case BOP_MUL: ss << "*"; break;
			case BOP_DIV: ss << "/"; break;
			case FUNC_DEF: ss << "fn"; break;
			case FUNC_SYMBOL: ss << "fs" << unit.funcSymbol.id; break;
			default: ss << "op" << (int)unit.type; break;
			}
			if (unit.storeIndex != -1) ss << "(s)";
			ss << "]";
		}
		std::cout << "ExprOpt: " << passName << ": " << ss.str() << std::endl;
	};

	// Track if any optimization was applied
	bool anyChanged = false;

	// Optimization passes in order:
	// 1. Algebraic simplification (x + 0 = x, x * 1 = x, etc.)
	bool changed = false;
	do {
		changed = false;
		root.children[ExprParse::CN_LEFT] = optimizeAlgebraicSimplification_R(root.children[ExprParse::CN_LEFT], &changed);
		anyChanged |= changed;
	} while (changed);
	logPostfix("AlgSimp");

	// 2. Constant folding
	changed = false;
	root.children[ExprParse::CN_LEFT] = optimizeConstantFolding_R(root.children[ExprParse::CN_LEFT], &changed);
	anyChanged |= changed;
	logPostfix("ConstFold");

	// 3. Strength reduction (x * 2 => x + x, x^2 => x * x, etc.)
	do {
		changed = false;
		changed |= optimizeStrengthReduction_R(root.children[ExprParse::CN_LEFT]);
		anyChanged |= changed;
	} while (changed);
	logPostfix("StrengthRed");

	// 4. Binary operator order optimization
	do {
		changed = false;
		changed |= optimizeNodeBOpOrder_R(root.children[ExprParse::CN_LEFT]);
		anyChanged |= changed;
	} while (changed);
	logPostfix("BOpOrder");

	// 5. Another constant folding pass (previous optimizations may have created new constants)
	changed = false;
	root.children[ExprParse::CN_LEFT] = optimizeConstantFolding_R(root.children[ExprParse::CN_LEFT], &changed);
	anyChanged |= changed;
	logPostfix("ConstFold2");

	// 6. Common Subexpression Elimination (CSE)
	// Must run before Node Order optimization which assumes final tree structure
	do {
		changed = eliminateCommonSubexpressions(treeData);
		anyChanged |= changed;
	} while (changed);
	logPostfix("CSE");

	// 7. Node order optimization (for better register allocation)
	changed = false;
	changed = optimizeNodeOrder();
	anyChanged |= changed;
	logPostfix("NodeOrder");

	return anyChanged;
}

bool ExprTreeOptimizer::optimizeNodeOrder()
{
	if (mNumNodes == 0)
		return false;

	Node& root = mTreeNodes[0];
	if (root.children[ExprParse::CN_LEFT] > 0)
	{
		bool changed = false;
		optimizeNodeOrder_R(root.children[ExprParse::CN_LEFT], &changed);
		return changed;
	}

	return false;
}

int ExprTreeOptimizer::optimizeNodeOrder_R(int idxNode, bool* outChanged)
{
	Node& node = mTreeNodes[idxNode];

	int depthL = 0;
	if (node.children[ExprParse::CN_LEFT] > 0)
		depthL = optimizeNodeOrder_R(node.children[ExprParse::CN_LEFT], outChanged);

	int depthR = 0;
	if (node.children[ExprParse::CN_RIGHT] > 0)
		depthR = optimizeNodeOrder_R(node.children[ExprParse::CN_RIGHT], outChanged);


	Unit& unit = mExprCodes[node.indexOp];
	if (ExprParse::IsBinaryOperator(unit.type) && ExprParse::CanReverse(unit.type))
	{
		bool needSwap = false;
		if (node.children[ExprParse::CN_LEFT] < 0)
		{
			if (node.children[ExprParse::CN_RIGHT] > 0)
			{
				Unit const& unitL = mExprCodes[node.getLeaf(ExprParse::CN_LEFT)];
				if (ExprParse::IsValue(unitL.type))
				{
					Node& nodeR = mTreeNodes[node.children[ExprParse::CN_RIGHT]];
					Unit& unitR = mExprCodes[nodeR.indexOp];

					if (ExprParse::IsOperator(unitR.type) ||
						ExprParse::IsFunction(unitR.type))
					{
						std::swap(node.children[ExprParse::CN_LEFT], node.children[ExprParse::CN_RIGHT]);
						unit.isReverse = !unit.isReverse;
						if (outChanged)
							*outChanged = true;  // 交换发生了
					}
				}
			}
		}
		else if (depthL < depthR)
		{
			std::swap(node.children[ExprParse::CN_LEFT], node.children[ExprParse::CN_RIGHT]);
			unit.isReverse = !unit.isReverse;
			if (outChanged)
				*outChanged = true;  // 交换发生了
		}
	}

	return ((depthL > depthR) ? depthL : depthR) + 1;
}

bool ExprTreeOptimizer::optimizeNodeConstValue(int idxNode)
{
	Node& node = mTreeNodes[idxNode];
	Unit& unit = mExprCodes[node.indexOp];

	RealType value = 0;

	if (ExprParse::IsBinaryOperator(unit.type))
	{
		if (node.children[ExprParse::CN_LEFT] >= 0)
			return false;
		if (node.children[ExprParse::CN_RIGHT] >= 0)
			return false;

		Unit const& unitL = mExprCodes[node.getLeaf(ExprParse::CN_LEFT)];
		Unit const& unitR = mExprCodes[node.getLeaf(ExprParse::CN_RIGHT)];

		if (unitL.type == ExprParse::VALUE_CONST)
		{
			if (unitR.type != ExprParse::VALUE_CONST)
			{
				return false;
			}
		}
		else
		{
			return false;
		}

		switch (unit.type)
		{
		case ExprParse::BOP_ADD:
			value = unitL.constValue.asReal + unitR.constValue.asReal;
			break;
		case ExprParse::BOP_MUL:
			value = unitL.constValue.asReal * unitR.constValue.asReal;
			break;
		case ExprParse::BOP_SUB:
			if (unit.isReverse)
				value = unitR.constValue.asReal - unitL.constValue.asReal;
			else
				value = unitL.constValue.asReal - unitR.constValue.asReal;
			break;
		case ExprParse::BOP_DIV:
			if (unit.isReverse)
				value = unitR.constValue.asReal / unitL.constValue.asReal;
			else
				value = unitL.constValue.asReal / unitR.constValue.asReal;
			break;
		case ExprParse::BOP_COMMA:
			value = unitR.constValue.asReal;
			break;
		default:
			return false;
		}
	}
	else if (ExprParse::IsUnaryOperator(unit.type))
	{
		if (node.children[ExprParse::CN_LEFT] >= 0)
			return false;

		Unit const& unitR = mExprCodes[node.getLeaf(ExprParse::CN_RIGHT)];

		if (unitR.type != ExprParse::VALUE_CONST)
			return false;

		switch (unit.type)
		{
		case ExprParse::UOP_MINS:
			value = -unitR.constValue.asReal;
			break;
		case ExprParse::UOP_PLUS:
			value = unitR.constValue.asReal;
			break;
		default:
			return false;
		}
	}
	else if (ExprParse::IsFunction(unit.type))
	{
		if (unit.type == ExprParse::FUNC_SYMBOL)
			return false;

		RealType params[5];
		int   numParam = unit.symbol->func.getArgNum();
		void* funcPtr = unit.symbol->func.funcPtr;

		if (numParam)
		{
			int idxParam = node.children[ExprParse::CN_LEFT];
			for (int n = 0; n < numParam - 1; ++n)
			{
				assert(idxParam > 0);
				Node& nodeParam = mTreeNodes[idxParam];
				CHECK(mExprCodes[nodeParam.indexOp].type == ExprParse::IDT_SEPARETOR);

				if (nodeParam.children[ExprParse::CN_RIGHT] >= 0)
					return false;

				Unit const& unitVal = mExprCodes[nodeParam.getLeaf(ExprParse::CN_RIGHT)];
				if (unitVal.type != ExprParse::VALUE_CONST)
					return false;

				params[n] = unitVal.constValue.asReal;
				idxParam = nodeParam.children[ExprParse::CN_LEFT];
			}

			Unit const& unitVal = mExprCodes[ExprParse::LEAF_UNIT_INDEX(idxParam)];
			if (unitVal.type != ExprParse::VALUE_CONST)
				return false;

			params[numParam - 1] = unitVal.constValue.asReal;
		}

		switch (numParam)
		{
		case 0:
			value = (*static_cast<FuncType0>(funcPtr))();
			break;
		case 1:
			value = (*static_cast<FuncType1>(funcPtr))(params[0]);
			break;
		case 2:
			value = (*static_cast<FuncType2>(funcPtr))(params[1], params[0]);
			break;
		case 3:
			value = (*static_cast<FuncType3>(funcPtr))(params[2], params[1], params[0]);
			break;
		case 4:
			value = (*static_cast<FuncType4>(funcPtr))(params[3], params[2], params[1], params[0]);
			break;
		case 5:
			value = (*static_cast<FuncType5>(funcPtr))(params[4], params[3], params[2], params[1], params[0]);
			break;
		}
	}
	else
	{
		return false;
	}

	unit = Unit::ConstValue(value);
	return true;
}

bool ExprTreeOptimizer::optimizeNodeBOpOrder(int idxNode)
{
	assert(idxNode > 0);

	Node& node = mTreeNodes[idxNode];
	Unit& unit = mExprCodes[node.indexOp];

	if (!ExprParse::IsBinaryOperator(unit.type) ||
		!ExprParse::CanExchange(unit.type))
		return false;

	// Optimization 1: Move constants to the right side
	if (isConstValueNode(node.children[ExprParse::CN_LEFT]) &&
		!isConstValueNode(node.children[ExprParse::CN_RIGHT]))
	{
		std::swap(node.children[ExprParse::CN_LEFT], node.children[ExprParse::CN_RIGHT]);
		unit.isReverse = !unit.isReverse;
		return true;
	}

	// Optimization 2: Associativity optimization for constant folding
	if (node.children[ExprParse::CN_LEFT] > 0)
	{
		Node& leftNode = mTreeNodes[node.children[ExprParse::CN_LEFT]];
		Unit& leftUnit = mExprCodes[leftNode.indexOp];

		// Check if left child is a binary operator with same precedence
		if (ExprParse::IsBinaryOperator(leftUnit.type) &&
			ExprParse::PrecedeceOrder(unit.type) == ExprParse::PrecedeceOrder(leftUnit.type) &&
			ExprParse::CanExchange(leftUnit.type))
		{
			// Both right children are constants - swap them to enable folding
			if (isConstValueNode(node.children[ExprParse::CN_RIGHT]) &&
				isConstValueNode(leftNode.children[ExprParse::CN_RIGHT]))
			{
				// Transform: (a op b) op c => (a op c) op b
				std::swap(node.children[ExprParse::CN_RIGHT], leftNode.children[ExprParse::CN_RIGHT]);
				return true;
			}

			if (isConstValueNode(node.children[ExprParse::CN_RIGHT]) &&
				isConstValueNode(leftNode.children[ExprParse::CN_LEFT]))
			{
				if (unit.type == ExprParse::BOP_ADD || unit.type == ExprParse::BOP_MUL)
				{
					std::swap(leftNode.children[ExprParse::CN_LEFT], leftNode.children[ExprParse::CN_RIGHT]);
					leftUnit.isReverse = !leftUnit.isReverse;
					return true;
				}
			}
		}
	}

	// Optimization 3: Reorder based on complexity
	if (node.children[ExprParse::CN_LEFT] < 0 && node.children[ExprParse::CN_RIGHT] > 0)
	{
		if (!isConstValueNode(node.children[ExprParse::CN_LEFT]))
		{
			std::swap(node.children[ExprParse::CN_LEFT], node.children[ExprParse::CN_RIGHT]);
			unit.isReverse = !unit.isReverse;
			return true;
		}
	}

	return false;
}

int ExprTreeOptimizer::optimizeConstantFolding_R(int idxNode, bool* outChanged)
{
	if (idxNode <= 0)
		return idxNode;

	Node& node = mTreeNodes[idxNode];

	int oldLeftChild = node.children[ExprParse::CN_LEFT];
	int oldRightChild = node.children[ExprParse::CN_RIGHT];

	node.children[ExprParse::CN_LEFT] = optimizeConstantFolding_R(node.children[ExprParse::CN_LEFT], outChanged);
	node.children[ExprParse::CN_RIGHT] = optimizeConstantFolding_R(node.children[ExprParse::CN_RIGHT], outChanged);

	if (outChanged)
	{
		if (oldLeftChild != node.children[ExprParse::CN_LEFT] || oldRightChild != node.children[ExprParse::CN_RIGHT])
			*outChanged = true;
	}

	if (optimizeNodeConstValue(idxNode))
	{
		if (outChanged)
			*outChanged = true;

		int indexCode = node.indexOp;
		return -(indexCode + 1);
	}

	return idxNode;
}

int ExprTreeOptimizer::optimizeAlgebraicSimplification_R(int idxNode, bool* outChanged)
{
	if (idxNode <= 0)
		return idxNode;

	Node& node = mTreeNodes[idxNode];

	int oldLeftChild = node.children[ExprParse::CN_LEFT];
	int oldRightChild = node.children[ExprParse::CN_RIGHT];

	node.children[ExprParse::CN_LEFT] = optimizeAlgebraicSimplification_R(node.children[ExprParse::CN_LEFT], outChanged);
	node.children[ExprParse::CN_RIGHT] = optimizeAlgebraicSimplification_R(node.children[ExprParse::CN_RIGHT], outChanged);

	if (outChanged)
	{
		if (oldLeftChild != node.children[ExprParse::CN_LEFT] || oldRightChild != node.children[ExprParse::CN_RIGHT])
			*outChanged = true;
	}

	Unit& unit = mExprCodes[node.indexOp];

	if (!ExprParse::IsBinaryOperator(unit.type))
		return idxNode;

	if (unit.type == ExprParse::BOP_ADD)
	{
		if (isConstZero(node.children[ExprParse::CN_RIGHT]))
		{
			if (outChanged) *outChanged = true;
			return node.children[ExprParse::CN_LEFT];
		}
		if (isConstZero(node.children[ExprParse::CN_LEFT]))
		{
			if (outChanged) *outChanged = true;
			return node.children[ExprParse::CN_RIGHT];
		}
	}

	if (unit.type == ExprParse::BOP_SUB)
	{
		if (isConstZero(node.children[ExprParse::CN_RIGHT]) && !unit.isReverse)
		{
			if (outChanged) *outChanged = true;
			return node.children[ExprParse::CN_LEFT];
		}

		if (areNodesEqual(node.children[ExprParse::CN_LEFT], node.children[ExprParse::CN_RIGHT]))
		{
			if (outChanged) *outChanged = true;
			mExprCodes[node.indexOp] = Unit::ConstValue(0.0f);
			return -(node.indexOp + 1);
		}
	}

	if (unit.type == ExprParse::BOP_MUL)
	{
		if (isConstZero(node.children[ExprParse::CN_LEFT]) || isConstZero(node.children[ExprParse::CN_RIGHT]))
		{
			if (outChanged) *outChanged = true;
			mExprCodes[node.indexOp] = Unit::ConstValue(0.0f);
			return -(node.indexOp + 1);
		}
		if (isConstOne(node.children[ExprParse::CN_RIGHT]))
		{
			if (outChanged) *outChanged = true;
			return node.children[ExprParse::CN_LEFT];
		}
		if (isConstOne(node.children[ExprParse::CN_LEFT]))
		{
			if (outChanged) *outChanged = true;
			return node.children[ExprParse::CN_RIGHT];
		}
	}

	if (unit.type == ExprParse::BOP_DIV)
	{
		if (isConstOne(node.children[ExprParse::CN_RIGHT]) && !unit.isReverse)
		{
			if (outChanged) *outChanged = true;
			return node.children[ExprParse::CN_LEFT];
		}

		if (areNodesEqual(node.children[ExprParse::CN_LEFT], node.children[ExprParse::CN_RIGHT]))
		{
			if (outChanged) *outChanged = true;
			mExprCodes[node.indexOp] = Unit::ConstValue(1.0f);
			return -(node.indexOp + 1);
		}
	}

	return idxNode;
}

bool ExprTreeOptimizer::optimizeStrengthReduction_R(int idxNode)
{
	if (idxNode <= 0) return false;
	Node& node = mTreeNodes[idxNode];
	bool changed = false;
	changed |= optimizeStrengthReduction_R(node.children[ExprParse::CN_LEFT]);
	changed |= optimizeStrengthReduction_R(node.children[ExprParse::CN_RIGHT]);
	return changed;
}

bool ExprTreeOptimizer::optimizeNodeBOpOrder_R(int idxNode)
{
	if (idxNode <= 0) return false;
	Node& node = mTreeNodes[idxNode];
	bool changed = false;
	changed |= optimizeNodeBOpOrder_R(node.children[ExprParse::CN_LEFT]);
	changed |= optimizeNodeBOpOrder_R(node.children[ExprParse::CN_RIGHT]);
	changed |= optimizeNodeBOpOrder(idxNode);
	return changed;
}

size_t ExprTreeOptimizer::hashSubtree(int idxNode)
{
	if (idxNode == 0) return 0;

	Unit const* pUnit = nullptr;
	if (ExprParse::IsLeaf(idxNode))
		pUnit = &mExprCodes[ExprParse::LEAF_UNIT_INDEX(idxNode)];
	else
		pUnit = &mExprCodes[mTreeNodes[idxNode].indexOp];

	Unit const& unit = *pUnit;

	uint32 h = HashValue((int)unit.type);
	switch (unit.type)
	{
	case ExprParse::VALUE_CONST:
		h = HashCombine(h, unit.constValue.asReal);
		break;
	case ExprParse::VALUE_VARIABLE:
		h = HashCombine(h, unit.variable.ptr);
		break;
	case ExprParse::VALUE_INPUT:
		h = HashCombine(h, unit.input.index);
		break;
	case ExprParse::VALUE_EXPR_TEMP:
		h = HashCombine(h, unit.exprTemp.tempIndex);
		break;
	case ExprParse::FUNC_SYMBOL:
		h = HashCombine(h, unit.funcSymbol.id);
		break;
	case ExprParse::FUNC_DEF:
		h = HashCombine(h, unit.symbol);
		break;
	}

	if (!ExprParse::IsLeaf(idxNode) && !ExprParse::IsValue(unit.type))
	{
		Node const& node = mTreeNodes[idxNode];
		h = CombineHash(h, (uint32)hashSubtree(node.children[ExprParse::CN_LEFT]));
		h = CombineHash(h, (uint32)hashSubtree(node.children[ExprParse::CN_RIGHT]));
	}
	return h;
}

bool ExprTreeOptimizer::areSubtreesIdentical(int idxA, int idxB)
{
	return areNodesEqual(idxA, idxB);
}

bool ExprTreeOptimizer::eliminateCommonSubexpressions(ExpressionTreeData& treeData)
{
	if (treeData.nodes.empty()) return false;

	int nextTempIndex = 0;
	// Scan for existing temps to avoid ID collision
	for (auto const& unit : treeData.codes)
	{
		if (unit.type == ExprParse::VALUE_EXPR_TEMP)
			nextTempIndex = std::max(nextTempIndex, (int)unit.exprTemp.tempIndex + 1);
		if (unit.storeIndex != -1)
			nextTempIndex = std::max(nextTempIndex, (int)unit.storeIndex + 1);
	}

	// Phase 1: Collect all subtree hashes WITHOUT any modification
	// Map from hash to list of node indices
	std::unordered_map<size_t, std::vector<int>> hashToNodes;
	
	Node& root = treeData.nodes[0];
	collectSubtreeHashes_R(root.children[ExprParse::CN_LEFT], hashToNodes);
	
	// Phase 2: Find duplicate subtrees and assign temp indices
	// Map from first node index to temp index (for nodes that should be stored)
	std::unordered_map<int, int> nodeToTempIndex;
	// Set of nodes that should be replaced with temp reads
	std::unordered_map<int, int> nodesToReplace; // nodeIndex -> tempIndex
	
	for (auto& [hash, nodeList] : hashToNodes)
	{
		if (nodeList.size() < 2) continue;
		
		// Group identical subtrees (hash collision handling)
		std::vector<std::vector<int>> identicalGroups;
		for (int nodeIdx : nodeList)
		{
			bool foundGroup = false;
			for (auto& group : identicalGroups)
			{
				if (areSubtreesIdentical(nodeIdx, group[0]))
				{
					group.push_back(nodeIdx);
					foundGroup = true;
					break;
				}
			}
			if (!foundGroup)
			{
				identicalGroups.push_back({nodeIdx});
			}
		}
		
		// For each group with 2+ members, assign a temp variable
		for (auto& group : identicalGroups)
		{
			if (group.size() < 2) continue;
			
			int tempIdx = nextTempIndex++;
			
			// First occurrence should store the result
			nodeToTempIndex[group[0]] = tempIdx;
			
			// Subsequent occurrences should read from temp
			for (size_t i = 1; i < group.size(); ++i)
			{
				nodesToReplace[group[i]] = tempIdx;
			}
		}
	}
	
	if (nodesToReplace.empty())
		return false;
	
	// Phase 3: Apply the transformations
	// Mark store nodes
	for (auto& [nodeIdx, tempIdx] : nodeToTempIndex)
	{
		Node& node = mTreeNodes[nodeIdx];
		mExprCodes[node.indexOp].storeIndex = (int16)tempIdx;
	}
	
	// Replace duplicate nodes with temp reads and update tree structure
	root.children[ExprParse::CN_LEFT] = eliminateCommonSubexpressions_R(root.children[ExprParse::CN_LEFT], nodesToReplace);
	
	// Phase 4: Remove unused stores and remap temp indices
	// Count how many times each temp is read
	std::unordered_map<int, int> tempReadCount;
	for (auto const& unit : treeData.codes)
	{
		if (unit.type == ExprParse::VALUE_EXPR_TEMP)
		{
			tempReadCount[unit.exprTemp.tempIndex]++;
		}
	}
	
	// Build remap table: old index -> new contiguous index
	std::unordered_map<int, int> tempRemap;
	int newTempIndex = 0;
	for (auto const& [oldIdx, count] : tempReadCount)
	{
		tempRemap[oldIdx] = newTempIndex++;
	}
	
	// Apply remapping and remove unused stores
	for (auto& unit : treeData.codes)
	{
		if (unit.storeIndex != -1)
		{
			auto it = tempRemap.find(unit.storeIndex);
			if (it != tempRemap.end())
			{
				unit.storeIndex = (int16)it->second;
			}
			else
			{
				unit.storeIndex = INDEX_NONE;
			}
		}
		if (unit.type == ExprParse::VALUE_EXPR_TEMP)
		{
			auto it = tempRemap.find(unit.exprTemp.tempIndex);
			if (it != tempRemap.end())
			{
				unit.exprTemp.tempIndex = (int16)it->second;
			}
		}
	}
	
	// Phase 5: Temp slot reuse based on liveness analysis
	// Generate postfix codes to analyze execution order
	struct PostfixVisitor
	{
		void visitValue(ExprParse::Unit const& unit) { codes.push_back(&unit); }
		void visitOp(ExprParse::Unit const& unit) { codes.push_back(&unit); }
		std::vector<Unit const*> codes;
	};
	PostfixVisitor visitor;
	TExprTreeVisitOp<PostfixVisitor> visitOp(treeData, visitor);
	visitOp.execute();
	
	// Find the last use position for each temp
	std::unordered_map<int, int> tempLastUse; // tempIndex -> last use position in postfix
	for (int i = 0; i < (int)visitor.codes.size(); ++i)
	{
		Unit const* pUnit = visitor.codes[i];
		if (pUnit->type == ExprParse::VALUE_EXPR_TEMP)
		{
			tempLastUse[pUnit->exprTemp.tempIndex] = i;
		}
	}
	
	// Reassign temp slots based on liveness
	std::unordered_map<int, int> tempSlotRemap; // old tempIndex -> new slot
	std::vector<int> slotLastUse; // for each slot, the position where it becomes free
	
	for (int i = 0; i < (int)visitor.codes.size(); ++i)
	{
		Unit const* pUnit = visitor.codes[i];
		if (pUnit->storeIndex != -1)
		{
			int oldTempIdx = pUnit->storeIndex;
			
			// Find a free slot (one whose last use is before this position)
			int assignedSlot = -1;
			for (int slot = 0; slot < (int)slotLastUse.size(); ++slot)
			{
				if (slotLastUse[slot] < i)
				{
					assignedSlot = slot;
					break;
				}
			}
			
			if (assignedSlot == -1)
			{
				// No free slot, allocate new one
				assignedSlot = (int)slotLastUse.size();
				slotLastUse.push_back(-1);
			}
			
			// Update when this slot will become free
			auto itLastUse = tempLastUse.find(oldTempIdx);
			if (itLastUse != tempLastUse.end())
			{
				slotLastUse[assignedSlot] = itLastUse->second;
			}
			
			tempSlotRemap[oldTempIdx] = assignedSlot;
		}
	}
	
	// Apply the slot remapping
	for (auto& unit : treeData.codes)
	{
		if (unit.storeIndex != -1)
		{
			auto it = tempSlotRemap.find(unit.storeIndex);
			if (it != tempSlotRemap.end())
			{
				unit.storeIndex = (int16)it->second;
			}
		}
		if (unit.type == ExprParse::VALUE_EXPR_TEMP)
		{
			auto it = tempSlotRemap.find(unit.exprTemp.tempIndex);
			if (it != tempSlotRemap.end())
			{
				unit.exprTemp.tempIndex = (int16)it->second;
			}
		}
	}
	
	return true;
}

void ExprTreeOptimizer::collectSubtreeHashes_R(int idxNode, std::unordered_map<size_t, std::vector<int>>& hashToNodes)
{
	if (idxNode <= 0) return;
	
	Node& node = mTreeNodes[idxNode];
	Unit const& unit = mExprCodes[node.indexOp];
	
	// Skip value nodes (they are leaves)
	if (ExprParse::IsValue(unit.type))
		return;
	
	// Recursively collect hashes from children first
	collectSubtreeHashes_R(node.children[ExprParse::CN_LEFT], hashToNodes);
	collectSubtreeHashes_R(node.children[ExprParse::CN_RIGHT], hashToNodes);
	
	// Compute hash for this subtree and add to map
	size_t h = hashSubtree(idxNode);
	hashToNodes[h].push_back(idxNode);
}

int ExprTreeOptimizer::eliminateCommonSubexpressions_R(int idxNode, std::unordered_map<int, int> const& nodesToReplace)
{
	if (idxNode <= 0) return idxNode;

	// Check if this node should be replaced with a temp read
	auto it = nodesToReplace.find(idxNode);
	if (it != nodesToReplace.end())
	{
		Node& node = mTreeNodes[idxNode];
		mExprCodes[node.indexOp] = Unit::ExprTemp(it->second);
		return -(node.indexOp + 1);
	}

	Node& node = mTreeNodes[idxNode];
	if (ExprParse::IsValue(mExprCodes[node.indexOp].type))
	{
		return -(node.indexOp + 1);
	}

	// Recursively process children
	node.children[ExprParse::CN_LEFT] = eliminateCommonSubexpressions_R(node.children[ExprParse::CN_LEFT], nodesToReplace);
	node.children[ExprParse::CN_RIGHT] = eliminateCommonSubexpressions_R(node.children[ExprParse::CN_RIGHT], nodesToReplace);

	return idxNode;
}
