#include "rcPCH.h"
#include "rcWorkerData.h"

#include "rcGameData.h"


int const g_workerTextureSize = 39;
int const g_DefultWorkerSpeed = 1000;

#define DEF_SPEED g_DefultWorkerSpeed
#define DEF_DIST  20

#define BEGIN_WORKER_ANIM_MAP()\
	void rcDataManager::loadWorkerList()\
	{\
		char tempBuf[ g_workerTextureSize * g_workerTextureSize * 4 * 20 ];\


#define WORKER( wTag ,  randDist , speed  , index , animBit )\
		loadWorker( wTag , randDist , speed );\
		loadWorkerTexture( wTag , index , animBit , tempBuf );

#define WORKER_DEFULT( wTag , index )\
	WORKER( wTag , DEF_DIST , DEF_SPEED , index , BIT( WKA_WALK )|BIT( WKA_DIE )  )

#define WORKER_DEFULT2( wTag , randDist , speed , index )\
	WORKER( wTag , randDist , speed , index , BIT( WKA_WALK )|BIT( WKA_DIE )  )

#define END_WORKER_ANIM_MAP()\
	}





BEGIN_WORKER_ANIM_MAP()
	////////////      [   TAG   ]   [ dist ]   [ speed ]  [imgIdx] [ animBit ]
 	WORKER_DEFULT2( WT_FIND_MAN    ,    80   ,    200     , 3409 )
	WORKER_DEFULT( WT_BATH_WORKER                         , 3513 )
	WORKER_DEFULT( WT_ACTOR                               , 3721 )

	WORKER(         WT_LOIN_TAMER  , DEF_DIST , DEF_SPEED , 3825 , BIT( WKA_WALK )|BIT( WKA_DIE )  )

	WORKER_DEFULT( WT_TAX_COLLECTOR                       , 4025 )
	WORKER_DEFULT( WT_SCHOOL_CHILD                        , 4129 )
	WORKER_DEFULT( WT_MARKET_TRADER                       , 4233 )
	WORKER_DEFULT( WT_DELIVERY_MAN                        , 4337 )
	WORKER_DEFULT( WT_SETTLER                             , 4441 )
	WORKER_DEFULT( WT_ENGINNER                            , 4545 )

	WORKER(              WT_LION  , DEF_DIST , DEF_SPEED  , 6608 , BIT( WKA_WALK ) )

	WORKER_DEFULT( WT_PATRICIAN                           , 7320  )
	WORKER_DEFULT( WT_DOCTOR                              , 7424  )
	//WORKER( WT_ENGINNER      , 4545 , WKA_WALK|WKA_DIE , 0 )
	//WORKER( WT_ENGINNER      , 4545 , WKA_WALK|WKA_DIE , 0 )
	//WORKER( WT_ENGINNER      , 4545 , WKA_WALK|WKA_DIE , 0 )
	//WORKER( WT_ENGINNER      , 4545 , WKA_WALK|WKA_DIE , 0 )
	//WORKER( WT_ENGINNER      , 4545 , WKA_WALK|WKA_DIE , 0 )
	//WORKER( WT_ENGINNER      , 4545 , WKA_WALK|WKA_DIE , 0 )

	
END_WORKER_ANIM_MAP()

#undef BEGIN_WORKER_ANIM_MAP
#undef WORKER
#undef WORKER_DEFULT
#undef WORKER_DEFULT2
#undef END_WORKER_ANIM_MAP



