#include "ExpressionTree.h"
#include <iostream>
#include <sstream>


struct TreeExprOutputContext : ExprOutputContext
{
	using ExprOutputContext::ExprOutputContext;
	ExprParse::Node* nodeOpLeft = nullptr;
};

void ExpressionTreeData::printExpression(SymbolTable const& table)
{
	if (!nodes.empty())
	{
		TreeExprOutputContext context(table);
		Node& root = nodes[0];
		output_R(context, root, root.children[CN_LEFT]);
	}
}

std::string ExpressionTreeData::getExpressionText(SymbolTable const& table)
{
	if (!nodes.empty())
	{
		std::stringstream ss;
		TreeExprOutputContext context(table, ss);
		Node& root = nodes[0];
		output_R(context, root, root.children[CN_LEFT]);
		return ss.str();
	}
	return "";
}

void ExpressionTreeData::output_R(TreeExprOutputContext& context, Node const& parent, int idxNode)
{
	if (idxNode < 0)
	{
		Unit const& code = codes[LEAF_UNIT_INDEX(idxNode)];
		context.output(code);
		return;
	}
	else if (idxNode == 0) return;

	Node& node = nodes[idxNode];
	Unit& unit = codes[node.indexOp];

	if (ExprParse::IsFunction(unit.type))
	{
		context.output(unit);
		context.output('(');
		context.nodeOpLeft = &node;
		output_R(context, node, node.children[CN_LEFT]);
		if (node.children[CN_RIGHT] > 0)
		{
			context.output(context.funcArgSeparetor);
			output_R(context, node, node.children[CN_RIGHT]);
		}
		context.output(')');
	}
	else if (ExprParse::IsBinaryOperator(unit.type))
	{
		bool bNeedBracket = true;
		if (parent.indexOp != INDEX_NONE)
		{
			Unit const& opParent = codes[parent.indexOp];
			if (IsFunction(opParent.type)) { bNeedBracket = false; }
			else if (IsBinaryOperator(opParent.type))
			{
				if (ExprParse::PrecedeceOrder(opParent.type) < ExprParse::PrecedeceOrder(unit.type)) { bNeedBracket = false; }
				else if (ExprParse::PrecedeceOrder(opParent.type) == ExprParse::PrecedeceOrder(unit.type) && (opParent.type != BOP_SUB && opParent.type != BOP_DIV)) { bNeedBracket = false; }
			}
		}
		else { bNeedBracket = false; }

		if (bNeedBracket) context.output('(');
		output_R(context, node, node.children[CN_LEFT]);
		context.output(unit);
		context.nodeOpLeft = &node;
		output_R(context, node, node.children[CN_RIGHT]);
		if (bNeedBracket) context.output(')');
	}
	else
	{
		bool bNeedBracket = false;
		if (context.nodeOpLeft)
		{
			Unit const& opLS = codes[context.nodeOpLeft->indexOp];
			if ((unit.type == UOP_PLUS && opLS.type == BOP_ADD) || (unit.type == UOP_MINS && opLS.type == BOP_SUB)) { bNeedBracket = true; }
		}
		if (bNeedBracket) context.output('(');
		context.output(unit);
		context.nodeOpLeft = &node;
		output_R(context, node, node.children[CN_LEFT]);
		if (bNeedBracket) context.output(')');
	}
}

ExprOutputContext::ExprOutputContext(SymbolTable const& table) :ExprOutputContext(table, std::cout) {}

void ExprOutputContext::output(Unit const& unit)
{
	switch (unit.type)
	{
	case TOKEN_LBAR: mStream << "("; break;
	case TOKEN_RBAR: mStream << ")"; break;
	case IDT_SEPARETOR: mStream << funcArgSeparetor; break;
	case BOP_COMMA:  mStream << ",";  break;
	case BOP_BIG:    mStream << ">";  break;
	case BOP_BIGEQU: mStream << ">="; break;
	case BOP_SML:    mStream << "<";  break;
	case BOP_SMLEQU: mStream << "<="; break;
	case BOP_EQU:    mStream << "=="; break;
	case BOP_NEQU:   mStream << "!="; break;
	case BOP_ADD:    mStream << "+";  break;
	case BOP_SUB:    mStream << "-";  break;
	case BOP_MUL:    mStream << "*";  break;
	case BOP_DIV:    mStream << "/";  break;
	case BOP_POW:    mStream << "^";  break;
	case UOP_MINS:   mStream << "-";  break;
	case BOP_ASSIGN: mStream << "=";  break;
	case VALUE_CONST:
		switch (unit.constValue.layout)
		{
		case ValueLayout::Double: mStream << unit.constValue.asDouble; break;
		case ValueLayout::Float:  mStream << unit.constValue.asFloat; break;
		case ValueLayout::Int32:  mStream << unit.constValue.asInt32; break;
		}
		break;
	case VALUE_INPUT:
	{
		char const* name = table.getInputName(unit.input.index);
		mStream << (name ? name : "unknowInput");
	}
	break;
	case FUNC_DEF:
	{
		char const* name = table.getFuncName(unit.symbol->func);
		mStream << (name ? name : "unknowFunc");
	}
	break;
	case FUNC_SYMBOL:
	{
		char const* name = table.getFuncName(unit.funcSymbol);
		mStream << (name ? name : "unknowFunc");
	}
	break;
	case VALUE_VARIABLE:
	{
		char const* name = table.getVarName(unit.variable.ptr);
		mStream << (name ? name : "unknowVar");
	}
	break;
	case VALUE_EXPR_TEMP: mStream << "Temp" << unit.exprTemp.tempIndex; break;
	}
}

