#include "CmtPCH.h"
#include "CmtScene.h"

#include "CmtDeviceID.h"
#include "CmtDevice.h"
#include "CmtLightTrace.h"

#include "SysMsg.h"

#include "RenderUtility.h"
#include "GameGlobal.h"

namespace Chromatron
{
	int const  CellLength = 28;
	int const  StorageMapCellGapLen = 3;
	int const  StorageMapOffset     = CellLength + StorageMapCellGapLen;
	int const  StorageMapRowSize    = 3;

	Vec2D const DefaultWorldMapPos  = Vec2D( 140 , 80 );
	Vec2D const WorldMapSize  = Vec2D( Level::MapSize * CellLength , Level::MapSize * CellLength );
	Vec2D const StorageMapPosOffset = Vec2D( Level::MapSize * CellLength  + 30 , 80 );
	Vec2D const StorageMapSize = Vec2D( StorageMapRowSize * StorageMapOffset  , 
		                          Level::MaxNumUserDC / StorageMapRowSize * StorageMapOffset );

	Vec2D const CellSize  = Vec2D( CellLength , CellLength );
	Vec2D const CMToolPos = Vec2D( 100 , 550 );
	int   const CMToolGap = 3;
	Vec2D const CMEditColorPos = Vec2D( 100 , 550 );
	int   const CMEditColorGap = 3;

	static int const gColorMap[] = 
	{
		//      000           001            010              011
		Color::eNull , Color::eRed , Color::eGreen , Color::eYellow ,
		//      100           101            110              111
		Color::eBlue , Color::ePink ,Color::eCyan  , Color::eWhite ,
	};

	Scene::Scene() 
		:mWorldPos( DefaultWorldMapPos )
	{
		mLevel = NULL;
		mIdCreateDC = ErrorDeviceId;
	}

	void Scene::reset()
	{
		mDownDC = mUpDC = mDragDC = NULL;
		mNeedUpdateLevel = true;
		mIdCreateDC = ErrorDeviceId;
		mEditColor = COLOR_W;
	}

