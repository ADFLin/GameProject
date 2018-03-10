#include "rcPCH.h"
#include "rcLevelScene.h"

#include "SgxFileLoader.h"
#include "rcRenderSystem.h"
#include "rcBuildingManager.h"
#include "rcLevelMap.h"
#include "rcGameData.h"
#include "rcMapLayerTag.h"
#include "rcBuilding.h"
#include "rcWorker.h"
#include "rcWorkerData.h"
#include "rcProductTag.h"

rcViewport::rcViewport() 
	:mOffset(0,0)
	,mViewSize( 800 , 600 )
	,mDir( eNorth )
{
	mUpdateRenderPos = false;
}


int g_dbgRenderCount = 0;
bool g_showMapPos = false;


void rcViewport::render( rcLevelMap& levelMap , rcLevelScene& scene )
{
	int nx = mOffset.x / ( tileWidth + 2 );
	int ny = mOffset.y / ( tileHeight );


	int xScanEnd = nx + mViewSize.x / ( tileWidth + 2 ) + 2;
	int yScanEnd = ny + mViewSize.y / tileHeight        + 2;

	int xScanStart = nx - 2;
	int yScanStart = ny - 2;

	Vec2i const offset = Vec2i( ( tileWidth + 2 ) / 2 , tileHeight / 2 );

	RenderCoord coord;

	g_dbgRenderCount = 0;

	for( int yScan = yScanStart ; yScan < yScanEnd ; ++yScan )
	{
		for( int xScan = xScanStart  ; xScan < xScanEnd ; ++xScan )
		{
			calcCoord( coord , xScan , yScan );

			if ( levelMap.isRange( coord.map.x , coord.map.y ) )
			{
				scene.renderTile( coord , levelMap.getData( coord.map ));
				++g_dbgRenderCount;
			}
		}

		for( int xScan = xScanStart ; xScan < xScanEnd ; ++xScan )
		{
			calcCoord( coord , xScan + 0.5f , yScan + 0.5f );
		
			if ( levelMap.isRange( coord.map.x , coord.map.y ) )
			{
				scene.renderTile( coord , levelMap.getData( coord.map ) );
				++g_dbgRenderCount;
			}
		}
	}
}

void rcViewport::calcCoord( RenderCoord& coord , Vec2i const& mapPos )
{
	coord.map    = mapPos;
	transMapPosToScanPos( mapPos , coord.xScan , coord.yScan );
	coord.screen = transScanPosToScreenPos( coord.xScan , coord.yScan );
}

void rcViewport::calcCoord( RenderCoord& coord , float xScan , float yScan )
{
	coord.xScan  = xScan;
	coord.yScan  = yScan;
	coord.map    = transScanPosToMapPos( xScan , yScan );
	coord.screen = transScanPosToScreenPos( xScan , yScan );
}



// 
//   --------> sx  
//  |   offset/ cx	
//	|        /	
// \|/       \   
// sy         \
//             cy

float const transToScanPosFactor[4][4] =
{
	//  sx         sy
	//cx  cy     cx  cy
	{ 0.5, 0.5,-0.5, 0.5 },
	{-0.5, 0.5,-0.5,-0.5 },
	{-0.5,-0.5, 0.5,-0.5 },
	{ 0.5,-0.5, 0.5, 0.5 },
};

static const int transToMapPosFactor[4][4] =
{
	//  cx      cy
	//sx  sy  sx  sy
	{ 1 ,-1 , 1 , 1 },
	{-1 ,-1 , 1 ,-1 },
	{-1 , 1 ,-1 ,-1 },
	{ 1 , 1 ,-1 , 1 },
};

Vec2i rcViewport::transScanPosToMapPos( int xScan , int yScan ) const
{
	int const (&factor)[4] = transToMapPosFactor[ mDir ];
	return Vec2i( xScan * factor[0] + yScan * factor[1] ,
		          xScan * factor[2] + yScan * factor[3] );
}
Vec2i rcViewport::transScanPosToMapPos( float xScan , float yScan ) const
{
	int const (&factor)[4] = transToMapPosFactor[ mDir ];
	return Vec2i( xScan * factor[0] + yScan * factor[1] ,
		          xScan * factor[2] + yScan * factor[3] );
}

