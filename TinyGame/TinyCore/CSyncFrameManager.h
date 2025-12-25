#ifndef CSyncFrameManager_h__
#define CSyncFrameManager_h__

#include "CFrameActionNetEngine.h"
#include "GameControl.h"
#include "GamePlayer.h"
#include "DataStreamBuffer.h"
#include "Holder.h"

#include <vector>
#include <queue>

class INetFrameHelper;
class IFrameActionTemplate;
class INetFrameCollector;
class CSyncFrameManager;

class ServerWorker;
class ClientWorker;
class FramePacket;
class GamePlayer;
class GDPFrameStream;

class FrameDataManager
{
public:
	FrameDataManager();

	void       beginFrame();
	void       endFrame();
	void       addFrameData(int32 frame , DataStreamBuffer& buffer );
	bool       canAdvanceFrame();
	void       setFrame(int32 frame ){ mCurFrame = frame; }
	int32      getFrame(){ return mCurFrame; }

	void       restoreData( IFrameActionTemplate* actionTemp );
	bool       haveFrameData(){ return !mProcessData.empty();  }
	long       getLastDataFrame(){ return mLastDataFrame;  }

	void       clearData()
	{
		mProcessData.clear();
		FrameDataQueue emptyQueue;
		mDataQueue.swap(emptyQueue);
	}

	typedef std::list< DataStreamBuffer > DataList;
	struct FrameData
	{
		int32              frame;
		DataList::iterator iter;
	};
	struct DataCmp
	{
		bool operator()( FrameData const& a , FrameData const& b )
		{
			return a.frame > b.frame;
		}
	};

	DataList       mDataList;
	typedef std::vector< FrameData > FrameDataVec;
	typedef std::priority_queue< FrameData , FrameDataVec , DataCmp > FrameDataQueue;

	FrameDataVec   mProcessData;
	FrameDataQueue mDataQueue;
	int32          mLastDataFrame;
	int32          mCurFrame;
};

class  CSyncFrameManager : public INetFrameManager
	                     , public IActionLauncher
					     , public IActionInput
{
public:
	CSyncFrameManager( IFrameActionTemplate* actionTemp , INetFrameCollector* frameCollector );

	// INetFrameManager
	void  attachTo(ActionProcessor& processor) override;
	ActionProcessor& getCollectionProcessor() override { return mProcessor; }
	int   evalFrame( IFrameUpdater& updater , int updateFrames , int maxDelayFrames ) override;
	void  resetFrameData() override = 0;
	void  release() override = 0;

	virtual void  onPrevEvalFrame(){}
	virtual bool  sendFrameData() = 0;

	// IActionInput - provides synchronized frame data during execution
	bool  scanInput( bool beUpdateFrame ) override;
	bool  checkAction( ActionParam& param ) override;

protected:
	void collectInputs();  // Trigger collection via own processor (launcher pattern)
	
	bool  bClearData;

	// Own processor for collection phase (uses this as launcher)
	ActionProcessor       mProcessor;
	// Reference to main processor (for providing synced input during execution)
	ActionProcessor*      mMainProcessor = nullptr;

	FrameDataManager      mFrameMgr;
	IFrameActionTemplate* mActionTemplate;
	INetFrameCollector*   mFrameCollector;
};

class SVSyncFrameManager : public CSyncFrameManager
	                     , public INetStateListener
{
	typedef CSyncFrameManager BaseClass;
public:
	TINY_API SVSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameCollector* frameCollector );
	TINY_API ~SVSyncFrameManager();

	bool     sendFrameData();
	void     refreshPlayerState();
	


	void     onPlayerStateMsg( unsigned pID , PlayerStateMsg state );
	void     onChangeActionState( NetActionState state );

	void     fireAction( ActionTrigger& trigger );

	void     resetFrameData();
	void     release();

private:
	unsigned calcLocalPlayerBit();
	void     procFrameData( IComPacket* cp);

	TPtrHolder< GDPFrameStream >   mFrameStream;
	unsigned         mCountDataDelay;
	unsigned         mLocalDataBits;
	unsigned         mCheckDataBits;
	unsigned         mUpdateDataBits;

	struct ClientFrameData
	{
		PlayerId id;
		int      recvFrame;
		int      sendFrame;
		DataStreamBuffer buffer;

		ClientFrameData() = default;
		ClientFrameData(ClientFrameData const&) = delete;
		ClientFrameData(ClientFrameData&& rhs)
			:id(rhs.id)
			,recvFrame(rhs.recvFrame)
			,sendFrame(rhs.sendFrame)
			,buffer(std::move(rhs.buffer))
		{

		}
	};
	typedef std::list< ClientFrameData > ClientFrameDataList;
	ClientFrameDataList mFrameDataList;

	ServerWorker*     mWorker;
};

class  CLSyncFrameManager : public CSyncFrameManager
{
public:
	TINY_API CLSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameCollector* frameCollector );
	TINY_API ~CLSyncFrameManager();

	bool sendFrameData();
	void fireAction( ActionTrigger& trigger );
	void resetFrameData();
	void release();

	virtual void  onPrevEvalFrame() override
	{
		mCalcuator.markSystemTime();
	}
private:
	void procFrameData( IComPacket* cp);

	TPtrHolder< GDPFrameStream >  mFrameStream;
	LatencyCalculator mCalcuator;
	long              mLastSendDataFrame;
	long              mLastRecvDataFrame;
	GamePlayer*       mUserPlayer;
	NetWorker*        mWorker;

};

#endif // CSyncFrameManager_h__
