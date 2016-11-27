#include "ProjectPCH.h"
#include "CHero.h"

#include "AbilityProp.h"
#include "CActorModel.h"
#include "ActorMovement.h"

#include "TRoleTemplate.h"
#include "TEventSystem.h"
#include "EventType.h"

#include "UtilityGlobal.h"


#define BONE_HEAD "Hero_Head"
#define BONE_UPPER_ARM_L "Hero_L_UpperArm"
#define BONE_UPPER_ARM_R "Hero_R_UpperArm"

DEFINE_DOWNCAST( CHero , LOT_PLAYER )


float gAttackEvalCDTime = 0.5;

static CHero* g_Player = NULL;
CHero* getPlayer(){ return g_Player; }

CHero::CHero() 
{
	m_roleID = ROLE_HERO;
	mObjectType |= LOT_PLAYER;

	g_Player = this;
	setTeam( TEAM_PLAYER );
}

CHero::~CHero()
{

}

bool CHero::init( GameObject* gameObj , GameObjectInitHelper const& helper )
{
	if ( !BaseClass::init( gameObj , helper ) )
		return false;

	m_moveSpeed  = 0;
	m_moveAcc    = 600;
	m_moveDeAcc  = 1000;

	m_curHeadAngle = 0;

	curExp = 0;
	nextLevelExp = 10;

	for( int i = 0 ; i < EQS_EQUIP_TABLE_SLOT_NUM ;++i )
	{
		equipBoneID[i] = ERROR_BONE_ID;
		mEquipObj[i] = nullptr;
	}

	CFActor* cfActor = mModel->getCFActor();

	equipBoneID[ EQS_HAND_L ]   = cfActor->getAttachmentParentBoneID(1);
	equipBoneID[ EQS_HAND_R ]   = cfActor->getAttachmentParentBoneID(0);
	equipBoneID[ EQS_HEAD   ]   = cfActor->getBoneID( BONE_HEAD );
	equipBoneID[ EQS_SHOULDER ] = cfActor->getBoneID( BONE_UPPER_ARM_L );

	shoulderBoneID_R = cfActor->getBoneID( BONE_UPPER_ARM_R );
	shoulderObjID_R = nullptr;

	CFObject* weapon;
	weapon = cfActor->getAttachment( 0 );
	assert( weapon );
	cfActor->takeOffAttachment( weapon );
	weapon->release();

	weapon = cfActor->getAttachment( 1 );
	assert( weapon );
	cfActor->takeOffAttachment( weapon );
	weapon->release();

	mModel->setAnimFlag( ANIM_HERO_ATTACK_2 , 0 );
	mModel->setAnimFlag( ANIM_HERO_ATTACK_2 , 1 );

	return true;
}


void CHero::keyHandle()
{
	if ( !canDoAction() )
		return;

	//if ( TCheckHotKeyStatus( KEY_UP ) ) 
	//{
	//	m_moveSpeed += m_moveAcc * g_GlobalVal.frameTime;
	//	m_moveSpeed = TMin( m_moveSpeed , getMaxMoveSpeed() );
	//}
	//else if ( m_actType == ACT_MOVING )
	//{
	//	m_moveSpeed -= m_moveDeAcc * g_GlobalVal.frameTime;
	//	m_moveSpeed = TMax( m_moveSpeed , 0.0f );
	//}

	if ( m_moveSpeed > 0 )
	{
		moveFront( m_moveSpeed );
	}
	else
	{
		mActState = ACT_IDLE;
		mModel->changeAction( ANIM_WAIT );
	}
}

void CHero::update( long time )
{
	BaseClass::update( time );

	if ( getActivityType() == ACT_MOVING ||
		 getActivityType() == ACT_IDLE )
	{
		//TEntityManager::iterator iter = TEntityManager::instance().getIter();

		//float    minDist2 = 1e8;
		//float    angle = 0;
		//TPhyEntity* findObj = NULL;

		//float searchRadius2 = 500 * 500;
		//while ( TPhyEntity* pe = TEntityManager::instance().findEntityInSphere( iter ,getPosition() , searchRadius2 ) )
		//{
		//	Vec3D dr = pe->getPosition() - getPosition();

		//	dr[2] = 0;

		//	float dist2 = dr.length2();
		//	if ( dist2 > 500 * 500 )
		//		continue;

		//	Vec3D faceDir = getFaceDir() ;
		//	float tAngle =  UM_Angle( dr , faceDir );

		//	if ( tAngle < DEG2RAD(90) && dist2 < minDist2 )
		//	{
		//		minDist2 = dist2;
		//		angle = TMin( DEG2RAD(60) , tAngle );
		//		if ( faceDir.cross( dr ).z() < 0 )
		//			angle = - angle;
		//		findObj = pe;
		//	}
		//}

		//m_hUsePE = findObj;

		//if ( findObj || fabs( m_curHeadAngle) > DEG2RAD(3) )
		//{
		//	turnHead( angle );
		//}
	}
}

