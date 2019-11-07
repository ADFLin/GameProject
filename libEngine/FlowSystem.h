#pragma once
#ifndef FlowSystem_H_EA29D857_5728_49E1_88F6_999CBE31DB15
#define FlowSystem_H_EA29D857_5728_49E1_88F6_999CBE31DB15

#include "Meta/MetaTypeList.h"
#include "Core/IntegerType.h"

#include <cassert>
#include <memory>
#include <vector>

class FlowNodeBase;
class FlowExecuteContext;

#define INDEX_NONE (-1)
enum FlowEvent
{
	FEVT_INIT  ,
	FEVT_ACTIVE ,
	FEVT_POST_ACTIVE ,
};

enum class FlowExecuteResult
{
	Keep ,
	Sucess ,
	Fail ,
	EnterChild ,
};

class FlowExecuteContext
{
public:

	void* getInstanceData()
	{
		assert(!mStacks.empty());
		return mStacks.back().instanceData;
	}

	template< class T >
	T&  getInstanceDataT() { return *static_cast<T*>(getInstanceData()); }


	void init(FlowNodeBase* node)
	{

	}

	void pushFlow(FlowNodeBase* node);

	void popFlow();

	bool resolveFlowResult();
	void* allocInstanceData(uint32 size)
	{
		return nullptr;
	}
	void  freeInstanceData(void* data)
	{

	}
	struct NodeStackData
	{
		FlowNodeBase* node;
		void* instanceData;
	};

	FlowExecuteResult   mLastResult;
	std::vector< NodeStackData > mStacks;
	std::vector< uint8 > mNodeData;
};

class FlowNodeBase
{
public:
	virtual ~FlowNodeBase(){}
	virtual FlowExecuteResult execEnter(FlowExecuteContext& context) { return FlowExecuteResult::Sucess; }
	virtual FlowExecuteResult execUpdate(FlowExecuteContext& context , float deltaTime ) { return FlowExecuteResult::Sucess; }
	virtual FlowExecuteResult execExitChild(FlowExecuteContext& context, FlowNodeBase* child , FlowExecuteResult result ) { return FlowExecuteResult::Sucess; }
	virtual void   execExit(FlowExecuteContext& context){}
	virtual uint32 getInstanceDataSize() { return 0; }
	virtual void   initInstanceData( void* data ){}
	virtual void   releaseInatanceData( void* data ){}
};


typedef std::shared_ptr< FlowNodeBase > FlowNodePtr;

class FlowNodeArrayComposite : public FlowNodeBase
{
protected:
	std::vector< FlowNodePtr > mNodes;
};

class FlowNodeSequence : public FlowNodeArrayComposite
{
public:
	virtual FlowExecuteResult execEnter(FlowExecuteContext& context) override;
	virtual FlowExecuteResult execExitChild(FlowExecuteContext& context , FlowNodeBase* child , FlowExecuteResult result ) override;

	FlowExecuteResult execNextChild(FlowExecuteContext& context, int idxStart);

	virtual uint32 getInstanceDataSize();
	virtual void   initInstanceData(void* data);

	int nextFlow(int indexStart = 0);
};

class FlowNodeSelect : public FlowNodeArrayComposite
{



};

class FlowNodeParallel : public FlowNodeArrayComposite
{
public:
	virtual FlowExecuteResult execEnter(FlowExecuteContext& context) override;


	virtual uint32 getInstanceDataSize() override;
	virtual void initInstanceData(void* data) override;
	virtual void releaseInatanceData(void* data) override;

};

class FlowNodeBranch
{





};

class FlowManager
{





};




enum FlowConntectType
{
	FCT_ANY   ,
	FCT_INT   ,
	FCT_FLOAT ,
	FCT_VEC3D ,
};

struct FlowInputInfo
{

};

struct FlowOutputInfo
{


};
enum
{
	
};

class FlowGraph
{
public:








};

#endif // FlowSystem_H_EA29D857_5728_49E1_88F6_999CBE31DB15