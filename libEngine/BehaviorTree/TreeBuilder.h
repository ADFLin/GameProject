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
		FORCEINLINE static BTNode* Generate( NodeExpr const& expr )
		{
			BuildDecNode root;
			BTNode* node =  expr.build( &root );
			return root.getChild();
		}
	private:
		class BuildDecNode : public DecoratorNode
		{
		public:
			BTNodeInstance* buildInstance( TreeWalker& walker ){ assert(0); return NULL; }
		};
	};

	template< class BTNode >
	FORCEINLINE NodeBuilder< BTNode >
	Node()
	{
		return NodeBuilder< BTNode >( new BTNode );
	}

	template< class T , class Base >
	FORCEINLINE ContextMemFuncRef< T, Base >  
	MemberFunc( T (Base::*func)() ){ return ContextMemFuncRef< T , Base >( func );  }

	template< class T , class Base >
	FORCEINLINE ContextMemberRef< T , Base > 
	MemberPtr( T Base::*m ){ return ContextMemberRef< T , Base >( m ); }

	template< class T >
	FORCEINLINE GlobalVarRef< T >  
	VarPtr( T* ptr ){ return GlobalVarRef< T >( ptr ); }

	template< class T >
	FORCEINLINE GlobalFuncRef< T >     
	FuncPtr( T (*func)() ){ return GlobalFuncRef< T >( func ); }

	template< class CmpOp , class Var , Var* VAR >
	FORCEINLINE auto Condition()
	{
		typedef typename TConditionNodeInstance< ConstVarRef< Var , VAR > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp  , class Var , Var (*FUNC)() >
	FORCEINLINE NodeBuilder< typename TConditionNodeInstance< ConstFuncRef< Var , FUNC > , CmpOp  >::NodeType >
	Condition()
	{
		typedef typename TConditionNodeInstance< ConstFuncRef< Var , FUNC > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp  , class Var >
	FORCEINLINE NodeBuilder< typename TConditionNodeInstance< ContextVarRef< Var > , CmpOp  >::NodeType >
	Condition()
	{
		typedef typename TConditionNodeInstance< ContextVarRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	FORCEINLINE NodeBuilder< typename TConditionNodeInstance< ContextFuncRef< Var > , CmpOp  >::NodeType >
	ConditionFunc()
	{
		typedef typename TConditionNodeInstance< ContextFuncRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class RefType >
	FORCEINLINE NodeBuilder< typename TConditionNodeInstance< RefType , CmpOp  >::NodeType >
	Condition( RefType const& ref )
	{
		typedef typename TConditionNodeInstance< RefType , CmpOp >::NodeType  Node;
		return NodeBuilder< Node >( new Node( ref ) );
	}

	template< class CmpOp , class RefType >
	FORCEINLINE NodeBuilder< typename TFilterNodeInstance< RefType , CmpOp >::NodeType >
	Filter( RefType const& ref )
	{
		typedef typename TFilterNodeInstance< RefType , CmpOp >::NodeType  Node;
		return NodeBuilder< Node >( new Node( ref ) );
	}


	template< class CmpOp , class Var , Var* VAR  >
	FORCEINLINE NodeBuilder< typename TFilterNodeInstance< ConstVarRef< Var , VAR > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterNodeInstance< ConstVarRef< Var , VAR > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var , Var (*FUNC)() >
	FORCEINLINE NodeBuilder< typename TFilterNodeInstance< ConstFuncRef< Var , FUNC > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterNodeInstance< ConstFuncRef< Var , FUNC > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	FORCEINLINE NodeBuilder< typename TFilterNodeInstance< ContextVarRef< Var > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterNodeInstance< ContextVarRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

	template< class CmpOp , class Var >
	FORCEINLINE NodeBuilder< typename TFilterNodeInstance< ContextFuncRef< Var > , CmpOp  >::NodeType >
	Filter()
	{
		typedef typename TFilterNodeInstance< ContextFuncRef< Var > , CmpOp  >::NodeType Node;
		return NodeBuilder< Node >( new Node );
	}

}//namespace BT
#endif // TreeBuilder_h__
