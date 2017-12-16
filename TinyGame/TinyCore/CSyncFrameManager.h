#ifndef CSyncFrameManager_h__
#define CSyncFrameManager_h__

#include "CFrameActionNetEngine.h"
#include "GameControl.h"
#include "GamePlayer.h"
#include "DataSteamBuffer.h"
#include "Holder.h"

#include <vector>
#include <queue>

class INetFrameHelper;
class IFrameActionTemplate;
class INetFrameGenerator;
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
	void       addFrameData( long frame , DataSteamBuffer& buffer );
	bool       canAdvanceFrame();
	void       setFrame( unsigned frame ){ mCurFrame = frame; }
	long       getFrame(){ return mCurFrame; }

	void       restoreData( IFrameActionTemplate* actionTemp );
	bool       haveFrameData(){ return !mProcessData.empty();  }
	long       getLastDataFrame(){ return mLastDataFrame;  }

	void       clearData()
	{
		mProcessData.clear();
		FrameDataQueue emptyQueue;
		mDataQueue.swap(emptyQueue);
	}

	typedef std::list< DataSteamBuffer > DataList;
	struct FrameData
	{
		long               frame;
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
	long           mLastDataFrame;
	long           mCurFrame;
};

class  CSyncFrameManager : public INetFrameManager
	                     , public IActionLanucher
{
public:
	CSyncFrameManager( IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator );
	//NetFrameManager
	int   evalFrame( IFrameUpdater& updater , int updateFrames , int maxDelayFrames );
	ActionProcessor& getActionProcessor(){ return mProcessor; }
	bool  sendFrameData() = 0;

	bool  scanInput( bool beUpdateFrame );
	bool  checkAction( ActionParam& param );

protected:

	bool  bClearData;

	ActionProcessor       mProcessor;
	FrameDataManager      mFrameMgr;
	IFrameActionTemplate* mActionTemplate;
	INetFrameGenerator*   mFrameGenerator;
};

class SVSyncFrameManager : public CSyncFrameManager
	                     , public INetStateListener
{
	typedef CSyncFrameManager BaseClass;
public:
	TINY_API SVSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator );
	TINY_API ~SVSyncFrameManager();

	bool     sendFrameData();
	void     refreshPlayerState();
	


	void     onPlayerStateMsg( unsigned pID , PlayerStateMsg state );
	void     onChangeActionState( NetActionState state );

	void     fireAction( ActionTrigger& trigger );

	void      resetFrameData();
	void     release();

private:
	unsigned calcLocalPlayerBit();
	void     procFrameData( IComPacket* cp);

	TPtrHolder< GDPFrameStream >   mFrameStream;
	unsigned         mCountDataDelay;
	unsigned         mLocalDataBits;
	unsigned         mCheckDataBits;
	unsigned         mUpdateDataBit;

	struct ClientFrameData
	{
		PlayerId id;
		int      recvFrame;
		int      sendFrame;
		DataSteamBuffer buffer;
	};
	typedef std::list< ClientFrameData > ClientFrameDataList;
	ClientFrameDataList mFrameDataList;

	ServerWorker*     mWorker;
};

class  CLSyncFrameManager : public CSyncFrameManager
{
public:
	TINY_API CLSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator );
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
