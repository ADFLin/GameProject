#include "TinyGamePCH.h"
#include "FBirdStage.h"

#include "RenderUtility.h"
#include "GameGUISystem.h"
#include "WidgetUtility.h"

namespace FlappyBird
{

	bool TestStage::onInit()
	{
		::Global::GUI().cleanupWidget();

		mMaxScore = 0;
		restart();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		return true;
	}

	void TestStage::onRender( float dFrame )
	{
		Graphics2D& g = Global::getGraphics2D();

		RenderUtility::setBrush( g , Color::eYellow );
		RenderUtility::setPen( g , Color::eBlack );

		for( ColObjectList::iterator iter = mColObjects.begin() , itEnd = mColObjects.end();
			iter != itEnd ; ++iter )
		{
			ColObject& obj = *iter;

			switch( obj.type )
			{
			case CT_BLOCK_DOWN:
			case CT_BLOCK_UP:
				{
					Vec2i rPos = convertToScreen( obj.pos );
					Vec2i rSize = Vec2i( GScale * obj.size );

					rPos.y -= rSize.y;
					g.drawRect( rPos , rSize );
				}
				break;
			}
		}

		Vec2i rPos = convertToScreen( mBird.getPos() );
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , Color::eYellow );
		g.drawCircle( rPos , BirdRadius * GScale );


		RenderUtility::setPen( g , Color::eRed );
		Vec2f dir = Vec2f( BirdVelX , -mBird.getVel() );
		dir /= sqrt( dir.length2() );
		g.drawLine( rPos , rPos + Vec2i( 15 * dir ) );



		FixString< 64 > str;
		str.format( "Count = %d" , mScore );
		g.drawText( Vec2i( 10 , 10 ) , str );
	}

	void TestStage::tick()
	{
		mBird.tick();

		Vec2f const& pos = mBird.getPos();

		if ( pos.y < BirdRadius )
		{
			mBird.setPos( Vec2f( pos.x , BirdRadius ) );
			overGame();
		}
		else if ( pos.y > WorldHeight - BirdRadius )
		{
			overGame();
		}

		if ( mIsOver )
			return;

		if ( mTimerProduce == 0 )
		{
			float const BlockDistance = 4.5;
			float const BlockWidth    = 1.5f;
			float const BlockGap      = 2.5f;

			float const HeightMax = WorldHeight - BlockGap - 0.5f;
			float const HeightMin = 0.5f;

			int const BlockTimer = int( BlockDistance / BirdTickOffset );


			float height = HeightMin + ( HeightMax - HeightMin ) * ::Global::Random() / float( RAND_MAX );
			float const posProduce = WorldWidth + 1.0f;

			ColObject obj;
			obj.type = CT_BLOCK_DOWN;
			obj.pos  = Vec2f( posProduce , 0 );
			obj.size = Vec2f( BlockWidth , height );
			mColObjects.push_back( obj );

			obj.type = CT_SCORE;
			obj.pos  = Vec2f( posProduce + ( BlockWidth - 0.5f ) / 2 , height );
			obj.size = Vec2f( 0.5f , BlockGap );
			mColObjects.push_back( obj );

			obj.type = CT_BLOCK_UP;
			obj.pos  = Vec2f( posProduce , height + BlockGap );
			obj.size = Vec2f( BlockWidth , WorldHeight - obj.pos.y );
			mColObjects.push_back( obj );

			mTimerProduce = BlockTimer;
		}
		else
		{
			--mTimerProduce;
		}


		for ( ColObjectList::iterator iter = mColObjects.begin() , itEnd = mColObjects.end() ;
			iter != itEnd ; )
		{
			ColObject& obj = *iter;

			obj.pos.x -= BirdTickOffset;


			bool needRemove = false;

			if ( !mIsOver )
			{
				Vec2f offset = obj.pos - mBird.getPos() + Vec2f( BirdRadius , BirdRadius );
				if ( testInterection( BirdSize , offset , obj.size ) )
				{
					switch( obj.type )
					{
					case CT_BLOCK_DOWN:
					case CT_BLOCK_UP:
						overGame();
						break;
					case CT_SCORE:
						++mScore;
						needRemove = true;
						break;
					}
				}
			}

			if ( obj.pos.x + obj.size.x < 0 )
				needRemove = true;

			if ( needRemove )
				iter = mColObjects.erase( iter );
			else
				++iter;
		}
	}

	void TestStage::overGame()
	{
		if ( mIsOver )
			return;

		if ( mScore > mMaxScore )
			mMaxScore = mScore;

		GMsgBox* box = new GMsgBox( UI_ANY , Vec2i( 0 , 0 ) , NULL , GMB_OK );

		Vec2i pos = GUISystem::calcScreenCenterPos( box->getSize() );
		box->setPos( pos );
		box->setTitle( "You Die!" );
		::Global::GUI().addWidget( box );

		mIsOver = true;
	}

}//namespace FlappyBird