void CHero::turnHead( float angle )
{
	//float dAngle = fabs( angle - m_curHeadAngle );

	//dAngle = TMin( DEG2RAD(6.0f) , dAngle );

	//if ( angle > m_curHeadAngle )
	//{
	//	m_curHeadAngle += dAngle;
	//}
	//else
	//{
	//	m_curHeadAngle -= dAngle;
	//}
	//FnObject headObj; 
	//headObj.Object( getFlyActor().GetBoneObject( BONE_HEAD ) );

	//headObj.Rotate( X_AXIS , RAD2DEG( m_curHeadAngle) , GLOBAL );
	//Vec3D f , u;
	//headObj.GetWorldDirection( f , u );
	//getFlyActor().PerformSkinDeformation();
}

void CHero::idleThink( ThinkContent& content )
{
	if ( mActState == ACT_IDLE )
	{
		m_idleTime += content.loopTime;
		if ( m_idleTime > 5 )
		{
			mModel->changeAction( AnimationType( ANIM_HERO_IDLE_1 ) );
		}
	}
	else
	{
		m_idleTime = 0;
		mActState = ACT_IDLE;
	}
}


void CHero::changeEquipmentModel( EquipSlot slot , CFObject* newObj )
{
	newObj->changeScene( mModel->getCFActor()->getScene() );

	if ( mEquipObj[slot] )
	{
		mModel->getCFActor()->takeOffAttachment( mEquipObj[slot] );
		mEquipObj[ slot ]->release();
		mEquipObj[ slot ] = nullptr;
	}

	int boneID = equipBoneID[ slot ];

	if ( boneID != ERROR_BONE_ID )
	{
		mModel->getCFActor()->applyAttachment( newObj , boneID );
		mEquipObj[ slot ] = newObj;
	}
}

void CHero::changeEquipmentModel( EquipSlot slot )
{
	removeEquipmentModel( slot );

	TEquipment* equip = getEquipTable().getEquipment( slot );

	if ( !equip )
		return;

	CFObject* equipObj[5];
	if ( equip->createModel( equipObj ) )
	{
		int boneID = equipBoneID[ slot ];

		assert( boneID != ERROR_BONE_ID );

		equipObj[0]->changeScene( mModel->getCFActor()->getScene() );
		mModel->getCFActor()->applyAttachment( equipObj[0] , boneID );
		mEquipObj[ slot ] = equipObj[0];

		if ( slot ==  EQS_SHOULDER )
		{
			equipObj[1]->changeScene( mModel->getCFActor()->getScene() );
			shoulderObjID_R = equipObj[1];
			mModel->getCFActor()->applyAttachment( equipObj[1] , shoulderBoneID_R );
		}
	}
}

void CHero::onEquipItem( EquipSlot slot , bool beRM )
{
	if ( beRM )
	{
		mModel->getCFActor()->takeOffAttachment( mEquipObj[slot] );
		mEquipObj[ slot ]->release();
		mEquipObj[ slot ] = nullptr;

		if ( slot ==  EQS_SHOULDER )
		{
			mModel->getCFActor()->takeOffAttachment( shoulderObjID_R );
			shoulderObjID_R->release();
			shoulderObjID_R = nullptr;
		}
	}
	else
	{
		if ( slot == EQS_HAND_L ||
			slot == EQS_HAND_R )
		{
			changeEquipmentModel( EQS_HAND_L );
			changeEquipmentModel( EQS_HAND_R );
		}
		else
		{
			changeEquipmentModel( slot );
		}
	}

	//TShadowManager::instance().refreshPlayerObj();
}

