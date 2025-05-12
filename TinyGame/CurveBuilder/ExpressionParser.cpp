#include "ExpressionParser.h"

#include "StringParse.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

void ExprParse::Print(ExprOutputContext& context, UnitCodes const& codes,  bool haveBracket)
{
	if (!haveBracket) 
	{
		for(auto const& unit : codes)
		{
			context.output(unit);
		}
	}
	else
	{
		for(auto const& unit : codes)
		{
			context.output("[");
			if ( IsBinaryOperator( unit.type ) && unit.isReverse )
				context.output("re");
			context.output(unit);
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
		}
	}
	return false;
}

bool ExpressionParser::analyzeTokenUnit( char const* expr , SymbolTable const& table , UnitCodes& infixCode )
{
	infixCode.clear();

	if ( *expr == 0 )
		return false;

	Tokenizer tok( expr , " \t\n" , "()+-*/^<>=!," );


	TokenType typePrev = TOKEN_NONE;
	StringView token;
	Tokenizer::TokenType tokenType;
	for( tokenType = tok.take(token) ; tokenType != EStringToken::None ; tokenType = tok.take(token) )
	{
		TokenType type = TOKEN_TYPE_ERROR;

		if ( token.size() == 1 )
		{
			switch( token.data()[0] )
			{
			case '+':
				{
					if ( tok.nextChar() == '+' )
					{
						type = UOP_INC_PRE;
						tok.offset( 1 );
					}
					else
					{
						if ( IsValue( typePrev ) || typePrev == TOKEN_RBAR )
							type = BOP_ADD;
						else
							type = UOP_PLUS;
					}
				}
				break;
			case '-': 
				{
					if ( tok.nextChar() == '-' )
					{
						type = UOP_INC_PRE;
						tok.offset( 1 );
					}
					else
					{
						if ( IsValue( typePrev ) || typePrev == TOKEN_RBAR )
							type = BOP_SUB;
						else
							type = UOP_MINS;
					}
				}
				break;
			case '*': type = BOP_MUL; break;
			case '/': type = BOP_DIV; break;
			case '^': type = BOP_POW; break;
			case ',': type = BOP_COMMA; break;
			case '(': type = TOKEN_LBAR; break;
			case ')': type = TOKEN_RBAR; break;
			case '>':
				{
					if ( tok.nextChar() == '=' )
					{	
						type = BOP_BIGEQU;
						tok.offset( 1 );	
					}
					else
					{
						type = BOP_BIG;
					}
				}
				break;
			case '<':
				{
					if ( tok.nextChar() == '=' )
					{	
						type = BOP_SMLEQU; 
						tok.offset( 1 );	
					}
					else
					{
						type = BOP_SML;
					}
				}
				break;
			case '=':
				{
					if ( tok.nextChar() == '=' )
					{	
						type = BOP_EQU; tok.offset( 1 ); 
					}
					else if ( typePrev == VALUE_VARIABLE || ( typePrev == VALUE_INPUT ) )
					{
						type = BOP_ASSIGN;
					}
					else
					{
						return false;
					}
				}
				break;
			case '!':
				{
					if ( tok.nextChar() == '=' )
					{
						type = BOP_NEQU; 
						tok.offset( 1 );
					}
					else 
					{
						mErrorMsg = "No = after !";
						return false;
					}
				}
				break;
			}

			if( type != TOKEN_TYPE_ERROR )
			{
				infixCode.emplace_back(type);
			}
		}


		if ( type == TOKEN_TYPE_ERROR )
		{
			SymbolEntry const* symbol = table.findSymbol(token.toCString());

			if ( symbol )
			{
				switch( symbol->type )
				{
				case SymbolEntry::eFunction:
					{
						type = FUNC_DEF;
						infixCode.emplace_back(type, symbol);
					}
					break;
				case SymbolEntry::eFunctionSymbol:
					{
						type = FUNC_SYMBOL;
						infixCode.emplace_back(symbol->funcSymbol);
					}
					break;
				case SymbolEntry::eConstValue:
					{
						type = VALUE_CONST;
						infixCode.emplace_back(symbol->constValue);
					}
					break;
				case SymbolEntry::eVariable:
					{
						type = VALUE_VARIABLE;
						infixCode.emplace_back(symbol->varValue);
					}
					break;
				case SymbolEntry::eInputVar:
					{
						type = VALUE_INPUT;
						infixCode.emplace_back(symbol->input);
					}
					break;
				}
			}
			else
			{
				char* ptrEnd;
				RealType val = RealType(strtod(token.toCString(), &ptrEnd));
				if( *ptrEnd == '\0' )
				{
					type = VALUE_CONST;
					infixCode.emplace_back(Unit::ConstValue(val));
				}
			}
		}

		if ( type == TOKEN_TYPE_ERROR )
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
	stack.emplace_back(TOKEN_LBAR);
	infixCode.emplace_back(TOKEN_RBAR);
	
	postfixCode.clear();

	for ( UnitCodes::iterator iter = infixCode.begin()
		  ;iter!= infixCode.end(); ++iter )
	{
		Unit& elem = *iter;
		if ( IsValue( elem.type ) )
		{
			postfixCode.push_back(*iter);
		}
		else if ( elem.type == TOKEN_LBAR )
		{
			stack.push_back(*iter);
		}
		else if ( ( elem.type == BOP_COMMA ) || 
			      ( elem.type == TOKEN_RBAR ) )
		{
			for(;;)
			{
				const Unit& tElem = stack.back();
				if ( (tElem.type != TOKEN_LBAR) &&
					 (tElem.type != BOP_COMMA  )  )
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
			if ( elem.type == BOP_COMMA  )
				stack.push_back(*iter);
		}
		else if ( IsFunction( elem.type ) )
		{
			stack.push_back(*iter);
		}
		else if (  IsOperator(elem.type)  )
		{

			for(;;)
			{
				if ( stack.empty() ) break;
				const Unit& tElem = stack.back();
				if ( (tElem.type == TOKEN_LBAR) ||
					 (tElem.type == BOP_COMMA )  )
					break;

				if ( PrecedeceOrder( tElem.type ) >=
					 PrecedeceOrder( elem.type  ) )
				{
					postfixCode.push_back(tElem);
					stack.pop_back();
				}
				else break;
			}

			stack.push_back(elem);
			if ( elem.type == BOP_ASSIGN )
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


bool ExpressionParser::parse( char const* expr , SymbolTable const& table )
{
	ParseResult result;
	if ( !analyzeTokenUnit( expr , table , result.mTreeData.codes) )
		return false;

	ExprTreeBuilder builder;
	try
	{
		builder.build(result.mTreeData);
	}
	catch( ExprParseException& e )
	{
		mErrorMsg = e.what();
		return false;
	}

	return true;
}

bool ExpressionParser::parse( char const* expr , SymbolTable const& table , ParseResult& result )
{

	result.mSymbolDefine = &table;

	if ( !analyzeTokenUnit( expr , table , result.mTreeData.codes ) )
		return false;

#if _DEBUG
	result.printInfixCodes();
#endif

	ExprTreeBuilder builder;
	try 
	{
		builder.build( result.mTreeData );
	}
	catch ( ExprParseException& e )
	{
		mErrorMsg = e.what();
		return false;
	}


#	if _DEBUG
	builder.printTree( table );
#	endif

	auto error = builder.checkTreeError();
	if ( error != ExprTreeBuilder::TREE_NO_ERROR )
	{
		return false;
	}

	builder.optimizeNodeOrder();
	builder.convertPostfixCode( result.mPFCodes );

#	if _DEBUG
	builder.printTree( table );
#	endif


#if _DEBUG
	result.printPostfixCodes();
#endif
	return true;
}


bool ExpressionParser::parse(char const* expr, SymbolTable const& table, ExpressionTreeData& outTreeData)
{
	if (!analyzeTokenUnit(expr, table, outTreeData.codes))
		return false;

	ExprTreeBuilder builder;
	try
	{
		builder.build(outTreeData);
	}
	catch (ExprParseException& e)
	{
		mErrorMsg = e.what();
		return false;
	}

	return true;
}

bool ParseResult::isUsingVar( char const* name ) const
{
	if( mSymbolDefine )
	{
		SymbolEntry const* symbol = mSymbolDefine->findSymbol(name);
		if( symbol && symbol->type == SymbolEntry::eVariable )
		{
			for( auto const& unit : mPFCodes )
			{
				if( unit.type == VALUE_VARIABLE && unit.variable.ptr == symbol->varValue.ptr )
					return true;
			}
		}
	}
	return false;
}

bool ParseResult::isUsingInput(char const* name) const
{
	if( mSymbolDefine )
	{
		SymbolEntry const* symbol = mSymbolDefine->findSymbol(name);
		if( symbol && symbol->type == SymbolEntry::eInputVar )
		{
			for (auto const& unit : mPFCodes)
			{
				if( unit.type == VALUE_INPUT && unit.input.index == symbol->input.index )
					return true;
			}
		}
	}
	return false;
}


void ParseResult::optimize()
{
#ifdef _DEBUG
	printPostfixCodes();
#endif
	int bIndex = 0;
	for(int index=0;index<mPFCodes.size();++index)
	{
		Unit& elem = mPFCodes[index];
		if ( optimizeZeroValue(index) )	index = 0;
		else if ( optimizeConstValue(index) ) index = 0;
		else if ( optimizeValueOrder(index)    ||
			optimizeOperatorOrder(index) ||
			optimizeVarOrder(index) )
			index = 0;
		else bIndex = index;
	}
}

//        1 2 1 2  1   0         problem
//    [3] x 4 * 2  +   -         [ 3 x - ] 4 +
//     |___________
//                 |                   re
//     x  4 * 2 + [3] (-r)        [ x 3 - ] 4 +
bool ParseResult::optimizeValueOrder( int index )
{
	if ( !(index < mPFCodes.size()) ) 
		return false;

	bool hadChange = false;

	Unit& elem = mPFCodes[index];
	if ( IsValue( elem.type ) )
	{
		int num = 0;
		int opIndex = index;
		// Find Eval Fun/Operator Index 
		do {
			++opIndex;
			if ( !( opIndex < mPFCodes.size() ) )
			{
				opIndex = index;
				break;
			}
			Unit& elem2 = mPFCodes[opIndex];

			if ( IsValue( elem2.type ) ) ++num;
			else if( IsBinaryOperator( elem2.type ) )
			{
				--num;
			}
			else if( IsFunction( elem2.type ) )
			{
				num -= (elem2.symbol->func.getArgNum() -1 );
			}

		} while ( num > 0 );

		if ( opIndex-index > 2 && IsBinaryOperator( mPFCodes[opIndex].type ) )
		{
			Unit temp = mPFCodes[index];

			MoveElement(index+1,opIndex ,-1);

			mPFCodes[opIndex-1] = temp;
			mPFCodes[opIndex].isReverse = true;

			hadChange = true;
#ifdef _DEBUG
			std::cout << "optimizeValueOrder :"<< std::endl;
			printPostfixCodes();
#endif
			optimizeValueOrder(index);
		}
		//		else if ( j-index == 2 &&
		//			      m_PostfixArray[index+1].Type == VALUE_VARIABLE )
		//		{
		//			m_PostfixArray[index+2].isReverse = true;
		//			std::swap(m_PostfixArray[index],m_PostfixArray[index+1]);
		//
		//		    hadChange = true;
		//#ifdef _DEBUG
		//			std::cout << "optimizeValueOrder :"<< std::endl;
		//			printPostfixArray();
		//#endif
		//			optimizeOperatorOrder( index +2 );
		//		}
	}

	return hadChange;
}

//           x 3 [-] 2 (-) 5 (+) 3 *
//                |____________
//                             |
//   trans:  x 3  2 (+) 5 (-) [-] 3 *
bool ParseResult::optimizeOperatorOrder( int index )
{
	if ( !(index < mPFCodes.size()) ) 
		return false;

	Unit& elem = mPFCodes[index];
	if ( !( ( elem.type == BOP_ADD || elem.type == BOP_SUB ||
		elem.type == BOP_MUL || elem.type == BOP_DIV ) &&
		mPFCodes[index-1].type == VALUE_CONST )
		/*&&( m_PostfixArray[index-2].Type == VALUE_VARIABLE ||
		m_PostfixArray[index-2].Type &  TOKEN_OPERATOR )  */)
		return false;



	bool hadChange = false;
	int bIndex = 0;

	bool isNegive = false;
	if ( ( elem.type == BOP_SUB || elem.type == BOP_DIV ) )
		isNegive = true;

	int moveIndex = index;
	for(int i=index+1;i<mPFCodes.size();++i)
	{
		Unit& testElem = mPFCodes[i];
		if ( IsFunction( testElem.type ) ) break;
		if ( IsBinaryOperator( testElem.type ) )
		{
			if ( PrecedeceOrder( elem.type ) ==  PrecedeceOrder( testElem.type ) &&
				 mPFCodes[i-1].type == VALUE_CONST )
			{
				//bool b = ( elem.isReverse && m_PostfixArray[i].isReverse ) ||
				//	     ( !elem.isReverse && !m_PostfixArray[i].isReverse );
				if (isNegive /*&& b */)
				{
					TokenType type = mPFCodes[i].type;
					switch (mPFCodes[i].type)
					{
					case BOP_ADD: type = BOP_SUB; break;
					case BOP_SUB: type = BOP_ADD; break;
					case BOP_MUL: type = BOP_DIV; break;
					case BOP_DIV: type = BOP_MUL; break;
					}
					mPFCodes[i].type = type;

				}
				moveIndex = i;
			}
			else break;
		}
	}

	if ( moveIndex != index)
	{
		Unit temp = mPFCodes[index];
		MoveElement(index+1,moveIndex + 1 ,-1);
		mPFCodes[moveIndex] = temp;
#ifdef _DEBUG
		std::cout << "optimizeOperatorOrder :" << std::endl;
		printPostfixCodes();
#endif
		optimizeConstValue(index+1);
		hadChange = true;
	}
	return hadChange;
}


//           x  3 -  2 - [y +] 3 *
//               _________|
//              |
//   trans:  x [y +] 3 -  2 -  3 *

bool ParseResult::optimizeVarOrder( int index )
{
	if ( ! (index+1 < mPFCodes.size() ) ) 
		return false;

	if (  mPFCodes[index].type != VALUE_VARIABLE ||
		  !IsBinaryOperator( mPFCodes[index+1].type )  )
		return false;

	Unit& elem = mPFCodes[index+1];
	int moveIndex = index;

	for(int i = index-2 ; i >= 0 ; i-=2 )
	{
		Unit& testElem = mPFCodes[i+1];
		if ( mPFCodes[i].type == VALUE_CONST &&
			 IsBinaryOperator( mPFCodes[i+1].type ) )
		{
			if ( PrecedeceOrder( mPFCodes[index+1].type ) ==
				 PrecedeceOrder( mPFCodes[i+1].type ) )
			{
				moveIndex = i;
			}
			else break;
		}
		else break;
	}
	if ( moveIndex != index)
	{
		Unit temp0 = mPFCodes[index];
		Unit temp1 = mPFCodes[index+1];

		MoveElement(moveIndex,index ,2);
		mPFCodes[moveIndex] = temp0;
		mPFCodes[moveIndex+1] = temp1;

#ifdef _DEBUG
		std::cout << "optimizeVarOrder :"<< std::endl;
		printPostfixCodes();
#endif
		return true;
	}

	return false;
}

//   x [3 2 +] -   x [3 2 pow] +
//       ___|          ____|
//      |             |
//   x [5] -       x [9] +


//   x 3 [-r] 2 +     x  2 / 3 *
//
//   x 
bool ParseResult::optimizeConstValue( int index )
{
	if ( !(index < mPFCodes.size()) ) 
		return false;

	bool hadChange = false;
	Unit& elem = mPFCodes[index];
	if ( IsBinaryOperator( elem.type ) )
	{
		Unit& elemL =  mPFCodes[index-2];
		Unit& elemR =  mPFCodes[index-1];
		if ( elemL.type == VALUE_CONST &&
			 elemR.type == VALUE_CONST  )
		{
			assert(elemL.constValue.layout == ValueLayout::Real && elemR.constValue.layout == ValueLayout::Real);
			bool done = false;
			RealType val = 0;

			switch (elem.type)
			{
			case BOP_ADD:
				val = elemL.constValue.asReal + elemR.constValue.asReal;
				done = true;
				break;
			case BOP_MUL:
				val = elemL.constValue.asReal * elemR.constValue.asReal;
				done = true;
				break;
			case BOP_SUB:
				if ( elem.isReverse )
					val = elemR.constValue.asReal - elemL.constValue.asReal;
				else
					val = elemL.constValue.asReal - elemR.constValue.asReal;
				done = true;
				break;
			case BOP_DIV:
				if ( elem.isReverse )
					val = elemR.constValue.asReal / elemL.constValue.asReal;
				else
					val = elemL.constValue.asReal / elemR.constValue.asReal;
				done = true;
				break;
			}
			if (done)
			{
				elem = Unit::ConstValue(val);

				MoveElement(index,mPFCodes.size() ,-2);
				mPFCodes.pop_back();
				mPFCodes.pop_back();
#ifdef _DEBUG
				std::cout << "optimizeConstValue :"<< std::endl;
				printPostfixCodes();
#endif
				optimizeConstValue(index-2);
				hadChange = true;
			}

		}
	}
	else if ( IsUnaryOperator( elem.type ) )
	{
		Unit& elemL =  mPFCodes[index-1];

		if ( elemL.type == VALUE_CONST )
		{
			bool done = false;
			RealType val = 0;

			switch (elem.type)
			{
			case UOP_MINS:
				val = -elemL.constValue.asDouble;
				done = true;
				break;
			}

			if (done)
			{
				elem = Unit::ConstValue(val);

				MoveElement(index,mPFCodes.size() ,-1);
				mPFCodes.pop_back();
#ifdef _DEBUG
				std::cout << "optimizeConstValue :"<< std::endl;
				printPostfixCodes();
#endif
				optimizeConstValue(index-1);
				hadChange = true;
			}
		}
	}
	else if ( IsFunction( elem.type ) )
	{
		if (elem.type == FUNC_SYMBOL)
			return false;

		RealType val[5];
		int num = elem.symbol->func.getArgNum();
		void* funPtr = elem.symbol->func.funcPtr;
		bool testOk =true;
		for ( int j = 0 ; j < num ;++j)
		{
			if( mPFCodes[index-j-1].type != VALUE_CONST )
			{
				testOk = false;
				break;
			}
			val[j] = mPFCodes[index-j-1].constValue.asReal;
		}
		if (testOk)
		{
			switch(num)
			{
			case 0:
				val[0] = (*(FuncType0)funPtr)();
				break;
			case 1:
				val[0] = (*(FuncType1)funPtr)(val[0]);
				break;
			case 2:
				val[0] = (*(FuncType2)funPtr)(val[1],val[0]);
				break;
			case 3:
				val[0] = (*(FuncType3)funPtr)(val[2],val[1],val[0]);
				break;
			case 4:
				val[0] = (*(FuncType4)funPtr)(val[3],val[2],val[1],val[0]);
				break;
			case 5:
				val[0] = (*(FuncType5)funPtr)(val[4],val[3],val[2],val[1],val[0]);
				break;
			}
			elem = Unit::ConstValue(val[0]);
			for(int n = index-num ;n< mPFCodes.size()-num;++n)
				mPFCodes[n] = mPFCodes[n+num];

			for ( int j = 0 ; j < num ;++j)
				mPFCodes.pop_back();
#ifdef _DEBUG
			std::cout << "optimizeConstValue :"<< std::endl;
			printPostfixCodes();
#endif
			optimizeConstValue(index-num);
			hadChange = true;
		}
	}

	return  hadChange;
}

//           x  [0 -]  2 -
//                _|
//               |
//   trans:  x  [ ] 2 -
bool ParseResult::optimizeZeroValue( int index )
{
	if ( !(index < mPFCodes.size()) ) 
		return false;

	bool hadChange = false;
	Unit& elem = mPFCodes[index];
	if ( elem.type == BOP_ADD || elem.type == BOP_SUB )
	{
		if ( mPFCodes[index-1].type == VALUE_CONST &&
			 mPFCodes[index-1].constValue.asReal == 0 )
		{
			MoveElement(index+1,mPFCodes.size() ,-2);
			mPFCodes.pop_back();
			mPFCodes.pop_back();
			hadChange = true;
#ifdef _DEBUG
			std::cout << "optimizeZeroValue :"<< std::endl;
			printPostfixCodes();
#endif
		}
	}
	return hadChange;
}

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

void ExprTreeBuilder::convertPostfixCode( UnitCodes& codes )
{
	codes.clear();
	if ( mNumNodes != 0 )
	{
		Node& root = mTreeNodes[ 0 ];
		convertPostfixCode_R( codes , root.children[ CN_LEFT ] );
	}
}

void ExprTreeBuilder::convertPostfixCode_R( UnitCodes& codes , int idxNode )
{
	if ( idxNode < 0 )
	{
		codes.push_back( mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ] );
		return;
	}
	else if ( idxNode == 0 )
		return;

	Node const& node = mTreeNodes[ idxNode ];
	Unit const& unit = mExprCodes[node.indexOp];

	if ( unit.type == BOP_ASSIGN )
	{
		convertPostfixCode_R( codes , node.children[ CN_RIGHT ] );
		convertPostfixCode_R( codes , node.children[ CN_LEFT ] );
		codes.push_back( unit );
	}
	else
	{
		convertPostfixCode_R( codes , node.children[ CN_LEFT ] );
		convertPostfixCode_R( codes , node.children[ CN_RIGHT ] );
		if ( unit.type != IDT_SEPARETOR )
		{
			codes.push_back( unit );
		}
	}
}

void ExprTreeBuilder::optimizeNodeOrder()
{
	if ( mNumNodes == 0 )
		return;

	Node& root = mTreeNodes[0];
	if ( root.children[ CN_LEFT ] > 0 )
		optimizeNodeOrder_R( root.children[ CN_LEFT ] );
}

int ExprTreeBuilder::optimizeNodeOrder_R( int idxNode )
{
	Node& node = mTreeNodes[ idxNode ];

	int depthL = 0;
	if ( node.children[ CN_LEFT ] > 0 )
		depthL = optimizeNodeOrder_R( node.children[ CN_LEFT ] );

	int depthR = 0;
	if ( node.children[ CN_RIGHT ] > 0 )
		depthR = optimizeNodeOrder_R( node.children[ CN_RIGHT ] );


	Unit& unit = mExprCodes[node.indexOp];
	if ( ExprParse::IsBinaryOperator( unit.type ) && ExprParse::CanReverse( unit.type ) )
	{
		bool needSwap = false;
		if ( node.children[ CN_LEFT ] < 0 )
		{
			if ( node.children[ CN_RIGHT ] > 0 )
			{
				Unit const& unitL = mExprCodes[ node.getLeaf( CN_LEFT ) ];
				if ( ExprParse::IsValue( unitL.type ) )
				{				
					Node& nodeR = mTreeNodes[ node.children[ CN_RIGHT ] ];
					Unit& unitR = mExprCodes[nodeR.indexOp]; 

					if ( ExprParse::IsOperator( unitR.type ) || 
						 ExprParse::IsFunction( unitR.type ) )
					{
						std::swap( node.children[ CN_LEFT ] , node.children[ CN_RIGHT ] );
						unit.isReverse = !unit.isReverse;
					}
				}
			}
		}
		else if ( depthL < depthR )
		{
			std::swap( node.children[ CN_LEFT ] , node.children[ CN_RIGHT ] );
			unit.isReverse = !unit.isReverse;
		}
	}

	return (( depthL > depthR ) ? depthL : depthR ) + 1 ;
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

bool ExprTreeBuilder::optimizeNodeConstValue( int idxNode )
{
	Node& node = mTreeNodes[idxNode];
	Unit& unit = mExprCodes[node.indexOp];

	RealType value = 0;

	if ( ExprParse::IsBinaryOperator( unit.type ) )
	{
		if ( node.children[ CN_LEFT ] >= 0 )
			return false;
		if ( node.children[ CN_RIGHT ] >= 0 )
			return false;

		Unit const& unitL = mExprCodes[ node.getLeaf( CN_LEFT ) ];
		Unit const& unitR = mExprCodes[ node.getLeaf( CN_RIGHT ) ];

		if ( unitL.type == ExprParse::VALUE_CONST )
		{
			if ( unitR.type != ExprParse::VALUE_CONST )
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	
		switch ( unit.type )
		{
		case BOP_ADD:
			value = unitL.constValue.asReal + unitR.constValue.asReal;
			break;
		case BOP_MUL:
			value = unitL.constValue.asReal * unitR.constValue.asReal;
			break;
		case BOP_SUB:
			if ( unit.isReverse )
				value = unitR.constValue.asReal - unitL.constValue.asReal;
			else
				value = unitL.constValue.asReal - unitR.constValue.asReal;
			break;
		case BOP_DIV:
			if ( unit.isReverse )
				value = unitR.constValue.asReal / unitL.constValue.asReal;
			else
				value = unitL.constValue.asReal / unitR.constValue.asReal;
			break;
		case BOP_COMMA:
			value = unitR.constValue.asReal;
			break;
		default:
			return false;
		}
	}
	else if ( ExprParse::IsUnaryOperator( unit.type ) )
	{
		if ( node.children[ CN_LEFT ] >= 0 )
			return false;

		Unit const& unitR = mExprCodes[ node.getLeaf(CN_RIGHT) ];

		if ( unitR.type != VALUE_CONST )
			return false;

		switch (unit.type)
		{
		case UOP_MINS:
			value = -unitR.constValue.asReal;
			break;
		case UOP_PLUS:
			value = unitR.constValue.asReal;
			break;
		default:
			return false;
		}
	}
	else if ( ExprParse::IsFunction( unit.type ) )
	{
		RealType params[5];
		int   numParam = unit.symbol->func.getArgNum();
		void* funcPtr = unit.symbol->func.funcPtr;

		if ( numParam )
		{
			int idxParam = node.children[ CN_LEFT ];
			for ( int n = 0 ; n < numParam - 1; ++n )
			{
				assert( idxParam > 0 );
				Node& nodeParam = mTreeNodes[ idxParam ];
				CHECK(mExprCodes[nodeParam.indexOp].type == IDT_SEPARETOR );
				
				if ( nodeParam.children[ CN_RIGHT ] >= 0 )
					return false;

				Unit const& unitVal = mExprCodes[ nodeParam.getLeaf( CN_RIGHT ) ];
				if ( unitVal.type != VALUE_CONST )
					return false;

				params[n] = unitVal.constValue.asReal;
				idxParam = nodeParam.children[ CN_LEFT ];
			}

			Unit const& unitVal = mExprCodes[ LEAF_UNIT_INDEX( idxParam ) ];
			if ( unitVal.type != VALUE_CONST )
				return false;

			params[ numParam - 1 ] = unitVal.constValue.asReal;
		}

		switch(numParam)
		{
		case 0:
			value = ( *static_cast< FuncType0 >( funcPtr ) )();
			break;
		case 1:
			value = ( *static_cast< FuncType1 >( funcPtr ) )(params[0]);
			break;
		case 2:
			value = ( *static_cast< FuncType2 >( funcPtr ) )(params[1],params[0]);
			break;
		case 3:
			value = ( *static_cast< FuncType3 >( funcPtr ) )(params[2],params[1],params[0]);
			break;
		case 4:
			value = ( *static_cast< FuncType4 >( funcPtr ) )(params[3],params[2],params[1],params[0]);
			break;
		case 5:
			value = ( *static_cast< FuncType5 >( funcPtr ) )(params[4],params[3],params[2],params[1],params[0]);
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

bool ExprTreeBuilder::optimizeNodeBOpOrder( int idxNode )
{
	assert( idxNode > 0 );

	Node& node = mTreeNodes[ idxNode ];
	Unit& unit = mExprCodes[node.indexOp];

	if ( !ExprParse::IsBinaryOperator( unit.type ) || 
		 !ExprParse::CanExchange( unit.type ) )
		return false;



	return false;
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

char const* SymbolTable::getFuncName(FuncInfo const& info) const
{
	for( auto const& pair : mNameToEntryMap )
	{
		if( pair.second.type == SymbolEntry::eFunction &&
		    pair.second.func == info )
			return pair.first.c_str();
	}
	return nullptr;
}

char const* SymbolTable::getFuncName(FuncSymbolInfo const& info) const
{
	for (auto const& pair : mNameToEntryMap)
	{
		if (pair.second.type == SymbolEntry::eFunctionSymbol &&
			pair.second.funcSymbol.id == info.id)
			return pair.first.c_str();
	}
	return nullptr;
}

char const* SymbolTable::getVarName(void* varPtr) const
{
	for( auto const& pair : mNameToEntryMap )
	{
		if( pair.second.type == SymbolEntry::eVariable &&
		    pair.second.varValue.ptr == varPtr)
			return pair.first.c_str();
	}
	return nullptr;
}

char const* SymbolTable::getInputName(int index) const
{
	for (auto const& pair : mNameToEntryMap)
	{
		if (pair.second.type == SymbolEntry::eInputVar &&
			pair.second.input.index == index)
			return pair.first.c_str();
	}
	return nullptr;
}

VariableInfo const* SymbolTable::findVar(char const* name) const
{
	auto entry = findSymbol(name, SymbolEntry::eVariable);
	if( entry )
	{
		return &entry->varValue;
	}
	return nullptr;
}

InputInfo const* SymbolTable::findInput(char const*  name) const
{
	auto entry = findSymbol(name, SymbolEntry::eInputVar);
	if( entry )
	{
		return &entry->input;
	}
	return nullptr;
}

FuncInfo const* SymbolTable::findFunc(char const* name) const
{
	auto entry = findSymbol(name, SymbolEntry::eFunction);
	if( entry )
	{
		return &entry->func;
	}
	return nullptr;
}

ConstValueInfo const* SymbolTable::findConst(char const* name) const
{
	auto entry = findSymbol(name, SymbolEntry::eConstValue);
	if( entry )
	{
		return &entry->constValue;
	}
	return nullptr;
}

int SymbolTable::getVarTable( char const* varStr[],double varVal[] ) const
{;
	int index = 0;
	for(auto const& pair : mNameToEntryMap)
	{
		if ( pair.second.type != SymbolEntry::eVariable )
			continue;

		if ( varStr ) varStr[index] = pair.first.c_str();
		if( varVal )
		{
			switch( pair.second.varValue.layout )
			{
			case ValueLayout::Double:
				varVal[index] = *(double*)pair.second.varValue.ptr;
				break;
			case ValueLayout::Float:
				varVal[index] = *(float*)pair.second.varValue.ptr;
				break;
			case ValueLayout::Int32:
				varVal[index] = *(int32*)pair.second.varValue.ptr;
				break;
			default:
				NEVER_REACH("SymbolTable::getVarTable");
			}
			
		}
		++index;
	}
	return index;
}

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
	else if (idxNode == 0)
		return;

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
			if (IsFunction(opParent.type))
			{
				bNeedBracket = false;
			}
			else if (IsBinaryOperator(opParent.type))
			{
				if (ExprParse::PrecedeceOrder(opParent.type) < ExprParse::PrecedeceOrder(unit.type))
				{
					bNeedBracket = false;
				}
				else if (ExprParse::PrecedeceOrder(opParent.type) == ExprParse::PrecedeceOrder(unit.type) &&
					(opParent.type != BOP_SUB && opParent.type != BOP_DIV))
				{
					bNeedBracket = false;
				}
			}
		}
		else
		{
			bNeedBracket = false;
		}

		if (bNeedBracket)
			context.output('(');
		output_R(context, node, node.children[CN_LEFT]);
		context.output(unit);
		context.nodeOpLeft = &node;
		output_R(context, node, node.children[CN_RIGHT]);
		if (bNeedBracket)
			context.output(')');
	}
	else
	{
		bool bNeedBracket = false;
		if (context.nodeOpLeft)
		{
			Unit const& opLS = codes[context.nodeOpLeft->indexOp];
			if ( (unit.type == UOP_PLUS && opLS.type == BOP_ADD) ||
				 (unit.type == UOP_MINS && opLS.type == BOP_SUB) )
			{
				//context.output(' ');
				bNeedBracket = true;
			}
		}

		if (bNeedBracket)
			context.output('(');

		context.output(unit);
		context.nodeOpLeft = &node;
		output_R(context, node, node.children[CN_LEFT]);
		if (bNeedBracket)
			context.output(')');
	}
}

ExprOutputContext::ExprOutputContext(SymbolTable const& table) 
	:ExprOutputContext(table, std::cout)
{

}

void ExprOutputContext::output(Unit const& unit)
{
	switch (unit.type)
	{
	case TOKEN_LBAR:mStream << "(";  break;
	case TOKEN_RBAR:mStream << ")";  break;
	case IDT_SEPARETOR:mStream << funcArgSeparetor;  break;
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
			if (name)
				mStream << name;
			else
				mStream << "unknowInput";

		}
		break;
	case FUNC_DEF:
		{
			char const* name = table.getFuncName(unit.symbol->func);
			if (name)
				mStream << name;
			else
				mStream << "unknowFunc";
		}
		break;
	case FUNC_SYMBOL:
		{
			char const* name = table.getFuncName(unit.funcSymbol);
			if (name)
				mStream << name;
			else
				mStream << "unknowFunc";
		}
		break;
	case VALUE_VARIABLE:
		{
			char const* name = table.getVarName(unit.variable.ptr);
			if (name)
				mStream << name;
			else
				mStream << "unknowVar";
		}
		break;
	}
}

void ExprOutputContext::output(char c)
{
	mStream << c;
}

void ExprOutputContext::output(char const* str)
{
	mStream << str;
}

void ExprOutputContext::outputSpace(int num)
{
	for (int i = 0; i < num; ++i)
	{
		mStream << ' ';
	}
}

void ExprOutputContext::outputEOL()
{
	mStream << "\n";
}
