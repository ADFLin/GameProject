#include "BTNode.h"

namespace BT
{
	BTTransform* TreeWalker::enter( BTNode& node , BTTransform* prevState )
	{
		BTTransform* result = node.buildTransform( this );

		result->mWalker    = this;
		result->mNode      = &node;
		result->mPrevState = prevState;

		if ( mInitializer )
			result->accept( *mInitializer );

		Entry entry;
		entry.state     = TS_RUNNING;
		entry.transform = result;

		mEntries.push_back( entry );
		result->mEntry = &mEntries.back();

		return result;
	}

	void TreeWalker::resume( BTTransform& transform )
	{
		transform.mEntry->state = TS_RUNNING;
	}


	bool TreeWalker::step()
	{
		Entry& entry = mEntries.front();

		if ( entry.state != TS_SUSPENDED )
			entry.state = entry.transform->execute();

		bool result = false;

		switch( entry.state )
		{
		case TS_FAILED:
		case TS_SUCCESS:
			dispatchState( entry.transform , entry.state );
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


	class RootTransform : public BTTransform
	{
	public:
		bool onRecvChildState( BTTransform* child , TaskState& state )
		{
			return false;
		}
	};

	void TreeWalker::start( BTNode* node )
	{
		if ( node == NULL )
			return;
		BTTransform* prevTransform = createTransform< RootTransform >();
		enter( *node , prevTransform );
	}

	void TreeWalker::close( BTTransform* transform )
	{
		BTNode* node = transform->getNode< BTNode >();

		for( EntryQueue::iterator iter = mEntries.begin();
			iter != mEntries.end() ; ++iter )
		{
			if ( iter->transform == transform )
			{
				mEntries.erase( iter );
				break;
			}
		}
		destroyTransform( transform );
	}

	void TreeWalker::dispatchState( BTTransform* transform , TaskState state )
	{
		BTTransform* parent =  transform->mPrevState;
		BTTransform* cur    =  transform;
		while( parent )
		{
			bool skip = !parent->onRecvChildState( cur , state );

			close( cur );
			if ( skip )
				break;
			cur    = parent;
			parent = parent->mPrevState;
		}
	}

	TaskState SelectorTransform::execute()
	{
		if ( mNodeIter == getNode< SelectorNode >()->_getChildren().end() )
			return TS_FAILED;

		mWalker->enter( **mNodeIter , this );
		++mNodeIter;
		return TS_SUSPENDED;
	}

	bool SelectorTransform::onRecvChildState( BTTransform* child , TaskState& state )
	{
		if ( state == TS_FAILED )
		{
			mWalker->resume( *this );
			return false;
		}	
		return true;
	}

	bool SequenceTransform::onRecvChildState( BTTransform* child , TaskState& state )
	{
		if ( state == TS_SUCCESS )
		{
			mWalker->resume( *this );
			return false;
		}
		return true;
	}

	TaskState SequenceTransform::execute()
	{
		if ( mNodeIter == getNode< SequenceNode >()->_getChildren().end() )
			return TS_FAILED;

		mWalker->enter( **mNodeIter , this );
		++mNodeIter;
		return TS_SUSPENDED;
	}


	BT::TaskState ParallelTransform::execute()
	{
		NodeList& children = getNode< ParallelNode >()->_getChildren();
		if ( children.empty() )
			return TS_FAILED;

		for( NodeList::iterator iter = children.begin() ; 
			iter != children.end(); ++iter )
		{
			BTNode* node = *iter;
			mWalker->enter( *node , this );
		}
		return  TS_SUSPENDED;
	}

	bool ParallelTransform::onRecvChildState( BTTransform* child , TaskState& state )
	{
		if ( state == TS_FAILED )
			++mCountFailure;
		else if ( state == TS_SUCCESS )
			++mCountSuccess;

		ParallelNode* node = getNode< ParallelNode >(); 
		int childrenSize = node->_getChildren().size();

		if ( mCountSuccess + mCountFailure == childrenSize )
		{
			if ( ( node->mSuccessPolicy == RequireAll && mCountSuccess == childrenSize ) ||
				( node->mSuccessPolicy == RequireOne && mCountSuccess > 0 ) )
				state = TS_SUCCESS;
			else
				state = TS_FAILED;
			return true;
		}
		return false;
	}
	
	ParallelTransform::ParallelTransform()
	{
		mCountFailure = 0;
		mCountSuccess = 0;
	}


	BTTransform* ParallelNode::buildTransform( TreeWalker* walker )
	{
		return walker->createTransform< ParallelTransform >();
	}


	void SelectorNode::setupTransform( TransformType* transform )
	{
		transform->mNodeIter = mChildren.begin();
	}


	void SequenceNode::setupTransform( TransformType* transform )
	{
		transform->mNodeIter = mChildren.begin();
	}


	bool CountDecTransform::onRecvChildState( BTTransform* child , TaskState& state )
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
		mWalker->resume( *this );
		return false;
	}


	bool LoopDecTransform::onRecvChildState( BTTransform* child , TaskState& state )
	{
		++mExecCount;
		if ( mExecCount >= getNode< Node >()->mMaxCount )
		{
			state = TS_SUCCESS;
			return true;
		}
		mWalker->resume( *this );
		return false;
	}

} //namespace ntu