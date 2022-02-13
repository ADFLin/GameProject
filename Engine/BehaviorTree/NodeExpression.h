#ifndef NodeExpression_h__
#define NodeExpression_h__

#include "CompilerConfig.h"

#define CHAIN_OP   operator |
#define SUBNODE_OP operator >>

namespace BT
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
		FORCEINLINE ChainNodeExpr< T , Q > CHAIN_OP( NodeExpr< Q > const& expr )
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
		FORCEINLINE ChainNodeExpr( LExpr const& lhs , RExpr const& rhs ):lhs(lhs),rhs(rhs){}

		enum { ExprNum = LExpr::ExprNum + RExpr::ExprNum,  };
		FORCEINLINE BTNode* build( BTNode* parent ) const
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
		FORCEINLINE SubNodeExpr( LExpr const& lhs , RExpr const& rhs ):lhs(lhs),rhs(rhs){}
		enum { ExprNum = LExpr::ExprNum + RExpr::ExprNum,  };
		FORCEINLINE BTNode* build( BTNode* parent ) const
		{
			parent = lhs.build( parent );
			parent = rhs.build( parent );
			return parent;
		}

		typename ExprTraits< LExpr >::SaveType lhs;
		typename ExprTraits< RExpr >::SaveType rhs;

		template< class Q >
		FORCEINLINE SubNodeExpr< SubNodeExpr , Q > SUBNODE_OP( NodeExpr< Q > const& expr ) const
		{
			return SubNodeExpr< SubNodeExpr , Q >( *this , static_cast< Q const& >( expr ) );
		}
	};


	struct FalseType{};
	struct TureType{};

	template< class Node >
	class NodeBuilder : public NodeExpr< NodeBuilder< Node > >
	{
	public:
		FORCEINLINE NodeBuilder( Node* n ){  mNode = n; }
		FORCEINLINE NodeBuilder( Node& n ){  mNode = &n; }

		enum { ExprNum = 1, };
		typedef NodeBuilder< Node > ThisType;
		FORCEINLINE BTNode* build( BTNode* parent ) const
		{
			assert( parent );
			mNode->setParent( parent );
			parent->construct( mNode );
			return mNode;
		}

		Node* operator->(){ return mNode; }
		template< class Q >
		FORCEINLINE SubNodeExpr< ThisType , Q > SUBNODE_OP( NodeExpr< Q > const& expr ) const
		{
			//STATIC_ASSERT( !Node::isLeaf );
			return SubNodeExpr< ThisType , Q >( *this , static_cast< Q const& >( expr ) );
		}
		Node* mNode;
	};


	template< class Node , class Expr >
	FORCEINLINE SubNodeExpr< NodeBuilder< Node > , Expr >
	SUBNODE_OP( Node& node , NodeExpr< Expr > const& expr )
	{
		return SubNodeExpr< NodeBuilder< Node > , Expr >(
			NodeBuilder< Node >( node ) , static_cast< Expr const& >( expr ) );
	}

	template< class Expr , class Node >
	FORCEINLINE SubNodeExpr< Expr , NodeBuilder< Node > >
	SUBNODE_OP( NodeExpr< Expr > const& expr , Node& node )
	{
		return SubNodeExpr< Expr , NodeBuilder< Node > >(
			static_cast< Expr const& >( expr )  , NodeBuilder< Node >( node ) );
	}

	template< class NodeA , class NodeB >
	FORCEINLINE SubNodeExpr< NodeBuilder< NodeA > , NodeBuilder< NodeB > >
	SUBNODE_OP( NodeA& node , NodeB& node2 )
	{
		return SubNodeExpr< NodeBuilder< NodeA > , NodeBuilder< NodeB > >(
			NodeBuilder< NodeA >( node ) , NodeBuilder< NodeB >( node2 ) );
	}

	template< class Node , class Expr >
	FORCEINLINE ChainNodeExpr< NodeBuilder< Node > , Expr >
	CHAIN_OP( Node& node , NodeExpr< Expr > const& expr )
	{
		return ChainNodeExpr< NodeBuilder< Node > , Expr >(
			NodeBuilder< Node >( node ) , static_cast< Expr const& >( expr ) );
	}

	template< class Expr , class Node >
	FORCEINLINE ChainNodeExpr< Expr , NodeBuilder< Node > >
	CHAIN_OP( NodeExpr< Expr > const& expr , Node& node )
	{
		return ChainNodeExpr< Expr , NodeBuilder< Node > >(
			static_cast< Expr const& >( expr )  , NodeBuilder< Node >( node ) );
	}


	template< class NodeA , class NodeB >
	FORCEINLINE ChainNodeExpr< NodeBuilder< NodeA > , NodeBuilder< NodeB > >
	CHAIN_OP( NodeA& node , NodeB& node2 )
	{
		return ChainNodeExpr< NodeBuilder< NodeA > , NodeBuilder< NodeB > >(
			NodeBuilder< NodeA >( node ) , NodeBuilder< NodeB >( node2 ) );
	}


}//namespace BT


#undef CHAIN_OP
#undef SUBNODE_OP

#endif // NodeExpression_h__
