#ifndef CHero_h__
#define CHero_h__


#include "common.h"
#include "CActor.h"

#define  ERROR_BONE_ID -1
class CNPCBase;

enum HeroAnimType
{
	ANIM_HERO_IDLE_1   = ANIM_FUN_1 ,
	ANIM_HERO_ATTACK_2 = ANIM_FUN_2 ,
	ANIM_HERO_ATTACK_3 = ANIM_FUN_3 ,	
};

class CHero;
CHero* getPlayer();

class CHero : public CActor
{
	DESCRIBLE_GAME_OBJECT_CLASS( CHero , CActor );
public:
	CHero();
	~CHero();
	
	bool init( GameObject* gameObj , GameObjectInitHelper const& helper );

	void update( long time );
	bool onAttackTest();


	void onDead();

	void takeDamage( DamageInfo const& info );

	//virtual
	void onKillActor( CActor* actor );
	virtual void evalAttackMove();

	virtual void onAnimationFinish( AnimationType finishPose );
protected:
	int   m_attackStep;
	float m_attackContinueTime;
	float m_lastAttackTime;

public:

	float getBodyHalfHeigh();

	void removeEquipmentModel( EquipSlot slot );
	void changeEquipmentModel( EquipSlot slot );
	void changeEquipmentModel( EquipSlot slot , CFObject* newObj );

	void talk( CNPCBase* npc );
	float m_idleTime;

	void idleThink( ThinkContent& content );
	bool tryBreakAction()
	{
		return false;
	}

	
	float m_frontSpeed;

	void processAction();

	float m_jumpSpeed;
	float m_moveSpeed;
	float m_moveAcc;
	float m_moveDeAcc;

	
	//void showPose()
	//{
	//	static int index = 0;
	//	m_curActionID = ActIDVec[ index ];
	//	index =  ( index + 1 ) % ActIDVec.size();
	//	setCurContextNextThink( g_GlobalVal.curtime + getPoseLoopTime(m_curActionID) );
	//}

	void makeSound( char const* name , float volume );

	void onEquipItem( EquipSlot slot , bool beRM );
	void keyHandle();
	void turnHead( float angle );

	void onDying();

	bool evalUse();


protected:
	float    m_curHeadAngle;
	//for use key
	EHandle  m_hUsePE;
	int      curExp;
	int      nextLevelExp;

	//OBJECTid  curSound;
	CFObject* mEquipObj[ EQS_EQUIP_TABLE_SLOT_NUM ];
	int       equipBoneID[ EQS_EQUIP_TABLE_SLOT_NUM ];

	int       shoulderBoneID_R;
	CFObject* shoulderObjID_R;

	float     m_attackTime;
};

#endif // CHero_h__
