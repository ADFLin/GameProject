#include "BubblePCH.h"
#include "BubbleScene.h"

#include "DrawEngine.h"
#include "GameGlobal.h"
#include "GameControl.h"

#include "RenderUtility.h"

#include <algorithm>

namespace Bubble
{
	int const g_BubbleColor[] =
	{
		EColor::Cyan ,
		EColor::Blue ,
		EColor::Orange,
		EColor::Yellow ,
		EColor::Green ,
		EColor::Purple,
		EColor::Red  ,
		EColor::Gray ,
		EColor::White ,
		EColor::Black ,
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

	void Scene::render(IGraphics2D& g , Vector2 const& pos )
	{
		for ( int i = 0 ; i < mLevel.mNumCellData ; ++i )
		{
			BubbleCell& cell = mLevel.getCell(i);

			if ( cell.isBlock() || cell.isEmpty() )
				continue;

			Vector2 cPos = mLevel.calcCellCenterPos( i );

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

	void Scene::render(IGraphics2D& g )
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

	void Scene::renderDbg(IGraphics2D& g , Vector2 const& pos , int index )
	{
		bool isEven = mLevel.isEvenLayer( index );

		Vector2 cPos = mLevel.calcCellCenterPos( index );
		g.setBrush( Color3ub( 0 , 0 , 255 ) );
		g.drawCircle( pos + cPos , int( g_BubbleRadius - 10) );
		g.setBrush( Color3ub( 255 , 255 , 0 ) );

		for( int i = 0 ; i < NUM_LINK_DIR ; ++i )
		{
			int linkIdx = mLevel.getLinkCellIndex( index , LinkDir(i) , isEven );
			//BubbleCell& linkCell = mLevel.getCell( linkIdx );
			Vector2 cPos = mLevel.calcCellCenterPos( linkIdx );

			g.drawCircle( pos + cPos , int( g_BubbleRadius -10) );
		}


		//g.setBrush( ColorKey3( 255 , 0 , 0 ) );
		//for( int i = 0 ; i < mLevel.mNumFallCell ; ++i )
		//{
		//	Vector2 cPos = mLevel.calcCellCenterPos( mLevel.mIndexFallCell[i] );
		//	g.drawCircle( pos + cPos , g_BubbleRadius - 5 );
		//}
	}

	void Scene::renderBubbleList(IGraphics2D& g , Vector2 const& pos , BubbleList& bList )
	{
		for( SceneBubble* bubble : bList )
		{
			Vector2 bPos = pos + bubble->pos;

			if ( bubble->alpha != 1.0f )
			{
				g.beginBlend( bPos - Vector2( g_BubbleRadius , g_BubbleRadius ) , 
					Vector2( g_BubbleDiameter , g_BubbleDiameter ) , bubble->alpha );

				renderBubble( g , pos + bubble->pos , bubble->color );

				g.endBlend();
			}
			else
			{
				renderBubble( g , pos + bubble->pos , bubble->color );
			}
		}
	}

	void Scene::renderBubble( IGraphics2D& g , Vector2 const& pos , int color )
	{
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , color , COLOR_NORMAL);
		g.drawCircle( pos , int( g_BubbleRadius )  );

		RenderUtility::SetBrush( g ,  color , COLOR_DEEP );
		RenderUtility::SetPen( g , EColor::Null );
		g.drawCircle( pos , int( g_BubbleRadius - 5.0 ) );

		RenderUtility::SetBrush( g ,  color , COLOR_LIGHT );
		g.drawCircle( pos + Vector2( 5 , -5 ) , 5 );
	}

	void Scene::renderBackground(IGraphics2D& g , Vector2 const& pos )
	{
		int width  = int( mLevel.mNumFreeCellLayer * g_BubbleDiameter );
		RenderUtility::DrawBlock( g , pos - Vector2( (float)BlockSize , (float)BlockSize ) , 
			Vec2i(   width + 2 * BlockSize , BlockSize ) , EColor::Gray );

		RenderUtility::DrawBlock( g , pos - Vector2( (float)BlockSize , 0 ) , 
			Vec2i( BlockSize , (int)mLevel.mMaxDepth ) , EColor::Gray );

		RenderUtility::DrawBlock( g , pos + Vector2( (float)width , 0 ) , 
			Vec2i( BlockSize , (int)mLevel.mMaxDepth ) , EColor::Gray );

	}

	void Scene::renderLauncher(IGraphics2D& g , Vector2 const& pos )
	{
		RenderUtility::SetBrush( g ,  mLevel.mShootBubbleColor );
		RenderUtility::SetPen( g ,  mLevel.mShootBubbleColor );

		Vector2 bPos = pos + mLevel.mLauncherPos;

		Vector2 startPos = bPos + 10 * mLevel.mLauncherDir;
		Vector2 offset = 15 * mLevel.mLauncherDir;
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
			bool bNeedDestroy = false;

			switch( bubble->type )
			{
			case SceneBubble::eFall:
				bubble->vel.y += mFallBubbleAcc * dt;
				bubble->update( dt );

				if ( bubble->pos.y > getLevel().getMaxDepth() + 200 )
					bNeedDestroy = true;

				break;

			case SceneBubble::eVanish:
				bubble->alpha -= 0.06f;
				bubble->update( dt );

				if ( bubble->alpha < 0 )
					bNeedDestroy = true;

				break;
			}

			if ( bNeedDestroy )
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

	void Scene::rotateRight( float delta )
	{
		getLevel().rotateRight( delta );
	}

	void Scene::fireAction(ActionPort port, ActionTrigger& trigger )
	{
		if ( !trigger.haveUpdateFrame() )
			return;

		float const angle = DEG2RAD(3);
		float const mouseAngle = - 0.01f; 

		int offset = 0;

		if ( trigger.detect(port, ACT_BB_SHOOT ) )
			shoot();

		if ( trigger.detect(port, ACT_BB_ROTATE_LEFT ) )
		{
			rotateRight( angle );
		}
		else if ( trigger.detect(port, ACT_BB_ROTATE_RIGHT ) )
		{
			rotateRight( -angle );
		}
		else if ( trigger.detect(port, ACT_BB_MOUSE_ROTATE , &offset ) )
		{
			rotateRight( mouseAngle * offset );
		}
	}

} //namespace Bubble