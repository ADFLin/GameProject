#ifndef CSyncFrameManager_h__
#define CSyncFrameManager_h__

#include "CFrameActionNetEngine.h"
#include "GameControl.h"
#include "GamePlayer.h"
#include "DataStreamBuffer.h"
#include "THolder.h"

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
	void       addFrameData( long frame , DataStreamBuffer& buffer );
	bool       checkUpdateFrame();
	void       setFrame( unsigned frame ){ mCurFrame = frame; }
	long       getFrame(){ return mCurFrame; }

	void       restoreData( IFrameActionTemplate* actionTemp );
	bool       haveFrameData(){ return !mProcessData.empty();  }
	long       getLastDataFrame(){ return mLastDataFrame;  }

	typedef std::list< DataStreamBuffer > DataList;
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
	                     , public ActionEnumer
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

	ActionProcessor       mProcessor;
	FrameDataManager      mFrameMgr;
	IFrameActionTemplate* mActionTemplate;
	INetFrameGenerator*   mFrameGenerator;
};

class SVSyncFrameManager : public CSyncFrameManager
	                     , public NetMessageListener
{
	typedef CSyncFrameManager BaseClass;
public:
	GAME_API SVSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator );
	GAME_API ~SVSyncFrameManager();

	bool     sendFrameData();
	void     refreshPlayerState();
	


	void     onPlayerStateMsg( unsigned pID , PlayerStateMsg state );
	void     onChangeActionState( NetActionState state );

	void     fireAction( ActionTrigger& trigger );
	void     release();

private:
	unsigned calcLocalPlayerBit();
	void     procFrameData( IComPacket* cp );

	TPtrHolder< GDPFrameStream >   mFrameStream;
	unsigned         mCountDataDelay;
	unsigned         mLocalDataBit;
	unsigned         mCheckDataBit;
	unsigned         mUpdateDataBit;

	struct ClientFrameData
	{
		PlayerId id;
		int      recvFrame;
		int      sendFrame;
		DataStreamBuffer buffer;
	};
	typedef std::list< ClientFrameData > ClientFrameDataList;
	ClientFrameDataList mFrameDataList;

	ServerWorker*     mWorker;
};

class  CLSyncFrameManager : public CSyncFrameManager
{
public:
	GAME_API CLSyncFrameManager( NetWorker* worker , IFrameActionTemplate* actionTemp , INetFrameGenerator* frameGenerator );
	GAME_API ~CLSyncFrameManager();

	bool sendFrameData();
	void fireAction( ActionTrigger& trigger );
	void release();
private:
	void procFrameData( IComPacket* cp );

	TPtrHolder< GDPFrameStream >  mFrameStream;
	LatencyCalculator mCalcuator;
	long              mLastSendDataFrame;
	long              mLastRecvDataFrame;
	GamePlayer*       mUserPlayer;
	NetWorker*        mWorker;

};

#endif // CSyncFrameManager_h__
