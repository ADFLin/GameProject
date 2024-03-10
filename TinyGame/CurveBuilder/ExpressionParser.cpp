#include "ExpressionParser.h"

#include "StringParse.h"

#include <algorithm>
#include <cctype>
#include <iostream>


void ExprParse::print(  Unit const& unit , SymbolTable const& table )
{
	using std::cout;

	switch( unit.type )
	{
	case TOKEN_LBAR:cout<<"(";  break;
	case TOKEN_RBAR:cout<<")";  break;
	case IDT_SEPARETOR:cout<<";";  break;
	case BOP_COMMA:  cout<<",";  break;
	case BOP_BIG:    cout<<">";  break;
	case BOP_BIGEQU: cout<<">="; break;
	case BOP_SML:    cout<<"<";  break;
	case BOP_SMLEQU: cout<<"<="; break;
	case BOP_EQU:    cout<<"=="; break;
	case BOP_NEQU:   cout<<"!="; break;
	case BOP_ADD:    cout<<"+";  break;
	case BOP_SUB:    cout<<"-";  break;
	case BOP_MUL:    cout<<"*";  break;
	case BOP_DIV:    cout<<"/";  break;
	case BOP_POW:    cout<<"^";  break;
	case UOP_MINS:   cout<<"-";  break;
	case BOP_ASSIGN: cout<<"=";  break;
	case VALUE_CONST: cout << unit.constValue.asReal ; break;
	case VALUE_INPUT:
		{
			char const* name = table.getInputName(unit.symbol->input.index);
			if (name)
				cout << name;
			else
				cout << "unknowInput";

		}
		break;
	case FUNC_DEF:
		{
			char const* name = table.getFuncName( unit.symbol->func );
			if ( name )
				cout << name;
			else
				cout << "unknowFun";
		}
		break;
	case VALUE_VARIABLE:
		{
			char const* name = table.getVarName( unit.symbol->varValue.ptr );
			if ( name )
				cout << name;
			else
				cout << "unknowVar";
		}
		break;
	}
}

