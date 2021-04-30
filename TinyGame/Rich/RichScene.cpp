#include "RichPCH.h"
#include "RichScene.h"

#include "RichPlayer.h"
#include "RichArea.h"

#include "RenderUtility.h"
#include "GameGlobal.h"
#include "InlineString.h"


namespace Rich
{
	Vec2i const MapPos( 20 , 20 );
	int const   TileVisualLength = 80;
	Vec2i const TileVisualSize( TileVisualLength , TileVisualLength );

	int gRoleColor[] = { EColor::Orange , EColor::Purple };

	uint16 const tileWidth  = 58;
	uint16 const tileHeight = 30;
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


	Scene::Scene()
	{
		mLevel = nullptr;
		mAnimCur = nullptr;
		mAnimLast = nullptr;

		mbAlwaySkipMoveAnim = false;
		mbSkipMoveAnim = false;
	}

	class AreaRenderer : public AreaVisitor
	{
	public:
		AreaRenderer( Graphics2D& g ):g(g){}

		virtual void visit( LandArea& area )
		{
			Player* owner = area.getOwner();
			if ( owner )
			{
				RenderUtility::SetBrush( g , gRoleColor[ owner->getRoleId() ] , COLOR_LIGHT );
				g.drawRect( rPos , TileVisualSize );
			}
		}

		virtual void visit( StationArea& area )
		{
			
		}

		virtual void visit( CardArea& area )
		{
			
		}

		virtual void visit( EmptyArea& area )
		{
			
		}

		virtual void visit( StartArea& area )
		{
			
		}

		Graphics2D& g;
		Vec2i       rPos;

	};

	void Scene::render( Graphics2D& g )
	{
		World& world = getLevel().getWorld();
		World::MapDataType& map = world.mMapData;

		RenderUtility::SetPen( g , EColor::Gray );
		RenderUtility::SetBrush( g , EColor::Gray );
		g.drawRect( Vec2i(0,0) , ::Global::GetScreenSize() );
		

		AreaRenderer drawer( g );
		for( int i = 0 ; i < map.getSizeX() ; ++i )
		{
			for( int j = 0 ; j < map.getSizeY() ; ++j )
			{
				Area* area = world.getArea( MapCoord(i,j) );
				if ( area )
				{
					RenderUtility::SetPen( g , EColor::Black );
					RenderUtility::SetBrush( g , EColor::White );
					g.drawRect( MapPos + TileVisualLength * Vec2i( i , j ) , TileVisualSize );
					drawer.rPos = MapPos + TileVisualLength * Vec2i( i , j );
					area->accept( drawer );
				}
			}
		}

		for( RenderCompList::iterator iter = mRenderList.begin() , itEnd = mRenderList.end() ;
			iter != itEnd ; ++iter )
		{
			RenderComp* comp = *iter;
			comp->render( g );
		}

		if ( mAnimCur )
			mAnimCur->render( g );

	}

	void Scene::render( Graphics2D& g , RenderView& view )
	{
		int nx = view.screenOffset.x / ( tileWidth + 2 );
		int ny = view.screenOffset.y / ( tileHeight );


		int xScanEnd = nx + view.screenSize.x / ( tileWidth + 2 ) + 2;
		int yScanEnd = ny + view.screenSize.y / tileHeight        + 2;

		int xScanStart = nx - 2;
		int yScanStart = ny - 2;

		Vec2i const offset = Vec2i( ( tileWidth + 2 ) / 2 , tileHeight / 2 );

		RenderCoord coord;

		for( int yScan = yScanStart ; yScan < yScanEnd ; ++yScan )
		{
			for( int xScan = xScanStart  ; xScan < xScanEnd ; ++xScan )
			{
				view.calcCoord( coord , xScan , yScan );
			}

			for( int xScan = xScanStart ; xScan < xScanEnd ; ++xScan )
			{
				view.calcCoord( coord , xScan + 0.5f , yScan + 0.5f );
			}
		}
	}