	void Scene::setupLevel( Level& level , bool beCreateMode )
	{
		mLevel = &level;
		mbCreateMdoe = beCreateMode;
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

		if ( mbCreateMdoe )
		{
			RenderUtility::setPen( g , Color::eGray );
			RenderUtility::setBrush( g , Color::eBlue , COLOR_LIGHT );
			g.drawRoundRect( CMToolPos - Vec2D( 10 , 10 ) ,
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

	Device* Scene::getDevice( Vec2D const& pos )
	{
		Vec2D cPos;
		Level::PosType pt = getCellPos( pos.x , pos.y , cPos );
		if ( pt == Level::PT_NONE )
			return NULL;
		return getLevel().getDevice( pt , cPos );
	}

	void Scene::procMouseMsg( MouseMsg const& msg )
	{
		mLastMousePos = msg.getPos();

		if ( msg.onLeftDown() )
		{
			mDownDC = getDevice( msg.getPos() );
			if ( mbCreateMdoe )
			{
				if ( mDownDC == NULL )
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
				Vec2D cPos;
				Level::PosType pt = getCellPos( msg.x() , msg.y() , cPos );
				bool haveMoveDC;
				if(  mUpDC == NULL )
					haveMoveDC = getLevel().moveDevice( *mDragDC , pt , cPos );
				else
					haveMoveDC = false;

				if  ( !haveMoveDC )
				{
					if ( !getLevel().moveDevice( *mDragDC , mOldPos , mOldInWorld ) )
					{


					}
				}

				mDragDC = NULL;
				mNeedUpdateLevel = true;
			}
			else if ( mbCreateMdoe && mIdCreateDC != ErrorDeviceId )
			{
				Vec2D cPos;
				Level::PosType pt = getCellPos( msg.x() , msg.y() , cPos );
				if ( getLevel().createDevice( mIdCreateDC , pt , cPos , Dir(0) , COLOR_W , true ) )
				{
					mNeedUpdateLevel = true;
				}
				mIdCreateDC = ErrorDeviceId;
			}

			mDownDC = NULL;
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
			if ( mDragDC == NULL )
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
					RenderUtility::setPen( g , Color::eWhite );
					g.setBrush( ColorKey3( ( color & COLOR_R ) ? 255 : 80  ,
						( color & COLOR_G ) ? 255 : 80  ,
						( color & COLOR_B ) ? 255 : 80  ) );
					g.drawCircle( Vec2i(0,0) , 9 );
				}
				else
				{
					RenderUtility::setPen( g , Color::eBlack );
					RenderUtility::setBrush( g , gColorMap[ color ] , COLOR_DEEP );
					g.drawCircle( Vec2i(0,0) , 8 );
				}
			}
			else
			{
				if ( flag & DFB_GOAL )
				{
					RenderUtility::setPen( g , Color::eBlack );
					RenderUtility::setBrush( g , Color::eBlack );
					g.drawCircle( Vec2i(0,0) , 9 );
				}
				else
				{
					RenderUtility::setPen( g , Color::eWhite );
					g.setBrush( ColorKey3( 30 , 30 , 30 ) );
					g.drawCircle( Vec2i(0,0) , 8 );
				}
			}
			break;
		case DC_SMIRROR_45:
			g.rotateXForm( -DEG2RAD( 22.5 ) );
		case DC_SMIRROR:
			{
				Vec2i size( 4 , 20 );
				RenderUtility::setPen( g , Color::eBlack );
				g.setBrush( ColorKey3( 200 , 200 , 200 ) );
				g.drawRect( Vec2i( - size.x / 2 + 1 , 0 ) - size / 2 , size );
				Vec2i size2( 2 , 20 );
				RenderUtility::setBrush( g , Color::eBlack );
				g.drawRect( Vec2i( - size2.x / 2 - size.x + 1 , 0 ) - size2 / 2 , size2 );
			}
			break;
		case DC_LIGHTSOURCE:
			{
				Vec2i size( 13 , 4 );
				RenderUtility::setPen( g , Color::eBlack );
				g.setBrush( ColorKey3( 200 , 200 , 200 ) );
				DRAW_RECT2( size , Vec2i( 5 , 0 ) );

				g.drawCircle( Vec2i( -size.x / 2 + 2 , 0 ) , 7 );
				RenderUtility::setBrush( g , gColorMap[ color ] );
				g.drawCircle( Vec2i( -size.x / 2 + 2 , 0 ) , 5 );
			}
			break;
		case DC_SPECTROSCOPE:
			{
				Vec2i size( 4 , 24 );
				RenderUtility::setPen( g , Color::eBlack );
				g.setBrush( ColorKey3( 200 , 200 , 200 ) );
				g.drawRoundRect( - size / 2 , size , Vec2i( 4 , 4 ) );
			}
			break;
		case DC_TELEPORTER:
			{
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eWhite );
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
				//RenderUtility::setPen( g , Color::eNull );
				//g.setBrush( ColorKey3( 130 , 130 , 130 ) );
				//g.drawRect( - size2 / 2 , size2 );

				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eBlack );
				DRAW_RECT2( size , offset );
				DRAW_RECT2( size , -offset );
			}
			break;
		case DC_PRISM:
			{
				Vec2i vtx[] = { Vec2i( 5 , 0 ) , Vec2i( 5 , 10 ) , Vec2i( -5 , -10 ) };
				RenderUtility::setPen( g , Color::eBlack );
				g.setBrush( ColorKey3( 200 , 200 , 200 ) );
				g.drawPolygon( vtx , 3 );
			}
			break;
		case DC_FILTER:
			{
				Vec2i size( 4 , 16 );
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eBlack );
				g.drawCircle( Vec2i( 0 , size.y / 2 ) , 2 );
				g.drawCircle( Vec2i( 0 , -size.y / 2 ) , 2 );
				RenderUtility::setBrush( g , gColorMap[ color ] );
				DRAW_RECT( size );
			}
			break;
		case DC_QTANGLER:
			{
				Vec2i size( 4 , 20 );
				Vec2i size2( 10 , 6 );

				RenderUtility::setPen( g , Color::eBlack );
				g.setBrush( ColorKey3( 50 , 50 , 50 ) );
				DRAW_RECT( size );
				g.drawRect( Vec2i( 0 ,  -size2.y / 2  ) , size2 );
				g.drawCircle( Vec2i( -1 , 0 ) , 8 );
			}
			break;
		case DC_DOPPLER:
			{
				Vec2i size( 20 , 6 );
				RenderUtility::setPen( g , Color::eBlack );
				g.setBrush( ColorKey3( 200 , 200 , 200 ) );
				DRAW_RECT( size );
				g.drawRect( - size / 2 , size );
				Vec2i size2( 4 , 6 );
				Vec2i offset2( 6 , 0 );
				RenderUtility::setBrush( g , Color::eRed );
				DRAW_RECT2( size2 , -offset2 );
				RenderUtility::setBrush( g , Color::eGreen );
				DRAW_RECT( size2 );
				RenderUtility::setBrush( g , Color::eBlue );
				DRAW_RECT2( size2 , offset2 );
			}
			break;
		case DC_MULTIFILTER:
			{
				Vec2i size( 4 , 7 );
				Vec2i offset( 10 , 0 ) ;

				RenderUtility::setPen( g , Color::eBlack );

				Color color[ ] = { Color::eWhite , Color::eRed , Color::eGreen , Color::eBlue };
				for( int i = 0 ; i < 4 ; ++i )
				{
					RenderUtility::setBrush( g , color[ i ] );

					DRAW_RECT2( size , offset );
					DRAW_RECT2( size , -offset );
					g.rotateXForm( -DEG2RAD( 45 ) );
				}
			}
			break;
		case DC_TWISTER:
			{
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eWhite );

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
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eWhite );

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
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eBlack );

				Vec2i size1( 2 , 22 );
				DRAW_RECT( size1 );
				Vec2i size2( 6 , 2 );
				Vec2i offset2( 0 , 10 );
				DRAW_RECT2( size2 , offset2 );
				DRAW_RECT2( size2 , -offset2 );

				g.setBrush( ColorKey3( 200 , 200 , 200 ) );

				Vec2i size3( 4 , 20 );
				Vec2i offset3( 3 , 0 );
				DRAW_RECT2( size3 , offset3 );
				DRAW_RECT2( size3 , -offset3 );
			}
			break;
		case DC_LOGICGATE_AND_PRIMARY:
		case DC_LOGICGATE_AND:
			{
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eGray );

				Vec2i size( 12 , 4 );
				DRAW_RECT2( size , Vec2i( -5 , 0 ) );

				RenderUtility::setBrush( g , Color::eWhite );
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
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , Color::eWhite );
				int w = 5;
				int h = 24;
				for( int i = 0 ; i < 2 ; ++i )
				{
					DRAW_RECT( Vec2i( w , h ) );
					DRAW_RECT( Vec2i( h , w ) );
					g.rotateXForm( DEG2RAD(45) );
				}

				RenderUtility::setBrush( g , gColorMap[ COLOR_CB ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 9 );
				RenderUtility::setBrush( g , gColorMap[ COLOR_CG ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 7 );
				RenderUtility::setBrush( g , gColorMap[ COLOR_CR ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 5 );
				RenderUtility::setBrush( g , gColorMap[ COLOR_W ] );
				g.drawCircle( Vec2i( 0 , 0 ) , 3 );

			}
			break;
		default:
			{
				Vec2i size( 20 , 20 );
				RenderUtility::setPen( g , Color::eBlack );
				RenderUtility::setBrush( g , id % 8 + 1 );
				g.drawRect( - size / 2 , size );

				g.finishXForm();
				FixString< 32 > str;
				RenderUtility::setFont( g , FONT_S12 );
				g.setTextColor( 255 , 255 , 255 );
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
				Tile const& data = world.getMapData( Vec2D(i,j) );

				Vec2i cellPos = pos + CellLength * Vec2i( i , j );

				switch( data.getType() )
				{
				case MT_EMPTY:
					RenderUtility::setPen( g , Color::eBlack );
					g.setBrush( ColorKey3( 130 , 130 , 130 ) );
					g.drawRect( cellPos , CellSize + Vec2i(1,1) );
					break;
				case MT_BLOCK:
					RenderUtility::drawBlock( g , cellPos , CellSize , Color::eGray );
					break;
				case MT_HOLE:
					break;
				}

				Device const* dc = data.getDevice();


				Vec2D centerPos  = cellPos + CellSize / 2;

				if( dc  )
				{
					if ( dc->getFlag().checkBits( DFB_DRAW_FRIST )  )
					{
						drawDevice( g , centerPos , *dc );
						drawLight( g , centerPos ,data );
					}
					else
					{
						drawLight( g , centerPos ,data );
						drawDevice( g , centerPos , *dc );
					}
				}
				else
				{
					drawLight( g , centerPos , data );
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

			RenderUtility::setPen( g , Color::eBlack );
			g.setBrush( ColorKey3( 150 , 150 , 150 ) );

			g.drawRoundRect( cellPos , CellSize + Vec2i(1,1) , Vec2i( 8 , 8 ) );

			if ( dc )
				drawDevice( g , cellPos + CellSize / 2 , *dc );
		}
	}

	void Scene::drawLight( Graphics2D& g , Vec2i const& pos , Tile const& data )
	{
		for(int i = 0; i < Dir::RestNumber ; ++i)
		{
			Dir dir = Dir::ValueNoCheck( i );
			int color = data.getLightPathColor( dir );

			if ( color == COLOR_NULL ) 
				continue;

			Vec2D p2 = pos + ( (CellLength / 2) + 1 ) * LightTrace::getDirOffset( dir );
			RenderUtility::setPen( g , gColorMap[ color ] );
			g.drawLine( pos , p2 );
		}
	}

	void Scene::restart()
	{
		getLevel().restart();
		if ( mbCreateMdoe )
		{
			getLevel().clearData();

		}
	}

	Level::PosType Scene::getCellPos( int x , int y , Vec2D& result )
	{
		Vec2D pos( x , y );

		pos -= mWorldPos;

		if ( isInRect( pos , Vec2i(0,0) , WorldMapSize ) )
		{	
			result = pos / CellLength;
			return Level::PT_WORLD;
		}
		else if ( isInRect( pos , StorageMapPosOffset , StorageMapSize ) )
		{
			pos -= StorageMapPosOffset;

			Vec2D temp( pos.x % StorageMapOffset , 
				        pos.y % StorageMapOffset );

			if ( isInRect( temp , Vec2D(0,0) , Vec2D( CellLength , CellLength ) ) )
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
		if ( !isInRect( pos , CMToolPos , ToolRect ) )
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