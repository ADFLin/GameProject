#include "rcPCH.h"

#include "GameLoop.h"
#include "Win32Platform.h"
#include "SysMsgHandler.h"
#include "ProfileSystem.h"

#include "rcBase.h"

#include "rcLevelScene.h"
#include "rcLevelMap.h"
#include "rcRenderSystem.h"
#include "rcMapLayerTag.h"
#include "rcBuilding.h"

#include "rcGameData.h"
#include "rcPathFinder.h"
#include "rcWorker.h"
#include "rcWorkerData.h"
#include "rcLevelCity.h"
#include "rcGUI.h"
#include "rcGUIData.h"
#include "rcControl.h"



rcGlobal g_Global;
rcGlobal const& getGloba()
{
	return g_Global;
}


class TMessageShow
{
public:
	typedef unsigned char BYTE;

	TMessageShow()
	{
		hFont = getRenderSystem()->createFont( 8 , TEXT("新細明體") );

		height = 15;
		setPos( 20 , 20 );
		setColor( 255 , 255 , 0 , 255 );
	}

	void shiftPos( int dx , int dy )
	{
		startX += dx;
		startY += dy;
	}

	void setPos( int x , int y )
	{
		startX = x ; startY = y;
	}

	void setColor( BYTE r, BYTE g , BYTE b , BYTE a = 255 )
	{
		cr = r ; cg = g; cb = b ; ca = a;  
	}
	void start()
	{
		lineNun = 0;
	}


	void pushStr( char const* str )
	{
		//getRenderSystem()->getGraphics2D().setTextColor(
		//	ColorKey3( cr , cg , cb ) );

		Graphics2D& g = getRenderSystem()->getGraphics2D();
		g.setFont( hFont );
		g.drawText( Vec2i( startX , startY + lineNun * height ) , str );
		++lineNun;
	}
	void push( char const* format, ... )
	{
		va_list argptr;
		va_start( argptr, format );
		vsprintf( string , format , argptr );
		va_end( argptr );

		pushStr( string );
	}

	void finish()
	{

	}

	void setOffsetY( int y )
	{
		height = y;
	}

	HFONT    hFont;

	char     string[512];
	int      height;
	int      lineNun;
	BYTE     cr,cg,cb,ca;
	int      startX , startY;
};


class CProfileViewer : public TProfileViewer< CProfileViewer >
{
public:

	void onRoot     ( SampleNode* node )
	{
		double time_since_reset = ProfileSystem::getInstance().getTimeSinceReset();
		msgShow.push( "--- Profiling: %s (total running time: %.3f ms) ---" , 
			node->getName() , time_since_reset );
	}

	void onNode     ( SampleNode* node , double parentTime )
	{
		msgShow.push( "|-> %d -- %s (%.2f %%) :: %.3f ms / frame (%d calls)",
			++mIdxChild , node->getName() ,
			parentTime > CLOCK_EPSILON ? ( node->getTotalTime()  / parentTime ) * 100 : 0.f ,
			node->getTotalTime()  / (double)ProfileSystem::getInstance().getFrameCountSinceReset() ,
			node->getTotalCalls() );
	}

	bool onEnterChild( SampleNode* node )
	{ 

		mIdxChild = 0;

		msgShow.shiftPos( 20 , 0 );
			
		return true; 
	}
	void onEnterParent( SampleNode* node , int numChildren , double accTime )
	{
		if ( numChildren )
		{
			double time;
			if ( node->getParent() != NULL )
				time = node->getTotalTime();
			else
				time = ProfileSystem::getInstance().getTimeSinceReset();

			double delta = time - accTime;
			msgShow.push( "|-> %s (%.3f %%) :: %.3f ms / frame", "Other",
				// (min(0, time_since_reset - totalTime) / time_since_reset) * 100);
				( time > CLOCK_EPSILON ) ? ( delta / time * 100 ) : 0.f  , 
				delta / (double)ProfileSystem::getInstance().getFrameCountSinceReset() );
			msgShow.push( "-------------------------------------------------" );
		}

		msgShow.shiftPos( -20 , 0 );
	}


