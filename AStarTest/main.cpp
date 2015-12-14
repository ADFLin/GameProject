
#include "GameLoop.h"
#include "Win32Platform.h"

#include "ProfileSystem.h"
#include "Win32Platform.h"

#include "SysMsgHandler.h"
#include "WinGDIRenderSystem.h"

#include "TQTPortalAStar.h"

#include "THolder.h"

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


class AStartTest : public GameLoopT< AStartTest , Win32Platform >
	             , public WinFrameT< AStartTest >
	             , public SysMsgHandlerT< AStartTest >
{
public:
	void onIdle( long time ){ ::Sleep( time / 2 ); }
	bool onInit()
	{ 
		if ( !WinFrame::create( TEXT("Test") , 800 , 800 , SysMsgHandler::MsgProc ) )
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
	void onEnd(){}
	long onUpdate( long shouldTime ){ return shouldTime; }

	typedef RegionManager::RegionList RegionList;
	typedef RegionManager::PortalList PortalList;

	static int const CellLength = 10;

	void onRender()
	{

		WinGdiGraphics2D& g = mRenderSystem->getGraphics2D();

		mRenderSystem->beginRender();


		RegionList& regionList = mRegionMgr->mRegionList;

		g.setBrush( ColorKey3( 255 , 255 , 255 ) );
		g.drawRect( Vec2i(0,0) , mRenderSystem->getClientSize() );


		g.setPen( ColorKey3( 0 , 0 , 0 ) );

		g.setBrush( ColorKey3( 255 , 255 , 0 ) );

		for( RegionList::iterator iter = regionList.begin();
			 iter != regionList.end() ; ++iter )
		{
			rect_t& rect = (*iter)->getRect();
			g.drawRect( CellLength * rect.getMin()  + MapOffset, CellLength * rect.getSize() );
		}


		g.setPen( ColorKey3( 255 , 0 , 0 ), 2 );
		PortalList& pList = mRegionMgr->mProtalList;
		for( PortalList::iterator iter = pList.begin() ;
			  iter != pList.end() ; ++iter )
		{
			Portal* portal = *iter;
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

			g.setBrush( ColorKey3(0 , 255, 255) );

			{
				rect_t& rect = portal->con[0]->getRect();
				g.drawRect( CellLength * rect.getMin()  + MapOffset, CellLength * rect.getSize() );

			}
			{
				rect_t& rect = portal->con[1]->getRect();
				g.drawRect( CellLength * rect.getMin()  + MapOffset , CellLength * rect.getSize() );
			}


			g.setPen( ColorKey3( 0 , 0 , 255 ), 2 );
			if ( portal->dir % 2 )
				g.drawLine( CellLength * Vec2i( portal->range.min , portal->value )+ MapOffset, 
				CellLength * Vec2i( portal->range.max , portal->value )+ MapOffset);
			else
				g.drawLine( CellLength * Vec2i( portal->value , portal->range.min )+ MapOffset, 
				CellLength * Vec2i( portal->value , portal->range.max )+ MapOffset);
		}

		g.setBrush( ColorKey3(0 , 255, 125 ) );
		for( std::list< AStar::FindState >::iterator iter = path.begin();
			 iter != path.end() ; ++iter )
		{
			rect_t& rect = iter->region->getRect();
			g.drawRect( CellLength * rect.getMin()  + MapOffset , CellLength * rect.getSize() );
		}


		if ( !path.empty() )
		{
			

			Vec2i prevPos = CellLength * path.front().pos + MapOffset;

			for( std::list< AStar::FindState >::iterator iter = path.begin();
				iter != path.end() ; ++iter )
			{
				Vec2i pos2 = CellLength * iter->pos + MapOffset;
				if ( iter->portal )
				{
					Vec2i pos1 = pos2;
					int conDir = iter->portal->getConnectDir( *iter->region );
					int idx = conDir % 2;

					pos1[idx] += ( conDir / 2  == 0 ) ? CellLength : -CellLength;

					g.setPen( ColorKey3( 255 , 0 , 0 ) , 3 );
					g.drawLine( prevPos , pos1 );
					

					g.setPen( ColorKey3( 255 , 255 , 0 ) , 3 );
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


	bool onChar( unsigned code )
	{


		return true;
	}
	bool onMouse( MouseMsg& msg )
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

	bool onKey( unsigned key , bool isDown )
	{
		if ( isDown)
		{
			switch( key )
			{
			case 'Q': ++mCellSize; break;
			case 'A': --mCellSize; break;
			case VK_LEFT : 
				if ( curPortal != mRegionMgr->mProtalList.begin() )
					--curPortal;
				break;
			case VK_RIGHT:
				if ( curPortal != mRegionMgr->mProtalList.end() )
					++curPortal;
				break;
			case 'W':
				startPos = curPos;
				curRegion = mRegionMgr->getRegion( startPos );
				break;
			case 'E':
				mRegionMgr->removeBlock( curPos );
				curPortal = mRegionMgr->mProtalList.begin();
				break;
			case 'S':
				endPos = curPos;
				if ( curRegion )
				{
					AStar::FindState state;
					state.pos = startPos;
					state.region = curRegion;

					astar.mEndPos = endPos;
					if ( astar.sreach( state ) )
					{
						AStar::TPortalAStar::NodeType* node = astar.getPath();

						path.clear();

						while( node )
						{
							path.push_back( node->state );
							node = node->child;
						}
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

	AStar::TPortalAStar astar;
	
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
}
