#include "rcPCH.h"
#include "rcGameData.h"

#include "rcBuilding.h"
#include "rcProductTag.h"
#include "rcWorkerData.h"
#include "rcMapLayerTag.h"
#include "rcRenderSystem.h"

rcDataManager::rcDataManager()
{
	mWorkerInfo.reset( new WorkerInfo[ WT_NUM_WORKER ] );
}

bool rcDataManager::init()
{
	mLoader.loadFormFile( "c3.sg2" );

	mNumExtraModel = 0;

	memset( (void*)mPStorage , 0 , ARRAY_SIZE( mPStorage ) * sizeof( mPStorage[0] ) );
	memset( (void*)mBaseInfo , 0 , ARRAY_SIZE( mBaseInfo ) * sizeof( mBaseInfo[0] ) );

	initAnimImageInfo();

	initWorkShopInfo();

	loadUI();

	loadBuildingTexture();
	loadFarmTexture();
	loadTerrainTexture();
	loadRoadTexture();
	loadAquaductTexture();
	loadWorkerList();
	loadProductTexture();

	mTexSettlerCart = createTexture( 4777 , 8 );


	for( int i = 0 ; i < ProductNum ; ++i )
	{
		ProductStorage& ps = mPStorage[i];
		ps.buildTag[ ps.num++ ] = BT_WAREHOUSE;
	}


	mLoader.releaseInternalData();

	return true;
}


void rcDataManager::loadProduct( unsigned pTag , int idxImage )
{
	int idx = 3337 +  4 * ( pTag - 1 ) ;
	for( int i = 0 ; i < 4 ; ++i)
		texProduct[ pTag ].warehouse[i] = createTexture( idx + i );

	int startIdx = ( idxImage ) ? idxImage : ( 4657 + 8 * ( pTag - 1 ) );
	texProduct[ pTag ].cart = createTexture( startIdx , 8 );

}



void rcDataManager::loadBuilding( unsigned tagBuilding , int idxImage , int animID , int sx , int sy )
{
	rcRenderSystem* rs = getRenderSystem();

	rcBuildingInfo& info = g_buildingInfo[ tagBuilding ];

	ModelInfo* modelInfo;
	if ( mBaseInfo[ tagBuilding ].texBase == RC_ERROR_TEXTURE_ID )
	{
		modelInfo = &mBaseInfo[ tagBuilding ];
	}
	else
	{
		modelInfo = &mExtraInfo[ mNumExtraModel ];

		if ( info.numExtraModel == 0)
			info.startExtraModel = ARRAY_SIZE( mBaseInfo ) + mNumExtraModel;

		++info.numExtraModel;
		++mNumExtraModel;
	}

	modelInfo->xSize = sx;
	modelInfo->ySize = sy;

	if ( idxImage == -1 )
	{
		modelInfo->texBase = RC_ERROR_TEXTURE_ID;
		modelInfo->animBGInfo  = NULL;
		modelInfo->texAnim = RC_ERROR_TEXTURE_ID;
	}
	else
	{
		modelInfo->texBase = createTexture( idxImage );
		modelInfo->imageInfo = getImageInfo( idxImage );

		if ( animID != RC_ERROR_ANIM_ID )
		{
			if ( animID != RC_ANIM_NO_BACKGROUND )
			{
				modelInfo->animBGInfo  = &g_AnimImageInfo[ animID ];
				//FIXME
				modelInfo->animBGInfo->texAnimBG = RC_ERROR_TEXTURE_ID;
			}
			modelInfo->texAnim   = createAnimTexture( idxImage );

		}
		else
		{
			modelInfo->animBGInfo  = NULL;
			modelInfo->texAnim = RC_ERROR_TEXTURE_ID;
		}
	}
	
}

void rcDataManager::loadRoadTexture()
{
	int const distRoadIndex = 639;
	int const typeIndexOffset[] = { 12 , 0 , 4 , 8 , 13 , 17 };

	mTexRoad[0] = createTexture( distRoadIndex + typeIndexOffset[0] );
	mTexRoad[5] = createTexture( distRoadIndex + typeIndexOffset[5] );

	for( int i = 1 ; i <= 4 ; ++i )
	{
		mTexRoad[i] = createTexture( distRoadIndex + typeIndexOffset[i] , 4 );
	}
}


void rcDataManager::loadAquaductTexture()
{
	int const startIndex = 665;
	int const numImag[] = { 2 , 2 , 4 , 2,  4 , 1 };

	int index = startIndex;

	for( int i = 0 ; i < 12; ++i )
	{
		int num = numImag[ i % 6 ];
		for( int n = 0 ; n < num ; ++n )
		{
			mTexAquaduct[i][n] = createTexture( index++ );
		}
	}
}

