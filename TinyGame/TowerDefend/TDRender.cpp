#include "TDPCH.h"
#include "TDRender.h"

#include "TDLevel.h"
#include "TDController.h"

#include "RenderUtility.h"
#include "GameGlobal.h"
#include "DrawEngine.h"

namespace TowerDefend
{
	Renderer::Renderer() 
		:mVPRender( nullptr )
	{

	}


	void Renderer::drawHealthBar( Vec2i const& sPos , int len , float ratio )
	{
		Graphics2D& g = getGraphics();
		RenderUtility::setPen( g , Color::eNull );
		RenderUtility::setBrush( g , Color::eGreen , COLOR_DEEP );
		g.drawRect( sPos , Vec2i( len , 5 ) );
		RenderUtility::setBrush( g , Color::eGreen , COLOR_NORMAL );
		g.drawRect( sPos , Vec2i( int( len*ratio ) , 5 ) );
	}

	void Renderer::drawGrid()
	{
		DrawEngine* de = Global::getDrawEngine();
		Vec2i nSize = de->getScreenSize() / gMapCellLength;

		Graphics2D& g = getGraphics();

		Viewport& vp = getViewport();

		Vec2i offset = vp.convertToScreenPos( Vec2f(0,0) );
		offset.x %= (int)gMapCellLength;
		offset.y %= (int)gMapCellLength;

		RenderUtility::setPen( g , Color::eWhite );
		for( int n = 0 ; n <= nSize.x ; ++n )
		{
			int x = int ( n * gMapCellLength );
			g.drawLine( offset + Vec2i( x , 0  ) ,  offset + Vec2i( x , de->getScreenHeight() ) );
		}
		for( int n = 0 ; n <= nSize.x ; ++n )
		{
			int y = int ( n * gMapCellLength );
			g.drawLine( offset + Vec2i( 0 , y  ) ,  offset + Vec2i( de->getScreenWidth() , y ) );
		}
	}

	void Renderer::drawBuilding( ActorId blgID , Vec2f const& pos , bool beSelected )
	{
		assert( isBuilding( blgID ) );

		Vec2i size = Building::getBuildingInfo( blgID ).blgSize;
		Vec2i broader( 2 , 2 );
		Vec2i renderPos = getViewport().convertToScreenPos( pos ) - gMapCellLength * ( size / 2 ) + broader;

		Graphics2D& g = getGraphics();

		if ( beSelected )
			RenderUtility::setPen( g , Color::eWhite );
		else
			RenderUtility::setPen( g , Color::eNull );

		RenderUtility::setBrush( g , Color::eRed );

		g.drawRect( renderPos , gMapCellLength * size - 2 * broader );
	}

	Graphics2D& Renderer::getGraphics()
	{
		return Global::getGraphics2D();
	}

	void Controller::renderMouse( Renderer& renderer )
	{
		Graphics2D& g = renderer.getGraphics();

		if ( mEnableRectSelect && mRectSelectStep == STEP_MOVE )
		{
			TDRect rect = mSelectRect;
			rect.sort();

			Vec2i size = rect.end - rect.start;

			g.setPen( ColorKey3( 80 , 200 , 80 ) );
			g.setBrush( ColorKey3( 80 , 200 , 80 ) );

			g.beginBlend( rect.start , size , 0.3f );
			g.drawRect( rect.start , size );
			g.endBlend();

			Vec2i m1( rect.end.x , rect.start.y );
			Vec2i m2( rect.start.x , rect.end.y );

			g.setPen( ColorKey3( 0 , 255 , 0 ) );
			g.drawLine( rect.start , m1 );
			g.drawLine(  m1 , rect.end );
			g.drawLine( rect.end , m2 );
			g.drawLine( m2 , rect.start );
		}
	}