Vec2i rcViewport::transScreenPosToMapPos( Vec2i const& sPos ) const
{
	//Vec2i pos = sPos + mOffset;
	//int px = ( pos.x + 2 * pos.y ) / (2 * tileHeight );
	//int py = ( - pos.x + 2 * pos.y ) / (2 * tileHeight );

	//return Vec2i( px , py );

	float xScan = float( sPos.x + mOffset.x ) / (2 * tileHeight);
	float yScan = float( sPos.y + mOffset.y ) / ( tileHeight );

	return transScanPosToMapPos( xScan , yScan );
}



Vec2i rcViewport::transMapPosToScreenPos( Vec2i const& mapPos ) const
{
	float xScan , yScan;
	transMapPosToScanPos( mapPos , xScan , yScan );

	return transScanPosToScreenPos( xScan , yScan );
}

Vec2i rcViewport::transMapPosToScreenPos( float xMap , float yMap )
{
	float const (&factor)[4] = transToScanPosFactor[ mDir ];

	return transScanPosToScreenPos(
		float( xMap * factor[0] + yMap * factor[1] )  ,
		float( xMap * factor[2] + yMap * factor[3] ) );
}

Vec2i rcViewport::transScanPosToScreenPos( float xScan , float yScan ) const
{
	return Vec2i( ( xScan + 0.5 ) * ( tileWidth + 2 ) - mOffset.x ,
		          ( yScan + 0.5 ) * ( tileHeight )  - mOffset.y );
}

void rcViewport::transMapPosToScanPos( Vec2i const& mapPos , float& sx , float& sy ) const
{
	float const (&factor)[4] = transToScanPosFactor[ mDir ];

	sx = float( mapPos.x * factor[0] + mapPos.y * factor[1] );
	sy = float( mapPos.x * factor[2] + mapPos.y * factor[3] );
}

Vec2i rcViewport::calcMapOffsetToScreenOffset( float dx , float dy )
{
	float const (&factor)[4] = transToScanPosFactor[ mDir ];

	return calcScanOffsetToScreenOffset(
		 dx * factor[0] + dy * factor[1] ,
		 dx * factor[2] + dy * factor[3] );
}

Vec2i rcViewport::calcScanOffsetToScreenOffset( float dx , float dy )
{
	return Vec2i( dx * ( tileWidth + 2 ) , dy * tileHeight );
}

static Vec2i calcTileOffset( int w , int h )
{
	return Vec2i( -w  / 2 , /*( tileHeight / 2 ) */- h  );
}

static Vec2i calcTileOffset( rcTexture* tex )
{
	return Vec2i( -tex->getWidth() / 2 , /*( tileHeight / 2 ) */- tex->getHeight() );
}

Color3ub const BaseMaskColor( 255 , 255 , 255 );


void rcLevelScene::init()
{
	mFont = getRenderSystem()->createFont( 10 , TEXT("µØ±d¤¤¶êÅé") );
}

void rcLevelScene::renderTile( RenderCoord const& coord , rcMapData& tileData )
{
	Vec2i const& mapPos    = coord.map;
	Vec2i const& screenPos = coord.screen;

	Vec2i rPos;
	renderTerrain( screenPos , tileData , false );

	if ( tileData.buildingID != RC_ERROR_BUILDING_ID )
	{
		rcBuilding* building = mBuildingMgr->getBuilding( tileData.buildingID );
		renderBuilding( coord , building );
	}

	if ( tileData.workerList )
	{
		renderFlatTerrain( coord ,    1 , 0   );
		renderFlatTerrain( coord , -0.5 , 0.5 );
		renderFlatTerrain( coord ,  0.5 , 0.5 );
		renderFlatTerrain( coord ,    0 , 1   );

		renderWorkerList( screenPos , tileData.workerList );
	}


	if ( g_showMapPos )
	{
		Graphics2D& graphics = getRenderSystem()->getGraphics2D();
		char str[ 32 ];
		//sprintf( str , "%d,%d" , mapPos.x , mapPos.y );
		sprintf_s( str , "%d" , g_dbgRenderCount );
		graphics.setFont( mFont );
		graphics.drawText( screenPos - Vec2i( 8 , tileHeight / 2 + 6 ) , str );
	}
}

