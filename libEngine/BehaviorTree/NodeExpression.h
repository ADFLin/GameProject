#ifndef NodeExpression_h__
#define NodeExpression_h__

#define CHAIN_OP   operator |
#define SUBNODE_OP operator >>

namespace ntu
{
	template< class LExpr , class RExpr >
	class ChainNodeExpr;
	template< class LExpr , class RExpr >
	class SubeNodeExpr;
	template< class Node >
	class NodeBuilder;

	template< class T >
	class NodeExpr
	{
	public:
		template< class Q >
		ChainNodeExpr< T , Q > CHAIN_OP( NodeExpr< Q > const& expr )
		{ 
			return ChainNodeExpr< T , Q >( static_cast< T const& >(*this) , static_cast< Q const& >( expr ) ); 
		}
	};

	template< class T >
	struct ExprTraits
	{
		typedef T const& SaveType;
	};

	template< class Node >
	struct ExprTraits< NodeBuilder< Node > >
	{
		typedef NodeBuilder< Node > SaveType;
	};

	template< class T >
	struct NodeTraits
	{
		enum { isLeaf = 0 , isComposite = 0 };
	};

	template< class LExpr , class RExpr >
	class ChainNodeExpr : public NodeExpr< ChainNodeExpr< LExpr , RExpr > >
	{
	public:
		ChainNodeExpr( LExpr const& lhs , RExpr const& rhs ):lhs(lhs),rhs(rhs){}

		enum { ExprNum = LExpr::ExprNum + RExpr::ExprNum,  };
		__forceinline BTNode* build( BTNode* parent ) const
		{
			BTNode* lNode = lhs.build( parent );
			rhs.build( parent );
			return lNode;
		}
		typename ExprTraits< LExpr >::SaveType lhs;
		typename ExprTraits< RExpr >::SaveType rhs;
	};

	template < class LExpr , class RExpr >
	class SubNodeExpr : public NodeExpr< SubNodeExpr< LExpr , RExpr > >
	{
	public:
		SubNodeExpr( LExpr const& lhs , RExpr const& rhs ):lhs(lhs),rhs(rhs){}
		enum { ExprNum = LExpr::ExprNum + RExpr::ExprNum,  };
		BTNode* build( BTNode* parent ) const
		{
			parent = lhs.build( parent );
			parent = rhs.build( parent );
			return parent;
		}

		typename ExprTraits< LExpr >::SaveType lhs;
		typename ExprTraits< RExpr >::SaveType rhs;

		template< class Q >
		SubNodeExpr< SubNodeExpr , Q > SUBNODE_OP( NodeExpr< Q > const& expr ) const
		{ 
			return SubNodeExpr< SubNodeExpr , Q >( *this , static_cast< Q const& >( expr ) ); 
		}
	};

#define STATIC_ASSERT( expr )\
	typedef int checkExprType##__LINE__[ ( expr ) ? 1 : -1  ];

	struct FalseType{};
	struct TrueType{}; 

	template< class Node >
	class NodeBuilder : public NodeExpr< NodeBuilder< Node > >
	{
	public:
		NodeBuilder( Node* n ){  mNode = n; }
		NodeBuilder( Node& n ){  mNode = &n; }

		enum { ExprNum = 1, };
		typedef NodeBuilder< Node > ThisType;
		BTNode* build( BTNode* parent ) const
		{
			assert( parent );
			mNode->setParent( parent );
			parent->construct( mNode );
			return mNode;
		}

		Node* operator->(){ return mNode; }
		template< class Q >
		SubNodeExpr< ThisType , Q > SUBNODE_OP( NodeExpr< Q > const& expr ) const
		{ 
			STATIC_ASSERT( !Node::isLeaf );
			return SubNodeExpr< ThisType , Q >( *this , static_cast< Q const& >( expr ) ); 
		}
		Node* mNode;
	};


	template< class Node , class Expr >
	SubNodeExpr< NodeBuilder< Node > , Expr > 
	SUBNODE_OP( Node& node , NodeExpr< Expr > const& expr )
	{
		return SubNodeExpr< NodeBuilder< Node > , Expr >( 
			NodeBuilder< Node >( node ) , static_cast< Expr const& >( expr ) );
	}

	template< class Node , class Expr >
	SubNodeExpr< Expr , NodeBuilder< Node > > 
	SUBNODE_OP( NodeExpr< Expr > const& expr , Node& node )
	{
		return SubNodeExpr< Expr , NodeBuilder< Node > >( 
			static_cast< Expr const& >( expr )  , NodeBuilder< Node >( node ) );
	}

	template< class NODE1 , class NODE2 >
	SubNodeExpr< NodeBuilder< NODE1 > , NodeBuilder< NODE2 > > 
	SUBNODE_OP( NODE1& node , NODE2& node2 )
	{
		return SubNodeExpr< NodeBuilder< NODE1 > , NodeBuilder< NODE2 > >( 
			NodeBuilder< NODE1 >( node ) , NodeBuilder< NODE2 >( node2 ) );
	}

	template< class Node , class Expr >
	ChainNodeExpr< NodeBuilder< Node > , Expr > 
	CHAIN_OP( Node& node , NodeExpr< Expr > const& expr )
	{
		return ChainNodeExpr< NodeBuilder< Node > , Expr >( 
			NodeBuilder< Node >( node ) , static_cast< Expr const& >( expr ) );
	}

	template< class Node , class Expr >
	ChainNodeExpr< Expr , NodeBuilder< Node > > 
	CHAIN_OP( NodeExpr< Expr > const& expr , Node& node )
	{
		return ChainNodeExpr< Expr , NodeBuilder< Node > >( 
			static_cast< Expr const& >( expr )  , NodeBuilder< Node >( node ) );
	}


	template< class NODE1 , class NODE2 >
	ChainNodeExpr< NodeBuilder< NODE1 > , NodeBuilder< NODE2 > > 
	CHAIN_OP( NODE1& node , NODE2& node2 )
	{
		return ChainNodeExpr< NodeBuilder< NODE1 > , NodeBuilder< NODE2 > >( 
			NodeBuilder< NODE1 >( node ) , NodeBuilder< NODE2 >( node2 ) );
	}


}//namespace ntu


#undef CHAIN_OP
#undef SUBNODE_OP

#endif // NodeExpression_h__