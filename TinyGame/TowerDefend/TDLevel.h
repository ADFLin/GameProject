#ifndef TDLevel_h__
#define TDLevel_h__

#include "TDDefine.h"

#include "TDEntity.h"
#include "TDCollision.h"
#include "TDController.h"
#include "TDEntityCommon.h"
#include "TDWorld.h"

#include <vector>
#include <algorithm>
#include <cmath>

#include "DrawEngine.h"

namespace TowerDefend
{
	class Level;
	class Renderer;
	class Actor;

	float const gMinColMargin = 0.01f;

	class Viewport
	{
	public:
		Viewport():mOrg(0,0){ setScreenRange( Vec2i(0,0) ); }


		Vec2i convertToScreenPos( Vector2 const& wPos )
		{
			return Vec2i( wPos - mOrg );
		}
		Vector2 convertToWorldPos( Vec2i const& sPos )
		{
			return Vector2( sPos ) + mOrg;
		}
		void  setCenterViewPos( Vector2 const& pos )
		{
			mOrg = pos - 0.5f * Vector2( mScreenSize );
		}
		Vector2 getCenterViewPos()
		{ 
			return mOrg + 0.5f * Vector2( mScreenSize );
		}

		void  setScreenRange( Vec2i const& size )
		{
			mScreenSize = size;
		}

		Vec2i  getScreenSize(){ return mScreenSize; }

		class RectSelector
		{
		public:
			RectSelector( Viewport& viewport , TDRect const& rect )
			{
				assert( rect.start.x <= rect.end.x && rect.start.y <= rect.end.y );
				mRectMapMin = viewport.convertToWorldPos( rect.start );
				mRectMapMax = viewport.convertToWorldPos( rect.end );
			}

			bool  test( Vector2 const& bMin , Vector2 const& bMax )
			{
				return  mRectMapMax.x >= bMin.x && mRectMapMin.x <= bMax.x && 
					mRectMapMax.y >= bMin.y && mRectMapMin.y <= bMax.y ; 
			}

			Vector2 mRectMapMin;
			Vector2 mRectMapMax;

		};

	private:
		Vec2i mScreenSize;
		Vector2 mOrg;
	};

	class TeamGroup
	{

	public:


		void  classifyActor()
		{
			removeInActor();

			struct ComFun
			{
				bool operator()( ActorPtr const& a , ActorPtr const& b )
				{
					return a->getActorID() < b->getActorID();
				}
			};
			std::sort( mActors.begin() , mActors.end() , ComFun() );
		}


		void removeInActor() 
		{
			for( ActorVec::iterator iter = mActors.begin();
				iter != mActors.end() ; )
			{
				if ( *iter == NULL )
					iter = mActors.erase( iter );
				else
					++iter;
			}
		}
		typedef std::vector< ActorPtr > ActorVec;
		ActorVec mActors;
	};



	class IControlUI
	{
	public:
		virtual void setComMap( ComMapID id ) = 0;
		virtual void onFireCom( int idx , int comId ) = 0;
	};


	class ActComMessage : public ActEnumer
	{
	public:
		void haveGoalPos( Vector2 const& pos ) override;
		void haveTarget( Entity* entity ) override;

		void render( Renderer& renderer );

		void updateFrame( int frame );

		static int const NumFrameShow = 20;
		struct Msg
		{
			Vector2 pos;
			int   frame;
		};
		typedef std::list< Msg > MsgList;
		MsgList msgList;
	};


	typedef std::vector< ActorPtr > ActorVec;
	typedef ActorVec::iterator        ActorVecIter;


	enum TDSelectMode
	{
		SM_NORMAL ,
		SM_LOCK_ACTOR ,
		SM_LOCK_POS   ,
		SM_LOCK_ACTOR_POS ,
	};

	class Level : public World
		        , public IActionLanucher

