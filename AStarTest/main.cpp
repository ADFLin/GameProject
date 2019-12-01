
#include "GameLoop.h"
#include "WindowsPlatform.h"

#include "ProfileSystem.h"
#include "SystemPlatform.h"
#include "WindowsPlatform.h"

#include "WindowsMessageHander.h"
#include "WinGDIRenderSystem.h"

#include "TQTPortalAStar.h"

#include "Holder.h"

RegionManager g_RegionMgr( Vec2i( 64 , 64 ) );

Vec2i const MapOffset( 20 , 20 );


const int MAP_WIDTH  = 32;
const int MAP_HEIGHT = 32;

int map[ MAP_HEIGHT][ MAP_WIDTH ] =
{
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,9,9,9,1,1,1,1,9,9,9,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,9,1,1,1,1,9,9,9,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,9,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,9,1,1,1,9,9,1,1,1,1,1,1,9,1,1,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,9,1,1,1,9,9,1,1,1,1,1,1,1,9,9,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,9,9,1,1,1,1,9,1,1,1,1,9,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,9,9,9,9,9,9,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,9,9,9,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,9,9,9,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,9,9,9,9,9,9,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,
	9,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	9,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,
	1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,9,1,1,1,1,1,1,1,1,9,1,1,1,1,1,
	1,1,1,1,1,9,9,9,1,1,1,1,1,9,9,9,1,9,1,1,1,1,1,1,1,1,9,9,1,1,1,1,
	1,1,1,1,1,9,9,9,1,1,1,1,9,1,1,1,9,9,1,1,1,1,1,1,1,1,1,9,9,9,9,1,
	1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,9,9,9,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1

};


