#include "CmtPCH.h"
#include "CmtScene.h"

#include "CmtDeviceID.h"
#include "CmtDevice.h"
#include "CmtLightTrace.h"

#include "SystemMessage.h"

#include "RenderUtility.h"
#include "GameGlobal.h"

namespace Chromatron
{
	int const  CellLength = 28;
	int const  StorageMapCellGapLen = 3;
	int const  StorageMapOffset     = CellLength + StorageMapCellGapLen;
	int const  StorageMapRowSize    = 3;

	Vec2i const DefaultWorldMapPos  = Vec2i( 140 , 80 );
	Vec2i const WorldMapSize  = Vec2i( Level::MapSize * CellLength , Level::MapSize * CellLength );
	Vec2i const StorageMapPosOffset = Vec2i( Level::MapSize * CellLength  + 30 , 80 );
	Vec2i const StorageMapSize = Vec2i( StorageMapRowSize * StorageMapOffset  , 
		                          Level::MaxNumUserDC / StorageMapRowSize * StorageMapOffset );

	Vec2i const CellSize  = Vec2i( CellLength , CellLength );
	Vec2i const CMToolPos = Vec2i( 100 , 550 );
	int   const CMToolGap = 3;
	Vec2i const CMEditColorPos = Vec2i( 100 , 550 );
	int   const CMEditColorGap = 3;

	static int const gColorMap[] = 
	{
		//      000           001            010              011
		EColor::Null , EColor::Red , EColor::Green , EColor::Yellow ,
		//      100           101            110              111
		EColor::Blue , EColor::Pink ,EColor::Cyan  , EColor::White ,
	};

	Scene::Scene() 
		:mWorldPos( DefaultWorldMapPos )
	{
		mLevel = nullptr;
		mIdCreateDC = ErrorDeviceId;
	}

	void Scene::reset()
	{
		mDownDC = mUpDC = mDragDC = nullptr;
		mNeedUpdateLevel = true;
		mIdCreateDC = ErrorDeviceId;
		mEditColor = COLOR_W;
	}

	void Scene::setupLevel( Level& level , bool bCreationMode )
	{
		mLevel = &level;
		mbCreationMode = bCreationMode;
		reset();
	}

	void Scene::tick()
	{
		if ( mNeedUpdateLevel )
		{
			getLevel().updateWorld();
			//::Msg( "Light Count = %d " , getLevel().getWorld().getLightCount() );
			mNeedUpdateLevel = false;
		}
	}

	void Scene::updateFrame( int frame )
	{

	}

	void Scene::render( Graphics2D& g )
	{
		drawWorld( g , mWorldPos , getLevel().getWorld() );

		drawStorage(g , mWorldPos + StorageMapPosOffset , getLevel() );

		if ( mbCreationMode )
		{
			RenderUtility::SetPen( g , EColor::Gray );
			RenderUtility::SetBrush( g , EColor::Blue , COLOR_LIGHT );
			g.drawRoundRect( CMToolPos - Vec2i( 10 , 10 ) ,
				Vec2i( DC_DEVICE_NUM * ( CellLength + CMToolGap ) - CMToolGap + 2 * 10 , CellLength + 2 * 10 ) ,
				Vec2i( 10 , 10 ) );
			for( int i = 0 ; i < DC_DEVICE_NUM ; ++i )
			{
				Vec2i pos = CMToolPos + Vec2i( i * ( CellLength + CMToolGap ) , 0 ) + CellSize / 2;
				drawDevice( g , pos , DeviceId( i ) , Dir( 0 ) , COLOR_W , 0 );
			}

			if ( mIdCreateDC != ErrorDeviceId )
			{
				drawDevice( g , mLastMousePos , mIdCreateDC , Dir( 0 ) , COLOR_W , 0 );
			}
		}

		if ( mDragDC )
		{
			drawDevice( g , mLastMousePos , *mDragDC );
		}
	}

