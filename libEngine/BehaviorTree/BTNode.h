#ifndef BTNode_h__
#define BTNode_h__

#include <list>
#include <vector>
#include <cassert>

#include "Visitor.h"

namespace BT
{
	class BTNode;
	class BTNodeInstance;
	class TreeWalker;


	struct EmptyType {};


	enum TaskState
	{
		TS_RUNNING ,
		TS_SUCCESS ,
		TS_FAILED  ,
		TS_SUSPENDED ,
	};


	class NodeInitializer : public BaseVisitor
		                 // , public Visitor< BTTransform >
	{
	public:

	};

	class TreeWalker
	{
	public:

		void  start( BTNode* node );
		void  closeNode( BTNodeInstance* transform );
		BTNodeInstance* enterNode( BTNode& node , BTNodeInstance* prevState );

		bool  step();
		void  update()
		{
			while( !mEntries.empty() )
			{
				step();
			}
		}
		void  resumeNode( BTNodeInstance& transform );

		struct Entry
		{
			TaskState       state;
			BTNodeInstance* node;
			bool            bProcessing;
		};

		void setInitializer( NodeInitializer* init ){ mInitializer = init;  }

		template< class NodeInstance >
		NodeInstance* createInstance()
		{
			auto nodeInstance = new NodeInstance;
			return nodeInstance;
		}
		template< class NodeInstance >
		void  destroyInstance( NodeInstance* nodeInstance)
		{
			delete nodeInstance;
		}

		void  dispatchState( BTNodeInstance* transform , TaskState state );
		typedef std::list< Entry > EntryQueue;
		NodeInitializer* mInitializer;
		EntryQueue       mEntries;

	};

	class BTNode
	{
	public:
		BTNode()
		{
			mParent = NULL;
		}
		void    setParent( BTNode* node ){  mParent = node;  }
		virtual void  construct( BTNode* child ){};
		virtual BTNodeInstance*  buildInstance( TreeWalker& walker ) = 0;
	private:
		BTNode* mParent;
	};

	class BTNodeInstance : public BaseVisitable< void >
	{
	public:
		void  setNode( BTNode* node ){  mNode = node;  }

		virtual TaskState execute(TreeWalker& walker){ return TS_RUNNING; }
		virtual bool resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state ){  return true;  }

	protected:
		template< class T >
		T* getNode(){ return static_cast< T* >( mNode );  }

