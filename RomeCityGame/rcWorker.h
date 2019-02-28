#ifndef rcWorker_h__
#define rcWorker_h__

#include "rcBase.h"

#include "DataStructure/TLinkable.h"
#include "DataStructure/TTable.h"


class rcLevelMap;
class rcPathFinder;
class rcPath;
class rcMission;
class rcBuilding;
class rcWorkerManager;
class rcBuildingManager;

enum WorkerType
{
	WT_DELIVERYMAN ,
};

#define RC_ERROR_WORKER_ID ((unsigned)-1)


enum
{
	MM_LOSE_BUILDING ,
	MM_LOSE_ROAD     ,
};


int const g_MapTileLength = 64;
Vec2i const g_DirOffset8[] =
{
	Vec2i( 1 , 0 ) , Vec2i( 1 , 1 ) , Vec2i( 0 , 1 ) , Vec2i( -1 , 1 ) , 
	Vec2i( -1 , 0 ) , Vec2i( -1 , -1 ) , Vec2i( 0 , -1 ) , Vec2i( 1 , -1 ) ,
};
class rcWorker : public TLinkable< rcWorker >
{
public:
	rcWorker();
	~rcWorker();

	unsigned     getType() const { return mType; }
	void         update( long time );

	void         setMapPos( Vec2i const& mapPos );
	Vec2i const& getMapPos()  { return mMapPos; }

	//void         setWorldPos( Vec2i const& wPos );
	//Vec2i const& getWorldPos(){ return mMapPos; }

	void          setDir( rcDirection& dir ){ mDir = dir; }
	void          moveNextTile();

	rcMission*    getMission(){ return mCurMission; }
	void          changeMission( rcMission* mission );
	bool          restartMission();
	void          finishMission();
	void          sendMissionException( unsigned content );
	
	void          setOwner( unsigned bID ){ mOwner = bID; }
	unsigned      getOwner(){ return mOwner; }
	rcWorkerManager* getManager();

	float       getMapOffset() const { return mOffset; }
	void        setMapOffset(float offset) { mOffset = offset; }
	rcDirection const&  getDir(){ return mDir; }

	struct TransportInfo
	{
		unsigned pTag : 8;
		unsigned num  : 24;
	};

	struct SettlerInfo
	{
		short num;
		int   type;
	};
	struct BuildingVar
	{
		union
		{
			unsigned      serviceTag;
			TransportInfo infoTP;
			SettlerInfo   infoSettler; 
			int           numSettler;
		};
	};
	BuildingVar& getBuildingVar(){ return mBuildingVar; }

	enum 
	{
		eEnableMission = BIT(1),
		eVisible       = BIT(3),
		eReturnHome    = BIT(2),
	};

	bool checkFlag ( unsigned bit ) {  return ( mFlag & bit ) != 0;  }
	void addFlag   ( unsigned bit ) { mFlag |=  bit; }
	void removeFlag( unsigned bit ) { mFlag &= ~bit; }

protected:

	void  destoryCurMission();
	friend class rcWorkerManager;
	friend class rcLevelScene;

	rcMission*  mCurMission;

	rcDirection mDir;
	float       mOffset;
	Vec2i       mMapPos;

	unsigned    mType;
	unsigned    mManageID;
	unsigned    mFlag;
	unsigned    mLastExceptionContent;

	int         mHealth;

	unsigned    mOwner;
	////
	friend class rcWorkerBuilding;
	int         mSlotPos; //use in building
	BuildingVar mBuildingVar;


};

class rcMission
{
public:
	rcMission():mWorker(NULL){}
	virtual ~rcMission(){}
	rcWorker* getWorker(){ return mWorker; }
protected:
	virtual void onAbort(){}
	virtual bool onStart() = 0;
	virtual void onUpdate( long time ){}


private:
	friend class rcWorker;
	rcWorker* mWorker;
};

#include "rcPathFinder.h"

class Navigator
{
public:
	Navigator():mWorker(NULL){}
	void     setWorker( rcWorker* worker );
	typedef fastdelegate::FastDelegate< void ( rcWorker*) > CallBackFun;
	CallBackFun onTile;
protected:
	float     mDistNextRoute;
	float     mDistNextTile;
	float     mDistMove;
	float     mSpeed;
	rcWorker* mWorker;
};

class PathNavigator : public Navigator
{
public:
	PathNavigator():mPath(NULL){}

	bool          startRouting( rcWorker* worker , rcPath* path );
	bool          updateMoving( long time );
	rcWorker*     getWorker(){ return mWorker; }
	rcPath*       getPath() const { return mPath; }
	Vec2i const&  getNextRotePos() const { return mNextRoutePos; }

	static int calcDirection( Vec2i const& v );
private:
	bool      refreshRoute();
	rcPath*   mPath;
	bool      mbIsometric;

	rcPath::Cookie mCookie;
	Vec2i     mNextRoutePos;

};

class RandMoveNavigator : public Navigator
{
public:
	bool   startRouting( rcWorker* worker );
	bool   updateMoving( long time );
	float  getTotalMoveDistance() const { return mTotalDistMove; }
private:
	bool   refreshRoute();
	int    evalNextDir();

	float     mTotalDistMove;
	int       mCurDir;
};

class RandomMoveMission : public rcMission
{
public:
	RandomMoveMission( float dist )
		:mMaxDist( dist ){}
	void onAbort(){}

	bool onStart();
	void onUpdate( long time );
	enum
	{
		MS_ROTING ,
		MS_GO_HOME ,

		MS_NEXT_STATE ,
	};

	template< class T , class Q >
	void  setOnTileCallback( T* ptr , void (Q::*fun)( rcWorker* ) )
	{
		mRandNavigator.onTile.bind( ptr , fun );
		mPathNavigator.onTile.bind( ptr , fun );
	}


protected:
	RandMoveNavigator mRandNavigator;
	PathNavigator     mPathNavigator;
	int    mState;
	float  mMaxDist;
};


class DestionMoveMission : public rcMission
{
public:
	DestionMoveMission( Vec2i const& pos );
	DestionMoveMission( rcPath* path );
	~DestionMoveMission();

	void onAbort(){}
	bool onStart();
	void onUpdate( long time );

protected:
	PathNavigator mNavigator;
	Vec2i         mGoalPos;
	rcPath*       mPath;
};


class TransportProductMission : public rcMission
{
public:
	TransportProductMission( unsigned blgID , bool beSend = true )
		:mBuildingID( blgID ), mbeSend( true ){}

	~TransportProductMission();

	void onAbort(){}
	bool onStart();
	void onUpdate( long time );
	enum
	{
		MS_ROTING_DEST ,
		MS_UNLOAD  ,
		MS_GO_HOME ,
	};


	bool       mbeSend;

	PathNavigator mRouter;
	int        mCount;
	int        mState;
	unsigned   mBuildingID;
};


enum WorkerMsg;



#endif // rcWorker_h__