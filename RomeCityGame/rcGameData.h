#ifndef rcGameData_h__
#define rcGameData_h__

#include "rcBase.h"
#include "Holder.h"
#include "SgxFileLoader.h"



#define RC_ERROR_ANIM_ID 0
#define RC_ANIM_NO_BACKGROUND 1


struct WorkerInfo;
struct rcWidgetInfo;

struct ImageBGInfo
{
	ImageBGInfo(){}
	ImageBGInfo( int bgX , int bgY )
		:xOffsetBG(bgX),yOffsetBG(bgY)
		,texAnimBG( RC_ERROR_TEXTURE_ID ){}

	int   xOffsetBG;
	int   yOffsetBG;
	unsigned  texAnimBG;
};

struct SgxImageInfo;



struct ModelInfo
{
	unsigned texBase;
	char     xSize;
	char     ySize;

	SgxImageInfo const* imageInfo;
	ImageBGInfo*   animBGInfo;
	unsigned       texAnim;
};

struct rcBuildingInfo;

extern int const      g_BuildingNum;
extern rcBuildingInfo g_buildingInfo[];
extern ImageBGInfo    g_AnimImageInfo[];

struct TerrainTexInfo
{
	int      numTex;
	unsigned tex[ 8 ];
};


struct ProductStorage
{
	unsigned buildTag[ 4 ];
	int      num;
};

class rcDataManager : public SingletonT< rcDataManager >
{
public:
	rcDataManager();

	bool init();

	rcBuildingInfo    const& getBuildingInfo( unsigned tagID );
	SgxImageInfo      const* getImageInfo( int index )     { return mLoader.getImageInfo( index ); }
	SgxImageGroupInfo const* getImageGroupInfo( int index ){ return mLoader.getImageGroupInfo( index ); }
	

	ModelInfo& getBuildingModel( unsigned idModel )
	{
		if ( idModel < ARRAY_SIZE( mBaseInfo ) )
			return mBaseInfo[idModel];
		return mExtraInfo[ idModel - ARRAY_SIZE( mBaseInfo ) ];
	}


	WorkerInfo&   getWorkerInfo( unsigned wTag );
	unsigned*     getAquaductTexture( unsigned type )
	{ 
		return &mTexAquaduct[ type ][0]; 
	}
	unsigned      getRoadTexture( unsigned type , int flag = 0 )
	{
		return mTexRoad[type];
	}

	unsigned  getSettlerCart(){ return mTexSettlerCart; }
	unsigned* getWareHouseProductTexture( unsigned pTag ){  return texProduct[ pTag ].warehouse;  }
	unsigned  getCartProductTexture( unsigned pTag ){  return texProduct[ pTag ].cart;  }
	unsigned* getFarmTexture( unsigned bTag );

	int      getTerrainTextureNum( unsigned tagID , int index ){  return mInfo[tagID].numTex; }
	unsigned getTerrainTexture( unsigned tagID , int index )
	{
		assert( index < mInfo[tagID].numTex );
		return mInfo[tagID].tex[ index ];
	}

	void initAnimImageInfo();
	void initWorkShopInfo();

	void loadWorkerTexture( unsigned wTag , int idxImage , unsigned animBit , char* tempBuf );
	void loadWorker( unsigned wTag , int randDist , int speed );

	void loadBuilding( unsigned tagBuilding , int idxImage , int animID , int sx , int sy );
	void loadTerrain ( unsigned tagID , int idxImage , int num );
	void loadProduct( unsigned pTag , int idxImage );

	void loadUI();
	void loadWorkerList();
	void loadBuildingTexture();
	void loadAquaductTexture();
	void loadRoadTexture();
	void loadTerrainTexture();
	void loadProductTexture();
	void loadFarmTexture();


	static int const NumByte = 4;


	static void bitBlt( char* srcBuf , Vec2i const& srcSize, char* destBuf , Vec2i const& destSize  , Vec2i const& pos );


	unsigned  createTexture    ( int indexImage );
	unsigned  createTexture    ( int startIndex , int num );
	unsigned  createAnimTexture( int indexImage );


	void  loadTexture( char* destBuf , Vec2i const& size  , int index );
	void  loadTextureSequence( char* destBuf , Vec2i const& size  , int index , int numSeq );
	void  loadTextureGird( char* destBuf , Vec2i const& size , int index , int nx , int ny , char* tempBuf );


	
	ProductStorage&  getProductStorage( unsigned pTag ){  assert( pTag < ARRAY_SIZE(mPStorage) );return mPStorage[ pTag ]; }

	//unsigned  mTexTerrain[];
	struct ProuctTexture
	{
		unsigned warehouse[4];
		unsigned cart;
	};


	void readBitmap( int idx , char* buf )                  { mLoader.readBitmap( idx , buf ); }
	
	rcWidgetInfo const& getWidgetInfo( int idUI );


	TArrayHolder< WorkerInfo > mWorkerInfo;


	unsigned  mTexSettlerCart;
	unsigned  mTexWarehouseSpace[ 32 ];
	unsigned  mTexAquaduct[ 6 * 2 ][4];
	unsigned  mTexRoad[ 6 ];
	unsigned  mTexFarm[ 6 ][ 5 ];

	ProuctTexture  texProduct[ 52 ];
	ProductStorage mPStorage[ 32 ];
	TerrainTexInfo mInfo[ 32 ];
	ModelInfo      mBaseInfo[ 128 ];
	
	ModelInfo      mExtraInfo[ 64 ];
	unsigned       mNumExtraModel;

	SgxFileLoader  mLoader;
};

#endif // rcGameData_h__