Vec2i rcLevelScene::renderTileTexture( unsigned texID , Vec2i const& pos )
{
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();
	rcTexture* tex = getRenderSystem()->getTexture( texID );
	Vec2i rPos = pos + calcTileOffset( tex );
	graphics.drawTexture( *tex , rPos , BaseMaskColor );
	return rPos;
}


Vec2i rcLevelScene::renderTileTexture( unsigned texID , Vec2i const& pos , int totalFrame , int frame )
{
	rcTexture* tex = getRenderSystem()->getTexture( texID );
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	int h = tex->getHeight() / totalFrame;

	Vec2i rPos =  pos + calcTileOffset( tex->getWidth() , h );

	graphics.drawTexture( *tex , rPos , 
		Vec2i( 0 , h * frame) , Vec2i( tex->getWidth() , h ) , BaseMaskColor );

	return rPos;
}



void rcLevelScene::renderTexture( unsigned texID , Vec2i const& pos )
{
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();
	rcTexture* tex = getRenderSystem()->getTexture( texID );
	graphics.drawTexture( *tex , pos , BaseMaskColor );
}


void rcLevelScene::renderTexture( unsigned texID , Vec2i const& pos , Vec2i const& totalGird , Vec2i const& girdPos )
{
	rcTexture* tex = getRenderSystem()->getTexture( texID );
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	int w = tex->getWidth() /  totalGird.x;
	int h = tex->getHeight() / totalGird.y;
	
	graphics.drawTexture( *tex , pos , 
		Vec2i( w * girdPos.x , h * girdPos.y ) , Vec2i( w , h ) , BaseMaskColor );
}


void rcLevelScene::renderTexture( unsigned texID , Vec2i const& pos , int totalFrame , int frame )
{
	rcTexture* tex = getRenderSystem()->getTexture( texID );
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	int h = tex->getHeight() / totalFrame;
	graphics.drawTexture( *tex , pos , 
		Vec2i( 0 , h * frame) , Vec2i( tex->getWidth() , h ) , BaseMaskColor );
}

void rcLevelScene::renderBuilding( RenderCoord const& coord , rcBuilding* building  )
{
	updateBuildingRenderPos( building );

	bool drawAnim = ( building->getRenderData().coord.map == coord.map );

	switch( building->getTypeTag() )
	{
	case BT_WAREHOUSE:
		renderWareHouse( coord , building );
		break;
	case BT_FARM_OLIVE: case BT_FARM_VEGETABLE: case BT_FARM_MEAT:
	case BT_FARM_WHEAT: case BT_FARM_VINEYARD:  case BT_FARM_FIG:
		renderFarm( coord , building );
		break;
	default:
		renderBuildingBase( coord , building , drawAnim );
	}
}


Vec2i rcLevelScene::renderMuitiTileTexture( unsigned texID , RenderCoord const& coord , RenderCoord const& renderCoord )
{
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	Vec2i const& mapPos    = renderCoord.map;
	Vec2i const& screenPos = renderCoord.screen;

	rcTexture* tex = getRenderSystem()->getTexture( texID );

	Vec2i rPos = screenPos  + calcTileOffset( tex );

	Vec2i offset = getViewPort().calcScanOffsetToScreenOffset( 
		coord.xScan - renderCoord.xScan  , coord.yScan - renderCoord.yScan );

	offset.x += tex->getWidth() / 2 - ( tileWidth )/2;
	offset.y = 0;

	graphics.drawTexture( *tex , rPos + offset , offset , 
		Vec2i( tileWidth  , tex->getHeight() ) , BaseMaskColor );

	return rPos;
}

void rcLevelScene::renderAnimation(  ModelInfo const& model , Vec2i const& renderPos , int frame )
{
	if (  model.texAnim == RC_ERROR_TEXTURE_ID  ) 
		return;

	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	if ( model.animBGInfo )
	{


	}

	SgxImageInfo const* imgInfo = model.imageInfo;
	rcTexture* tex = getRenderSystem()->getTexture( model.texAnim );
	int h = tex->getHeight() / imgInfo->numAnimImage;
	//FIXME
	frame = frame % imgInfo->numAnimImage;

	graphics.drawTexture( *tex , 
		renderPos + Vec2i( imgInfo->xAnimOffset , imgInfo->yAnimOffset ) , 
		Vec2i( 0 , h * frame) , Vec2i( tex->getWidth() , h ) , BaseMaskColor );


}

