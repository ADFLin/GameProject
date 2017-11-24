#ifndef TDEntityCommon_h__
#define TDEntityCommon_h__

#include "TDDefine.h"
#include "TDEntity.h"
#include "TDCollision.h"

#include <deque>

namespace TowerDefend
{

	enum EntityType
	{
		ET_BULLET   = BIT(0),
		ET_ACTOR    = BIT(1),
		ET_UNIT     = BIT(2),
		ET_BUILDER  = BIT(3),
		ET_BUILDING = BIT(4),
		ET_TOWER    = BIT(5),
	};


	class Actor;
	class Unit;
	class Building;
	class Bullet;
	class Builder;
	class Tower;

	typedef Handle< Bullet >   BulletPtr;
	typedef Handle< Builder >  BuilderPtr;
	typedef Handle< Tower >    TowerPtr;
	typedef Handle< Actor >    ActorPtr;
	typedef Handle< Unit  >    UnitPtr;
	typedef Handle< Building > BuildingPtr;

	class IActorFactory
	{
	public:
		virtual Actor* create( ActorId aID ) = 0;
	};

	enum ComFlag
	{
		CF_OP_APPEND     = BIT( 0 ) ,
		CF_OP_INVERSE    = BIT( 1 ) ,
		CF_OP_GOURP      = BIT( 2 ) ,
	};

	class ActEnumer
	{
	public:
		virtual ~ActEnumer(){}
		virtual void haveGoalPos( Vector2 const& pos ) = 0;
		virtual void haveTarget ( Entity* entity ) = 0;
	};
	class ActCommand
	{
	public:
		virtual ComID getComID(){  return  CID_NULL;  }
		virtual void onStart( Actor* actor ){}
		virtual bool onTick( Actor* actor ) = 0;
		virtual void onEnd( Actor* actor , bool beCompelete ){}
		virtual void onSkip( Actor* actor ){}

		virtual void describe( ActEnumer& enumer ){}
	};

	class Actor : public Entity
	{
		typedef Entity BaseClass;
		DEF_TD_ENTITY_TYPE( ET_ACTOR )
	public:
		Actor( ActorId aID );
		~Actor();
		virtual void onTick();
		virtual void onDestroy();

		bool testFilter( EntityFilter const& filter ) override;

		virtual bool attack( Actor* entity , Vector2 const& pos ){ return true; }

		ActCommand* getCurActCommand(){ return mCurAct; }
		void        changeAction( ActCommand* act , bool beRemoveQueue );
		void        pushAction( ActCommand* act );
		void        addAction( ActCommand* act , unsigned flag );

		void        removeActQueue();
		ActCommand* getLastQueueAct(){ return mActQueue.back(); }
		ActCommand* getQueueAct( int idx ){ return mActQueue[ idx ]; }
		size_t      getActQueueNum(){  return mActQueue.size();  }


		float       recvDamage( DamageInfo const& info );

		virtual float calcDamage( DamageInfo const& info ){ return info.power;  }
		virtual void  onTakeDamage( DamageInfo const& info , float damageValue ){}
		virtual void  onKilled(){ destroy(); }

		virtual bool evalCommand        ( ComID comID , Actor* target , Vector2 const& pos , unsigned flag ){  return false;  }
		virtual bool evalDefaultCommand ( Actor* target , Vector2 const& pos , unsigned flag ){ return false; }
		virtual bool evalActorCommand   ( ComID comID , ActorId aID , Vector2 const& pos , unsigned flag ){ return false; }


		unsigned      getOwnerID() const { return mOwner->id; }
		PlayerInfo*   getOwner(){ return mOwner;}
		void          setOwner( PlayerInfo* info ) { mOwner = info;  }
		ActorId       getActorID() const { return  mActorID;  }

		static  ComMap&   getComMap( ComMapID id );
		virtual ComMapID  getComMapID( ComID id ){ return ComMapID(0); }

		float         getLifeValue(){ return mLifeValue; }
	protected:

		ActorId       mActorID;
		PlayerInfo*   mOwner;
		unsigned      mControlBit;
		typedef std::deque< ActCommand* > ActComQueue;
		ActCommand*   mCurAct;
		ActComQueue   mActQueue;

		float         mLifeValue;
	};


	class Unit : public Actor
	{
		typedef Actor BaseClass;
		DEF_TD_ENTITY_TYPE( ET_UNIT )
	public:
		Unit( ActorId aID );

		void      doRender( Renderer& rs );
		float     getMoveSpeed() const { return getUnitInfo().moveSpeed;  }
		void      onChangePos();

		virtual bool evalCommand( ComID comID , Actor* target , Vector2 const& pos , unsigned flag );
		virtual bool evalDefaultCommand( Actor* target , Vector2 const& pos , unsigned flag );

		virtual void addMoveActionCom( Vector2 const& pos , unsigned flag );
		virtual void addAttackCom( Vector2 const& pos , Actor* target , unsigned flag ){}

