#ifndef CActor_h__
#define CActor_h__

#include "common.h"

#include "GameObjectComponent.h"
#include "Thinkable.h"
#include "Holder.h"

#include "TAblilityProp.h"
#include "TItemSystem.h"

#include "CActorModel.h"

class PhyCollision;
class SNSpatialComponent;
class ActorMovement;
class AbilityProp;

class TSkill;
class TItemStorage;
class TEquipTable;
class TSkillBook;
class TColdDownManager;
class CActorModel;

enum ActivityType
{
	ACT_IDLE = 0    ,
	ACT_MOVING      ,
	ACT_ATTACK      ,
	ACT_BUF_SKILL   ,
	ACT_DEFENSE     ,
	ACT_JUMP        ,
	ACT_LADDER      ,
	ACT_DYING       ,
};



class CActor : public ILevelObject
{
	DESCRIBLE_GAME_OBJECT_CLASS( CActor , ILevelObject );
public:
	enum RenderSlot
	{
		RS_MODEL_SLOT = 0,
		RS_NEXT_SLOT ,
	};

	CActor();
	~CActor();
	virtual bool init( GameObject* gameObject , GameObjectInitHelper const& helper );
	virtual void release();

	ActorMovement* getMovement(){  return mMovement;  }
	CActorModel*   getModel(){  return mModel;  }
	//virtual 
	void update( long time );

	void onSpawn(){}
	virtual void processAction(){}

	virtual void onCollision( 
		PhyCollision* comp , float depth , 
		Vec3D const& selfPos , Vec3D const& colObjPos , 
		Vec3D const& normalOnColObj ){}


	//virtual float getBodyHalfHeigh();
	virtual Vec3D getEyePos();
	virtual Vec3D getHandPos(bool isRight);


	bool         attack();
	void         defense( bool beD );

	virtual void evalAttackMove();
	virtual void inputWeaponDamage( ILevelObject* dtEntity );
	virtual void takeDamage( DamageInfo const& info );
	virtual void onKillActor( CActor* actor ){}
	virtual bool onAttackTest();

	virtual void onDying(){}
	virtual void onDead(){}

	CActor* findEmpty( float dist );

	float  getAttackRange();
	float  getVisibleDistance();
	virtual float getFaceOffest();
	void   attackRangeTest( Vec3D const& pos , Vec3D const& dir , float r , float angle );

	void    attackThink( ThinkContent& content );

	bool    isDead();
	bool    isEmpty( CActor* actor );
	Vec3D   getFaceDir() const;
	void    setFaceDir( Vec3D const& front );


	virtual void onSkillEnd();
	virtual bool canUseSkill(char const* name);

	void    cancelSkill();
	void    castSkillThink( ThinkContent& content );
	void    castSustainedSkillThink( ThinkContent& content );
	void    castSkill( char const* name );
	float   getCurBufTime(){ return m_skillbufTime; }

	void          setActivity( ActivityType type );

	TSkillBook&   getSkillBook(){ return *m_skillBook; }
	TColdDownManager&   getCDManager(){ return *m_cdManager; }


protected:
	

	TPtrHolder< TColdDownManager > m_cdManager;
	TPtrHolder< TSkillBook > m_skillBook;

	TSkill*       m_curSkill;
	float         m_skillbufTime;

public:
	virtual float getMaxMoveSpeed();
	virtual float getMaxRotateAngle();


	void       turnRight( float angle );
	void       moveFront( float offset );
	void       moveSpace( Vec3D const& offset );
	void       jump( Vec3D const& vecticy );


protected:
	void       animationEndThink( ThinkContent& content );
	void       addAnimationEndThink( AnimationType anim );

public:

	int  getHP() const;
	int  getMP() const;

	void setHP(int val);
	void setMP(int val);

	float getPropValue( PropType prop ) const;

	void addPropModify( PropModifyInfo& info );
	void addStateBit( ActorState state )
	{
		m_stateBit |= state;
	}
	unsigned getStateBit(){ return m_stateBit; }
	void     removeStateBit(ActorState state ){ m_stateBit &= (~state); }

protected:
	
	unsigned      m_stateBit;

public:
	virtual bool  canUseItem( unsigned itemID ){ return true; }

	virtual void  onUseItem( unsigned itemID ){}
	virtual void  onEquipItem( EquipSlot slot , bool beRM ){}
	//virtual void  onEquip( unsigned itemID );
	bool          equip( char const* itemName);
	bool          equip( unsigned itemID );
	void          removeEquipment( EquipSlot slot );

	bool          playItemBySlotPos( int pos );

	bool          useItem( char const* itemName );
	bool          useItem( unsigned itemID );
	void          doUseItem( unsigned itemID );
	bool          doEquip( unsigned itemID );

	bool          addItem( char const* itemName , int num = 1);
	bool          addItem( unsigned itemID , int num = 1);
	void          removeItem( char const* itemName );
	TItemStorage& getItemStorage(){ return *mItemStroage; }
	TEquipTable&  getEquipTable(){ return *mEquipmentTable; }


	unsigned     getMoney(){ return m_money; }
	bool         spendMoney( unsigned  much ){ return true; }
	void         addMoney( unsigned  much );
protected:
	void         computeAbilityProp();

	unsigned   m_money;
	TPtrHolder< TItemStorage > mItemStroage;
	TPtrHolder< TEquipTable >  mEquipmentTable;

public:
	virtual void onAnimationFinish( AnimationType finishPose );

	ActivityType getActivityType(){ return mActState; }

	bool         canDoAction();
	unsigned     getTeam(){ return m_team; }
	void         setTeam( unsigned team ){ m_team = team; }
	unsigned     getRoleID();

	AbilityProp* getAbilityProp() { return mAbilityProp; }
protected:

	CActorModel*                mModel;
	SNSpatialComponent*         mSpatialComp;
	TPtrHolder< ActorMovement > mMovement;
	TPtrHolder< AbilityProp >   mAbilityProp;

	unsigned      m_roleID;
	unsigned      m_team;
	ActivityType  mActState;

	friend struct TActorModelData;
};

class CActorScript : public IEntityScript
{
public:
	virtual void  createComponent( CEntity& entity , EntitySpawnParams& params );
};


#endif // CActor_h__