void CHero::onKillActor( CActor* actor )
{
	int exp = actor->getAbilityProp()->getPropValue( PROP_KEXP );

	curExp += exp;

	while ( curExp > nextLevelExp )
	{
		TRoleManager::getInstance().getRoleInfo( m_roleID )->levelUp();
		computeAbilityProp();

		nextLevelExp = nextLevelExp + nextLevelExp * 1.5;

		TEvent event( EVT_LEVEL_UP , EVENT_ANY_ID , this , this );
		UG_SendEvent( event );
	}
}

void CHero::evalAttackMove()
{
	if ( g_GlobalVal.curtime - m_lastAttackTime > 0.5 ||
		m_attackStep >= 3 )
		m_attackStep = 0;

	m_attackStep++;

	int anim = 0;

	if ( getActivityType() == ACT_JUMP )
	{

	}

	float testFactor = 0.8f;
	if ( m_attackStep == 1 )
	{
		anim = ANIM_ATTACK;
	}
	else if ( m_attackStep == 2 )
	{
		anim = ANIM_HERO_ATTACK_2;

		if ( getEquipTable().haveEquipment( EQS_HAND_L ) )
			anim = ANIM_HERO_ATTACK_2;
		else if ( getEquipTable().haveEquipment( EQS_HAND_R ) )
		{
			m_attackStep = 3;
			anim = ANIM_HERO_ATTACK_3;
		}
		else
		{
			m_attackStep = 1;
			anim = ANIM_ATTACK;
		}
	}
	else if ( m_attackStep == 3 )
	{
		anim = ANIM_HERO_ATTACK_3;
	}

	mModel->changeAction( AnimationType( anim ) , false );
	addAnimationEndThink(  AnimationType(anim) );

	setActivity( ACT_ATTACK );
	
	ThinkContent* tc = setupThink( this , &ThisClass::attackThink );
	tc->nextTime = g_GlobalVal.curtime + testFactor * mModel->getAnimTimeLength( AnimationType(anim) );

	m_attackTime = 0.0f;

	makeSound( "hero_attack" , 1.0 );
}

void CHero::onAnimationFinish( AnimationType finishPose )
{
	
	switch ( finishPose )
	{
	case ANIM_ATTACK:
		m_lastAttackTime = g_GlobalVal.curtime;
		break;
	case ANIM_HERO_ATTACK_2:
	case ANIM_HERO_ATTACK_3:
		setActivity( ACT_IDLE );
		mModel->changeAction( ANIM_WAIT );
		m_lastAttackTime = g_GlobalVal.curtime;
		break;
	}

	BaseClass::onAnimationFinish( finishPose );
}

void CHero::onDead()
{
	BaseClass::onDead();

	TEvent event( EVT_PlAYER_DEAD , getRefID() , this , this );
	UG_SendEvent( event );
}

void CHero::takeDamage( DamageInfo const& info )
{
	BaseClass::takeDamage( info );

	if ( getHP() > 0 )
	{
		makeSound( "hero_hurt" , 1.0 );
	}
}


void CHero::makeSound( char const* name , float volume )
{
	//Fn3DAudio audio;
	//audio.Object( curSound );
	//if ( audio.IsPlaying() )
	//	return;

	//curSound = UG_PlaySound3D( getFlyActor().GetBaseObject() , name , volume );
}

void CHero::onDying()
{
	BaseClass::onDying();
	makeSound( "hero_dying" , 1.0 );
}

bool CHero::evalUse()
{
	//TPhyEntity* pe = m_hUsePE;
	//if ( !pe )
	//	return false;

	//pe->interact();
	return true;
}

void CHero::processAction()
{
	if ( mActState == ACT_JUMP )
	{
		if ( mMovement->onGround() )
		{
			setActivity( ACT_IDLE );
		}
	}
}

void CHero::removeEquipmentModel( EquipSlot slot )
{
	if ( mEquipObj[slot] )
	{
		mModel->getCFActor()->takeOffAttachment( mEquipObj[slot] );
		mEquipObj[slot]->release();
		mEquipObj[slot] = nullptr;
	}
}

bool CHero::onAttackTest()
{
	if ( !BaseClass::onAttackTest() )
		return false;

	return true;
}

float CHero::getBodyHalfHeigh()
{
	//FIXME
	return 100;
}