void ExprOutputContext::outputSpace(int num) { for (int i = 0; i < num; ++i) { mStream << ' '; } }

void ExprTreeBuilder::build(ExpressionTreeData& treeData) /*throw ParseException */
{
	NodeVec& nodes = treeData.nodes;
	TArrayView< Unit > exprCodes = treeData.codes;

	mExprCodes = exprCodes.data();

	mIdxOpNext.resize(exprCodes.size());
	TArray< int > idxDepthStack;

	int numNode = 0;
	int idxNext = exprCodes.size();
	for( int idx = int(exprCodes.size()) - 1  ; idx >= 0 ; --idx )
	{
		Unit const& unit = mExprCodes[ idx ];

		mIdxOpNext[ idx ] = idxNext;

		if ( IsOperator( unit.type ) )
		{
			idxNext = idx;
			++numNode;
		}
		else if ( unit.type == TOKEN_RBAR )
		{
			idxDepthStack.push_back( idx + 1 );
			idxNext = idx;
		}
		else if ( unit.type == TOKEN_LBAR )
		{
			if ( idxDepthStack.empty() )
				throw ExprParseException( eExprError , "Error bar format" );

			idxNext = idxDepthStack.back();
			idxDepthStack.pop_back();
			mIdxOpNext[ idx ] = idxNext;
		}
		else if ( IsFunction( unit.type ) )
		{
			++numNode;
		}
	}

	if ( !idxDepthStack.empty() )
		throw ExprParseException( eExprError , "Error bar format" );

	nodes.resize( numNode + 1 );

	mTreeNodes = &nodes[0];
	mNumNodes  = 1;

	int idxNode = 0;
	int idxLeft = build_R(0, exprCodes.size(), false);

	Node& node = mTreeNodes[ idxNode ];
	node.indexOp = INDEX_NONE;
	node.children[ CN_LEFT  ]  = idxLeft;
	node.children[ CN_RIGHT ]  = 0;
}

int ExprTreeBuilder::build_R(int idxStart, int idxEnd, bool bFuncDef)
{	
	if ( idxEnd == idxStart )
		return 0;
	
	if ( idxEnd == idxStart + 1 )
	{
		Unit const& unit = mExprCodes[idxStart];

		if ( !IsValue( unit.type ) )
			throw ExprParseException( eExprError , "Leaf is not value");

		return -( idxStart + 1 );
	}

	
	int   idxOp = INDEX_NONE;
	{
		int const NoOpOrder = PRECEDENCE_MASK;
		int orderOp  = NoOpOrder;

		int idx = idxStart;
		if ( !IsOperator( mExprCodes[ idx ].type ) )
			idx = mIdxOpNext[ idx ];
		for(  ; idx < idxEnd ; idx = mIdxOpNext[ idx ] )
		{
			Unit const& unit = mExprCodes[idx];

			assert( IsOperator( unit.type ) );

			int order = PrecedeceOrder( unit.type );
			if ( order < orderOp )
			{
				orderOp = order;
				idxOp   = idx;
			}
			else if ( order == orderOp && !IsAssocLR( unit.type ) )
			{
				idxOp   = idx;
			}
		}
	}

	if ( idxOp != INDEX_NONE )
	{
		Unit& op = mExprCodes[ idxOp ];
		if ( bFuncDef )
		{
			if ( op.type != BOP_COMMA )
			{
				bFuncDef = false;
			}
			else
			{
				op.type = IDT_SEPARETOR;
			}
		}

		int idxNode = mNumNodes++;

		int idxLeft  = build_R(idxStart, idxOp, bFuncDef);
		int idxRight = build_R(idxOp + 1, idxEnd, bFuncDef);

		Node& node = mTreeNodes[ idxNode ];
		node.indexOp = idxOp;
		node.children[ CN_LEFT  ] = idxLeft;
		node.children[ CN_RIGHT ] = idxRight;
		return idxNode;
	}

	int indexOp = idxStart;
	Unit& unit = mExprCodes[ idxStart ];
	if ( ExprParse::IsFunction( unit.type ) )
	{
		++idxStart;
		if ( mExprCodes[ idxStart ].type != TOKEN_LBAR )
			throw ExprParseException( eExprError , "Error function format" );

		--idxEnd;
		if ( mExprCodes[ idxEnd ].type != TOKEN_RBAR )
			throw ExprParseException( eExprError , "Error function format" );

		int idxNode = mNumNodes++;

		++idxStart;
		int idxLeft  = build_R(idxStart, idxEnd, true);

		Node& node = mTreeNodes[ idxNode ];
		node.indexOp = indexOp;
		node.children[ CN_LEFT  ] = idxLeft;
		node.children[ CN_RIGHT ] = 0;
		return idxNode;
	}

	if ( mExprCodes[idxStart].type != TOKEN_LBAR )
		throw ExprParseException( eExprError , "Error format" );

	--idxEnd;
	if ( mExprCodes[ idxEnd ].type != TOKEN_RBAR )
		throw ExprParseException( eExprError , "Error format" );

	++idxStart;
	return build_R(idxStart, idxEnd, false);
}

