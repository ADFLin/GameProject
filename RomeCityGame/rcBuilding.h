#ifndef rcBuilding_h__
#define rcBuilding_h__

#include "rcBase.h"


struct ModelInfo;
class  rcCityInfo;
class  rcWorker;
class  rcLevelMap;

class rcBuildingManager;

enum BuildingClass
{
	BGC_UNDEFINE = 0 ,
	BGC_BASIC  ,
	BGC_HOUSE  ,
	BGC_STORAGE  ,
	BGC_SERVICE  ,
	BGC_MARKET   ,
	BGC_FARM     ,
	BGC_ROW_FACTORY  ,
	BGC_WORKSHOP ,
};

struct rcBuildingInfo
{
	unsigned      typeTag;
	BuildingClass classGroup;
	short         numMaxPeople;
	unsigned      startExtraModel;
	short         numExtraModel;

	unsigned      classTypeBit;
};

extern rcBuildingInfo g_buildingInfo[];


struct RenderCoord
{
	float xScan;
	float yScan;
	Vec2i screen;
	Vec2i map;
};


enum BuildingState
{
	BS_UNKNOWN     ,
	BS_CLOSE       ,
	BS_NEED_WORKER ,
	BS_NEED_ROAD   ,
	BS_WORKING     ,
	BS_IDLE        ,
};

enum WorkerMsg
{
	WMT_FINISH ,
	WMT_FAIL  ,
	WMT_ABORT ,
	WMT_EXCEPTION ,
};


class rcBuildingManager;


enum SecurityType
{
	SET_FIRE ,
	SET_COLLAPSE ,
};



class rcBuilding
{

#define DEFINE_BUILING_CLASS( CLASS , BASE , CLASS_ID )\
private:\
	typedef CLASS ThisClass;\
	typedef BASE  BaseClass;\
public:\
	static unsigned getClassGroup(){ return BIT( CLASS_ID ) | BaseClass::getClassGroup(); }\

public:
	rcBuilding();

	static unsigned getClassGroup(){ return 0; }

	void          update( rcCityInfo& cInfo , long time );
	BuildingState getState(){ return mState; }
	unsigned      getManageID(){ return mManageID; }
	unsigned      getTypeTag(){ return mTagType; }

	Vec2i const& getSize();
	Vec2i const& getMapPos() const { return mPos; }

 
	void         setModelID( unsigned id );

	ModelInfo const& getModel();

	enum
	{
		eSwapAxis     = BIT(0),
		eHaveWorker   = BIT(1),
		eHaveStuff    = BIT(2),
		eNoNeedRoad   = BIT(3),
		eAccessRoad   = BIT(4),
		eForceClose   = BIT(5),
		eNoFire       = BIT(6),
		eNoCollapse   = BIT(7),
		eSingleTile   = BIT(8),

		/////////////////////////
		eRemveUpdateWorker = BIT(11),
		/////////////////////////////

		eWaterSource  = BIT(12),
		eWaterFilled  = BIT(13),
		eCheckWaterSupport = BIT(15),

		/////////////////////////
		eWaterSupport = BIT(22),
		eFoodSupport  = BIT(23),
		eEntSupport   = BIT(24),
		eGodSupport   = BIT(25),

	};

	void updateState();

	bool checkFlag ( unsigned bit ) {  return ( mFlag & bit ) != 0;  }
	void addFlag   ( unsigned bit ) { mFlag |=  bit; mState = BS_UNKNOWN; }
	void removeFlag( unsigned bit ) { mFlag &= ~bit; mState = BS_UNKNOWN; }
	void changeFlag( unsigned bit , bool beM ) 
	{
		if ( beM ) addFlag( bit );
		else removeFlag( bit );
	}

	void         setEntryPos( Vec2i const& pos ){ mEntryPos = pos; }
	Vec2i const& getEntryPos() const { return mEntryPos; }
	int          getPeopleNum() const { return mNumPeople; }
	
	virtual bool updateEntryPos( rcLevelMap& levelMap );
	virtual int  getFreeProductStorage( unsigned pTag ){ return 0; }
	virtual bool recvGoods( unsigned pTag , int num ){ return false; }
	virtual int  takeGoods( unsigned pTag , int num , bool testEnough ){ return 0; }
public:
	virtual void _OnWorkerMsg( rcWorker* worker , WorkerMsg msg , unsigned content ){}
protected:
	virtual void onCreate( rcCityInfo& cInfo ){}
	virtual void onDestroy( rcCityInfo& cInfo ){}
	virtual void onUpdate( rcCityInfo& cInfo , long time ){}
	virtual void doUpdateState( ){}

