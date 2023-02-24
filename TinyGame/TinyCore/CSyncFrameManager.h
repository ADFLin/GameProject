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
	                     , public IActionLanucher
					     , public IActionInput
{
public:
	CSyncFrameManager( IFrameActionTemplate* actionTemp , INetFrameCollector* frameCollector );
	//NetFrameManager
	int   evalFrame( IFrameUpdater& updater , int updateFrames , int maxDelayFrames );
	void  setupInput(ActionProcessor& processor)
	{
		processor.addInput(*this);
	}
	ActionProcessor& getActionProcessor(){ return mProcessor; }
	virtual bool  sendFrameData() = 0;
	//ActionInpput
	bool  scanInput( bool beUpdateFrame );
	bool  checkAction( ActionParam& param );

protected:

	bool  bClearData;

	ActionProcessor       mProcessor;
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