	void showText()
	{
		msgShow.setPos( 20 , 10 );
		msgShow.start();

		visit();
		msgShow.finish();
	}

	int    mIdxChild;

	TMessageShow msgShow;
};

class RomeCityGame : public GameLoopT< RomeCityGame , Win32Platform >
				   , public WinFrameT< RomeCityGame >
				   , public SysMsgHandlerT< RomeCityGame >
{
public:
	RomeCityGame(){}
	~RomeCityGame(){}

public:  //GameCore
	bool onInit()
	{
		PROFILE_ENTRY( "Game::Init" )

			if ( !WinFrame::create( TEXT("Tetris") , 800 , 600 , SysMsgHandler::MsgProc  ) )
			return false;

		mRenderSystem.reset( new rcRenderSystem( getHWnd() , getHDC() ) );

		hFont = mRenderSystem->createFont( 12 , TEXT("華康中圓體") );		
		mProfileViewer.reset( new CProfileViewer );

		mControl.reset( new rcControl );
		

		//mRenderSystem->createFont( 12 , TEXT("華康中圓體") );
		rcDataManager::getInstance().init();

		rcGUISystem::getInstance().setControl( mControl );
		if ( !initTestVariable() )
			return false;

		return true;
	}




	void onEnd()
	{
		rcDataManager::releaseInstance();

		mRenderSystem.reset( NULL );
	}
	long onUpdate( long shouldTime )
	{ 
		ProfileSystem::getInstance().incrementFrameCount();

		PROFILE_ENTRY( "Game::Update" )

		g_Global.systemTime += shouldTime;

		long time = g_UpdateFrameRate;
		g_Global.gameTime += time;


		mLevelCity->update( time );
		rcGUISystem::getInstance().update( time );

		return shouldTime; 
	}
	void onRender()
	{
		PROFILE_ENTRY( "Game::Render" )

		getRenderSystem()->beginRender();
		{
			mLevelCity->render();

			Graphics2D& g = getRenderSystem()->getGraphics2D();
			g.setTextColor( 255 , 0 , 0 );


			char str[256];
			sprintf_s( str , "MapPos = %d,%d" , mapPos.x , mapPos.y );
			g.setFont( hFont );
			g.drawText( Vec2i( 10 ,10 ) , str );

			renderPath();

			for( int i = 0 ; i < 1 ; ++i )
			{
				WorkerInfo& info = rcDataManager::getInstance().getWorkerInfo( WT_MARKET_TRADER );
				//unsigned texID = rcDataManager::getInstance().getRoadTexture( i );
				//unsigned texID = rcDataManager::getInstance().getWareHouseProductTexture( i );

				//mLevelScene->renderTexture( info.texAnim[ WKA_WALK ] , Vec2i( 70 * i + 10 , 30 ) );
			}

			rcGUISystem::getInstance().render();

			mProfileViewer->showText();

		}
		getRenderSystem()->endRender();
	}


	void onIdle( long time )
	{  
		PROFILE_ENTRY( "Game::onIdle" )
		::Sleep( time );  
	}



	void fillGrassTarrain()
	{
		struct TerrainModifyVisitor : rcLevelMap::Visitor
		{
			bool visit( rcMapData& data )
			{
				data.layer[ ML_TERRAIN ] = TT_GRASS;
				//data.texTerrain =  mLevelScene.getRandGrassTexture();
				return true;
			}
		} visitor;
		mLevelMap->visitMapData( visitor );
	}




	void removeContruction()
	{

	}


public:// System Message
	bool  onMouse( MouseMsg& msg )
	{
		if ( !rcGUISystem::getInstance().procMouse( msg ) )
			return true;

		if ( msg.onLeftDown() )
		{
			rcViewport& vp = mLevelScene->getViewPort();
			mapPos = vp.transScreenPosToMapPos( msg.getPos() );

			mControl->action( mapPos );

			//mLevelCity->buildContruction( mapPos , mChioceBuildingTag ,  mIdxModel , mSwapAxis );
		}
		else if ( msg.onRightDown() )
		{
			rcViewport& vp = mLevelScene->getViewPort();
			mapPos = vp.transScreenPosToMapPos( msg.getPos() );
			mLevelCity->destoryContruction( mapPos );

			//static TCycleNumber< 2 > start(0);
			//rcViewport& vp = mLevelScene->getViewPort();
			//mapPos = vp.transScreenPosToMapPos( msg.getPos() );

			//if ( !start.getValue() )
			//{
			//	if ( mLevelMap->isVaildRange( mapPos ) )
			//	{

			//		start+= 1;
			//	}
			//}
			//else
			//{
			//	if ( mLevelMap->isVaildRange( mapPos ) )
			//	{
			//		if( mPathFinder.sreach( mapPos ) )
			//		{
			//			rcPathFinder::SNode* node = mPathFinder.getPath();

			//			int num = mPathFinder.getPathNodeNum();

			//			mPath.clear();

			//			while( node )
			//			{
			//				mPath.push_back( node->state );
			//				node = node->child;
			//			}
			//		}
			//		start+= 1;
			//	}
			//}

		}

		return true; 
	}

	typedef std::list< Vec2i > PathType;
	void  renderPath()
	{
		Graphics2D& g = getRenderSystem()->getGraphics2D();

		g.setBrush( ColorKey3( 255 , 0 , 0 ) );
		for( PathType::iterator iter = mPath.begin();
			  iter != mPath.end(); ++iter )
		{
			Vec2i sPos = mLevelScene->getViewPort().transMapPosToScreenPos( *iter );
			g.drawCircle( sPos , 3 );
		}
	}
	bool  onKey( unsigned key , bool isDown )
	{ 
		if ( !isDown )
			return true;


		unsigned buildingList[] =
		{
			BT_WATER_WELL ,
			BT_AQUADUCT ,
			BT_RESEVIOR ,
			BT_HOUSE_SIGN ,
			BT_ROAD ,
			BT_WAREHOUSE ,
			BT_WS_POTTERY , 
			BT_CALY_PIT ,
			BT_MINE ,
		};


		static TCycleNumber< ARRAY_SIZE( buildingList ) > idx(0);

		static TCycleNumber< 4 > dir;
		rcViewport& vp = mLevelScene->getViewPort();

		int moveSpeed = 5;
		switch( key )
		{
		case 'S': vp.shiftScreenOffset( Vec2i( 0 , moveSpeed ) ); break;
		case 'W': vp.shiftScreenOffset( Vec2i( 0 , -moveSpeed ) ); break;
		case 'A': vp.shiftScreenOffset( Vec2i( -moveSpeed , 0 ) ); break;
		case 'D': vp.shiftScreenOffset( Vec2i( moveSpeed , 0 ) ); break;
		case 'Q':
			idx += 1;
			mChioceBuildingTag = buildingList[idx.getValue() ];
			break;
		case 'E':
			idx -= 1;
			mChioceBuildingTag = buildingList[idx.getValue() ];
			break;
		case VK_F1:
			dir += 1;
			mLevelScene->getViewPort().setDir( rcViewport::ViewDir( dir.getValue() ) );
			break;
		case VK_F2:
			ProfileSystem::getInstance().cleanup();
			break;
		}

		return true; 
	}
	bool  onChar( unsigned code ){ return true; }

	bool initTestVariable()
	{
		mLevelCity.reset( new rcLevelCity );
		if ( !mLevelCity->init() )
			return false;

		mControl->setLevelCity( mLevelCity );

		mLevelCity->setupMap( Vec2i( 1024 , 1024 ) );

		mWorkerMgr   = &mLevelCity->getWorkerManager();
		mBuildingMgr = &mLevelCity->getBuildingManager();
		mLevelScene  = mLevelCity->mLevelScene;
		mLevelMap    = &mLevelCity->getLevelMap();

		mLevelCity->mNumSettler = 1000;

		mChioceBuildingTag = BT_ROAD;
		mSwapAxis = false;
		mIdxModel = 0;

		unsigned const R = BT_ROAD;
		unsigned const P = BT_CALY_PIT;
		unsigned const H = BT_HOUSE_SIGN;
		unsigned const A = BT_AQUADUCT;
		unsigned const S = BT_RESEVIOR;
		unsigned const F = BT_FARM_WHEAT;
		unsigned const G = BT_GRANERY;
		unsigned const W = BT_WAREHOUSE;

		unsigned testMap[ 10 ][ 20 ] =
		{
			{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,F ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,0 ,R ,R ,P ,0 ,S ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,H ,R ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,H ,R ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,R ,R ,R ,R ,R ,0 ,A ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ W ,0 ,0 ,R ,0 ,0 ,0 ,0 ,A ,0 ,G ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,0 ,R ,0 ,0 ,S ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,0 ,R ,A ,A ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,0 ,R ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
			{ 0 ,0 ,0 ,R ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		};

		//unsigned testMap[ 10 ][ 20 ] =
		//{
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,F ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },
		//	{ 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 },

		//};


		for( int i = 0 ; i < 10 ; ++ i )
		for( int j = 0 ; j < 20 ; ++ j )
		{
			if ( testMap[i][j] == 0 )
				continue;

			if ( testMap[i][j] == BT_FARM_WHEAT )
			{
				int kk = 1;
			}
			
			mLevelCity->buildContruction( Vec2i(i,j) , testMap[i][j] , 0 , false );
		}


		//for( int i = 2 ; i < mLevelMap->getSizeX() ; i +=3 )
		//{
		//	int j = 0;
		//	for( int j = 0 ; j < mLevelMap->getSizeY(); j += 1 )
		//	{
		//		mLevelCity->buildContruction( Vec2i(i,j) , BT_ROAD , 0 , false );
		//		mLevelCity->buildContruction( Vec2i(j,i) , BT_ROAD , 0 , false );
		//	}
		//}

		//for( int i = 0 ; i < mLevelMap->getSizeX() ; i +=3 )
		//{
		//	//int j = 0;
		//	for( int j = 0 ; j < 30; j += 3 )
		//	{
		//		mLevelCity->buildContruction( Vec2i(i,j) , 
		//			((i+j)&1)? BT_CALY_PIT : BT_CALY_PIT/*:BT_POTTERY_WS*/ , 0 , false );
		//	}

		//	int k = 2;
		//}

		rcWidget* widget;
		widget = rcGUISystem::getInstance().loadUI( UI_CTRL_PANEL );
		rcGUISystem::getInstance().loadUI( UI_BLD_HOUSE_BTN , widget );
		rcGUISystem::getInstance().loadUI( UI_BLD_ROAD_BTN , widget );
		rcGUISystem::getInstance().loadUI( UI_BLD_CLEAR_BTN , widget );

		return true;
	}


private:

	TPtrHolder< rcLevelCity > mLevelCity;
	TPtrHolder< rcRenderSystem > mRenderSystem;
	TPtrHolder< CProfileViewer > mProfileViewer;

	rcWorkerManager*   mWorkerMgr;
	rcBuildingManager* mBuildingMgr;
	rcLevelMap*        mLevelMap;
	rcLevelScene*      mLevelScene;

	TPtrHolder< rcControl > mControl;
	rcWorker* mWorker;
	std::list< Vec2i > mPath;
	unsigned mChioceBuildingTag;
	bool     mSwapAxis;
	unsigned mIdxModel;
	Vec2i    mapPos;
	HFONT    hFont;

	int      mFrame;
	int      indexImage;
};

RomeCityGame game;
int main( int argc , char* argv[] )
{    
	game.setUpdateTime( 15 );
	game.run();
}