ExprTreeBuilder::ErrorCode ExprTreeBuilder::checkTreeError()
{
	if ( mNumNodes != 0 )
	{
		Node& root = mTreeNodes[ 0 ];
		return checkTreeError_R( root.children[ CN_LEFT ]);
	}
	return TREE_NO_ERROR;
}

ExprTreeBuilder::ErrorCode ExprTreeBuilder::checkTreeError_R( int idxNode )
{
	if ( idxNode <= 0 )
		return TREE_NO_ERROR;

	Node const& node = mTreeNodes[ idxNode ];
	Unit const& unit = mExprCodes[node.indexOp];

	if ( ExprParse::IsBinaryOperator( unit.type ) )
	{
		if ( node.children[ CN_LEFT ] == 0 || node.children[ CN_RIGHT ] == 0 )
			return TREE_OP_NO_CHILD;
		if ( unit.type == BOP_ASSIGN )
		{
			if ( node.children[ CN_LEFT ] > 0 )
				return TREE_ASSIGN_LEFT_NOT_VAR;

			auto const& leftNode = mExprCodes[node.getLeaf(CN_LEFT)];
			if (leftNode.type != ExprParse::VALUE_VARIABLE &&
				leftNode.type != ExprParse::VALUE_INPUT )
				return TREE_ASSIGN_LEFT_NOT_VAR;
		}
	}
	else if ( ExprParse::IsUnaryOperator( unit.type ) )
	{
		if ( node.children[CN_RIGHT] == 0 )
			return TREE_OP_NO_CHILD;
	}
	else if ( ExprParse::IsFunction( unit.type ) )
	{
		int numVar = 0;
		int idxChild = node.children[ CN_LEFT ];
		while( idxChild > 0 )
		{
			Node& nodeChild = mTreeNodes[ idxChild ];
			if ( mExprCodes[nodeChild.indexOp].type != ExprParse::IDT_SEPARETOR )
				break;

			++numVar;
			idxChild = nodeChild.children[ CN_LEFT ];
		}

		if ( idxChild != 0 )
		{
			++numVar;
		}

		if (unit.type == FUNC_DEF)
		{
			if (numVar != unit.symbol->func.getArgNum())
				return TREE_FUN_PARAM_NUM_NO_MATCH;
		}
		else
		{
			if (numVar != unit.funcSymbol.numArgs)
				return TREE_FUN_PARAM_NUM_NO_MATCH;
		}
	}

	ErrorCode error;
	error = checkTreeError_R( node.children[ CN_LEFT ] );
	if ( error != TREE_NO_ERROR )
		return error;

	error = checkTreeError_R( node.children[ CN_RIGHT ] );
	if ( error != TREE_NO_ERROR )
		return error;

	return TREE_NO_ERROR;
}

void ExprTreeBuilder::printTree_R(ExprOutputContext& context, int idxNode , int depth )
{
	
	if ( idxNode == 0 )
	{
		context.outputSpace( depth * 4 );
		context.output("[]");
		context.outputEOL();
	}
	else if ( idxNode < 0 )
	{
		Unit const& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];

		context.outputSpace( depth * 4 );
		context.output('[');
		context.output(unit);
		context.output(']');
		context.outputEOL();
	}
	else
	{
		Node const& node = mTreeNodes[ idxNode ];

		printTree_R(context, node.children[ CN_RIGHT ] , depth + 1 );

		context.outputSpace( depth * 4 );
		context.output('[');
		context.output(mExprCodes[node.indexOp]);
		context.output(']');
		context.outputEOL();
		printTree_R(context, node.children[ CN_LEFT ] , depth + 1 );
	}
}


void ExprTreeBuilder::printTree(SymbolTable const& table)
{
	if ( mNumNodes != 0 )
	{
		ExprOutputContext context(table);
		Node& root = mTreeNodes[ 0 ];
		printTree_R(context, root.children[ CN_LEFT ] , 0 );
	}
}