	Device* Scene::getDevice( Vec2i const& pos )
	{
		Vec2i cPos;
		Level::PosType pt = getCellPos( pos.x , pos.y , cPos );
		if ( pt == Level::PT_NONE )
			return nullptr;
		return getLevel().getDevice( pt , cPos );
	}

	void Scene::procMouseMsg( MouseMsg const& msg )
	{
		mLastMousePos = msg.getPos();

		if ( msg.onLeftDown() )
		{
			mDownDC = getDevice( msg.getPos() );
			if ( mbCreationMode )
			{
				if ( mDownDC == nullptr )
				{
					mIdCreateDC = getToolDevice( msg.getPos() );
					if ( mIdCreateDC == ErrorDeviceId )
					{
						Color color = getEditColor( msg.getPos() );
					}
				}
			}
		}
		else if ( msg.onLeftUp() )
		{
			mUpDC = getDevice( msg.getPos() );

			if( mDownDC && mDownDC == mUpDC ) 
			{
				mDownDC->rotate( Dir( 1 ) );
				mNeedUpdateLevel = true;
			}
			else if ( mDragDC )
			{
				Vec2i cPos;
				Level::PosType pt = getCellPos( msg.x() , msg.y() , cPos );
				bool haveMoveDC;
				if(  mUpDC == nullptr )
					haveMoveDC = getLevel().moveDevice( *mDragDC , pt , cPos );
				else
					haveMoveDC = false;

				if  ( !haveMoveDC )
				{
					if ( !getLevel().moveDevice( *mDragDC , mOldPos , mOldInWorld ) )
					{


					}
				}

				mDragDC = nullptr;
				mNeedUpdateLevel = true;
			}
			else if ( mbCreationMode && mIdCreateDC != ErrorDeviceId )
			{
				Vec2i cPos;
				Level::PosType pt = getCellPos( msg.x() , msg.y() , cPos );
				if ( getLevel().createDevice( mIdCreateDC , pt , cPos , Dir(0) , COLOR_W , true ) )
				{
					mNeedUpdateLevel = true;
				}
				mIdCreateDC = ErrorDeviceId;
			}

			mDownDC = nullptr;
		}
		else if ( msg.onRightDown() )
		{
			Device* dc = getDevice( msg.getPos() );
			if(dc) 
			{
				dc->rotate( Dir( -1 ) );
				mNeedUpdateLevel = true;
			}
		}
		else if ( msg.onMoving() )
		{
			if ( mDragDC == nullptr )
			{
				if ( mDownDC )
				{
					if ( getDevice( msg.getPos() ) != mDownDC &&
						!mDownDC->isStatic() )
					{
						mOldPos     = mDownDC->getPos();
						mOldInWorld = mDownDC->isInWorld();
						mDragDC     = mDownDC;

						getLevel().uninstallDevice( *mDragDC );
						mNeedUpdateLevel = true;
					}
				}
			}
		}
	}

