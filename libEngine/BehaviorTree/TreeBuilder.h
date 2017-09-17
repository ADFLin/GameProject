#ifndef TreeBuilder_h__
#define TreeBuilder_h__

#include "BTNode.h"
#include "NodeExpression.h"

namespace BT
{
	class TreeBuilder
	{
	public:
		template< class NodeExpr >
		FORCEINLINE static BTNode* generate( NodeExpr const& expr )
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

	template< class BTNode >
	FORCEINLINE NodeBuilder< BTNode >
	Node()
	{
		return NodeBuilder< BTNode >( new BTNode );
	}

	template< class T , class Base >
	FORCEINLINE ContextMemFunRef< T, Base >  
	MemberFun( T (Base::*fun)() ){ return ContextMemFunRef< T , Base >( fun );  }

	template< class T , class Base >
	FORCEINLINE ContextMemberRef< T , Base > 
	MemberPtr( T Base::*m ){ return ContextMemberRef< T , Base >( m ); }

	template< class T >
	FORCEINLINE GlobalVarRef< T >  
	VarPtr( T* ptr ){ return GlobalVarRef< T >( ptr ); }

	template< class T >
	FORCEINLINE GlobalFunRef< T >     
	FunPtr( T (*fun)() ){ return GlobalFunRef< T >( fun ); }

	template< class CmpOp , class Var , Var* VAR >
	FORCEINLINE NodeBuilder< typename TConditionTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType >
	Condition()
	{
		typedef typename TConditionTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp  , class Var , Var (*FUN)() >
	FORCEINLINE NodeBuilder< typename TConditionTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType >
	Condition()
	{
		typedef typename TConditionTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp  , class Var >
	FORCEINLINE NodeBuilder< typename TConditionTransform< ContextVarRef< Var > , CmpOp  >::NodeType >
	Condition()
	{
		typedef typename TConditionTransform< ContextVarRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	FORCEINLINE NodeBuilder< typename TConditionTransform< ContextFunRef< Var > , CmpOp  >::NodeType >
	ConditionFun()
	{
		typedef typename TConditionTransform< ContextFunRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class RefType >
	FORCEINLINE NodeBuilder< typename TConditionTransform< RefType , CmpOp  >::NodeType >
	Condition( RefType const& ref )
	{
		typedef typename TConditionTransform< RefType , CmpOp >::NodeType  Node;
		return NodeBuilder< Node >( new Node( ref ) );
	}

	template< class CmpOp , class RefType >
	FORCEINLINE NodeBuilder< typename TFilterTransform< RefType , CmpOp >::NodeType >
	Filter( RefType const& ref )
	{
		typedef typename TFilterTransform< RefType , CmpOp >::NodeType  Node;
		return NodeBuilder< Node >( new Node( ref ) );
	}


	template< class CmpOp , class Var , Var* VAR  >
	FORCEINLINE NodeBuilder< typename TFilterTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterTransform< ConstVarRef< Var , VAR > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var , Var (*FUN)() >
	FORCEINLINE NodeBuilder< typename TFilterTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterTransform< ConstFunRef< Var , FUN > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	FORCEINLINE NodeBuilder< typename TFilterTransform< ContextVarRef< Var > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterTransform< ContextVarRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	FORCEINLINE NodeBuilder< typename TFilterTransform< ContextFunRef< Var > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterTransform< ContextFunRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}



}//namespace BT
#endif // TreeBuilder_h__
