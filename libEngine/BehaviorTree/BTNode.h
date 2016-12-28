#ifndef BTNode_h__
#define BTNode_h__

#include <list>
#include <vector>
#include <cassert>

#include "Visitor.h"
namespace ntu
{
	class BTNode;
	class BTTransform;
	class TreeWalker;


	struct EmptyType {};


	enum TaskState
	{
		TS_RUNNING ,
		TS_SUCCESS ,
		TS_FAILED  ,
		TS_SUSPENDED ,
	};


	class TreeInitializer : public BaseVisitor
		                 // , public Visitor< BTTransform >
	{
	public:

	};

	class TreeWalker
	{
	public:

		void  start( BTNode* node );
		void  close( BTTransform* transform );
		BTTransform* enter( BTNode& node , BTTransform* prevState );

		bool  step();
		void  update()
		{
			while( !mEntries.empty() )
			{
				step();
			}
		}
		void  resume( BTTransform& transform );

		struct Entry
		{
			TaskState    state;
			BTTransform* transform;
			bool         beProcessing;
		};

		void setInitializer( TreeInitializer* init ){ mInitializer = init;  }
		template< class Transform >
		Transform* createTransform()
		{
			Transform* transform = new Transform;
			return transform;
		}
		template< class Transform >
		void  destroyTransform( Transform* transform )
		{
			delete transform;
		}
		void  dispatchState( BTTransform* transform , TaskState state );
		typedef std::list< Entry > EntryQueue;
		TreeInitializer* mInitializer;
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
		virtual BTTransform*  buildTransform( TreeWalker* walker ) = 0;
		virtual bool  execute(){ return true; }
	private:
		BTNode* mParent;
	};

	class BTTransform : public BaseVisitable< void >
	{
	public:
		void  setNode( BTNode* node ){  mNode = node;  }

		virtual TaskState execute(){ return TS_RUNNING; }
		virtual bool onRecvChildState( BTTransform* child , TaskState& state ){  return true;  }

	protected:
		template< class T >
		T* getNode(){ return static_cast< T* >( mNode );  }

		friend class TreeWalker;
		TreeWalker*  mWalker;
		BTTransform* mPrevState;
		BTNode*      mNode;
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


	template< class NodeType , class Transform >
	class BaseCompositeNode : public CompositeNode
	{
	public:
		typedef Transform TransformType;
		BTTransform*  buildTransform( TreeWalker* walker )
		{
			TransformType* transform =  walker->createTransform< TransformType >();
			static_cast< NodeType* >( this )->setupTransform( transform );
			return transform;
		}
		void setupTransform( TransformType* ){}
	};

	class SelectorTransform;
	class SequenceTransform;

	class SelectorTransform : public BTTransform
	{
	public:
		TaskState execute();
		bool      onRecvChildState( BTTransform* child , TaskState& state );
	protected:
		friend class SelectorNode;
		NodeList::iterator mNodeIter;
	};

	class SelectorNode : public BaseCompositeNode< SelectorNode , SelectorTransform >
	{
	public:
		void setupTransform( TransformType* transform );
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



	class SequenceNode : public BaseCompositeNode< SequenceNode , SequenceTransform >
	{
	public:
		typedef SequenceTransform TransformType;
		void setupTransform( TransformType* transform );
	};

	class SequenceTransform : public BTTransform
	{
	public:
		TaskState execute();
		bool onRecvChildState( BTTransform* child , TaskState& state );
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
	class BaseDecTransform : public BTTransform
	{
	public:
		class Node : public DecoratorNode
		{
		protected:
			BTTransform* buildTransform( TreeWalker* walker )
			{
				Transform* transform = walker->createTransform< Transform >();
				transform->init( mChild );
				return transform;
			}
		};
		typedef Node NodeType;

		void init( BTNode* parent ){}
		TaskState execute()
		{ 
			BTNode* node = getNode< DecoratorNode >()->getChild();
			if ( !node )
				return TS_FAILED;
			mWalker->enter( *node , this );
			return  TS_SUSPENDED;  
		}
	};

	class NotDecTransform : public BaseDecTransform< NotDecTransform >
	{
	public:
		bool onRecvChildState( BTTransform* child , TaskState& state )
		{
			if ( state == TS_FAILED )
				state = TS_SUCCESS;
			else if ( state == TS_SUCCESS )
				state = TS_FAILED;
			return true;
		}
	};
	typedef NotDecTransform::NodeType NotDecNode;

	class BTLeafNode : public BTNode
	{
	public:
		enum { isLeaf = 1 , isComposite = 0 };
		void construct( BTNode* child ){  assert( 0 );  }
	};

	template< class Transform >
	class BaseLeafNode : public BTLeafNode
	{
	public:
		BTTransform* buildTransform( TreeWalker* walker )
		{
			return walker->createTransform< Transform >();
		}
	};


	template< class Transform >
	class BaseConditionNode : public BaseLeafNode< Transform >
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
	class ConditionTransform;

	template< class T , T* VAR >
	struct ConstVarRef
	{
		typedef T         RetType;
		typedef EmptyType Context;
		typedef EmptyType RefHolder;
		T operator()() const { return  (*VAR);  }
	};

	template < class T , T (*FUN)() >
	struct ConstFunRef
	{
		typedef T          RetType;
		typedef EmptyType  Context;
		typedef EmptyType  RefHolder;
		T operator()() const { return  (*FUN)();  }
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
	struct GlobalFunRef
	{
		typedef T            RetType;
		typedef EmptyType    Context;
		typedef GlobalFunRef RefHolder;
		T operator()() const { return  (*mFun)();  }