class AStartTest : public GameLoopT< AStartTest , WindowsPlatform >
	             , public WinFrameT< AStartTest >
	             , public WindowsMessageHandlerT< AStartTest >
{
public:
	void onIdle( long time ){ SystemPlatform::Sleep( time / 2 ); }
	bool onInit()
	{ 
		if ( !WinFrame::create( TEXT("Test") , 800 , 800 , WindowsMessageHandler::MsgProc ) )
			return false;

		mRenderSystem = ( new WinGdiRenderSystem( getHWnd() , getHDC() ) );

		//mRegionMgr.reset( new RegionManager( Vec2i( 64 , 64 ) ) );
		//mRegionMgr = new RegionManager( Vec2i( 64 , 64 ) );

		mRegionMgr = &g_RegionMgr;
		

		for( int j = 0 ; j < 32 ; ++j )
		for( int i = 0 ; i < 32 ; ++i )
		{
			if ( map[i][j] == 9 )
				mRegionMgr->productBlock( Vec2i(i,j) , Vec2i( 1 ,1 ) ); 
		}



		//mRegionMgr->productHole( Vec2i(10,10) , Vec2i(5, 6 ) ); 

		hFont = mRenderSystem->createFont( 8 , TEXT("·s²Ó©úÅé") );


		astar.mManager = mRegionMgr;

		//mRegionMgr->splitMap( Vec2i(0,0) , Vec2i(128,128) );

		curPortal = mRegionMgr->mProtalList.begin();
		curRegion = NULL;
		mCellSize = 4;



		return true; 
	}
	void finalizeGame() CRTP_OVERRIDE {}
	long handleGameUpdate( long shouldTime ) CRTP_OVERRIDE { return shouldTime; }

	typedef RegionManager::RegionList RegionList;
	typedef RegionManager::PortalList PortalList;

	static int const CellLength = 10;

	void handleGameRender() CRTP_OVERRIDE
	{

		WinGdiGraphics2D& g = mRenderSystem->getGraphics2D();

		mRenderSystem->beginRender();


		RegionList& regionList = mRegionMgr->mRegionList;

		g.setBrush( Color3ub( 255 , 255 , 255 ) );
		g.drawRect( Vec2i(0,0) , mRenderSystem->getClientSize() );


		g.setPen( Color3ub( 0 , 0 , 0 ) );

		g.setBrush( Color3ub( 255 , 255 , 0 ) );

		for( Region* region : regionList )
		{
			rect_t& rect = region->getRect();
			g.drawRect( CellLength * rect.getMin()  + MapOffset, CellLength * rect.getSize() );
		}


		g.setPen( Color3ub( 255 , 0 , 0 ), 2 );
		PortalList& pList = mRegionMgr->mProtalList;
		for( Portal* portal : pList )
		{
			if ( portal->dir % 2 )
				g.drawLine( CellLength * Vec2i( portal->range.min , portal->value )+ MapOffset, 
						   CellLength * Vec2i( portal->range.max , portal->value )+ MapOffset);
			else
				g.drawLine( CellLength * Vec2i( portal->value , portal->range.min )+ MapOffset , 
				            CellLength * Vec2i( portal->value , portal->range.max )+ MapOffset);
		}

		if ( curPortal != pList.end() )
		{
			Portal* portal = *curPortal;

			g.setBrush( Color3ub(0 , 255, 255) );

			{
				rect_t& rect = portal->con[0]->getRect();
				g.drawRect( CellLength * rect.getMin()  + MapOffset, CellLength * rect.getSize() );

			}
			{
				rect_t& rect = portal->con[1]->getRect();
				g.drawRect( CellLength * rect.getMin()  + MapOffset , CellLength * rect.getSize() );
			}


			g.setPen( Color3ub( 0 , 0 , 255 ), 2 );
			if ( portal->dir % 2 )
				g.drawLine( CellLength * Vec2i( portal->range.min , portal->value )+ MapOffset, 
				CellLength * Vec2i( portal->range.max , portal->value )+ MapOffset);
			else
				g.drawLine( CellLength * Vec2i( portal->value , portal->range.min )+ MapOffset, 
				CellLength * Vec2i( portal->value , portal->range.max )+ MapOffset);
		}

		g.setBrush( Color3ub(0 , 255, 125 ) );
		for( std::list< AStar::FindState >::iterator iter = path.begin();
			 iter != path.end() ; ++iter )
		{
			rect_t& rect = iter->region->getRect();
			g.drawRect( CellLength * rect.getMin()  + MapOffset , CellLength * rect.getSize() );
		}


		if ( !path.empty() )
		{
			Vec2i prevPos = CellLength * path.front().pos + MapOffset;

			for( auto& state : path )
			{
				Vec2i pos2 = CellLength * state.pos + MapOffset;
				if ( state.portal )
				{
					Vec2i pos1 = pos2;
					int conDir = state.portal->getConnectDir( *state.region );
					int idx = conDir % 2;

					pos1[idx] += ( conDir / 2  == 0 ) ? CellLength : -CellLength;

					g.setPen( Color3ub( 255 , 0 , 0 ) , 3 );
					g.drawLine( prevPos , pos1 );
					

					g.setPen( Color3ub( 255 , 255 , 0 ) , 3 );
					g.drawLine( pos1 , pos2 );
				}
				prevPos = pos2;
			}
		}

		char str[256];
		sprintf( str, "Region Num = %d , Portal Num = %d" ,
			mRegionMgr->mRegionList.size() ,
			mRegionMgr->mProtalList.size() );
		//g.drawText( hFont , Vec2i(0,0) , str );

		mRenderSystem->endRender();


	}


	bool handleCharEvent( unsigned code ) CRTP_OVERRIDE
	{


		return true;
	}
	bool handleMouseEvent( MouseMsg const& msg ) CRTP_OVERRIDE
	{ 
		Vec2i mapPos = ( msg.getPos() - Vec2i( 20 , 20 ) ) / CellLength ;
		if ( msg.onLeftDown() )
		{
			path.clear();
			mRegionMgr->productBlock( mapPos , Vec2i( mCellSize , mCellSize ) );
			curPortal = mRegionMgr->mProtalList.begin();
		}
		else if ( msg.onRightDown() )
		{
			curPos = mapPos;
		}
		return true; 
	}

	bool handleKeyEvent( unsigned key , bool isDown ) CRTP_OVERRIDE
	{
		if ( isDown)
		{
			switch( key )
			{
			case Keyboard::eQ: ++mCellSize; break;
			case Keyboard::eA: --mCellSize; break;
			case Keyboard::eLEFT:
				if ( curPortal != mRegionMgr->mProtalList.begin() )
					--curPortal;
				break;
			case Keyboard::eRIGHT:
				if ( curPortal != mRegionMgr->mProtalList.end() )
					++curPortal;
				break;
			case Keyboard::eW:
				startPos = curPos;
				curRegion = mRegionMgr->getRegion( startPos );
				break;
			case Keyboard::eE:
				mRegionMgr->removeBlock( curPos );
				curPortal = mRegionMgr->mProtalList.begin();
				break;
			case Keyboard::eS:
				endPos = curPos;
				if ( curRegion )
				{
					AStar::FindState state;
					state.pos = startPos;
					state.region = curRegion;
					AStar::PortalAStar::SreachResult sreachResult;
					astar.mEndPos = endPos;
					if ( astar.sreach( state , sreachResult) )
					{
						path.clear();
						astar.constructPath( sreachResult.globalNode ,
							[this](AStar::PortalAStar::NodeType* node)
							{
								path.push_back(node->state);
							}
						);
					}
				}
				break;
			}
		}
		return true;
	}


	PortalList::iterator curPortal;
	WinGdiRenderSystem*   mRenderSystem;
	std::list< AStar::FindState > path;

	RegionManager* mRegionMgr;

	AStar::PortalAStar astar;
	
	Vec2i    curPos;
	Region*  curRegion;
	
	Vec2i    startPos;
	Vec2i    endPos;
	HFONT hFont;
	int   mCellSize;

	//THolder< RegionManager > mRegionMgr;

};


AStartTest game;


int WINAPI WinMain (HINSTANCE hThisInstance,
					HINSTANCE hPrevInstance,
					LPSTR lpszArgument,
					int nFunsterStil)

{
	game.setUpdateTime( 15 );
	game.run();
	return 0;
}