void rcDataManager::loadTerrain( unsigned tagID , int idxImage , int num )
{
	TerrainTexInfo& info = mInfo[ tagID ];
	for( int i = 0 ; i < num ; ++i)
		info.tex[ info.numTex++ ] = createTexture( idxImage + i );
}

rcBuildingInfo const& rcDataManager::getBuildingInfo( unsigned tagID )
{
	return g_buildingInfo[ tagID ];
}

void rcDataManager::loadFarmTexture()
{
	int idxFarmStart = 2883;
	for( int i = 0 ; i < 6 ; ++i )
	{
		for( int n = 0 ; n < 5 ; ++n )
			mTexFarm[i][n] = createTexture( idxFarmStart++ );
	}
}

unsigned* rcDataManager::getFarmTexture( unsigned bTag )
{
	return &mTexFarm[ bTag - BT_FARM_START ][0];
}

void rcDataManager::loadWorkerTexture( unsigned wTag , int idxImage , unsigned animBit , char* tempBuf )
{
	WorkerInfo& info = mWorkerInfo[ wTag ];
	SgxImageInfo const* imageInfo = getImageInfo( idxImage );

	info.animBit = animBit;
	int curIndex = idxImage;

	rcTexture* tex;
	for( unsigned i = 0 ; i < WAK_ANIM_NUM ; ++i )
	{
		int const totalDir = 8;
		int const walkFrmae = 12;
		switch( i )
		{
		case WKA_WALK:
			tex = getRenderSystem()->createEmptyTexture( 
				imageInfo->width * walkFrmae , imageInfo->height * totalDir );

			info.texAnim[ i ] = tex->getManageID();
			loadTextureGird( ( char* )tex->getRawData() , tex->getSize() , 
				curIndex , walkFrmae , totalDir , tempBuf );
			break;
		case WKA_DIE:
			break;
		}
	}
}

void rcDataManager::loadTextureSequence( char* destBuf , Vec2i const& size , int index , int numSeq )
{
	SgxImageInfo const* info = getImageInfo( index );

	assert( size.x == info->width && size.y == numSeq * info->height );

	int len = info->width * NumByte;
	int imgSize = len * info->height;

	char* buf  = destBuf + numSeq * imgSize;
	std::fill_n( (DWORD*)destBuf  , numSeq * imgSize / NumByte , RGB(255,255,255) );

	int num = info->height / 2;
	for( int i = 0 ; i < numSeq ; ++i )
	{
		buf -= imgSize;

		readBitmap( index + i , buf );

		char temp[ 10240 ];
		assert( sizeof( temp ) > len );
		for( int i = 0 ; i < num ; ++i )
		{
			memcpy( temp , buf + i * len , len );
			memcpy( buf + i * len , buf + ( info->height - 1 - i ) * len , len );
			memcpy( buf + ( info->height - 1 - i ) * len , temp , len );
		}
	}
}

void rcDataManager::loadTextureGird( char* destBuf , Vec2i const& size , int index , int nx , int ny , char* tempBuf )
{
	SgxImageInfo const* info = getImageInfo( index );
	Vec2i seqSize( info->width , info->height * ny );
	for( int i = 0 ; i < nx ; ++i )
	{
		loadTextureSequence( tempBuf , seqSize , index + i * ny , ny );
		bitBlt( tempBuf , seqSize , destBuf , size , Vec2i( info->width * i , 0 ) );
	}
}

void rcDataManager::bitBlt( char* srcBuf , Vec2i const& srcSize, char* destBuf , Vec2i const& destSize , Vec2i const& pos )
{
	int const NumByte = 4;

	char* dBuf = destBuf + NumByte * (  pos.x + pos.y * destSize.x );
	char* sBuf = srcBuf;

	int offDest = NumByte * destSize.x;
	int offSrc  = NumByte * srcSize.x;

	for ( int i = 0 ; i < srcSize.y ; ++i )
	{
		memcpy( dBuf , sBuf , offSrc);

		sBuf += offSrc;
		dBuf += offDest;
	}
}

