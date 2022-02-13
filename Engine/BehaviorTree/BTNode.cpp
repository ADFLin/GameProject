#include "BTNode.h"

namespace BT
{
	BTNodeInstance* TreeWalker::enterNode( BTNode& node , BTNodeInstance* prevState )
	{
		BTNodeInstance* result = node.buildInstance( *this );
		result->mNode      = &node;
		result->mPrevState = prevState;

		if ( mInitializer )
			result->accept( *mInitializer );

		Entry entry;
		entry.state = TS_RUNNING;
		entry.node  = result;

		mEntries.push_back( entry );
		result->mEntry = &mEntries.back();

		return result;
	}

	void TreeWalker::resumeNode( BTNodeInstance& transform )
	{
		transform.mEntry->state = TS_RUNNING;
	}


	bool TreeWalker::step()
	{
		Entry& entry = mEntries.front();

		if ( entry.state != TS_SUSPENDED )
			entry.state = entry.node->execute(*this);

		bool result = false;

		switch( entry.state )
		{
		case TS_FAILED:
		case TS_SUCCESS:
			dispatchState( entry.node , entry.state );
			break;
		case TS_RUNNING:
			mEntries.splice( mEntries.end() , mEntries , mEntries.begin() );
			result = true;
			break;
		case TS_SUSPENDED:
			mEntries.splice( mEntries.end() , mEntries , mEntries.begin() );
			break;
		}

		return result;
	}


	class RootNodeInstance : public BTNodeInstance
	{
	public:
		bool resolveChildState( TreeWalker& walker , BTNodeInstance* child , TaskState& state )
		{
			return false;
		}
	};

	void TreeWalker::start( BTNode* node )
	{
		if ( node == NULL )
			return;
		BTNodeInstance* prevTransform = createInstance< RootNodeInstance >();
		enterNode( *node , prevTransform );
	}

	void TreeWalker::closeNode( BTNodeInstance* transform )
	{
		BTNode* node = transform->getNode< BTNode >();

		for( EntryQueue::iterator iter = mEntries.begin();
			iter != mEntries.end() ; ++iter )
		{
			if ( iter->node == transform )
			{
				mEntries.erase( iter );
				break;
			}
		}
		destroyInstance( transform );
	}

	void TreeWalker::dispatchState( BTNodeInstance* nodeInstance , TaskState state )
	{
		BTNodeInstance* parent = nodeInstance->mPrevState;
		BTNodeInstance* cur    = nodeInstance;
		while( parent )
		{
			bool skip = !parent->resolveChildState( *this , cur , state );

			closeNode( cur );
			if ( skip )
				break;
			cur    = parent;
			parent = parent->mPrevState;
		}
	}

	TaskState SelectorNodeInstance::execute( TreeWalker& walker )
	{
		if ( mNodeIter == getNode< SelectorNode >()->_getChildren().end() )
			return TS_FAILED;

		BTNode& executionNode = **mNodeIter;
		++mNodeIter;
		walker.enterNode( executionNode , this );
		return TS_SUSPENDED;
	}

	bool SelectorNodeInstance::resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state )
	{
		if ( state == TS_FAILED )
		{
			walker.resumeNode( *this );
			return false;
		}	
		return true;
	}

	bool SequenceNodeInatance::resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state )
	{
		if ( state == TS_SUCCESS )
		{
			walker.resumeNode( *this );
			return false;
		}
		return true;
	}

	TaskState SequenceNodeInatance::execute(TreeWalker& walker)
	{
		if ( mNodeIter == getNode< SequenceNode >()->_getChildren().end() )
			return TS_FAILED;

		BTNode& executionNode = **mNodeIter;
		++mNodeIter;
		walker.enterNode(executionNode, this );
		return TS_SUSPENDED;
	}


	TaskState ParallelNodeInstance::execute(TreeWalker& walker)
	{
		NodeList& children = getNode< ParallelNode >()->_getChildren();
		if ( children.empty() )
			return TS_FAILED;

		for( NodeList::iterator iter = children.begin() ; 
			iter != children.end(); ++iter )
		{
			BTNode* node = *iter;
			walker.enterNode( *node , this );
		}
		return  TS_SUSPENDED;
	}

	bool ParallelNodeInstance::resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state )
	{
		if ( state == TS_FAILED )
			++mCountFailure;
		else if ( state == TS_SUCCESS )
			++mCountSuccess;

		ParallelNode* node = getNode< ParallelNode >(); 
		int childrenSize = node->_getChildren().size();

		if ( mCountSuccess + mCountFailure == childrenSize )
		{
			if ( ( node->mSuccessCondition == RequireAll && mCountSuccess == childrenSize ) ||
				 ( node->mSuccessCondition == RequireOne && mCountSuccess > 0 ) )
				state = TS_SUCCESS;
			else
				state = TS_FAILED;
			return true;
		}
		return false;
	}
	
	ParallelNodeInstance::ParallelNodeInstance()
	{
		mCountFailure = 0;
		mCountSuccess = 0;
	}


	BTNodeInstance* ParallelNode::buildInstance( TreeWalker& walker )
	{
		return walker.createInstance< ParallelNodeInstance >();
	}


	void SelectorNode::setupInstance(SelectorNodeInstance& instnace )
	{
		instnace.mNodeIter = mChildren.begin();
	}


	void SequenceNode::setupInstance(SequenceNodeInatance& instnace)
	{
		instnace.mNodeIter = mChildren.begin();
	}


	bool CountDecNodeInstance::resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state )
	{
		++mExecCount;

		if ( state == TS_SUCCESS )
		{
			++mCurCount;
			if ( mCurCount >= getNode< CountDecNode >()->mConditionCount )
				return true;
		}
		else if ( mExecCount >= getNode< CountDecNode >()->mMaxCount )
		{
			return true;
		}
		walker.resumeNode( *this );
		return false;
	}


	bool LoopDecNodeInstance::resolveChildState(TreeWalker& walker, BTNodeInstance* child , TaskState& state )
	{
		++mExecCount;
		if ( mExecCount >= getNode< Node >()->mMaxCount )
		{
			state = TS_SUCCESS;
			return true;
		}
		walker.resumeNode( *this );
		return false;
	}

} //namespace ntu