	void Level::render( Renderer& renderer , RenderParam const& param )
	{
		renderer.setViewport( &mVPCtrl );

		renderer.drawGrid();

		mEntityMgr.render( renderer );

		Graphics2D& g = renderer.getGraphics();

		//g.drawCircle( testPos , testR );
		//RenderUtility::setGraphicsBursh( g , ( testHaveCol ) ? Color::eRed : Color::eYellow );
		//g.drawCircle( mBuiler->getPos() + testOffset , mBuiler->getColRadius() );



		if ( mComID == CID_BUILD )
		{
			if ( mController )
			{
				Vec2i const& pos = mController->getLastMosuePos();

				Vec2i mapPos;
				if ( canBuild( mComActorID , NULL , pos , mapPos , &mPlayerInfo , false ) )
				{
					renderer.drawBuilding( mComActorID , Vec2f( mapPos ) * gMapCellLength , false  );
				}
			}
		}

		mActComMsg.render( renderer );

		if ( param.drawMouse )
		{
			if ( mSelectMode != SM_NORMAL )
			{
				Vec2i const& pos = mController->getLastMosuePos();
				g.drawCircle( pos , 3 );
			}
		}


		int offset = 15;
		int px = ::Global::getDrawEngine()->getScreenWidth() - 200;
		int py = 5 - offset;
		FixString< 256 > str;
		str.format( "Gold = %4d  Wood = %4d" , mPlayerInfo.gold , mPlayerInfo.wood );
		g.drawText( px , py += offset , str );
		str.format( "Com Mode = %d" , mComID );
		g.drawText( px , py += offset , str );


		Vec2i pos = mBuiler->getPos() / gMapCellLength;
		str.format( "%d %d" , pos.x , pos.y );
		g.drawText( 10 , 10 , str );
		str.format( "hash ( %d %d %d %d )" , 
			mBuiler->getColBody().mColHashValue[0] , 
			mBuiler->getColBody().mColHashValue[1] , 
			mBuiler->getColBody().mColHashValue[2] , 
			mBuiler->getColBody().mColHashValue[3] );
		g.drawText( 10 , 25 , str );
	}


	void Building::doRender( Renderer& renderer )
	{
		Vec2i pos = renderer.getViewport().convertToScreenPos( getPos() );
		renderer.drawBuilding( mActorID , getPos() , checkFlag( EF_SELECTED ) );

		if ( checkFlag( EF_SELECTED ) )
		{
			renderer.drawHealthBar( 
				pos - int( gMapCellLength ) * ( getBuildingInfo().blgSize / 2 ) - Vec2i( 0 , 8 ) , 
				getBuildingInfo().blgSize.x * gMapCellLength , 
				getLifeValue() / getBuildingInfo().maxHealthValue );
		}
	}

	void Unit::doRender( Renderer& renderer )
	{
		Graphics2D& g = renderer.getGraphics();

		if ( checkFlag( EF_SELECTED ) )
			RenderUtility::setPen( g , Color::eWhite );
		else
			RenderUtility::setPen( g , Color::eNull );
		switch ( getActorID() )
		{
		case AID_UT_BUILDER_1:
			RenderUtility::setBrush( g , Color::eRed );
			break;
		case AID_UT_MOUSTER_1:
			RenderUtility::setBrush( g , Color::eGreen );
			break;
		}

		Vec2i pos = renderer.getViewport().convertToScreenPos( getPos() );

		g.drawCircle( pos , getColRadius() );

		if ( checkFlag( EF_SELECTED ) )
		{
			renderer.drawHealthBar( 
				pos - Vec2i( int( getColRadius() ) , int( getColRadius() ) + 8 ) , 
				2 * int( getColRadius() ) , 
				getLifeValue() / getUnitInfo().maxHealthValue );
		}
	}


	void Bullet::doRender( Renderer& renderer )
	{
		Graphics2D& g = renderer.getGraphics();

		RenderUtility::setPen( g , Color::eNull );
		RenderUtility::setBrush( g , Color::eBlue );
		g.drawCircle( getPos() , 5 );
	}


	void ActComMessage::render( Renderer& renderer )
	{
		Graphics2D& g = renderer.getGraphics();

		RenderUtility::setBrush( g , Color::eNull );
		RenderUtility::setPen( g , Color::eGreen );
		for ( MsgList::iterator iter = msgList.begin();
			iter != msgList.end(); ++iter )
		{
			Vec2i pos = renderer.getViewport().convertToScreenPos( iter->pos );
			g.drawCircle( pos , 15 * ( NumFrameShow - iter->frame ) / NumFrameShow );
			g.drawPixel( pos , ColorKey3( 0 , 255 ,0 ) );
		}
	}

}//namespace TowerDefend