void rcDataManager::loadTexture( char* destBuf , Vec2i const& size , int index )
{
	SgxImageInfo const* info = rcDataManager::Get().getImageInfo( index );

	assert( size.x == info->width && size.y == info->height );

	char* buf = destBuf;

	std::fill_n( (DWORD*)destBuf , size.x * size.y , RGB(255,255,255) );

	readBitmap( index , buf );

	int num = info->height / 2;
	int len = info->width * 4;
	char temp[ 10240 ];
	assert( sizeof( temp ) > len );
	for( int i = 0 ; i < num ; ++i )
	{
		memcpy( temp , buf + i * len , len );
		memcpy( buf + i * len , buf + ( info->height - 1 - i ) * len , len );
		memcpy( buf + ( info->height - 1 - i ) * len , temp , len );
	}
}

unsigned rcDataManager::createTexture( int indexImage )
{
	SgxImageInfo const* info = getImageInfo( indexImage );
	rcTexture* tex = getRenderSystem()->createEmptyTexture( info->width , info->height );

	if ( ! tex )
		return RC_ERROR_TEXTURE_ID;

	loadTexture( ( char*)tex->getRawData() , tex->getSize() , indexImage );

	return tex->getManageID();
}

unsigned  rcDataManager::createTexture( int startIndex , int num )
{
	SgxImageInfo const* info = getImageInfo( startIndex );

	rcTexture* tex = getRenderSystem()->createEmptyTexture( info->width , num * info->height );

	if ( ! tex )
		return RC_ERROR_TEXTURE_ID;

	loadTextureSequence( ( char*)tex->getRawData() , tex->getSize() , startIndex , num );
	return tex->getManageID();
}

unsigned rcDataManager::createAnimTexture( int indexImage )
{
	SgxImageInfo const* info = getImageInfo( indexImage );
	SgxImageInfo const* infoAnim = getImageInfo( indexImage + 1 );

	rcTexture* tex = getRenderSystem()->createEmptyTexture( 
		infoAnim->width , infoAnim->height * info->numAnimImage );

	if ( ! tex )
		return RC_ERROR_TEXTURE_ID;

	loadTextureSequence( ( char*)tex->getRawData() , tex->getSize() , indexImage+ 1 , info->numAnimImage );
	return tex->getManageID();
}

void rcDataManager::loadWorker( unsigned wTag , int randDist , int speed )
{
	WorkerInfo& info = mWorkerInfo[ wTag ];
	info.randMoveDist = randDist;
	info.moveSpeed    = speed;
}

WorkerInfo& rcDataManager::getWorkerInfo( unsigned wTag )
{
	return mWorkerInfo[ wTag ];
}