	{
	public:
		Level()
		{
			mController  = NULL;
			mControlUI   = NULL;
			mComID       = CID_NULL;
			mSelectMode  = SM_NORMAL;
			mCurComMapID = COM_MAP_NULL;
			mPlayerInfo.id = 0;
			mMonsterPlayerInfo.id = 1;


			EntityFilter filter;
			filter.bitType  = ET_UNIT;
			filter.bitOwner = BIT( mMonsterPlayerInfo.id );
			getEntityMgr().listenEvent( EntityEventFun( this , &Level::onMonsterEvent ) , filter );

			filter.bitType  = ET_ACTOR;
			filter.bitOwner = BIT( mPlayerInfo.id );
			getEntityMgr().listenEvent( EntityEventFun( this , &Level::onPlayerEvent ) , filter );


			DrawEngine* de = ::Global::getDrawEngine();
			getCtrlViewport().setScreenRange( de->getScreenSize() );

			testPos.setValue( 100 , 100 );

			testR = 30;
		}

		void setController( Controller* controller ){ mController = controller;  }
		void setControlUI( IControlUI* controlUI ){ mControlUI = controlUI; }

		bool  testHaveCol;
		Vector2 testPos;
		float testR;
		Vector2 testOffset;

		void restart()
		{
			Vec2i mapSize( 256 , 256 );
			mMap.setup( mapSize.x , mapSize.y );
			mCollisionMgr.init( mapSize );
			mEntityMgr.cleanup();

			mPlayerInfo.id   = 0;
			mPlayerInfo.gold = 2000;
			mPlayerInfo.wood = 1000;
			mPlayerInfo.maxPopulation = 200;
			mPlayerInfo.curPopulation = 0;

			Builder* builder = new Builder( AID_UT_BUILDER_1 );
			builder->setPos( Vector2( 50 , 100 ) );
			builder->setOwner( &mPlayerInfo );
			getEntityMgr().addEntity( builder );
			mBuiler = builder;
			{

				Builder* builder = new Builder( AID_UT_BUILDER_1 );
				builder->setPos( Vector2( 100 , 100 ) );
				builder->setOwner( &mPlayerInfo );
				getEntityMgr().addEntity( builder );
			}


			{
				//TDUnit* monster = new TDUnit( AID_UT_MOUSTER_1 );
				//monster->setPos( Vector2( 150 , 150 ) );
				//monster->setOwner( &mMonsterPlayerInfo );


				//TDUnit::MoveLandAct* act = new TDUnit::MoveLandAct;
				//act->destPos = Vector2( 200 , 200 );
				//monster->addAction( act );

				//mEntityMgr.addEntity( monster );
			}



			//constructBuilding( AID_BT_TOWER_CUBE , gMapCellLength * Vector2(10,10) , &mPlayerInfo );

			//spawnCount = 100;
		}

		long spawnCount;
		void tick()
		{
			mEntityMgr.tick();

			testOffset.setValue( -10 , 0 );
			testHaveCol = testCollision( testPos , testR , mBuiler->getPos() , mBuiler->getColRadius() , testOffset );

			int const spawnSpeed = 100;

			if ( spawnCount >= spawnSpeed )
			{
				{
					Unit* monster = new Unit( AID_UT_MOUSTER_1 );
					monster->setPos( Vector2( 10 , 10 ) );
					monster->setOwner( &mPlayerInfo );

					Unit::MoveLandAct* act = new Unit::MoveLandAct;
					act->destPos = Vector2( 400 , 400 );
					monster->pushAction( act );

					mEntityMgr.addEntity( monster );
				}
				//{
				//	TDUnit* monster = new TDUnit( AID_UT_MOUSTER_1 );
				//	monster->setPos( Vector2( 800 , 800 ) );
				//	monster->setOwner( &mPlayerInfo );

				//	TDUnit::MoveLandAct* act = new TDUnit::MoveLandAct;
				//	act->destPos = Vector2( 400 , 400 );
				//	monster->pushAction( act );

				//	mEntityMgr.addEntity( monster );
				//}

				spawnCount -= spawnSpeed;
			}
		}


		void update( int frame )
		{
			mEntityMgr.updateFrame( frame );
			mActComMsg.updateFrame( frame );
			//spawnCount += frame;
		}

		struct RenderParam
		{
			bool drawMouse;

		};

		void render( Renderer& renderer , RenderParam const& param );



		void     fireAction( ActionTrigger& trigger );

		void     changeBaseComMap( ActorId aID );
		Actor*   evalGroupActorCom( ActorId groupActorID , Vector2 wPos , unsigned flag );
		void     removeInSelectActor();

		void selectActor( Actor* actor )
		{
			//FIXEME
			mSelectActors.push_back( actor );
			actor->addFlag( EF_SELECTED );
		}

		bool tryPlaceUnit( Unit* unit , Building* building , Vector2 const& targetPos );

		bool tryPlaceUnitInternal( Unit* unit , Vector2 const& startPos, Vector2 const& offsetDir, float maxOffset );

		void removeAllSelect()
		{
			for( ActorVec::iterator iter = mSelectActors.begin();
				iter != mSelectActors.end() ; ++iter )
			{
				if ( *iter )
					(*iter)->removeFlag( EF_SELECTED );
			}
			mSelectActors.clear();
			mComID      = CID_NULL;
			mSelectMode = SM_NORMAL;
		}

		ActorId getCurGroupID()
		{
			if ( mSelectActors.empty() )
				return AID_NONE;
			//FIAXME
			return mSelectActors.back()->getActorID();
		}

		ActorVecIter getGroupIteraotr( ActorId aID )
		{
			ActorVecIter iter = mSelectActors.begin();
			for( ; iter != mSelectActors.end() ;++iter )
			{
				if ( (*iter)->getActorID() == aID )
					break;
			}
			return iter;
		}

		Actor* getNextActor( ActorId aID , ActorVecIter& iter )
		{
			for( ; iter != mSelectActors.end() ;++iter )
			{
				Actor* actor = (*iter);
				if ( actor->getActorID() == aID )
				{
					++iter;
					return actor;
				}
			}
			return NULL;
		}



		Actor*  choiceGroupBestActor( ActorId aID )
		{
			ActorVecIter iter = getGroupIteraotr( aID );

			Actor* bestActor = NULL;
			size_t   numCom    = 2048;

			while( Actor* curActor = getNextActor( aID , iter ) )
			{
				if ( curActor->getCurActCommand() == NULL )
				{
					return curActor;
				}

				if ( curActor->getActQueueNum() < numCom )
				{
					bestActor = curActor;
					numCom = curActor->getActQueueNum();
				}
			}
			return bestActor;
		}


		Building*   constructBuilding( ActorId type , Unit* builder , Vector2 const& pos );
		
		void        selectActorInRange( TDRect const& rect , ActorId actorID  );
		Actor*      getTargetActor( Vector2 const& wPos );
		unsigned    getActorBaseComMapID( ActorId aID );
		void        selectActorInRange( TDRect const& rect , EntityFilter const& filter );
		
		void        onPlayerEvent( EntityEvent event , Entity* entity );
		void        onMonsterEvent( EntityEvent event , Entity* entity );
		Viewport&   getCtrlViewport(){  return  mVPCtrl; }
		void        changeComMap( ComMapID id )
		{
			mCurComMapID = id;
			if ( mControlUI )
				mControlUI->setComMap( id );
		}

		void            setSelectMode( TDSelectMode mode );

		ActComMessage    mActComMsg;
		ActorId          mComActorID;
		ComID            mComID;
		TDSelectMode     mSelectMode;
		ActorVec         mSelectActors;
		Viewport         mVPCtrl;
		PlayerInfo       mPlayerInfo;
		unsigned         mCurComMapID;

		typedef std::list< Builder::BuildAct* > BuildActList;

		PlayerInfo      mMonsterPlayerInfo;

		Builder*        mBuiler;
		Controller*     mController;
		IControlUI*     mControlUI;
	};

}//namespace TowerDefend

#endif // TDLevel_h__