	friend  class rcBuildingManager;

	int           mSecurityValue[3];

	BuildingState mState;
	unsigned      mModelID;
	unsigned      mFlag;

	Vec2i         mPos;
	Vec2i         mEntryPos;

	unsigned      mTagType;
	unsigned      mManageID;
	int           walkerState;
	int           mNumPeople;

private:
	Vec2i         mSize;
public:
	template < class T >
	T* downCast()
	{
		if ( BIT( getBuildingInfo().classGroup ) & T::getClassGroup() )
			return static_cast< T* >( this );

		return NULL;
	}


	rcBuildingInfo& getBuildingInfo()
	{  
		return g_buildingInfo[mTagType]; 
	}

	struct RenderData
	{
		RenderCoord coord;
		unsigned    updatePosCount;
		RenderData()
		{
			updatePosCount = -1;
		}
	};

	RenderData&  getRenderData(){ return mRenderData;  }
private:
	RenderData  mRenderData;
};




class rcBasicBuilding : public rcBuilding
{
	DEFINE_BUILING_CLASS( rcBasicBuilding , rcBuilding , BGC_BASIC )

public:
	virtual void onCreate( rcCityInfo& cInfo );
	virtual void onDestory( rcCityInfo& cInfo );
	virtual void doUpdateState();
private:
	void setResviorWaterWay( rcLevelMap& levelMap , bool beE );
	void supportWater( rcCityInfo& cInfo , bool beSupport , int range );
};


class rcWorkerBuilding : public rcBuilding
{
	typedef rcBuilding BaseClass;
public:
	rcWorkerBuilding()
		:mNumWorker(0){}

	void  onFindLabor( rcWorker* worker );
	void  onService  ( rcWorker* worker );


protected:
	void createLaborFindingWorker( rcCityInfo& cInfo );
	enum
	{
		FIND_HOUSE_SLOT ,
		NEXT_SLOT ,
	};

	enum WorkerState
	{
		WS_IDLE  ,
		WS_RUN_MISSION  ,
		WS_REMOVE ,
		WS_DIE ,
	};

	struct WorkerSlot
	{
		int         id;
		rcWorker*   worker;
		WorkerState state;
		long        time;
		union
		{
			rcBuilding* destBuilding;
		};

		WorkerSlot()
			:worker(NULL)
			,state (WS_IDLE)
			,time(0){}
	};

	virtual void  onDestroy( rcCityInfo& cInfo );
	virtual void  onUpdate( rcCityInfo& cInfo , long time );
	virtual void  onUpdateWorker( WorkerSlot& slot , long time ){}
	virtual void  onWorkerMsg   ( WorkerSlot& slot , WorkerMsg msg , unsigned content ){}

	WorkerSlot* createWorker( rcCityInfo& cInfo , int slotID , unsigned workerTag );
	bool        changeWorkerState( WorkerSlot& slot , WorkerState state );
	void        _OnWorkerMsg( rcWorker* worker , WorkerMsg msg , unsigned content );

private:
	void        removeWorker( rcCityInfo& cInfo , WorkerSlot& slot );
	void        updateWorker( rcCityInfo& cInfo , long time );
	static int const MaxWorkerSlotNum = 5;
	WorkerSlot mSlot[ MaxWorkerSlotNum ];
	int        mNumWorker;
};


enum ServiceTag
{
	ST_SEC_FIRE ,

	ST_WATER_SOURCE ,
	ST_WATER_FOUNTAIN ,

	ST_MARKET_SERVICE_START ,
	ST_FOOD_WHEAT = ST_MARKET_SERVICE_START,
	ST_FOOD_MEAT  ,
	ST_FOOD_FISH  ,

	ST_PT_POTTERY ,
	ST_PT_FURNLTRUE ,
	ST_PT_WINE ,
	ST_PT_OID  ,
	ST_MARKET_SERVICE_END = ST_PT_OID ,

	ST_EDU_SCHOOL ,
	ST_EDU_LIBRARY ,

	ST_GOD_1 ,
	ST_GOD_2 ,
	ST_GOD_3 ,
	ST_GOD_4 ,
	ST_GOD_5 ,

	ST_ENT_THEATRE ,
	ST_ENT_1 ,
	ST_ENT_2 ,
	ST_ENT_3 ,
	ST_ENT_4 ,
	ST_ENT_5 ,

	NumHouseService ,
};