void rcLevelScene::renderBuildingBase( RenderCoord const& coord , rcBuilding* building , bool drawAnim )
{
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	RenderCoord const& renderCoord = building->getRenderData().coord;
	Vec2i const& mapPos    = renderCoord.map;
	Vec2i const& screenPos = renderCoord.screen;

	ModelInfo const& model = building->getModel();

	Vec2i rPos;
	if ( !building->checkFlag( rcBuilding::eSingleTile ) )
	{
		rPos = renderMuitiTileTexture( model.texBase , coord , renderCoord );
	}
	else
	{
		rPos = renderTileTexture(  model.texBase , screenPos );
	}

	if ( drawAnim )
	{
		if ( building->getState() == BS_WORKING )
			renderAnimation( model , rPos , mRenderFrame / 10 );
	}
}


void rcLevelScene::renderWareHouse( RenderCoord const& coord , rcBuilding* building )
{
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	assert( building->downCast< rcStorageHouse >() );
	rcStorageHouse* sHouse = static_cast< rcStorageHouse* >( building );

	Vec2i dif = sHouse->getMapPos() - coord.map;
	int index = 3 * abs( dif.x ) + abs(dif.y);
	if ( index == 0 )	
	{
		renderBuildingBase( coord , building , true );
	}
	else
	{
		rcStorageHouse::StroageCell& cell = sHouse->mStroageCell[ index - 1 ];
		unsigned* tIDList = rcDataManager::Get().getWareHouseProductTexture( cell.productTag );

		int frame = 0;
		if ( cell.productTag != PT_NULL_PRODUCT )
		{
			frame = 4 * ( cell.numGoods ) / sHouse->mNumMaxSave - 1;
		}
		renderTileTexture( tIDList[frame] , coord.screen );
	}

}

void rcLevelScene::renderFarm( RenderCoord const& coord , rcBuilding* building )
{
	assert( building->downCast< rcFarm >() );
	rcFarm* farm = static_cast< rcFarm* >( building );

	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	Vec2i dif = farm->getMapPos() - coord.map;
	
	dif.x = abs( dif.x );
	dif.y = abs( dif.y );

	int index = 3 * dif.x + dif.y;
	if ( dif.x <= 1 && dif.y <= 1 )
	{
		float sx = farm->getRenderData().coord.xScan;
		float sy = farm->getRenderData().coord.yScan;

		switch( getViewPort().getDir() )
		{
		case rcViewport::eNorth: sx += -0.5f; sy += -0.5f; break;
		case rcViewport::eSouth: sx += 0.5f;  sy += -0.5f; break;
		case rcViewport::eWest:  sx += 0.5f;  sy += -1.0f; break;
		case rcViewport::eEast:  break;
		}

		RenderCoord renderCoord;
		getViewPort().calcCoord( renderCoord , sx , sy );

		ModelInfo const* model = &farm->getModel();
		if ( model->texBase == RC_ERROR_TEXTURE_ID )
			model = &rcDataManager::Get().getBuildingModel( BT_FARM_START );
		
		renderMuitiTileTexture( model->texBase , coord , renderCoord );

	}
	else
	{
		unsigned* texList = rcDataManager::Get().getFarmTexture( farm->getTypeTag() );

		int frame = 5 * farm->mProgressProduct / rcFarm::MaxProgressFinish;
		renderTileTexture( texList[ frame ] , coord.screen );
	}

}