		typedef T (*Fun)();
		GlobalFunRef( Fun fun ):mFun( Fun ){}
		Fun  mFun;
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
	struct ContextFunRef
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
	struct ContextMemFunRef
	{
		typedef T    RetType;
		typedef Base Context;
		typedef ContextMemFunRef RefHolder;
		T operator()( Context* c ) const { return  (c->*mMemFun)();  }

		typedef T   (Base::*MemberFun)();
		ContextMemFunRef( MemberFun fun ):mMemFun(fun){}
		MemberFun mMemFun;
	};


	template< class Context >
	class ContextHolder
	{
	public:
		ContextHolder(){ mHolder = NULL; }
		void  setHolder( Context* ptr ){ mHolder = ptr; }
		bool  isVaild(){ return mHolder != 0; }
		template< class RetType , class RefType >
		RetType _evalValue( RefType const& ref ) {  return ref( mHolder );  }
	protected:
		Context* mHolder;
	};

	template<>
	class ContextHolder< EmptyType >
	{
	public:
		bool  isVaild(){ return true; }
		template< class RetType , class RefType >
		RetType _evalValue( RefType const& ref ){  return ref();  }
	protected:
	};

	template< class Context >
	class ContextTransform : public BTTransform
		                   , public ContextHolder< Context >
	{
	protected:
		DEFNE_VISITABLE()
	};

	template< class RefType >
	class BaseBlackBoardTransform : public ContextTransform< typename RefType::Context >
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
	class ConditionTransform : public BaseBlackBoardTransform< RefType >
	{
	public:
		class Node : public  BaseConditionNode< ConditionTransform< RefType , CmpOp > >
				   , public  NodeData< Node >
		{
		public:
			Node(){}
			Node( RefHolder const& holder ):NodeData< Node >( holder ){}
		};
		typedef Node NodeType;

	protected:
		TaskState execute()
		{
			if ( !ContextHolder< Context >::isVaild() )
				return TS_SUCCESS;
			if ( CmpOp()( Eval< RefHolder >::evalValue( *this , *getNode< NodeType >() ) ,
					      getNode< NodeType >()->getValue() ) )
				return TS_SUCCESS;
			return TS_FAILED;
		}
	};

	template< class RefType , class CmpOp >
	class FilterTransform : public BaseBlackBoardTransform< RefType >
	{
	public:
		class Node : public  DecoratorNode
			       , public  NodeData< Node >
		{
		public:
			Node(){}
			Node( RefHolder const& holder ):NodeData< Node >( holder ){}
			BTTransform* buildTransform( TreeWalker* walker )
			{
				return walker->createTransform< FilterTransform >();
			}
		};

		typedef Node NodeType;

	protected:
		TaskState execute()
		{
			if ( ContextHolder< Context >::isVaild() && 
				 !CmpOp()( Eval< RefHolder >::evalValue( *this , *getNode< NodeType >() ) ,
				           getNode< NodeType >()->getValue() ) )
				return TS_FAILED;
			if ( !getNode< Node >()->getChild() )
				return TS_FAILED;
			mWalker->enter( *getNode< Node >()->getChild() , this );
			return TS_SUSPENDED;
		}
		bool  onRecvChildState( BTTransform* child , TaskState& state )
		{
			return true;
		}
	};

	class CountDecTransform : public BaseDecTransform< CountDecTransform >
	{
		typedef int   CountType;
	public:
		class Node : public BaseDecTransform< CountDecTransform >::Node
		{
			typedef int  CountType;
			typedef Node ThisClass;
		public:
			ThisClass& setMaxCount( CountType val ){ mMaxCount = val; return *this;  }
			ThisClass& setConditionCount( CountType val ){ mConditionCount = val; return *this;  }
		protected:
			friend class CountDecTransform;
			CountType mMaxCount;
			CountType mConditionCount;
		};
		typedef Node NodeType;

		CountDecTransform()
		{
			mCurCount = 0;
			mExecCount = 0;
		}

		bool onRecvChildState( BTTransform* child , TaskState& state );

		CountType mExecCount;
		CountType mCurCount;
	};

	typedef CountDecTransform::NodeType CountDecNode;


	class LoopDecTransform : public BaseDecTransform< LoopDecTransform >
	{
		typedef int         CountType;
	public:
		LoopDecTransform()
		{
			mExecCount = 0;
		}

		class Node : public BaseDecTransform< LoopDecTransform >::Node
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
		bool onRecvChildState( BTTransform* child , TaskState& state );
		CountType mExecCount;
	};

	typedef LoopDecTransform::NodeType LoopDecNode;


	template< class Node ,class ActionData >
	class BaseActionNode;

	template< class Node , class ActionData >
	class ActionTransform : public BTTransform
		                  , public ActionData
	{
	public:
		TaskState execute(){ return getNode< Node >()->action( this ); }
	};


	template< class Node , class ActionData = EmptyType >
	class BaseActionNode : public BTLeafNode
	{
	public:
		typedef ActionTransform< Node , ActionData > TransformType;
		BTTransform* buildTransform( TreeWalker* walker )
		{
			return walker->createTransform< TransformType >();
		}
		TaskState action( TransformType* transform )
		{
			return TS_SUCCESS;
		}
	};


	enum Policy
	{
		RequireOne ,
		RequireAll ,
	};
	class ParallelNode : public CompositeNode
	{
	public:
		BTTransform* buildTransform( TreeWalker* walker );
		ParallelNode& setSuccessPolicy( Policy p ){  mSuccessPolicy = p; return *this;  }
	protected:
		Policy mSuccessPolicy;
		friend class ParallelTransform;
	};

	class ParallelTransform : public BTTransform
	{
	public:
		ParallelTransform();
		TaskState execute();
		bool onRecvChildState( BTTransform* child , TaskState& state );

		int mCountFailure;
		int mCountSuccess;
	};




}//namespace ntu

#endif // BTNode_h__