enum LevelRequest
{
	LR_00_ROAD_ACCESS = 0,
	LR_01_PEOPLE   , 
	LR_02_WATER_SOURCE , //b
	LR_03_ONE_FOOD ,
	LR_04_ONE_GOD_ACCESS ,
	LR_05_WATER_FOUNTAIN ,
	LR_06_1ST_ENTERTAINMENT ,
	LR_07_BASIC_EDUCATION , 
	LR_08_BATH_HOUSE_AND_POTTERY ,
	LR_09_2ND_ENTERTAINMENT ,
	LR_10_DOCTOR_AND_FURNITURE ,
	LR_11_SCHOOL_AND_LIBRARY ,
	LR_12_3RD_ENTERTAINMENT ,
	LR_13_WINE_AND_TWO_GODS ,
	LR_14_BATH_HOUSE_AND_DUCTOR_AND_HOSPITAL ,
	LR_15_ACADEMY ,
	LR_16_THREE_FOOD_TYPE_AND_THREE_GODS ,
	LR_17_2ND_WINE ,
	LR_18_FOUR_GODS ,
	LR_19_4TH_EENTERTAINMENT ,
	LR_20_HIGHT_DESIRABILITY ,

};
class rcHouse : public rcWorkerBuilding
{
	DEFINE_BUILING_CLASS( rcHouse , rcWorkerBuilding , BGC_HOUSE )
public:
	rcHouse();

	void onCreate( rcCityInfo& cInfo );
	int  getEmptySpaceNum()
	{
		return mMaxLivePeopleNum - mNumPeople - mNumPeopleComing;
	}

	bool  addSettler( rcCityInfo& cInfo , Vec2i const& mapPos , int numPepole );
	void  removeResident( int numPepole )
	{

	}

	virtual void onUpdate( rcCityInfo& cInfo , long time );
	virtual void onWorkerMsg( WorkerSlot& slot , WorkerMsg msg , unsigned content );
	virtual bool updateEntryPos( rcLevelMap& levelMap );

	int  getServiceValue( unsigned serviceTag ){ return mServiceValue[ serviceTag ]; }
protected:

	unsigned checkHouseLevel( rcCityInfo& cInfo );
	void     updateHouseLevel( rcCityInfo& cInfo );
	void     updateService( rcLevelMap& levelMap , long time );
	enum
	{
		SETTLER_SLOT = BaseClass::NEXT_SLOT ,	
		NEXT_SLOT ,
	};


	int mMaxLivePeopleNum;
	int mNumPeopleComing;

	int mServiceValue[ NumHouseService ];
	int mTimeService [ NumHouseService ];

	struct MarketServiceInfo
	{
		rcBuilding* market;
		long        time;
	};
	static int const MaxHistoryNum = 4;
	MarketServiceInfo  mServiceHistory[ MaxHistoryNum ];
	
	//int mNumFoodType;
	//int mNumGoodType;
	//int mNumEntType;

	int level;
};

class rcServiceBuilding : public rcWorkerBuilding
{
	DEFINE_BUILING_CLASS( rcServiceBuilding , rcWorkerBuilding , BGC_SERVICE )
public:
	virtual void onCreate( rcCityInfo& cInfo );
	enum
	{
		SERVICE_SLOT = BaseClass::NEXT_SLOT ,
		NEXT_SLOT ,
	};

	ServiceTag mServiceType;
};

class rcMarket : public rcServiceBuilding
{
	DEFINE_BUILING_CLASS( rcMarket , rcServiceBuilding , BGC_MARKET )
public:

	rcMarket();

	enum
	{
		TAKE_GOODS_SLOT = BaseClass::NEXT_SLOT ,
		NEXT_SLOT ,
	};

	void onMarketService( rcWorker* worker );
	
	virtual void onCreate( rcCityInfo& cInfo );
	virtual void onUpdateWorker( WorkerSlot& slot , long time );

	
	static int const numSerivce = ST_MARKET_SERVICE_END - ST_MARKET_SERVICE_START + 1 ;
	
	unsigned mProvideServiceBit;
	unsigned mTakeServiceBit;
	int mCurTakeIdx;
	int mNextProvideIdx;
	int mPruductSave[ numSerivce ];


};


struct rcStuffInfo
{
	int productID;
	int storage;
	int cast;
};


int const MaxNumStuff = 4;
struct rcFactoryInfo
{
	int productID;
	int produceSpeed;
	int NumProduct;
	int maxProductStorage;

	int numStuff;
	rcStuffInfo stuff[ MaxNumStuff ];
};

rcFactoryInfo gFactoryInfo[];