	void Scene::drawDevice( Graphics2D& g , Vec2i const& pos , DeviceId id , Dir dir , Color color , unsigned flag )
	{
		g.beginXForm();
		g.translateXForm( pos.x , pos.y );
		if ( dir != 0 )
			g.rotateXForm( -DEG2RAD( 45 * dir ) );
		switch( id )
		{
		case DC_PINWHEEL:
#define DRAW_RECT( size_ ) g.drawRect( -( size_ ) / 2 , ( size_ ) )
#define DRAW_RECT2( size_ , offset_ ) g.drawRect( -( size_ ) / 2 + ( offset_ ) , ( size_ ) )

			if ( color != COLOR_NULL )
			{
				if ( flag & DFB_GOAL )
				{
					RenderUtility::SetPen( g , EColor::White );
					g.setBrush( Color3ub( ( color & COLOR_R ) ? 255 : 80  ,
						( color & COLOR_G ) ? 255 : 80  ,
						( color & COLOR_B ) ? 255 : 80  ) );
					g.drawCircle( Vec2i(0,0) , 9 );
				}
				else
				{
					RenderUtility::SetPen( g , EColor::Black );
					RenderUtility::SetBrush( g , gColorMap[ color ] , COLOR_DEEP );
					g.drawCircle( Vec2i(0,0) , 8 );
				}
			}
			else
			{
				if ( flag & DFB_GOAL )
				{
					RenderUtility::SetPen( g , EColor::Black );
					RenderUtility::SetBrush( g , EColor::Black );
					g.drawCircle( Vec2i(0,0) , 9 );
				}
				else
				{
					RenderUtility::SetPen( g , EColor::White );
					g.setBrush( Color3ub( 30 , 30 , 30 ) );
					g.drawCircle( Vec2i(0,0) , 8 );
				}
			}
			break;
		case DC_SMIRROR_45:
			g.rotateXForm( -DEG2RAD( 22.5 ) );
		case DC_SMIRROR:
			{
				Vec2i size( 4 , 20 );
				RenderUtility::SetPen( g , EColor::Black );
				g.setBrush( Color3ub( 200 , 200 , 200 ) );
				g.drawRect( Vec2i( - size.x / 2 + 1 , 0 ) - size / 2 , size );
				Vec2i size2( 2 , 20 );
				RenderUtility::SetBrush( g , EColor::Black );
				g.drawRect( Vec2i( - size2.x / 2 - size.x + 1 , 0 ) - size2 / 2 , size2 );
			}
			break;
		case DC_LIGHTSOURCE:
			{
				Vec2i size( 13 , 4 );
				RenderUtility::SetPen( g , EColor::Black );
				g.setBrush( Color3ub( 200 , 200 , 200 ) );
				DRAW_RECT2( size , Vec2i( 5 , 0 ) );

				g.drawCircle( Vec2i( -size.x / 2 + 2 , 0 ) , 7 );
				RenderUtility::SetBrush( g , gColorMap[ color ] );
				g.drawCircle( Vec2i( -size.x / 2 + 2 , 0 ) , 5 );
			}
			break;
		case DC_SPECTROSCOPE:
			{
				Vec2i size( 4 , 24 );
				RenderUtility::SetPen( g , EColor::Black );
				g.setBrush( Color3ub( 200 , 200 , 200 ) );
				g.drawRoundRect( - size / 2 , size , Vec2i( 4 , 4 ) );
			}
			break;
		case DC_TELEPORTER:
			{
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::White );
				Vec2i size( 4 , 24 );
				Vec2i size2( 24 , 4 );
				DRAW_RECT( size );
				DRAW_RECT( size2 );
				g.rotateXForm( DEG2RAD( 45 ) );
				DRAW_RECT( size );
				DRAW_RECT( size2 );
				g.drawCircle( Vec2i(0,0) , 9 );
				g.drawCircle( Vec2i(0,0) , 7 );
			}
			break;
		case DC_CONDUITS:
			{
				Vec2i size( 24 , 2 );
				Vec2i size2( 24 , 6 );
				Vec2i offset( 0 , 3 );
				//RenderUtility::setPen( g , EColor::Null );
				//g.setBrush( ColorKey3( 130 , 130 , 130 ) );
				//g.drawRect( - size2 / 2 , size2 );

				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::Black );
				DRAW_RECT2( size , offset );
				DRAW_RECT2( size , -offset );
			}
			break;
		case DC_PRISM:
			{
				Vec2i vtx[] = { Vec2i( 5 , 0 ) , Vec2i( 5 , 10 ) , Vec2i( -5 , -10 ) };
				RenderUtility::SetPen( g , EColor::Black );
				g.setBrush( Color3ub( 200 , 200 , 200 ) );
				g.drawPolygon( vtx , 3 );
			}
			break;
		case DC_FILTER:
			{
				Vec2i size( 4 , 16 );
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::Black );
				g.drawCircle( Vec2i( 0 , size.y / 2 ) , 2 );
				g.drawCircle( Vec2i( 0 , -size.y / 2 ) , 2 );
				RenderUtility::SetBrush( g , gColorMap[ color ] );
				DRAW_RECT( size );
			}
			break;
		case DC_QTANGLER:
			{
				Vec2i size( 4 , 20 );
				Vec2i size2( 10 , 6 );

				RenderUtility::SetPen( g , EColor::Black );
				g.setBrush( Color3ub( 50 , 50 , 50 ) );
				DRAW_RECT( size );
				g.drawRect( Vec2i( 0 ,  -size2.y / 2  ) , size2 );
				g.drawCircle( Vec2i( -1 , 0 ) , 8 );
			}
			break;
		case DC_DOPPLER:
			{
				Vec2i size( 20 , 6 );
				RenderUtility::SetPen( g , EColor::Black );
				g.setBrush( Color3ub( 200 , 200 , 200 ) );
				DRAW_RECT( size );
				g.drawRect( - size / 2 , size );
				Vec2i size2( 4 , 6 );
				Vec2i offset2( 6 , 0 );
				RenderUtility::SetBrush( g , EColor::Red );
				DRAW_RECT2( size2 , -offset2 );
				RenderUtility::SetBrush( g , EColor::Green );
				DRAW_RECT( size2 );
				RenderUtility::SetBrush( g , EColor::Blue );
				DRAW_RECT2( size2 , offset2 );
			}
			break;
		case  DC_SPINSPLITTER:
			{
				RenderUtility::SetPen(g, EColor::Black);
				g.setBrush(Color3ub(200, 200, 200));
				DRAW_RECT(Vec2i(10,10));
			}
			break;
		case DC_MULTIFILTER:
			{
				Vec2i size( 4 , 7 );
				Vec2i offset( 10 , 0 ) ;

				RenderUtility::SetPen( g , EColor::Black );

				Color color[ ] = { EColor::White , EColor::Red , EColor::Green , EColor::Blue };
				for( int i = 0 ; i < 4 ; ++i )
				{
					RenderUtility::SetBrush( g , color[ i ] );

					DRAW_RECT2( size , offset );
					DRAW_RECT2( size , -offset );
					g.rotateXForm( -DEG2RAD( 45 ) );
				}
			}
			break;
		case DC_TWISTER:
			{
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::White );

				g.drawCircle( Vec2i(0,0) , 12 );

				int len = 8;
				for( int i = 0 ; i < 4 ; ++i )
				{
					g.drawLine( Vec2i( -len , 0 ) , Vec2i( len , 0 ) );
					g.drawLine( Vec2i( len , 0 ) , Vec2i( len , 3 ) );
					g.drawLine( Vec2i( -len , 0 ) , Vec2i( -len , -3 ) );
					g.rotateXForm( -DEG2RAD( 45 ) );
				}
			}
			break;
		case DC_COMPLEMENTOR:
			{
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::White );

				Vec2i size2a( 20 , 4 );
				Vec2i size2b(  4 , 20);
				g.drawRect( -size2a / 2 , size2a );
				g.drawRect( -size2b / 2 , size2b );

				Vec2i size1( 8 , 8 );
				g.drawRect( -size1 / 2 , size1 );
			}
			break;
		case DC_QUADBENDER:
			g.rotateXForm( -DEG2RAD( 22.5 ) );
		case DC_DUALREFLECTOR:
			{
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::Black );

				Vec2i size1( 2 , 22 );
				DRAW_RECT( size1 );
				Vec2i size2( 6 , 2 );
				Vec2i offset2( 0 , 10 );
				DRAW_RECT2( size2 , offset2 );
				DRAW_RECT2( size2 , -offset2 );

				g.setBrush( Color3ub( 200 , 200 , 200 ) );

				Vec2i size3( 4 , 20 );
				Vec2i offset3( 3 , 0 );
				DRAW_RECT2( size3 , offset3 );
				DRAW_RECT2( size3 , -offset3 );
			}
			break;
		case DC_LOGICGATE_AND_PRIMARY:
		case DC_LOGICGATE_AND:
			{
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::Gray );

				Vec2i size( 12 , 4 );
				DRAW_RECT2( size , Vec2i( -5 , 0 ) );

				RenderUtility::SetBrush( g , EColor::White );
				g.rotateXForm( DEG2RAD( -45 ) );

				int w = 4;
				int len = 12;
				DRAW_RECT2( Vec2i( w , len ) , Vec2i( 0 , len / 2 ) );
				DRAW_RECT2( Vec2i( len , w ) , Vec2i( len / 2 , 0 ) );

				g.rotateXForm( DEG2RAD( 45 ) );

				if ( id == DC_LOGICGATE_AND &&
					 id == DC_LOGICGATE_AND_PRIMARY )
				{
					g.drawCircle( Vec2i( 2 ,0) , 7 );
					int len2 = 6;
					g.drawLine( Vec2i( 2 , len2 - 1 ) , Vec2i( 2, -len2 ) );
					g.drawLine( Vec2i( 2 + len2 - 1 , 0 ) , Vec2i( 2 - len2 , 0 ) );
				}
				else
				{
					DRAW_RECT2( Vec2i( 8 , 8 ) , Vec2i( 2 , 0 ) );
				}
			}
			break;
		case DC_STARBURST:
			{
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , EColor::White );
				int w = 5;
				int h = 24;
				for( int i = 0 ; i < 2 ; ++i )
				{
					DRAW_RECT( Vec2i( w , h ) );
					DRAW_RECT( Vec2i( h , w ) );
					g.rotateXForm( DEG2RAD(45) );
				}

				RenderUtility::SetBrush( g , gColorMap[ COLOR_CB ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 9 );
				RenderUtility::SetBrush( g , gColorMap[ COLOR_CG ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 7 );
				RenderUtility::SetBrush( g , gColorMap[ COLOR_CR ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 5 );
				RenderUtility::SetBrush( g , gColorMap[ COLOR_W ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 3 );

			}
			break;
		default:
			{
				Vec2i size( 20 , 20 );
				RenderUtility::SetPen( g , EColor::Black );
				RenderUtility::SetBrush( g , id % 8 + 1 );
				g.drawRect( - size / 2 , size );

				g.finishXForm();
				FixString< 32 > str;
				RenderUtility::SetFont( g , FONT_S12 );
				g.setTextColor( Color3ub(255 , 255 , 255) );
				str.format( "%d" , id );
				g.drawText( pos - CellSize / 2 , CellSize , str );
				g.beginXForm();
			}
#undef DRAW_RECT
#undef DRAW_RECT2
		}
		g.finishXForm();

	}
	void Scene::drawDevice( Graphics2D& g , Vec2i const& pos , Device const& dc )
	{
		drawDevice( g , pos , dc.getId() , dc.getDir() , dc.getColor() , dc.getFlag() );
	}

	void Scene::drawWorld( Graphics2D& g , Vec2i const& pos , World const& world)
	{
		for (int i = world.getMapSizeX()-1; i >= 0 ; --i)
		{
			for (int j = 0; j<world.getMapSizeY(); ++j )
			{
				Tile const& tile = world.getTile( Vec2i(i,j) );

				Vec2i cellPos = pos + CellLength * Vec2i( i , j );

				switch( tile.getType() )
				{
				case MT_EMPTY:
					RenderUtility::SetPen( g , EColor::Black );
					g.setBrush( Color3ub( 130 , 130 , 130 ) );
					g.drawRect( cellPos , CellSize + Vec2i(1,1) );
					break;
				case MT_BLOCK:
					RenderUtility::DrawBlock( g , cellPos , CellSize , EColor::Gray );
					break;
				case MT_HOLE:
					break;
				}

				Device const* dc = tile.getDevice();


				Vec2i centerPos  = cellPos + CellSize / 2;

				if( dc  )
				{
					if ( dc->getFlag().checkBits( DFB_DRAW_FIRST )  )
					{
						drawDevice( g , centerPos , *dc );
						drawLight( g , centerPos ,tile );
					}
					else
					{
						drawLight( g , centerPos ,tile );
						drawDevice( g , centerPos , *dc );
					}
				}
				else
				{
					drawLight( g , centerPos , tile );
				}
			}
		}
	}

	void Scene::drawStorage( Graphics2D &g , Vec2i const& pos , Level const& level )
	{
		int const cellOffset = CellLength + StorageMapCellGapLen;
		for( int i = 0 ; i < Level::MaxNumUserDC ; ++i )
		{
			Device const* dc = level.getStorageDevice( i );

			int x = ( i % StorageMapRowSize ) * cellOffset;
			int y = ( i / StorageMapRowSize ) * cellOffset;

			Vec2i cellPos = pos + Vec2i( x , y );

			RenderUtility::SetPen( g , EColor::Black );
			g.setBrush( Color3ub( 150 , 150 , 150 ) );

			g.drawRoundRect( cellPos , CellSize + Vec2i(1,1) , Vec2i( 8 , 8 ) );

			if ( dc )
				drawDevice( g , cellPos + CellSize / 2 , *dc );
		}
	}

	void Scene::drawLight( Graphics2D& g , Vec2i const& pos , Tile const& tile )
	{
		for(int i = 0; i < Dir::RestNumber ; ++i)
		{
			Dir dir = Dir::ValueChecked( i );
			int color = tile.getLightPathColor( dir );

			if ( color == COLOR_NULL ) 
				continue;

			Vec2i p2 = pos + ( (CellLength / 2) + 1 ) * LightTrace::GetDirOffset( dir );
			RenderUtility::SetPen( g , gColorMap[ color ] );
			g.drawLine( pos , p2 );
		}
	}

	void Scene::restart()
	{
		getLevel().restart();
		if ( mbCreationMode )
		{
			getLevel().clearData();

		}
	}

	Level::PosType Scene::getCellPos( int x , int y , Vec2i& result )
	{
		Vec2i pos( x , y );

		pos -= mWorldPos;

		if ( IsInRect( pos , Vec2i(0,0) , WorldMapSize ) )
		{	
			result = pos / CellLength;
			return Level::PT_WORLD;
		}
		else if ( IsInRect( pos , StorageMapPosOffset , StorageMapSize ) )
		{
			pos -= StorageMapPosOffset;

			Vec2i temp( pos.x % StorageMapOffset , 
				        pos.y % StorageMapOffset );

			if ( IsInRect( temp , Vec2i(0,0) , Vec2i( CellLength , CellLength ) ) )
			{
				int idx = pos.x / StorageMapOffset + StorageMapRowSize * ( pos.y / StorageMapOffset );
				assert( idx < Level::MaxNumUserDC );
				result.x = idx;
				result.y = 0;
				return Level::PT_STROAGE;
			}
		}

		return Level::PT_NONE;
	}

	DeviceId Scene::getToolDevice( Vec2i const& pos )
	{
		Vec2i const ToolRect = Vec2i( ( CellLength + CMToolGap ) * DC_DEVICE_NUM , CellLength );
		if ( !IsInRect( pos , CMToolPos , ToolRect ) )
			return ErrorDeviceId;

		Vec2i lPos = pos - CMToolPos;
		int offset = lPos.x % ( CellLength + CMToolGap );
		if ( offset > CellLength )
			return ErrorDeviceId;

		return DeviceId( lPos.x / ( CellLength + CMToolGap ) );
	}

	Color Scene::getEditColor( Vec2i const& pos )
	{

		return COLOR_NULL;
	}

}//namespace Chromatron