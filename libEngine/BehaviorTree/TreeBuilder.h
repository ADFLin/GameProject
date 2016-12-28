#ifndef TreeBuilder_h__
#define TreeBuilder_h__

#include "BTNode.h"
#include "NodeExpression.h"

namespace ntu
{
	class TreeBuilder
	{
	public:
		template< class NodeExpr >
		static BTNode* generate( NodeExpr const& expr )
		{
			BuildDecNode root;
			BTNode* node =  expr.build( &root );
			return root.getChild();
		}
	private:
		class BuildDecNode : public DecoratorNode
		{
		public:
			BTTransform* buildTransform( TreeWalker* walker ){ assert(0); return NULL; }
		};
	};

	template< class Node >
	NodeBuilder< Node >
	node()
	{
		return NodeBuilder< Node >( new Node );
	}

	template< class T , class Base >
	ContextMemFunRef< T, Base >  memberFun( T (Base::*fun)() ){ return ContextMemFunRef< T , Base >( fun );  }

	template< class T , class Base >
	ContextMemberRef< T , Base > member( T Base::*m ){ return ContextMemberRef< T , Base >( m ); }

	template< class T >
	GlobalVarRef< T >     varPtr( T* ptr ){ return GlobalVarRef< T >( ptr ); }

	template< class T >
	GlobalFunRef< T >     FunPtr( T (*fun)() ){ return GlobalFunRef< T >( fun ); }


	template< class CmpOp , class Var , Var* VAR >
	NodeBuilder< typename ConditionTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType >
	condition()
	{
		typedef ConditionTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp  , class Var , Var (*FUN)() >
	NodeBuilder< typename ConditionTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType >
	condition()
	{
		typedef ConditionTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp  , class Var >
	NodeBuilder< typename ConditionTransform< ContextVarRef< Var > , CmpOp  >::NodeType >
	condition()
	{
		typedef ConditionTransform< ContextVarRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	NodeBuilder< typename ConditionTransform< ContextFunRef< Var > , CmpOp  >::NodeType >
	conditionFun()
	{
		typedef ConditionTransform< ContextFunRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class RefType >
	NodeBuilder< typename ConditionTransform< RefType , CmpOp  >::NodeType >
	condition( RefType const& ref )
	{
		typedef typename ConditionTransform< RefType , CmpOp >::NodeType  Node;
		return NodeBuilder< Node >( new Node( ref ) );
	}

	template< class CmpOp , class RefType >
	NodeBuilder< typename FilterTransform< RefType , CmpOp >::NodeType >
	filter( RefType const& ref )
	{
		typedef typename FilterTransform< RefType , CmpOp >::NodeType  Node;
		return NodeBuilder< Node >( new Node( ref ) );
	}


	template< class CmpOp , class Var , Var* VAR  >
	NodeBuilder< typename FilterTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType >
	filter()
	{
		typedef FilterTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var , Var (*FUN)() >
	NodeBuilder< typename FilterTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType >
	filter()
	{
		typedef FilterTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	NodeBuilder< typename FilterTransform< ContextVarRef< Var > , CmpOp  >::NodeType >
	filter()
	{
		typedef FilterTransform< ContextVarRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	NodeBuilder< typename FilterTransform< ContextFunRef< Var > , CmpOp  >::NodeType >
	filter()
	{
		typedef FilterTransform< ContextFunRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}



}//namespace ntu
#endif // TreeBuilder_h__