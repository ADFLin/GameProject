#include "TaskBase.h"

#include <algorithm>

void TaskHandler::runTask( long time , unsigned updateMask )
{
	for( TaskList::iterator iter = mRunList.begin() ;
		iter != mRunList.end();  )
	{
		TaskNode& node = *iter;
		if ( !( node.task->mUpdatePolicy & updateMask ) )
		{
			++iter;
			continue;
		}

		if ( !node.task->update( time ) )
		{
			sendMessage( node , TF_STEP_END );
			node.task->onEnd( true );

			TaskBase* task = node.task->mNextTask;

			sendMessage( node , TF_STEP_DESTROY );
			node.task->release();

			node.task = task;

			if ( node.task )
			{
				startNode( node );
			}
			else iter  = mRunList.erase( iter );
		}
		else
		{
			++iter;
		}
	}
}

void TaskHandler::addTask( TaskBase* task , TaskListener* listener  , int taskGroup  )
{
	TaskNode node;
	node.groupId = taskGroup;
	node.listenser = listener;
	node.task = task;

	//new task destroy first
	mRunList.push_front( node );

	TaskBase* t = task;
	while( t )
	{
		if ( t->mUpdatePolicy == TUP_HANDLER_DEFAULT )
			t->mUpdatePolicy = mDefaultPolicy;

		t = t->mNextTask;
	}

	TaskNode& newNode = mRunList.front();
	startNode( newNode );

}

void TaskHandler::clearTask()
{
	for ( TaskList::iterator iter = mRunList.begin();
		iter != mRunList.end() ; ++iter )
	{
		TaskNode& node = *iter;

		sendMessage( node , TF_STEP_END | TF_NOT_COMPLETE );
		node.task->onEnd( false );

		sendMessage( node , TF_STEP_DESTROY );
		node.task->release();
	}
	mRunList.clear();
}


void TaskHandler::removeTask( int id  )
{
	TaskList::iterator start = mRunList.begin();

	while ( 1 )
	{
		TaskList::iterator iter = std::find_if( 
			start , mRunList.end() , 
			[id](TaskNode const& node) -> bool { return node.groupId == id; });

		if ( iter == mRunList.end() )
			break;

		start = iter;
		++start;

		removeTaskInternal( iter , true );
	}
}

bool TaskHandler::removeTaskInternal( TaskList::iterator  iter , bool beAll  )
{
	if ( iter == mRunList.end() )
		return false;

	TaskNode& node = *iter;
	sendMessage( node , TF_STEP_END | TF_NOT_COMPLETE );
	node.task->onEnd( false);

	TaskBase* task = node.task->mNextTask;

	sendMessage( node , TF_STEP_DESTROY );
	node.task->release();

	node.task = task;

	if ( beAll || node.task == NULL )
	{
		while ( node.task )
		{
			task = node.task->mNextTask;
			sendMessage( node , TF_STEP_DESTROY | TF_NEVER_START );
			node.task->release();

			node.task = task;
		}

		mRunList.erase( iter );
	}
	else
	{
		startNode( node );
	}

	return true;
}

bool TaskHandler::removeTask( TaskBase* task , bool beAll )
{
	TaskList::iterator iter = std::find_if(
		mRunList.begin() , mRunList.end() , 
		[task](TaskNode const& node)-> bool { return node.task == task; });
	return removeTaskInternal( iter , beAll );
}

void TaskHandler::sendMessage( TaskNode const& node , unsigned flag )
{
	if ( node.task->mNextTask )
		flag |= TF_HAVE_NEXT_TASK;

	TaskMsg msg( node.groupId , flag );
	if ( node.listenser )
		node.listenser->onTaskMessage( node.task , msg );
}

void TaskHandler::startNode( TaskNode& node )
{
	node.task->mHandler = this;
	sendMessage( node , TF_STEP_START );
	node.task->onStart();
}

TaskBase::TaskBase() 
	:mNextTask( NULL )
	,mHandler( NULL )
	,mUpdatePolicy( TUP_HANDLER_DEFAULT )
{

}


bool TaskBase::update( long time )
{
	return onUpdate( time );
}

TaskBase* TaskBase::setNextTask( TaskBase* task )
{
	assert ( this != NULL );
	mNextTask = task;
	return this;
}

bool ParallelTask::onUpdate( long time )
{
	runTask( time );
	return mRunList.empty();
}