	void Scene::update( long time )
	{
		if ( mAnimCur )
		{
			if ( !mAnimCur->update( time ) )
			{
				Animation* next = mAnimCur->mNext;

				delete mAnimCur;
				mAnimCur = next;
				if ( mAnimCur )
				{
					mAnimCur->setup( *this );
				}
				else
				{
					mAnimLast = nullptr;
				}
			}
		}

		for( RenderCompList::iterator iter = mRenderList.begin() , itEnd = mRenderList.end() ;
			iter != itEnd ; ++iter )
		{
			RenderComp* comp = *iter;
			comp->update( time );
		}
	}

	class DiceAnimation : public Animation
	{
	public:
		DiceAnimation()
		{
			curTime = 0;
		}
		virtual bool update( long time )
		{
			curTime += time;
			if ( curTime > duration )
				return false;
			return true;
		}

		virtual void render( Graphics2D& g )
		{
			for( int i = 0 ; i < numDice ; ++i )
			{
				drawDice( g , Vec2i( 150 + i * 100 , 150 ) , value[i]);
			}
		}

		void drawDice( Graphics2D& g , Vec2i const& pos , int value )
		{
			RenderUtility::SetPen( g , EColor::Black );
			RenderUtility::SetBrush( g , EColor::White );
			RenderUtility::SetFont( g , FONT_S24 );

			g.drawRoundRect( pos , Vec2i( 80 , 80 ) , Vec2i( 10 , 10 ) );
			g.setTextColor(Color3ub(0 , 0 , 0) );
			InlineString< 32 > str;
			if ( curTime < duration / 2 )
				str.format( "%d" , ::Global::Random() % 6 + 1 );
			else
				str.format( "%d" , value );
			g.drawText( pos , Vec2i( 80 , 80 ) , str );
		}
		long curTime;
		long duration;
		int  numDice;
		int  value[ MAX_MOVE_POWER ];
	};

	class ActorMoveAnimation : public Animation
	{
	public:
		ActorMoveAnimation( ActorComp& actor , long duration )
		{
			comp = actor.getOwner()->getComponentT< ActorRenderComp >( COMP_RENDER );
			comp->bUpdatePos = false;

			from = actor.getPrevPos();
			dif  = actor.getPos() - actor.getPrevPos();
			timeCur = 0;
			timeTotal = duration;
		}
		virtual bool update( long time )
		{
			timeCur += time;
			if ( timeCur > timeTotal )
			{
				comp->pos = from + dif;
				comp->bUpdatePos = true;
				return false;
			}
			
			comp->pos = from + dif * float( timeCur ) / timeTotal;
			return true;
		}
		virtual void skip()
		{

		}
		virtual void render( Graphics2D& g )
		{


		}
		ActorRenderComp* comp;
		long      timeTotal;
		long      timeCur;
		Vector2     from;
		Vector2     dif;
	};


	void Scene::onWorldMsg( WorldMsg const& msg )
	{
		switch( msg.id )
		{
		case MSG_MOVE_START:
			mbSkipMoveAnim = mbAlwaySkipMoveAnim;
			break;
		case MSG_MOVE_STEP:
			{
				if ( !mbSkipMoveAnim )
					addAnim( new ActorMoveAnimation( *msg.getParam<Player*>() , 500 ) );
			}
			break;
		case MSG_THROW_DICE:
			{
				DiceAnimation* anim = new DiceAnimation;
				anim->duration = 1000;
				anim->numDice = msg.getParam<int>(0);
				for( int i = 0 ; i < anim->numDice ; ++i )
					anim->value[i] = msg.getParam<int*>(1)[i];

				addAnim( anim );
			}
			break;
		}
		
	}

	void Scene::addAnim( Animation* anim )
	{
		if ( mAnimLast )
		{
			mAnimLast->mNext = anim;
			mAnimLast = anim;
		}
		else
		{
			mAnimCur = anim;
			mAnimCur->setup( *this );
		}
		mAnimLast = anim;
	}

	bool Scene::calcCoord( Vec2i const& sPos , MapCoord& coord )
	{
		coord = ( sPos - MapPos ) / TileVisualLength;
		return true;
	}

