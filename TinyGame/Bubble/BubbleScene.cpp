#include "BubblePCH.h"
#include "BubbleScene.h"

#include "DrawEngine.h"
#include "GameGlobal.h"
#include "GameControl.h"

#include "RenderUtility.h"

namespace Bubble
{
	int const g_BubbleColor[] =
	{
		Color::eCyan ,
		Color::eBlue ,
		Color::eOrange,
		Color::eYellow ,
		Color::eGreen ,
		Color::ePurple,
		Color::eRed  ,
		Color::eGray ,
		Color::eWhite ,
		Color::eBlack ,
	};

	struct Scene::SceneBubble : public Level::Bubble
	{
		enum
		{
			eFall ,
			eVanish ,
		};
		int   type;
		float alpha;

		int   action;
		BubbleList::iterator iter;
	};

	Scene::Scene(  LevelListener* listener  )
		:mLevel( listener )
	{
		mFallBubbleAcc = 600.0f;
		mTimeCount = 0;
	}

	void Scene::render( Graphics2D& g , Vec2f const& pos )
	{
		for ( int i = 0 ; i < mLevel.mNumCellData ; ++i )
		{
			BubbleCell& cell = mLevel.getCell(i);

			if ( cell.isBlock() || cell.isEmpty() )
				continue;

			Vec2f cPos = mLevel.calcCellCenterPos( i );

			//if ( cell.isBlock() )
			//{
			//	g.setBrush( ColorKey3( 255 , 0 , 0 ) );
			//	g.drawCircle( pos + cPos , g_BubbleRadius );
			//}
			//else if ( cell.isEmpty() )
			//{
			//	g.setBrush( ColorKey3( 255 , 255 , 255 ) );
			//	g.drawCircle( pos + cPos , g_BubbleRadius );
			//}
			//else
			{
				renderBubble( g , pos + cPos , cell.getColor() );
			}	
		}

		renderBackground( g , pos );

		renderBubbleList( g , pos , mShootList );
		renderBubbleList( g , pos , mFallList );
		renderLauncher( g , pos );

	}

	void Scene::render( Graphics2D& g )
	{
		if ( mScale != 1.0f )
		{
			g.beginXForm();
			g.translateXForm( getSurfacePos().x , getSurfacePos().y );
			g.scaleXForm( mScale , mScale );
			render( g , Vec2i(0,0) );
			g.finishXForm();
		}
		else
		{
			render( g , getSurfacePos() );
		}
	}

	void Scene::renderDbg( Graphics2D& g , Vec2f const& pos , int index )
	{
		bool isEven = mLevel.isEvenLayer( index );

		Vec2f cPos = mLevel.calcCellCenterPos( index );
		g.setBrush( ColorKey3( 0 , 0 , 255 ) );
		g.drawCircle( pos + cPos , int( g_BubbleRadius - 10) );
		g.setBrush( ColorKey3( 255 , 255 , 0 ) );

		for( int i = 0 ; i < NUM_LINK_DIR ; ++i )
		{
			int linkIdx = mLevel.getLinkCellIndex( index , LinkDir(i) , isEven );
			//BubbleCell& linkCell = mLevel.getCell( linkIdx );
			Vec2f cPos = mLevel.calcCellCenterPos( linkIdx );

			g.drawCircle( pos + cPos , int( g_BubbleRadius -10) );
		}


		//g.setBrush( ColorKey3( 255 , 0 , 0 ) );
		//for( int i = 0 ; i < mLevel.mNumFallCell ; ++i )
		//{
		//	Vec2f cPos = mLevel.calcCellCenterPos( mLevel.mIndexFallCell[i] );
		//	g.drawCircle( pos + cPos , g_BubbleRadius - 5 );
		//}
	}

	void Scene::renderBubbleList( Graphics2D& g , Vec2f const& pos , BubbleList& bList )
	{
		for( BubbleList::iterator iter = bList.begin() ,iterEnd = bList.end() ;
			iter != iterEnd ; ++iter )
		{
			SceneBubble* bubble = *iter;
			Vec2f bPos = pos + bubble->pos;

			if ( bubble->alpha != 1.0f )
			{
				g.beginBlend( bPos - Vec2f( g_BubbleRadius , g_BubbleRadius ) , 
					Vec2f( g_BubbleDiameter , g_BubbleDiameter ) , bubble->alpha );

				renderBubble( g , pos + bubble->pos , bubble->color );

				g.endBlend();
			}
			else
			{
				renderBubble( g , pos + bubble->pos , bubble->color );
			}
		}

	}

	void Scene::renderBubble( Graphics2D& g , Vec2f const& pos , int color )
	{
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , color , COLOR_NORMAL);
		g.drawCircle( pos , int( g_BubbleRadius )  );

		RenderUtility::setBrush( g ,  color , COLOR_DEEP );
		RenderUtility::setPen( g , Color::eNull );
		g.drawCircle( pos , int( g_BubbleRadius - 5.0 ) );