void ExprParse::print( UnitCodes const& codes , SymbolTable const& table , bool haveBracket )
{
	if (!haveBracket) 
	{
		for(auto const& unit : codes)
		{
			print( unit , table );
		}
	}
	else
	{
		for(auto const& unit : codes)
		{
			std::cout << "[";
			if ( IsBinaryOperator( unit.type ) && unit.isReverse )
				std::cout << "re" ;
			print( unit , table );
			std::cout << "]";
		}
	}
	std::cout << '\n';
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
				case SymbolEntry::eConstValue:
					{
						type = VALUE_CONST;
						infixCode.emplace_back(type, symbol->constValue);
					}
					break;
				case SymbolEntry::eVariable:
					{
						type = VALUE_VARIABLE;
						infixCode.emplace_back(type, symbol);
					}
					break;
				case SymbolEntry::eInputVar:
					{
						type = VALUE_INPUT;
						infixCode.emplace_back(type, symbol);
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
					infixCode.emplace_back(type, val);
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
	if ( !analyzeTokenUnit( expr , table , result.mIFCodes) )
		return false;

	ExprTreeBuilder builder;
	try
	{
		builder.build(result.mTreeNodes, &result.mIFCodes[0], result.mIFCodes.size());
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

	if ( !analyzeTokenUnit( expr , table , result.mIFCodes ) ) 
		return false;

#if _DEBUG
	result.printInfixCodes();
#endif

	ExprTreeBuilder builder;
	try 
	{
		builder.build( result.mTreeNodes , &result.mIFCodes[0] , result.mIFCodes.size() );
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


bool ParseResult::isUsingVar( char const* name ) const
{
	if( mSymbolDefine )
	{
		SymbolEntry const* symbol = mSymbolDefine->findSymbol(name);
		if( symbol )
		{
			for( auto const& unit : mPFCodes )
			{
				if( unit.type == VALUE_VARIABLE && unit.symbol == symbol )
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
		if( symbol )
		{
			for (auto const& unit : mPFCodes)
			{
				if( unit.type == VALUE_INPUT && unit.symbol == symbol )
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

				elem = Unit( VALUE_CONST , ConstValueInfo( val ) );

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
				elem = Unit(VALUE_CONST,val);

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
			elem = Unit(VALUE_CONST,val[0]);
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

void ExprTreeBuilder::build( NodeVec& nodes , Unit* exprCode , int numUnit ) /*throw ParseException */
{
	mExprCodes = exprCode;

	mIdxOpNext.resize( numUnit );
	TArray< int > idxDepthStack;

	int numNode = 0;
	int idxNext = numUnit;
	for( int idx = numUnit - 1  ; idx >= 0 ; --idx )
	{
		Unit& unit = mExprCodes[ idx ];

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
	int idxLeft = buildTree_R( idxNode , 0 , numUnit , false );

	Node& node = mTreeNodes[ idxNode ];
	node.parent  = 0;
	node.opUnit  = nullptr;
	node.children[ CN_LEFT  ]  = idxLeft;
	node.children[ CN_RIGHT ]  = 0;
}

int ExprTreeBuilder::buildTree_R( int idxParent, int idxStart, int idxEnd, bool bFuncDef)
{	
	if ( idxEnd == idxStart )
		return 0;
	
	if ( idxEnd == idxStart + 1 )
	{
		Unit& unit = mExprCodes[idxStart];

		if ( !IsValue( unit.type ) )
			throw ExprParseException( eExprError , "Leaf is not value");

		return -( idxStart + 1 );
	}

	
	int   idxOp = -1;
	{
		int const NoOpOrder = PRECEDENCE_MASK;
		int orderOp  = NoOpOrder;

		int idx = idxStart;
		if ( !IsOperator( mExprCodes[ idx ].type ) )
			idx = mIdxOpNext[ idx ];
		for(  ; idx < idxEnd ; idx = mIdxOpNext[ idx ] )
		{
			Unit& unit = mExprCodes[idx];

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

	if ( idxOp != -1 )
	{
		Unit& opUnit = mExprCodes[ idxOp ];
		if ( bFuncDef )
		{
			if ( opUnit.type != BOP_COMMA )
			{
				bFuncDef = false;
			}
			else
			{
				opUnit.type = IDT_SEPARETOR;
			}
		}

		int idxNode = mNumNodes++;

		int idxLeft  = buildTree_R( idxNode , idxStart , idxOp , bFuncDef );
		int idxRight = buildTree_R( idxNode , idxOp + 1 , idxEnd , bFuncDef );

		Node& node = mTreeNodes[ idxNode ];
		node.parent  = idxParent;
		node.opUnit  = &opUnit;
		node.children[ CN_LEFT  ] = idxLeft;
		node.children[ CN_RIGHT ] = idxRight;
		return idxNode;
	}

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
		int idxLeft  = buildTree_R( idxNode , idxStart , idxEnd , true );

		Node& node = mTreeNodes[ idxNode ];
		node.parent  = idxParent;
		node.opUnit  = &unit;
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
	return buildTree_R( idxParent , idxStart , idxEnd , false );
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

	Node& node = mTreeNodes[ idxNode ];
	Unit& unit = *node.opUnit;

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


	Unit& unit = *node.opUnit;
	if ( ExprParse::IsBinaryOperator( unit.type ) && ExprParse::CanReverse( unit.type ) )
	{
		bool needSwap = false;
		if ( node.children[ CN_LEFT ] < 0 )
		{
			if ( node.children[ CN_RIGHT ] > 0 )
			{
				Unit& unitL = mExprCodes[ node.getLeaf( CN_LEFT ) ];
				if ( ExprParse::IsValue( unitL.type ) )
				{				
					Node& nodeR = mTreeNodes[ node.children[ CN_RIGHT ] ];
					Unit& unitR = *nodeR.opUnit;

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

	Node& node = mTreeNodes[ idxNode ];
	Unit& unit = *node.opUnit;

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
			if ( nodeChild.opUnit->type != ExprParse::IDT_SEPARETOR )
				break;

			++numVar;
			idxChild = nodeChild.children[ CN_LEFT ];
		}

		if ( idxChild != 0 )
		{
			++numVar;
		}

		if ( numVar != unit.symbol->func.getArgNum() )
			return TREE_FUN_PARAM_NUM_NO_MATCH;
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
	Node& node = mTreeNodes[ idxNode ];
	Unit& unit = *node.opUnit;

	RealType value = 0;

	if ( ExprParse::IsBinaryOperator( unit.type ) )
	{
		if ( node.children[ CN_LEFT ] >= 0 )
			return false;
		if ( node.children[ CN_RIGHT ] >= 0 )
			return false;

		Unit& unitL = mExprCodes[ node.getLeaf( CN_LEFT ) ];
		Unit& unitR = mExprCodes[ node.getLeaf( CN_RIGHT ) ];

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

		Unit& unitR = mExprCodes[ node.getLeaf(CN_RIGHT) ];

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
				assert( nodeParam.opUnit->type == IDT_SEPARETOR );
				
				if ( nodeParam.children[ CN_RIGHT ] >= 0 )
					return false;

				Unit& unitVal = mExprCodes[ nodeParam.getLeaf( CN_RIGHT ) ];
				if ( unitVal.type != VALUE_CONST )
					return false;

				params[n] = unitVal.constValue.asReal;
				idxParam = nodeParam.children[ CN_LEFT ];
			}

			Unit& unitVal = mExprCodes[ LEAF_UNIT_INDEX( idxParam ) ];
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


	unit.type       = ExprParse::VALUE_CONST;
	unit.constValue.layout = ValueLayout::Real;
	unit.constValue.asReal = value;
	return true;
}

bool ExprTreeBuilder::optimizeNodeBOpOrder( int idxNode )
{
	assert( idxNode > 0 );

	Node& node = mTreeNodes[ idxNode ];
	Unit& unit = *node.opUnit;

	if ( !ExprParse::IsBinaryOperator( unit.type ) || 
		 !ExprParse::CanExchange( unit.type ) )
		return false;



	return false;
}

void ExprTreeBuilder::printTree_R( int idxNode , int depth )
{
	
	if ( idxNode == 0 )
	{
		printSpace( depth * 4 );
		std::cout << "[]" << std::endl;
	}
	else if ( idxNode < 0 )
	{
		Unit& unit = mExprCodes[ LEAF_UNIT_INDEX( idxNode ) ];

		printSpace( depth * 4 );
		std::cout << '[';
		print( unit , *mTable );
		std::cout << ']' << std::endl;
	}
	else
	{
		Node const& node = mTreeNodes[ idxNode ];

		printTree_R( node.children[ CN_RIGHT ] , depth + 1 );

		printSpace( depth * 4 );
		std::cout << '[';
		print( *node.opUnit , *mTable );
		std::cout << ']' << std::endl;

		printTree_R( node.children[ CN_LEFT ] , depth + 1 );
	}
}

void ExprTreeBuilder::printSpace(int num)
{
	for ( int i = 0 ; i < num ; ++i )
	{
		std::cout << ' '; 
	}
}

void ExprTreeBuilder::printTree(SymbolTable const& table)
{
	mTable = &table;
	if ( mNumNodes != 0 )
	{
		Node& root = mTreeNodes[ 0 ];
		printTree_R( root.children[ CN_LEFT ] , 0 );
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