void rcLevelScene::updateBuildingRenderPos( rcBuilding* building )
{
	if ( mUpdatePosCount && building->getRenderData().updatePosCount == mUpdatePosCount )
		return;

	building->getRenderData().updatePosCount = mUpdatePosCount;

	rcBuildingInfo const& bInfo = rcDataManager::Get().getBuildingInfo( building->getTypeTag() );

	RenderCoord& coord = building->getRenderData().coord;
	Vec2i& renderPos = coord.map;
	renderPos = building->getMapPos();

	if ( !building->checkFlag( rcBuilding::eSingleTile) )
	{
		Vec2i bSize = building->getSize();

		switch( mViewport.getDir() )
		{
		case rcViewport::eNorth:
			renderPos.y += bSize.y - 1;
			break;
		case rcViewport::eSouth:
			renderPos.x += bSize.x - 1;
			break;
		case rcViewport::eEast:
			break;
		case rcViewport::eWest:
			renderPos.x += bSize.x - 1;
			renderPos.y += bSize.y - 1;
			break;
		}
	}

	getViewPort().transMapPosToScanPos( renderPos , coord.xScan , coord.yScan );
	coord.screen = getViewPort().transScanPosToScreenPos( coord.xScan , coord.yScan );
}


void rcLevelScene::renderAquaduct( Vec2i const& pos , rcMapData const& data )
{
	rcMapData::ConnectInfo const& info = data.conInfo[ CT_WATER_WAY ];

	unsigned* texList;
	if ( data.haveConnect( CT_LINK_WATER ) )
		texList = rcDataManager::Get().getAquaductTexture( info.type );
	else
		texList = rcDataManager::Get().getAquaductTexture( 6 + info.type );

	unsigned texID;
	switch( info.type )
	{
	case 5: 
		texID = texList[0];
		break;
	case 0: case 1: case 3:
		texID = texList[ ( info.dir + 4 - getViewPort().getDir() ) % 2 ];
		break;
	case 2: case 4:
		texID = texList[ ( info.dir + 4 - getViewPort().getDir() ) % 4 ];
		break;

	default:
		return;
	}

	renderTileTexture( texID , pos );
}

void rcLevelScene::renderRoad( Vec2i const& pos , rcMapData const& data )
{
	rcMapData::ConnectInfo const& info = data.conInfo[ CT_ROAD ];
	unsigned texID = rcDataManager::Get().getRoadTexture( info.type );

	Vec2i rPos;
	if ( info.type == 0 || info.type == 5 )
		renderTileTexture( texID , pos );
	else
	{
		unsigned texID2 = rcDataManager::Get().getRoadTexture( 0 );
		rPos = pos + calcTileOffset( getRenderSystem()->getTexture( texID2 ) );
		renderTexture( texID ,rPos , 4 , ( info.dir + 4 - getViewPort().getDir() ) % 4 );
	}

}