		friend class TreeWalker;
		BTNodeInstance* mPrevState;
		BTNode*         mNode;
	private:
		TreeWalker::Entry* mEntry;
	};

	typedef std::vector< BTNode* > NodeList;
	class CompositeNode : public BTNode
	{
	public:
		enum { isLeaf = 0 , isComposite = 1 };

		NodeList& _getChildren(){ return mChildren;  }
	protected:

		void construct( BTNode* child )
		{
			mChildren.push_back( child );
		}
		NodeList mChildren;
	};


	template< class NodeType , class NodeInstance >
	class BaseCompositeNodeT : public CompositeNode
	{
	public:
		BTNodeInstance*  buildInstance( TreeWalker& walker )
		{
			NodeInstance* instnace =  walker.createInstance< NodeInstance >();
			static_cast< NodeType* >( this )->setupInstance( *instnace );
			return instnace;
		}
		void setupInstance( NodeInstance& ){}
	};

	class SelectorNodeInstance;
	class SequenceNodeInatance;

	class SelectorNodeInstance : public BTNodeInstance
	{
	public:
		TaskState execute(TreeWalker& walker);
		bool      resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state );
	protected:
		friend class SelectorNode;
		NodeList::iterator mNodeIter;
	};

	class SelectorNode : public BaseCompositeNodeT< SelectorNode , SelectorNodeInstance >
	{
	public:
		void setupInstance( SelectorNodeInstance& instnace);
	};

	class OrderedIterator
	{

	};

	class ProbabilityIterator
	{

	};

	class RandSelectorNode
	{
	public:
		typedef RandSelectorNode ThisNode;
		ThisNode& Weight( float value ){ mWeights.push_back( value ); return *this;}
		std::vector< float > mWeights;
	};



	class SequenceNode : public BaseCompositeNodeT< SequenceNode , SequenceNodeInatance >
	{
	public:
		typedef SequenceNodeInatance TransformType;
		void setupInstance(SequenceNodeInatance& instnace);
	};

	class SequenceNodeInatance : public BTNodeInstance
	{
	public:
		TaskState execute(TreeWalker& walker);
		bool resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state );
	protected:
		friend class SequenceNode;
		NodeList::iterator mNodeIter;
	};

	class DecoratorNode : public BTNode
	{
	public:
		enum { isLeaf = 0 , isComposite = 0 };
		DecoratorNode()
		{
			mChild = NULL;
		}

		BTNode* getChild(){ return mChild; }
		void construct( BTNode* child )
		{
			assert( mChild == NULL );
			mChild = child;
		}
	protected:
		BTNode* mChild;
	};


	template< class Transform >
	class BaseDecNodeInstanceT : public BTNodeInstance
	{
	public:
		class Node : public DecoratorNode
		{
		protected:
			BTNodeInstance* buildInstance( TreeWalker& walker )
			{
				Transform* transform = walker.createInstance< Transform >();
				transform->init( mChild );
				return transform;
			}
		};
		typedef Node NodeType;

		void init( BTNode* parent ){}
		TaskState execute(TreeWalker& walker)
		{ 
			BTNode* node = getNode< DecoratorNode >()->getChild();
			if ( !node )
				return TS_FAILED;
			walker.enterNode( *node , this );
			return  TS_SUSPENDED;  
		}
	};

	class NotDecNodeInstance : public BaseDecNodeInstanceT< NotDecNodeInstance >
	{
	public:
		bool resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state )
		{
			if ( state == TS_FAILED )
				state = TS_SUCCESS;
			else if ( state == TS_SUCCESS )
				state = TS_FAILED;
			return true;
		}
	};
	typedef NotDecNodeInstance::NodeType NotDecNode;

	class BTLeafNode : public BTNode
	{
	public:
		enum { isLeaf = 1 , isComposite = 0 };
		void construct( BTNode* child ){  assert( 0 );  }
	};

	template< class NodeInstance >
	class TBaseLeafNode : public BTLeafNode
	{
	public:
		BTNodeInstance* buildInstance( TreeWalker& walker )
		{
			return walker.createInstance< NodeInstance >();
		}
	};


	template< class NodeInstance >
	class TBaseConditionNode : public TBaseLeafNode< NodeInstance >
	{

	};


#define DEFINE_COMP_OP( NAME , OP )\
	struct NAME \
	{\
		template < class T >\
		bool operator()( T const& lhs , T const& rhs ){ return lhs OP rhs;  }\
	};

	DEFINE_COMP_OP( EqualCmp         , == )
	DEFINE_COMP_OP( NotEqualCmp      , != )
	DEFINE_COMP_OP( GreaterCmp       , >  )
	DEFINE_COMP_OP( GreaterEqualCmp  , >= )
	DEFINE_COMP_OP( LessCmp          , <  )
	DEFINE_COMP_OP( LessEqualCmp     , <= )