	void ActorRenderComp::render( Graphics2D& g )
	{
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , EColor::Red );
		Vec2i rPos = Vec2i( TileVisualLength * pos ) + TileVisualSize / 2;
		g.drawCircle( rPos  , 10 );
	}

	void ActorRenderComp::update( long time )
	{
		if ( !bUpdatePos )
			return;
		pos = getOwner()->getComponentT< ActorComp >(COMP_ACTOR)->getPos();
	}

	void PlayerRenderComp::render( Graphics2D& g )
	{
		Player* player = getOwner()->getComponentT< Player >(COMP_ACTOR);
		RenderUtility::SetPen( g , EColor::Black );
		RenderUtility::SetBrush( g , gRoleColor[ player->getRoleId() ] );
		Vec2i rPos = Vec2i( TileVisualLength * pos ) + TileVisualSize / 2;
		g.drawCircle( rPos  , 10 );
	}


	Vec2i RenderView::convScanToMapPos( int xScan , int yScan ) const
	{
		int const (&factor)[4] = transToMapPosFactor[ dir ];
		return Vec2i( xScan * factor[0] + yScan * factor[1] ,
			xScan * factor[2] + yScan * factor[3] );
	}

	Vec2i RenderView::convScanToMapPos( float xScan , float yScan ) const
	{
		int const (&factor)[4] = transToMapPosFactor[ dir ];
		return Vec2i( xScan * factor[0] + yScan * factor[1] ,
			xScan * factor[2] + yScan * factor[3] );
	}

	Vec2i RenderView::convScreenToMapPos( Vec2i const& sPos ) const
	{
		//Vec2i pos = sPos + mOffset;
		//int px = ( pos.x + 2 * pos.y ) / (2 * tileHeight );
		//int py = ( - pos.x + 2 * pos.y ) / (2 * tileHeight );

		//return Vec2i( px , py );

		float xScan = float( sPos.x + screenOffset.x ) / (2 * tileHeight);
		float yScan = float( sPos.y + screenOffset.y ) / ( tileHeight );

		return convScanToMapPos( xScan , yScan );
	}

	Vec2i RenderView::convMapToScreenPos( Vec2i const& mapPos ) const
	{
		float xScan , yScan;
		convMapToScanPos( mapPos , xScan , yScan );

		return convScanToScreenPos( xScan , yScan );
	}

	Vec2i RenderView::convMapToScreenPos( float xMap , float yMap )
	{
		float const (&factor)[4] = transToScanPosFactor[ dir ];

		return convScanToScreenPos(
			float( xMap * factor[0] + yMap * factor[1] )  ,
			float( xMap * factor[2] + yMap * factor[3] ) );
	}

	void RenderView::convMapToScanPos( Vec2i const& mapPos , float& sx , float& sy ) const
	{
		float const (&factor)[4] = transToScanPosFactor[ dir ];

		sx = float( mapPos.x * factor[0] + mapPos.y * factor[1] );
		sy = float( mapPos.x * factor[2] + mapPos.y * factor[3] );
	}

	Vec2i RenderView::convScanToScreenPos( float xScan , float yScan ) const
	{
		return Vec2i( ( xScan + 0.5 ) * ( tileWidth + 2 ) - screenOffset.x ,
			( yScan + 0.5 ) * ( tileHeight )  - screenOffset.y );
	}

	Vec2i RenderView::calcMapToScreenOffset( float dx , float dy )
	{
		float const (&factor)[4] = transToScanPosFactor[ dir ];

		return calcScanToScreenOffset(
			dx * factor[0] + dy * factor[1] ,
			dx * factor[2] + dy * factor[3] );
	}

	Vec2i RenderView::calcScanToScreenOffset( float dx , float dy )
	{
		return Vec2i( dx * ( tileWidth + 2 ) , dy * tileHeight );
	}

	Vec2i RenderView::calcTileOffset( int w , int h )
	{
		return Vec2i( -w  / 2 , /*( tileHeight / 2 ) */- h  );
	}

	void RenderView::calcCoord( RenderCoord& coord , Vec2i const& mapPos )
	{
		coord.map    = mapPos;
		convMapToScanPos( mapPos , coord.xScan , coord.yScan );
		coord.screen = convScanToScreenPos( coord.xScan , coord.yScan );
	}

	void RenderView::calcCoord( RenderCoord& coord , float xScan , float yScan )
	{
		coord.xScan  = xScan;
		coord.yScan  = yScan;
		coord.map    = convScanToMapPos( xScan , yScan );
		coord.screen = convScanToScreenPos( xScan , yScan );
	}

}//namespace Rich