void rcLevelScene::renderWorkerList(  Vec2i const& screenPos , rcWorker* workerList )
{
	if ( workerList == NULL )
		return;

	Vec2i const& mapPos = workerList->getMapPos();
	Graphics2D& graphics = getRenderSystem()->getGraphics2D();

	rcWorker::Iterator iter = workerList->getIterator();
	while( iter.haveMore() )
	{
		rcWorker* worker =  iter.getElement();

		if ( !worker->checkFlag( rcWorker::eVisible ) )
		{
			iter.goNext();
			continue;
		}

		int dir = worker->getDir().getValue();

		Vec2i const baseOffset = Vec2i( 0 , -tileHeight / 2 ) + Vec2i( -20 , -25 );

		float dx = g_DirOffset8[ dir ].x * worker->getMapOffset();
		float dy = g_DirOffset8[ dir ].y * worker->getMapOffset();

		Vec2i offPos = getViewPort().calcMapOffsetToScreenOffset( dx , dy );


		Vec2i renderPos = screenPos + offPos + baseOffset;

		WorkerInfo& texInfo = rcDataManager::Get().getWorkerInfo( worker->getType() );

		int fixDir = ( dir + 8 - 2 * getViewPort().getDir() ) % 8;
		//FIXME
		int frame = mRenderFrame / 10 % 12;


		Vec2i const SrceenDirOffset[] = 
		{
			Vec2i( 2 , - 1 ) , Vec2i( 1 , 0 ) ,Vec2i( 2 , 1 ), Vec2i( 0 , 1 ) ,
			Vec2i( -2 ,  1 ) , Vec2i( -1 , 0 ) ,Vec2i( -2 , -1 ), Vec2i( 0 , -1 ) ,
		};


		switch( worker->getType() )
		{
		case WT_DELIVERY_MAN:
			{
				rcWorker::TransportInfo& info = worker->getBuildingVar().infoTP;
				unsigned tIDCart = rcDataManager::Get().getCartProductTexture( info.pTag );
				rcTexture* texCart = getRenderSystem()->getTexture( tIDCart );

				Vec2i delPos = renderPos - 7 * SrceenDirOffset[ fixDir ];

				if ( 1 < fixDir && fixDir < 5 )
				{
					renderTexture( texInfo.texAnim[ WKA_WALK ] , delPos  , Vec2i( 12 , 8 ) , Vec2i(  frame , fixDir ) );
					renderTexture( tIDCart , renderPos , 8 , fixDir );
				}
				else
				{
					renderTexture( tIDCart , renderPos , 8 , fixDir );
					renderTexture( texInfo.texAnim[ WKA_WALK ] , delPos  , Vec2i( 12 , 8 ) , Vec2i(  frame , fixDir ) );
				}
			}
			break;
		case WT_SETTLER:
			{
				unsigned texIDCart = rcDataManager::Get().getSettlerCart();

				Vec2i cartPos = renderPos - 5 * SrceenDirOffset[ fixDir ];

				if ( 1 < fixDir && fixDir < 5 )
				{
					renderTexture( texIDCart , cartPos  , 8 , fixDir );
					renderTexture( texInfo.texAnim[ WKA_WALK ] , renderPos  , Vec2i( 12 , 8 ) , Vec2i(  frame , fixDir ) );
				}
				else
				{
					renderTexture( texInfo.texAnim[ WKA_WALK ] ,  renderPos , Vec2i( 12 , 8 ) , Vec2i(  frame , fixDir ) );
					renderTexture( texIDCart , cartPos , 8 , fixDir );
				}
			}
			break;
		default:
			graphics.setBrush( Color3ub( 255 , 255 , 0 ) );
			graphics.drawCircle( screenPos + Vec2i( 0 , -tileHeight / 2 ) + offPos , 5 );


			renderTexture( texInfo.texAnim[ WKA_WALK ] , renderPos  , Vec2i( 12 , 8 ) , Vec2i(  frame , fixDir ) );
			break;

		}


		


		iter.goNext();
	}
}

bool isFlatTerrain( rcMapData& data )
{
	if ( data.haveConnect( CT_WATER_WAY ) )
		return false;

	return true;
}


bool rcLevelScene::renderTerrain( Vec2i const& screenPos , rcMapData& data , bool onlyFlat )
{
	if ( data.renderFrame == mRenderFrame )
		return true;

	if ( onlyFlat )
	{

	}


	if ( onlyFlat && !isFlatTerrain( data ) )
		return false;

	data.renderFrame = mRenderFrame;

	if ( data.texBase != RC_ERROR_TEXTURE_ID )
	{
		renderTileTexture( data.texBase , screenPos );
	}
	else
	{

		if ( data.layer[ ML_BUILDING ] == BT_ROAD ||
			 data.layer[ ML_BUILDING ] == BT_AQUADUCT )
		{
			if ( data.haveConnect( CT_WATER_WAY ) )
				renderAquaduct( screenPos , data );
			else
				renderRoad( screenPos , data );
		}
		else
		{

			unsigned id = rcDataManager::Get().getTerrainTexture( TT_GRASS , 0 );
			renderTileTexture( id , screenPos );
		}
	}



	return true;
}

void rcLevelScene::renderFlatTerrain( RenderCoord const& coord , float dx, float dy )
{
	Vec2i mapPos = getViewPort().transScanPosToMapPos( coord.xScan + dx , coord.yScan + dy );

	if ( !mLevelMap->isRange( mapPos ) )
		return;
	Vec2i screenPos = coord.screen + getViewPort().calcScanOffsetToScreenOffset( dx , dy );
	renderTerrain( screenPos , mLevelMap->getData( mapPos ) , true );
}

void rcLevelScene::renderLevel()
{
	assert( mLevelMap );
	if ( mViewport.mUpdateRenderPos )
	{
		++mUpdatePosCount;
		//mViewport.mUpdateRenderPos = false;
	}
	mViewport.render( *mLevelMap , *this );
}