class rcFartory : public rcWorkerBuilding
{
	typedef rcWorkerBuilding BaseClass;
public:
	rcFartory()
		:mNumProduct( 0 )
		,mProgressProduct( 0 )
	{}


protected:
	static int const MaxProgressFinish = 100000;
	virtual void onCreate( rcCityInfo& cInfo );
	virtual void onUpdate( rcCityInfo& cInfo ,long time );
	virtual void doUpdateState();

protected:
 	void   produceGoods( long time );
	rcFactoryInfo& getFactoryInfo();
	void   onUpdateWorker( WorkerSlot& slot , long time );
	void   onWorkerMsg( WorkerSlot& slot , WorkerMsg msg , unsigned content );
	enum
	{
		DELMAN_SLOT = BaseClass::NEXT_SLOT ,

		NEXT_SLOT ,
	};
	int mProgressProduct;
	int mNumProduct;

	friend class rcLevelScene;
};


class rcFarm : public rcFartory
{
	DEFINE_BUILING_CLASS( rcFarm , rcFartory , BGC_FARM )
public:
	void onUpdate( rcCityInfo& cInfo , long time )
	{
		BaseClass::onUpdate( cInfo , time );
	}
};

class rcRowFactory : public rcFartory
{
	DEFINE_BUILING_CLASS( rcRowFactory , rcFartory , BGC_ROW_FACTORY )
public:
	void onCreate( rcCityInfo& cInfo )
	{
		BaseClass::onCreate( cInfo );
	}
	void onUpdate( rcCityInfo& cInfo , long time )
	{
		BaseClass::onUpdate( cInfo , time );
	}

};

class rcWorkShop : public rcFartory
{
	DEFINE_BUILING_CLASS( rcWorkShop , rcFartory , BGC_WORKSHOP )
public:
	rcWorkShop()
	{
		std::fill_n( mNumStuff , MaxNumStuff , 0 );
	}

	int getFreeProductStorage( unsigned pTag );
protected:
	bool recvGoods( unsigned pTag , int num );


	int  getStuffIndex( unsigned pTag );
	virtual void onCreate( rcCityInfo& cInfo );
	virtual void onUpdate( rcCityInfo& cInfo , long time );
	virtual void doUpdateState();
	bool tryCastSuff( unsigned& pTagLost );

	int mNumStuff[ MaxNumStuff ];

};

class rcStorageHouse : public rcWorkerBuilding
{
	DEFINE_BUILING_CLASS( rcStorageHouse , rcWorkerBuilding , BGC_STORAGE )
public:

	rcStorageHouse()
		:mWantProductBit(0)
		,mRemoveProductBit(0)
	{

	}

	enum Rule
	{
		SR_WANT ,
		SR_STOP ,
		SR_ALLOW ,
		SR_REMOVED ,
	};

	int  getFreeProductStorage( unsigned pTag );
	void setupGoodsRule( unsigned pTagBit , Rule rule );
	bool recvGoods( unsigned pTag , int num );
	int  takeGoods( unsigned pTag , int num , bool testEnough );
	static int const NumStageCell = 8;

	int  findPorductCell( unsigned pTag );
	virtual void onCreate( rcCityInfo& cInfo );
	virtual void onDestroy( rcCityInfo& cInfo );

	virtual bool updateEntryPos( rcLevelMap& levelMap );

	static unsigned const WareHouseProductBit;
	static unsigned const FoodProductBit;
	static unsigned const GranaryProductBit;


	bool     mCanMixSave;

	unsigned mStorgeProductBit;
	unsigned mAllowProductBit;
	unsigned mWantProductBit;
	unsigned mRemoveProductBit;
	int      mNumCurSave;
	int      mNumMaxSave;
	struct   StroageCell
	{
		StroageCell();

		unsigned productTag;
		int      numGoods;
	};

	friend class rcLevelScene;
	StroageCell mStroageCell[ 8 ];
};

#include "TTable.h"
#include <list>

class rcBuildingQuery
{
public:
	rcBuilding*  getBuilding( unsigned buildingID );

	typedef std::list< rcBuilding* > BuildingList;
	struct FindCookie
	{
		FindCookie(){ restart(); }
		void restart(){ var = -1; }
	private:
		int var;
		BuildingList::iterator iter;
		friend class rcBuildingManager;
	};
	virtual rcBuilding*  findStorage( FindCookie& cookie , unsigned pTag ) = 0;
	virtual rcHouse*     findEmptyHouse( FindCookie& cookie , int& numEmptySpace ) = 0;

protected:
	typedef TTable< rcBuilding*> BuildingTable;
	BuildingTable  mBTable;
};
#endif // rcBuliding_h__
