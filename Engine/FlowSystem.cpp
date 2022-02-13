#include "FlowSystem.h"



FlowExecuteResult FlowNodeSequence::execEnter(FlowExecuteContext& context)
{
	if( mNodes.empty() )
		return FlowExecuteResult::Sucess;
	return execNextChild(context, 0);
}

FlowExecuteResult FlowNodeSequence::execExitChild(FlowExecuteContext& context, FlowNodeBase* child, FlowExecuteResult result)
{
	if( result == FlowExecuteResult::Fail )
		return FlowExecuteResult::Fail;
	return execNextChild(context, context.getInstanceDataT<int32>() + 1);
}

FlowExecuteResult FlowNodeSequence::execNextChild(FlowExecuteContext& context, int idxStart)
{
	int& indexExecuting = context.getInstanceDataT<int32>();
	indexExecuting = nextFlow(idxStart);
	if( indexExecuting == INDEX_NONE )
	{
		return FlowExecuteResult::Sucess;
	}
	context.pushFlow(mNodes[indexExecuting].get());
	return FlowExecuteResult::EnterChild;
}



uint32 FlowNodeSequence::getInstanceDataSize()
{
	return sizeof(int32);
}

void FlowNodeSequence::initInstanceData(void* data)
{
	*static_cast<int32*>(data) = 0;
}

int FlowNodeSequence::nextFlow(int indexStart /*= 0*/)
{
	for( int i = indexStart; i < mNodes.size(); ++i )
	{
		if( mNodes[i] )
			return i;
	}
	return INDEX_NONE;
}

struct FlowNodeParallelPayload
{
	std::vector< FlowExecuteContext > contexts;
};

FlowExecuteResult FlowNodeParallel::execEnter(FlowExecuteContext& context)
{
	if( mNodes.empty() )
		return FlowExecuteResult::Sucess;

	FlowNodeParallelPayload& payload = context.getInstanceDataT< FlowNodeParallelPayload >();
	payload.contexts.resize(mNodes.size());

	int idx = 0;
	for( int i = 0; i < mNodes.size(); ++i )
	{
		FlowNodeBase* node = mNodes[i].get();
		if( node )
		{
			payload.contexts[i].init(node);
			if( payload.contexts[idx].resolveFlowResult() )
			{
				++idx;
			}
		}
	}

	if( idx != payload.contexts.size() )
	{
		payload.contexts.resize(idx);
	}

	if( !payload.contexts.empty() )
	{
		return FlowExecuteResult::Keep;
	}

	return FlowExecuteResult::Sucess;
}

uint32 FlowNodeParallel::getInstanceDataSize()
{
	return sizeof(FlowNodeParallelPayload);
}

void FlowNodeParallel::initInstanceData(void* data)
{
	new (data) FlowNodeParallelPayload;
}

void FlowNodeParallel::releaseInatanceData(void* data)
{
	FlowNodeParallelPayload* payload = static_cast<FlowNodeParallelPayload*>(data);
	payload->~FlowNodeParallelPayload();
}

void FlowExecuteContext::pushFlow(FlowNodeBase* node)
{
	NodeStackData data;
	data.node = node;
	uint32 instanceDataSize = data.node->getInstanceDataSize();
	if( instanceDataSize )
	{
		data.instanceData = allocInstanceData(instanceDataSize);
		data.node->initInstanceData(data.instanceData);
	}
	mStacks.push_back(data);
}

void FlowExecuteContext::popFlow()
{
	assert(!mStacks.empty());
	NodeStackData& data = mStacks.back();
	if( data.instanceData )
	{
		data.node->releaseInatanceData(data.instanceData);
		freeInstanceData(data.instanceData);
	}
	mStacks.pop_back();
}

bool FlowExecuteContext::resolveFlowResult()
{
	for( ;;)
	{
		switch( mLastResult )
		{
		case FlowExecuteResult::Keep:
			return true;
		case FlowExecuteResult::Sucess:
		case FlowExecuteResult::Fail:
			{
				FlowNodeBase* childNode = mStacks.back().node;
				childNode->execExit(*this);
				popFlow();
				if( mStacks.empty() )
					return false;
				mLastResult = mStacks.back().node->execExitChild(*this, childNode, mLastResult);
			}
			break;
		case FlowExecuteResult::EnterChild:
			{
				NodeStackData& data = mStacks.back();
				mLastResult = data.node->execEnter(*this);
			}
			break;
		}
	}
}