		RenderUtility::setBrush( g ,  color , COLOR_LIGHT );
		g.drawCircle( pos + Vec2f( 5 , -5 ) , 5 );
	}

	void Scene::renderBackground( Graphics2D& g , Vec2f const& pos )
	{
		int width  = int( mLevel.mNumFreeCellLayer * g_BubbleDiameter );
		RenderUtility::drawBlock( g , pos - Vec2f( (float)BlockSize , (float)BlockSize ) , 
			Vec2i(   width + 2 * BlockSize , BlockSize ) , Color::eGray );

		RenderUtility::drawBlock( g , pos - Vec2f( (float)BlockSize , 0 ) , 
			Vec2i( BlockSize , (int)mLevel.mMaxDepth ) , Color::eGray );

		RenderUtility::drawBlock( g , pos + Vec2f( (float)width , 0 ) , 
			Vec2i( BlockSize , (int)mLevel.mMaxDepth ) , Color::eGray );

	}

	void Scene::renderLauncher( Graphics2D& g , Vec2f const& pos )
	{
		RenderUtility::setBrush( g ,  mLevel.mShootBubbleColor );
		RenderUtility::setPen( g ,  mLevel.mShootBubbleColor );

		Vec2f bPos = pos + mLevel.mLauncherPos;

		Vec2f startPos = bPos + 10 * mLevel.mLauncherDir;
		Vec2f offset = 15 * mLevel.mLauncherDir;
		int idx = ( mTimeCount / 120 ) % 13 - 4;
		for( int i = 0 ; i < 5 ; ++ i )
		{
			int r = std::max( 1 , 5 - abs( idx - i ) );
			startPos += offset;
			g.drawCircle( startPos  , r  );

		}
		//g.drawLine( bPos , bPos + 60 * mLevel.mLauncherDir );
		renderBubble( g , bPos , mLevel.mShootBubbleColor );

	}

	void Scene::onUpdateShootBubble( Level::Bubble* bubble , unsigned result )
	{
		(void)bubble;
		(void)result;

	}

	void Scene::onRemoveShootBubble( Level::Bubble* bubble )
	{
		SceneBubble* sBubble = static_cast< SceneBubble* >( bubble );
		mShootList.erase( sBubble->iter );
		delete sBubble;
	}

	void Scene::onPopCell( BubbleCell& cell , int index )
	{
		SceneBubble* sBubble = new SceneBubble;

		sBubble->type = SceneBubble::eVanish;
		sBubble->alpha = 1.0f;
		sBubble->pos = getLevel().calcCellCenterPos( index );
		sBubble->vel.setValue( 0,0 );
		sBubble->color = cell.getColor();

		mFallList.push_back( sBubble );

	}

	void Scene::onAddFallCell( BubbleCell& cell , int index )
	{
		SceneBubble* sBubble = new SceneBubble;

		sBubble->type = SceneBubble::eFall;
		sBubble->alpha = 1.0f;
		sBubble->pos = getLevel().calcCellCenterPos( index );
		sBubble->vel.setValue( 0,0 );
		sBubble->color = cell.getColor();

		mFallList.push_back( sBubble );
	}

	void Scene::shoot()
	{
		SceneBubble* bubble = new SceneBubble;

		bubble->alpha = 1.0f;

		getLevel().shoot( bubble );
		mShootList.push_front( bubble );

		bubble->iter = mShootList.begin();
	}

	void Scene::tick()
	{
		getLevel().update( gDefaultTickTime / 1000.0f );
	}

	void Scene::updateFrame( int frame )
	{
		long time = gDefaultTickTime * frame;

		mTimeCount += time;

		float dt = time / 1000.0f;

		for( BubbleList::iterator iter = mFallList.begin();
			iter != mFallList.end() ; )
		{
			SceneBubble* bubble = *iter;
			bool beDelete = false;

			switch( bubble->type )
			{
			case SceneBubble::eFall:
				bubble->vel.y += mFallBubbleAcc * dt;
				bubble->update( dt );

				if ( bubble->pos.y > getLevel().getMaxDepth() + 200 )
					beDelete = true;

				break;

			case SceneBubble::eVanish:
				bubble->alpha -= 0.06f;
				bubble->update( dt );

				if ( bubble->alpha < 0 )
					beDelete = true;

				break;
			}

			if ( beDelete )
			{
				delete bubble;
				iter = mFallList.erase( iter );
			}
			else
			{
				++iter;
			}
		}
	}

	void Scene::roteRight( float delta )
	{
		getLevel().roteRight( delta );
	}

	void Scene::fireAction( ActionTrigger& trigger )
	{
		if ( !trigger.haveUpdateFrame() )
			return;

		float const angle = DEG2RAD(3);
		float const mouseAngle = - 0.01f; 

		int offset = 0;

		if ( trigger.detect( ACT_BB_SHOOT ) )
			shoot();

		if ( trigger.detect( ACT_BB_ROTATE_LEFT ) )
		{
			roteRight( angle );
		}
		else if ( trigger.detect( ACT_BB_ROTATE_RIGHT ) )
		{
			roteRight( -angle );
		}
		else if ( trigger.detect( ACT_BB_MOUSE_ROTATE , &offset ) )
		{
			roteRight( mouseAngle * offset );
		}
	}

} //namespace Bubble