#undef DEFINE_COMP_OP

	template< class Var , class CmpOp >
	class TConditionNodeInstance;

	template< class T , T* VAR >
	struct ConstVarRef
	{
		typedef T         RetType;
		typedef EmptyType Context;
		typedef EmptyType RefHolder;
		T operator()() const { return  (*VAR);  }
	};

	template < class T , T (*FUNC)() >
	struct ConstFuncRef
	{
		typedef T          RetType;
		typedef EmptyType  Context;
		typedef EmptyType  RefHolder;
		T operator()() const { return  (*FUNC)();  }
	};

	template< class T >
	struct GlobalVarRef
	{
		typedef T            RetType;
		typedef EmptyType    Context;
		typedef GlobalVarRef RefHolder;
		T operator()() const { return  (*mPtr);  }

		GlobalVarRef(T* ptr):mPtr(ptr){}
		T* mPtr;
	};


	template< class T >
	struct GlobalFuncRef
	{
		typedef T             RetType;
		typedef EmptyType     Context;
		typedef GlobalFuncRef RefHolder;
		T operator()() const { return  (*mFunc)();  }

		typedef T (*Func)();
		GlobalFuncRef(Func func):mFunc( Func ){}
		Func  mFunc;
	};

	template< class T >
	struct ContextVarRef
	{
		typedef T         RetType;
		typedef T         Context;
		typedef EmptyType RefHolder;
		T operator()( Context* c ) const { return  (*c);  }
	};

	template< class T >
	struct ContextFuncRef
	{
		typedef T RetType;
		typedef T (Context)();
		typedef EmptyType RefHolder;
		T operator()( Context* c ) const { return  (*c)();  }
	};

	template< class T , class Base >
	struct ContextMemberRef
	{
		typedef T         RetType;
		typedef Base      Context;
		typedef ContextMemberRef RefHolder; 
		T operator()( Context* c ) const { return  (c->*mMember);  }

		typedef T Base::*Member;
		ContextMemberRef( Member member ):mMember( member ){}
		Member mMember;
	};

	template < class T , class Base >
	struct ContextMemFuncRef
	{
		typedef T    RetType;
		typedef Base Context;
		typedef ContextMemFuncRef RefHolder;
		T operator()( Context* c ) const { return  (c->*mMemFunc)();  }

		typedef T   (Base::*MemberFunc)();
		ContextMemFuncRef( MemberFunc func ):mMemFunc(func){}
		MemberFunc mMemFunc;
	};


	template< class Context >
	class ContextHolder
	{
	public:
		ContextHolder(){ mHolder = NULL; }
		void  setHolder( Context* ptr ){ mHolder = ptr; }
		bool  isValid(){ return mHolder != 0; }
		template< class RetType , class RefType >
		RetType _evalValue( RefType const& ref ) {  return ref( mHolder );  }
	protected:
		Context* mHolder;
	};

	template<>
	class ContextHolder< EmptyType >
	{
	public:
		bool  isValid(){ return true; }
		template< class RetType , class RefType >
		RetType _evalValue( RefType const& ref ){  return ref();  }
	protected:
	};

	template< class Context >
	class ContextNodeInstance : public BTNodeInstance
		                      , public ContextHolder< Context >
	{
	protected:
		DEFINE_VISITABLE()
	};

	template< class RefType >
	class TBaseBlackBoardNodeInstance : public ContextNodeInstance< typename RefType::Context >
	{
	public:
		typedef typename RefType::RetType   VarType;
		typedef typename RefType::Context   Context;
		typedef typename RefType::RefHolder RefHolder;
	protected:

		template< class Node >
		class NodeData : public RefHolder
		{
			typedef Node ThisClass;
		public:
			NodeData(){}
			NodeData( RefHolder const& holder ):RefHolder( holder ){}
			ThisClass&  setValue( VarType value ){ mValue = value; return static_cast< Node& >(*this ); }
			VarType     getValue(){ return mValue; }
		protected:
			VarType     mValue;
		};

		template< class RefHolder >
		struct Eval
		{
			static VarType evalValue( ContextHolder< Context >& cHolder , RefHolder const& holder )
			{  return cHolder._evalValue< VarType >( holder );  }
		};

		template<>
		struct Eval< EmptyType >
		{
			static VarType evalValue( ContextHolder< Context >& cHolder , EmptyType const& )
			{  return cHolder._evalValue< VarType >( RefType() ); }
		};

	};

	template< class RefType , class CmpOp  >
	class TConditionNodeInstance : public TBaseBlackBoardNodeInstance< RefType >
	{
	public:
		class Node : public  TBaseConditionNode< TConditionNodeInstance< RefType , CmpOp > >
				   , public  NodeData< Node >
		{
		public:
			Node(){}
			Node( RefHolder const& holder ):NodeData< Node >( holder ){}
		};
		typedef Node NodeType;

	protected:
		TaskState execute(TreeWalker& walker)
		{
			if ( !ContextHolder< Context >::isValid() )
				return TS_SUCCESS;
			if ( CmpOp()( Eval< RefHolder >::evalValue( *this , *getNode< NodeType >() ) ,
					      getNode< NodeType >()->getValue() ) )
				return TS_SUCCESS;
			return TS_FAILED;
		}
	};

	template< class RefType , class CmpOp >
	class TFilterNodeInstance : public TBaseBlackBoardNodeInstance< RefType >
	{
	public:
		class Node : public  DecoratorNode
			       , public  NodeData< Node >
		{
		public:
			Node(){}
			Node( RefHolder const& holder ):NodeData< Node >( holder ){}
			BTNodeInstance* buildInstance( TreeWalker& walker )
			{
				return walker.createInstance< TFilterNodeInstance >();
			}
		};

		typedef Node NodeType;

	protected:
		TaskState execute(TreeWalker& walker)
		{
			if ( ContextHolder< Context >::isValid() &&
				 !CmpOp()( Eval< RefHolder >::evalValue( *this , *getNode< NodeType >() ) ,
				           getNode< NodeType >()->getValue() ) )
				return TS_FAILED;
			if ( !getNode< Node >()->getChild() )
				return TS_FAILED;
			walker.enterNode( *getNode< Node >()->getChild() , this );
			return TS_SUSPENDED;
		}
		bool  resolveChildState( BTNodeInstance* child , TaskState& state )
		{
			return true;
		}
	};

	class CountDecNodeInstance : public BaseDecNodeInstanceT< CountDecNodeInstance >
	{
		typedef int   CountType;
	public:
		class Node : public BaseDecNodeInstanceT< CountDecNodeInstance >::Node
		{
			typedef int  CountType;
			typedef Node ThisClass;
		public:
			ThisClass& setMaxCount( CountType val ){ mMaxCount = val; return *this;  }
			ThisClass& setConditionCount( CountType val ){ mConditionCount = val; return *this;  }
		protected:
			friend class CountDecNodeInstance;
			CountType mMaxCount;
			CountType mConditionCount;
		};
		typedef Node NodeType;

		CountDecNodeInstance()
		{
			mCurCount = 0;
			mExecCount = 0;
		}

		bool resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state );

		CountType mExecCount;
		CountType mCurCount;
	};

	typedef CountDecNodeInstance::NodeType CountDecNode;


	class LoopDecNodeInstance : public BaseDecNodeInstanceT< LoopDecNodeInstance >
	{
		typedef int         CountType;
	public:
		LoopDecNodeInstance()
		{
			mExecCount = 0;
		}

		class Node : public BaseDecNodeInstanceT< LoopDecNodeInstance >::Node
		{
			typedef Node ThisClass;
			typedef int  CountType;
		public:
			Node()
			{
				mMaxCount = 1;
			}

			ThisClass& setMaxCount( CountType val ){ mMaxCount = val;  return *this;  }
			CountType mMaxCount;
		};
		typedef Node NodeType;
		bool resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state );
		CountType mExecCount;
	};

	typedef LoopDecNodeInstance::NodeType LoopDecNode;


	template< class Node ,class ActionData >
	class BaseActionNodeT;

	template< class Node , class ActionData >
	class TActionNodeInstance : public BTNodeInstance
		                      , public ActionData
	{
	public:
		TaskState execute(TreeWalker& walker){ return getNode< Node >()->action( this ); }
	};


	template< class Node , class ActionData = EmptyType >
	class BaseActionNodeT : public BTLeafNode
	{
	public:
		typedef TActionNodeInstance< Node , ActionData > NodeInstance;
		BTNodeInstance* buildInstance( TreeWalker& walker )
		{
			return walker.createInstance< NodeInstance >();
		}
		TaskState action(NodeInstance* transform )
		{
			return TS_SUCCESS;
		}
	};


	enum ConditionPolicy
	{
		RequireOne ,
		RequireAll ,
	};
	class ParallelNode : public CompositeNode
	{
	public:
		BTNodeInstance* buildInstance( TreeWalker& walker );
		ParallelNode&   setSuccessPolicy(ConditionPolicy p ){ mSuccessCondition = p; return *this;  }
	protected:
		ConditionPolicy mSuccessCondition;
		friend class ParallelNodeInstance;
	};

	class ParallelNodeInstance : public BTNodeInstance
	{
	public:
		ParallelNodeInstance();
		TaskState execute(TreeWalker& walker);
		bool resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state );

		int mCountFailure;
		int mCountSuccess;
	};




}//namespace ntu

#endif // BTNode_h__