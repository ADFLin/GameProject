#include "RichPCH.h"
#include "RichScene.h"

#include "RichPlayer.h"
#include "RichCell.h"

#include "RenderUtility.h"
#include "GameGlobal.h"
#include "FixString.h"


namespace Rich
{
	Vec2i const MapPos( 20 , 20 );
	int const CellLength = 80;
	Vec2i const CellSize( CellLength , CellLength );

	int gRoleColor[] = { Color::eOrange , Color::ePurple };

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

	class CellDrawer : public CellVisitor
	{
	public:
		CellDrawer( Graphics2D& g ):g(g){}

		virtual void visit( LandCell& land )
		{
			Player* owner = land.getOwner();
			if ( owner )
			{
				RenderUtility::setBrush( g , gRoleColor[ owner->getRoleId() ] , COLOR_LIGHT );
				g.drawRect( rPos , CellSize );
			}
		}

		virtual void visit( StationCell& ts )
		{
			
		}

		virtual void visit( CardCell& cardCell )
		{
			
		}

		virtual void visit( EmptyCell& cell )
		{
			
		}

		virtual void visit( StartCell& cell )
		{
			
		}

		Graphics2D& g;
		Vec2i       rPos;

	};

	void Scene::render( Graphics2D& g )
	{
		World& world = getLevel().getWorld();
		World::MapDataType& map = world.mMapData;

		RenderUtility::setPen( g , Color::eGray );
		RenderUtility::setBrush( g , Color::eGray );
		g.drawRect( Vec2i(0,0) , ::Global::getDrawEngine()->getScreenSize() );
		

		CellDrawer drawer( g );
		for( int i = 0 ; i < map.getSizeX() ; ++i )
		{
			for( int j = 0 ; j < map.getSizeY() ; ++j )
			{
				Cell* cell = world.getCell( MapCoord(i,j) );
				if ( cell )
				{
					RenderUtility::setPen( g , Color::eBlack );
					RenderUtility::setBrush( g , Color::eWhite );
					g.drawRect( MapPos + CellLength * Vec2i( i , j ) , CellSize );
					drawer.rPos = MapPos + CellLength * Vec2i( i , j );
					cell->accept( drawer );
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
			if ( curTime > durtion )
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
			RenderUtility::setPen( g , Color::eBlack );
			RenderUtility::setBrush( g , Color::eWhite );
			RenderUtility::setFont( g , FONT_S24 );

			g.drawRoundRect( pos , Vec2i( 80 , 80 ) , Vec2i( 10 , 10 ) );
			g.setTextColor( 0 , 0 , 0 );
			FixString< 32 > str;
			if ( curTime < durtion / 2 )
				str.format( "%d" , ::Global::Random() % 6 + 1 );
			else
				str.format( "%d" , value );
			g.drawText( pos , Vec2i( 80 , 80 ) , str );
		}
		long curTime;
		long durtion;
		int  numDice;
		int  value[ MAX_MOVE_POWER ];
	};
	class ActorMoveAnimation : public Animation
	{
	public:
		ActorMoveAnimation( Actor& actor , long durtion )
		{
			comp = actor.getComponentT< ActorRenderComp >( COMP_RENDER );
			comp->bUpdatePos = false;

			from = actor.getPrevPos();
			dif  = actor.getPos() - actor.getPrevPos();
			timeCur = 0;
			timeTotal = durtion;
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
		Vec2f     from;
		Vec2f     dif;
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
					addAnim( new ActorMoveAnimation( *msg.player , 500 ) );
			}
			break;
		case MSG_THROW_DICE:
			{
				DiceAnimation* anim = new DiceAnimation;
				anim->durtion = 1000;
				anim->numDice = msg.numDice;
				for( int i = 0 ; i < anim->numDice ; ++i )
					anim->value[i] = msg.diceValue[i];

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
		coord = ( sPos - MapPos ) / CellLength;
		return true;
	}

	void ActorRenderComp::render( Graphics2D& g )
	{
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , Color::eRed );
		Vec2i rPos = Vec2i( CellLength * pos ) + CellSize / 2;
		g.drawCircle( rPos  , 10 );
	}

	void ActorRenderComp::update( long time )
	{
		if ( !bUpdatePos )
			return;
		pos = static_cast< Actor* >( getOwner() )->getPos();
	}

	void PlayerRenderComp::render( Graphics2D& g )
	{
		Player* player = static_cast< Player* >( getOwner() );
		RenderUtility::setPen( g , Color::eBlack );
		RenderUtility::setBrush( g , gRoleColor[ player->getRoleId() ] );
		Vec2i rPos = Vec2i( CellLength * pos ) + CellSize / 2;
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