#define BEGIN_DEFINE_TEXTURE_MAP()          \
	void rcDataManager::loadTerrainTexture()\
	{                                       \
		memset( ( char*)mInfo , 0 , sizeof( mInfo[0]) * ARRAY_SIZE( mInfo ) );\
		unsigned tagID;

#define BEGIN_IMAGE_MAP( dataID ) \
		tagID = dataID;

#define IMAGE_MAP( index  , num ) \
		loadTerrain( tagID , index , num );

#define END_IMAGE_MAP() 


#define END_DEINE_IMAGE_MAP() }

BEGIN_DEFINE_TEXTURE_MAP()

	BEGIN_IMAGE_MAP( TT_GRASS )
		IMAGE_MAP( 245  , 8 )
	END_IMAGE_MAP()

END_DEINE_IMAGE_MAP()

#undef BEGIN_DEFINE_TEXTURE_MAP
#undef BEGIN_IMAGE_MAP
#undef IMAGE_MAP
#undef END_IMAGE_MAP
#undef END_DEINE_IMAGE_MAP

#define  BEGIN_FACTORY()\
	rcFactoryInfo gFactoryInfo[] = {
	
#define FACTORY( tag , pID , pSpeed , pNumPorduct , pStorage )\
	{ pID , pSpeed , pNumPorduct , pStorage } ,

#define END_FACTORY() };


BEGIN_FACTORY()
	FACTORY( BT_FARM_WHEAT     , PT_WHEAT     , 100 , 1 , 5)
	FACTORY( BT_FARM_VEGETABLE , PT_VEGETABLES, 100 , 1 , 5)
	FACTORY( BT_FARM_FIG       , PT_FRUIT     , 100 , 1 , 5)
	FACTORY( BT_FARM_OLIVE     , PT_OLIVES    , 100 , 1 , 5)
	FACTORY( BT_FARM_VINEYARD  , PT_VINES     , 100 , 1 , 5)
	FACTORY( BT_FARM_MEAT      , PT_MEAT      , 100 , 1 , 5)

	FACTORY( BT_MINE       , PT_IRON_MINE, 100 , 1 , 5 )
	FACTORY( BT_CALY_PIT   , PT_CALY_PIT , 100 , 1 , 5 )

	FACTORY( BT_WS_WINE , PT_WINE , 50  , 1 , 5 )
	FACTORY( BT_WS_OIL  , PT_OIL  , 50  , 1 , 5 )
	FACTORY( BT_WS_WEAPONS   , PT_WEAPONS  , 50  , 1 , 5 )
	FACTORY( BT_WS_FURNITURE , PT_FURNLTRUE  , 50  , 1 , 5 )
	FACTORY( BT_WS_POTTERY   , PT_POTTERY    , 50  , 1 , 5 )
END_FACTORY()


#undef BEGIN_FACTORY
#undef FACTORY
#undef END_FACTORY



#define  BEGIN_WORK_SHOP_MAP()             \
	void rcDataManager::initWorkShopInfo(){\

#define DEFINE_WORK_SHOP( tag )   \
	{                             \
		unsigned bTag = tag;      \
		rcFactoryInfo& info =  gFactoryInfo[ tag - BT_FACTORY_START ];\
		int numStuff = 0;

#define USE_STUFF( pTag , numCast , numStorage )    \
		info.stuff[ numStuff ].productID = pTag;    \
		info.stuff[ numStuff ].cast      = numCast; \
		info.stuff[ numStuff ].storage   = numStorage;\
		++numStuff;                                 \
		{                                           \
			ProductStorage& ps = mPStorage[ pTag ]; \
			ps.buildTag[ ps.num++ ] = bTag;         \
		}

#define END_WORK_SHOP()           \
		info.numStuff = numStuff; \
	}                             \

#define END_WORK_SHOP_MAP() }


BEGIN_WORK_SHOP_MAP()

	DEFINE_WORK_SHOP( BT_WS_WINE )
		USE_STUFF( PT_VINES , 1 , 5 )
	END_WORK_SHOP()

	DEFINE_WORK_SHOP( BT_WS_OIL )
		USE_STUFF( PT_OLIVES , 1 , 5 )
	END_WORK_SHOP()

	DEFINE_WORK_SHOP( BT_WS_WEAPONS )
		USE_STUFF( PT_IRON_MINE , 1 , 5 )
	END_WORK_SHOP()

	DEFINE_WORK_SHOP( BT_WS_FURNITURE )
		USE_STUFF( PT_FURNLTRUE , 1 , 5 )
	END_WORK_SHOP()

	DEFINE_WORK_SHOP( BT_WS_POTTERY )
		USE_STUFF( PT_CALY_PIT , 1 , 5 )
	END_WORK_SHOP()

END_WORK_SHOP_MAP()


#undef BEGIN_WORK_SHOP_MAP
#undef DEFINE_WORK_SHOP
#undef USE_STUFF
#undef END_WORK_SHOP
#undef END_WORK_SHOP_MAP



#define BEGIN_WAREHOUSE_TEXTURE()\
	void rcDataManager::loadProductTexture(){ \
		texProduct[ PT_NULL_PRODUCT ].warehouse[0] = createTexture( 3336 ); \
		texProduct[ PT_NULL_PRODUCT ].cart = createTexture( 4649 , 8 );

#define PRODUCT2( pTag , idxImage ) \
		loadProduct( pTag , idxImage );

#define PRODUCT( pTag ) \
		loadProduct( pTag , 0 );

#define END_WAREHOUSE_TEXTURE() }


BEGIN_WAREHOUSE_TEXTURE()

	//PRODUCT( PT_NULL_PRODUCT , 3336 , 4649 )
	PRODUCT( PT_WHEAT )
	PRODUCT( PT_VEGETABLES )
	PRODUCT( PT_FRUIT  )
	PRODUCT( PT_OLIVES )
	PRODUCT( PT_VINES  )
	PRODUCT( PT_MEAT  )
	
	PRODUCT( PT_WINE  )
	PRODUCT( PT_OIL )

	PRODUCT( PT_IRON_MINE  )
	PRODUCT( PT_TIMBER  )
	PRODUCT( PT_CALY_PIT  )
	PRODUCT( PT_MARBLE_STONE  )
	
	PRODUCT( PT_WEAPONS )
	PRODUCT( PT_FURNLTRUE )
	PRODUCT( PT_POTTERY )

	PRODUCT2( PT_FISHING , 5345 )
	
END_WAREHOUSE_TEXTURE()


#undef BEGIN_WAREHOUSE_TEXTURE
#undef PRODUCT
#undef END_WAREHOUSE_TEXTURE