		class MoveAirAct : public ActCommand
		{
		public:
			bool onTick( Actor* actor ) override;
			void describe( ActEnumer& enumer ) override;
			Vector2   destPos;
		};
		class MoveLandAct : public ActCommand
		{
		public:
			MoveLandAct()
			{
				mMode = TM_NONE;
			}
			bool onTick( Actor* actor );
			Vector2   destPos;
		private:
			enum MoveMode
			{
				TM_NONE ,
				TM_BLOCK    ,
				TM_POSITIVE ,
				TM_NEGATIVE ,
			};

			MoveMode  mMode;
			EntityPtr mColEntity;
		};
		void onSpawn();
		void onDestroy();

		class FollowAct : public ActCommand
		{
		public:
			bool onTick( Actor* actor ) override;
			void describe( ActEnumer& enumer ) override;
			ActorPtr target;
		};

		float    getColRadius(){ return getUnitInfo().colRadius; }
		UnitInfo const& getUnitInfo() const { return getUnitInfo( mActorID );}

		ColObject& getColBody(){ return mColBody; }

		static UnitInfo const& getUnitInfo( ActorId type );
		friend class CollisionManager;
		ColObject mColBody;
	};

	class Building : public Actor
	{
		typedef Actor BaseClass;
		DEF_TD_ENTITY_TYPE( ET_BUILDING )
	public:
		Building( ActorId aID );

		virtual bool evalDefaultCommand( Actor* target , Vector2 const& pos , unsigned flag )
		{
			return false;
		}
		virtual bool evalActorCommand( ComID comID , ActorId aID , Vector2 const& pos , unsigned flag );
		void doRender( Renderer& renderer );

		BuildingInfo const& getBuildingInfo(){ return getBuildingInfo( getActorID() ); }
		static BuildingInfo const& getBuildingInfo( ActorId type );


		struct UnitActTarget
		{
			ActorPtr actor;
			Vector2      pos;
		};


		class ConstructAct : public ActCommand
		{
		public:
			void onStart( Actor* actor ) override;
			bool onTick( Actor* actor ) override;
			void onSkip( Actor* actor ) override;
			void onEnd( Actor* actor , bool beCompelete ) override;
			float  progressCount;
		};

		ConstructAct* setupBuild();

		class ProductUnitAct : public ActCommand
		{
		public:
			void onStart( Actor* actor ) override;

			bool onTick( Actor* actor ) override;
			void onEnd( Actor* actor , bool beCompelete ) override;
			void onSkip( Actor* actor ) override;

			enum Step
			{
				eCheckPopu ,
				eProduce  ,
				ePutUnit  ,
			};

			Unit*   mUnit;
			Step      mStep;
			float     progressCount;    
			ActorId unitID;
		};
	};


	class Bullet : public Entity
	{
		typedef Entity BaseClass;
		DEF_TD_ENTITY_TYPE( ET_BULLET )
	public:
		typedef fastdelegate::FastDelegate< void ( Bullet& ) > HitFun;
		typedef fastdelegate::FastDelegate< bool ( Bullet& ) > MoveFun;

		Bullet( MoveFun const& fun ): mMoveFun( fun ){ ADD_ENTITY_TYPE() }

		void onTick() override;

		void doRender( Renderer& renderer );
		void setHitFun( HitFun const& fun ){ mHitFun = fun; }

		HitFun     mHitFun;
		MoveFun    mMoveFun;
		Vector2      mLocationParam;
		ActorPtr   mTarget;
	};

	class Builder : public Unit
	{
		typedef Unit BaseClass;
		DEF_TD_ENTITY_TYPE( ET_BUILDER )
	public:
		Builder( ActorId aID );

		bool  evalActorCommand( ComID comID , ActorId aID , Vector2 const& pos , unsigned flag ) override;
		bool  evalDefaultCommand( Actor* target , Vector2 const& pos , unsigned flag ) override;



		void  addMoveActionCom( Vector2 const& pos , unsigned flag );

		class BuildAct : public ActCommand
		{
		public:
			typedef Building::ConstructAct ConstructAct;
			void onStart( Actor* actor ) override;
			bool onTick( Actor* actor ) override;
			void onEnd( Actor* actor , bool beCompelete ) override;
			ActorId      blgID;
			Vector2          destPos;
			BuildingPtr  building;
			ConstructAct*  constructAct;
		};
	};



	class Tower : public Building
	{
		typedef Building BaseClass;
		DEF_TD_ENTITY_TYPE( ET_TOWER )

	public:
		Tower( ActorId type );

		bool attack( Actor* entity , Vector2 const& pos );



		virtual bool evalDefaultCommand( Actor* target , Vector2 const& pos , unsigned flag );

		void bulletNormalHit( Bullet& bullet );
		bool bulletNormalMove( Bullet& bullet );


		void onTick();

		class AttackCom : public ActCommand
		{
		public:
			AttackCom()
			{
				attackCount = 0;
			}
			bool onTick( Actor* actor );
			ActorPtr  target;
			float     attackCount;
		};

		float getAttackPower(){ return mATPower; }
		float getAttackRange(){ return mATRange; }

		// ( times per-sec )
		float getActtackSpeed(){ return mATSpeed;  }

		float     mATRange;
		float     mATPower;
		float     mATSpeed;
	};

}//namespace TowerDefend

#endif // TDEntityCommon_h__
