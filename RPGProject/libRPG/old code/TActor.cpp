#include "TActor.h"
#include "TObject.h"
#include "TResManager.h"
#include "TEntityManager.h"
#include "shareDef.h"
#include "TEffect.h"
#include "TPhySystem.h"
#include "UtilityGlobal.h"
#include "TItemSystem.h"
#include "TCombatSystem.h"
#include "TEventSystem.h"
#include "EventType.h"
#include "TMovement.h"
#include "TAnimation.h"
#include "TSkill.h"
#include "debug.h"
#include "shareDef.h"
#include "UtilityMath.h"
#include "TSkillBook.h"
#include "TActorTemplate.h"
#include "TFxPlayer.h"
#include "TLevel.h"

#define  FX_HIT "hit"

DEFINE_UPCAST( TActor , ET_ACTOR );
DEFINE_HANDLE( TActor );

TActor::TActor( unsigned roleID , XForm const& trans  ) 
	:TPhyEntity( PHY_SENSOR_BODY, TRoleManager::instance().getModelID( roleID ) )
{
	m_entityType |= ET_ACTOR;
	m_roleID = roleID;

	TResManager& manager = TResManager::instance();

	TActorModelData* data = (TActorModelData*) manager.getModelData( m_ModelID );
	char const* modelName = TRoleManager::instance().getRoleData( roleID )->modelName.c_str();

	m_actor.Object( manager.cloneActor( modelName ) );

	m_motionState =  new TObjMotionState( trans , m_actor.GetBaseObject() , data->modelOffset );
	getPhyBody()->setMotionState( m_motionState );

	init();
}

TActor::TActor( ACTORid id , TPhyBodyInfo const& info ) 
	:TPhyEntity( PHY_SENSOR_BODY , info )
{
	m_roleID = DEFULT_ROLE_ID;
	m_entityType |= ET_ACTOR;
	m_actor.Object( id );
	//m_actor.SetDirectionAlignment( X_AXIS , Z_AXIS );

	m_motionState =  new TObjMotionState( info.m_startWorldTransform , m_actor.GetBaseObject() );
	getPhyBody()->setMotionState( m_motionState );

	init();
}

TActor::TActor( char const* modelName , XForm const& trans ) 
	:TPhyEntity( PHY_SENSOR_BODY, TResManager::instance().getActorModelID( modelName ) )
{
	m_entityType |= ET_ACTOR;
	m_roleID = DEFULT_ROLE_ID;

	TResManager& manager = TResManager::instance();

	TActorModelData* data = (TActorModelData*) manager.getModelData( m_ModelID );
	m_actor.Object( manager.cloneActor( modelName ) );

	m_motionState =  new TObjMotionState( trans , m_actor.GetBaseObject() , data->modelOffset );
	getPhyBody()->setMotionState( m_motionState );

	init();
}

TActor::~TActor()
{
	FnScene scene;
	scene.Object( getFlyActor().GetScene() );
	scene.DeleteActor( getFlyActor().Object() );

}

void TActor::init()
{
	m_money = 0;
	m_stateBit = 0;


	btBoxShape* colShape = (btBoxShape*) getCollisionShape();
	Vec3D halfExtent = colShape->getHalfExtentsWithMargin();
	
	m_movement.reset(  
		new TMovement( this , ( btConvexShape* )getCollisionShape() , 
		               g_DefultStepHeight , 2 * halfExtent.z() ) );
	
	m_animation.reset( new TAnimation( *this ) );
	m_itemStroage.reset( new TItemStorage( 30 ) );
	m_equipmentTable.reset( new TEquipTable );
	m_skillBook.reset(  new TSkillBook( getRefID() ) );
	m_cdManager.reset(  new TCDManager( getRefID() ) );

	m_abilityProp.reset( new TAbilityProp( m_roleID ) );

	m_animation->changeAction( ANIM_WAIT );
	m_movement->setJumpHeight( getPropValue(PROP_JUMP_HEIGHT) );

	m_curSkill = NULL;
}


void TActor::updateFrame()
{
	BaseClass::updateFrame();

	//§ó·sÄÝ©Êª¬ºA­È
	class ModifyCallBack : public PropModifyCallBack
	{
	public:
		ModifyCallBack( TActor* _actor ) : actor(_actor){}
		void process( PropModifyInfo& info , bool beD )
		{
			if ( beD )actor->removeStateBit( info.state );
		}
		TActor* actor;
	} callback( this );
	m_abilityProp->updateFrame( callback );

	processAction();

	m_movement->updateAction(  g_GlobalVal.frameTime );

	updateAnimation();

	m_cdManager->updateFrame();

}

TActorModelData& TActor::getModelData( unsigned modelID )
{
	TObjModelData* data = TResManager::instance().getModelData(modelID);
	assert( data != NULL );
	return *( (TActorModelData*)data );
}

void TActor::OnTakeDamage( DamageInfo const& info )
{
	if ( isDead() )
	{

	}
	else
	{
		PropModifyInfo PMInfo;
		PMInfo.prop = PROP_HP;
		PMInfo.val =  info.damageResult;
		PMInfo.flag = PF_VOP_SUB;

		m_abilityProp->addPropModify( PMInfo );

		switch ( m_actType )
		{
		case ACT_BUF_SKILL:
			m_skillbufTime = TMax( m_skillbufTime - 1 , 0.0f );
			break;
		}

		if ( getHP() <= 0 )
		{
			OnDying();
			
			if ( TActor* actor = TActor::upCast(info.attacker) )
			{
				actor->OnKillActor( this );
			}
		}
		else
		{
			setAnimation( ANIM_HURT );
		}
	}
}


TActor* TActor::findEmpty( float dist )
{
	TEntityManager& manager = TEntityManager::instance();
	TEntityManager::iterator iter = manager.getIter();

	Float dist2 = dist * dist;

	TPhyEntity* pe = NULL;

	while ( pe = manager.findEntityInSphere( iter , getPosition() , dist2 ) )
	{
		TActor* actor = TActor::upCast( pe );
		if ( actor && actor != this && 
			!actor->isDead() && isEmpty(*actor) )
		{
			return actor;
		}
	}
	return NULL;
}


void TActor::attackRangeTest( Vec3D const& pos , Vec3D const& dir , Float r , Float angle )
{
	TEntityManager& manager = TEntityManager::instance();

	TEntityManager::iterator iter = manager.getIter();
	float r2 = r * r;

	TPhyEntity* pe = NULL;

	while ( pe = manager.findEntityInSphere( iter , pos ,r2 )  )
	{
		if ( pe == this ) 
			continue;

		TActor* actor = TActor::upCast( pe );
		if ( actor )
		{
			if ( !isEmpty( *actor ) )
				continue;

			Vec3D objPos = pe->getPosition();
			Vec3D objDir = objPos - pos;

			float dotAngle = UM_Angle( dir , objDir );

			if ( dotAngle < angle / 2  )
			{
				inputWeaponDamage( pe );
			}
		}
		else
		{
			TObject* obj = TObject::upCast( pe );
			if ( !obj )
				continue;

			Vec3D objPos = pe->getPosition();
			Vec3D objDir = objPos - pos;

			float dotAngle = UM_Angle( dir , objDir );

			if ( dotAngle < angle / 2  )
			{
				objDir .normalize();
				obj->getPhyBody()->applyCentralImpulse( 100 * dir );
				obj->getPhyBody()->activate();
			}
		}
	}
}

float TActor::getFaceOffest()
{
	PhyShape* shape =  getPhyBody()->getCollisionShape();
	assert( shape->getShapeType() == BOX_SHAPE_PROXYTYPE );
	return 0.8 *  UPCAST_PTR( btBoxShape , shape )->getHalfExtentsWithMargin().y();
}

bool TActor::attack()
{
	if ( isDead() )
		return false;

	if ( getActivityType() == ACT_BUF_SKILL || 
		 getActivityType() == ACT_ATTACK )
		 return false;

	setActivity( ACT_ATTACK );

	evalAttackMove();


	return true;
}

Float TActor::getMaxMoveSpeed()
{
	return m_abilityProp->getPropVal( PROP_MV_SPEED );
}

#include "TNPCBase.h"
void TActor::setAnimation( int  anim , bool beForce )
{
	m_animation->changeAction( (AnimationType)anim  , beForce );
}

bool TActor::OnAttackTest()
{
	if ( m_actType != ACT_ATTACK )
		return false;

	Vec3D pos = getPosition() + getFaceOffest() * getFaceDir() ;
	attackRangeTest( pos , getFaceDir() , getAttackRange() , DEG2RAD(120) );

	return true;
}

void TActor::OnAnimationFinish( AnimationType finishPose )
{
	switch ( finishPose )
	{
	case ANIM_HURT:
	case ANIM_ATTACK :
		setActivity( ACT_IDLE );
		setAnimation( ANIM_WAIT );
		break;

	case ANIM_DYING :
		if ( ( getStateBit() & AS_DIE ) == 0 )
		{
			DevMsg( 5 , "Dying" );
			addStateBit( AS_DIE );
			OnDead();
		}
		else DevMsg( 5 , "Double Dying" );
		break;
	}
}

bool TActor::updateAnimation()
{
	return m_animation->update();
}

void TActor::setActivity( ActivityType type )
{
	switch ( m_actType )
	{
	case ACT_MOVING:
		setAnimation( ANIM_WAIT );
		break;
	case ACT_BUF_SKILL:
		break;
	case ACT_IDLE:
		break;
	case ACT_LADDER:
		getMovement().getMoveResult().type = MT_NORMAL_MOVE;
		break;
	}

	switch ( type )
	{
	case  ACT_LADDER:
		getMovement().getMoveResult().type = MT_LADDER;
		break;
	}

	m_actType = type;
}

void TActor::turnRight( float angle )
{
	Mat3x3 orn = getTransform().getBasis();
	orn *= Mat3x3( Quat( Vec3D(0,0,1), -angle ) );

	getPhyBody()->getWorldTransform().setBasis( orn );
	if ( getMotionState() )
		getMotionState()->setWorldTransform( getTransform() );
}

void TActor::moveFront( float offset )
{
	m_movement->setVelocityForTimeInterval( offset * getFaceDir() ,g_GlobalVal.frameTime );
	m_actType = ACT_MOVING;
	setAnimation( ANIM_RUN );
}

void TActor::moveSpace( Vec3D const& offset  )
{
	m_movement->setVelocityForTimeInterval( offset ,g_GlobalVal.frameTime );
}


float TActor::getBodyHalfHeigh()
{
	btBoxShape* shape = (btBoxShape*) getCollisionShape();
	return shape->getHalfExtentsWithoutMargin().z();
}

void TActor::setMovementType( MoveType type )
{
	getMovement().getMoveResult().type = type;
}


bool TActor::addItem( char const* itemName , int num )
{
	unsigned id = UG_GetItemID( itemName );
	if ( id == ITEM_ERROR_ID )
		return false;

	return addItem( id , num );
}

bool TActor::addItem( unsigned itemID , int num )
{
	assert( UG_GetItem( itemID ) );
	return m_itemStroage->addItem( itemID , num );
}

void TActor::setFaceDir( Vec3D const& front )
{
	getFlyActor().SetWorldDirection( (float*)&front , Vec3D(0,0,1) );
	updateTransformFromFlyData();
}

bool TActor::useItem( unsigned itemID )
{
	TItemBase* item = UG_GetItem(itemID);

	if ( !item )
	{
		DevMsg( 1 , "Use unknown ItemID = %d" , itemID );
		return false;
	}


	int itemNum = m_itemStroage->queryItemNum( itemID );

	int slotPos = getItemStorage().findItemSoltPos( itemID );

	if ( slotPos < 0 )
		return false;

	return playItemBySlotPos( slotPos );
}

bool TActor::useItem( char const* itemName )
{
	unsigned itemID = UG_GetItemID( itemName );
	if ( itemID == ITEM_ERROR_ID  )
	{
		DevMsg( 1 , "use unknown Item = %s" , itemName );
		return false;
	}
	return useItem( itemID );
}

void TActor::doUseItem( unsigned itemID )
{
	TItemBase* item = UG_GetItem( itemID );
	TItemBase::PMInfoVec& PMVec = item->getPropModifyVec();

	for ( int i = 0; i < PMVec.size() ; ++ i )
	{
		PropModifyInfo& info = PMVec[i];
		addStateBit( info.state );
		addPropModify( info );
	}

	if ( getHP() <= 0 )
	{
		OnDying();
	}

	item->use( this );
}

bool TActor::equip( char const* itemName )
{
	unsigned itemID = UG_GetItemID( itemName );
	if ( itemID == ITEM_ERROR_ID  )
	{
		DevMsg( 1 , "equip unknown Item = %s" , itemName );
		return false;
	}

	return equip( itemID );
}

bool TActor::equip( unsigned itemID )
{
	TItemBase* equipItem = UG_GetItem( itemID );

	if ( !equipItem )
	{
		DevMsg( 1 , "equip unknown ItemID = %d" , itemID );
		return false;
	}

	if ( !equipItem->isEquipment() )
		return false;

	int slotPos  = getItemStorage().findItemSoltPos(itemID);

	if ( slotPos < 0 )
		return false;

	return playItemBySlotPos( slotPos );
}



bool TActor::doEquip( unsigned itemID )
{
	TItemBase* equipItem = UG_GetItem( itemID );
	
	if ( !equipItem->isEquipment() )
		return false;
	
	EquipSlot removeSlot[10];
	int numRemove = m_equipmentTable->queryRemoveEquip( itemID , removeSlot );

	if ( !m_itemStroage->haveEmptySolt(numRemove) )
		return false;

	unsigned removeID[10];
	for( int i = 0 ; i < numRemove ; ++ i )
	{
		unsigned rmItemID = getEquipTable().removeEquip( removeSlot[i]);
		
		m_itemStroage->addItem( rmItemID );

		removeID[i] = rmItemID;
	}

	EquipSlot slot = m_equipmentTable->equip( itemID );

	//int chPropBit = 0;

	//for( int i = 0; i < numRemove ; ++i )
	//{
	//	TItemBase* item = UG_GetItem( removeItem[i] );
	//	TItemBase::PMInfoVec& PMVec = item->getPropModifyVec();
	//	for ( int n = 0 ; n < PMVec.size(); ++n )
	//	{
	//		chPropBit |= ( 1 << PMVec[n].prop );
	//	}
	//}

	computAbilityProp();
	OnEquipItem( slot , false );

	return true;
}


void TActor::computAbilityProp()
{
	int HP = getHP();
	int MP = getMP();

	m_abilityProp->resetAllPropVal();

	for ( int i = 0 ; i < EQS_EQUIP_TABLE_SLOT_NUM ; ++i )
	{
		TEquipment* equi = m_equipmentTable->getEquipment( EquipSlot(i) );

		if ( !equi )
			continue;

		PropModifyInfo info;
		info.val  = equi->getEquiVal();
		info.flag   = PF_VOP_ADD;

		if ( i == EQS_HAND_R )
			info.prop = PROP_MAT;
		else if( i == EQS_HAND_L  && equi->isWeapon() )
			info.prop = PROP_SAT;
		else
			info.prop = PROP_DT;

		m_abilityProp->addPropModify( info );

		TItemBase::PMInfoVec& PMVec = equi->getPropModifyVec();
		for( int n = 0; n < PMVec.size() ; ++n )
		{
			m_abilityProp->addPropModify( PMVec[n] );
		}
	}
	for( int i = 0 ; i < PROP_TYPE_NUM ; ++ i )
	{
		m_abilityProp->updatePropVal( PropType(i) );
	}

	m_abilityProp->setHP( HP );
	m_abilityProp->setMP( MP );
}

void TActor::removeEquipment( EquipSlot slot )
{
	unsigned itemID = getEquipTable().removeEquip( slot );
	if ( itemID != ITEM_ERROR_ID )
	{
		getItemStorage().addItem( itemID , 1 );
		OnEquipItem( slot , true );
		computAbilityProp();
	}
}
bool TActor::playItemBySlotPos( int slotPos )
{
	TItemBase* item = m_itemStroage->getItem( slotPos );
	if ( !item || m_itemStroage->getItemNum( slotPos ) <= 0 )
		return false;

	if ( item->isEquipment() )
	{
		if ( doEquip( item->getID() ) )
		{
			getItemStorage().removeItemInSlotPos( slotPos );
			return true;
		}
	}
	else
	{

		if ( !m_cdManager->canPlayItem( item->getName() ) )
			return false;

		doUseItem( item->getID() );

		int num = m_itemStroage->removeItemInSlotPos( slotPos );
		assert( num == 1 );

		m_cdManager->pushItem( item->getName() , item->getCDTime() );
		OnUseItem( item->getID() );

		return true;
	}

	return false;
}

void TActor::processCastSkill()
{
	if ( !m_curSkill )
		return;

	if ( m_skillbufTime < m_curSkill->getInfo().bufTime )
	{
		m_skillbufTime += g_GlobalVal.frameTime;
		setCurContextNextThink( g_GlobalVal.nextTime );
		return;
	}

	bool successCast = false;
	unsigned flag = m_curSkill->getInfo().flag;
	TPhyEntity* entity = NULL;

	setMP( getMP() - m_curSkill->getInfo().castMP );

	if ( flag & SF_SELECT_EMPTY )
	{
		entity = findEmpty( m_curSkill->getInfo().attackRange );
	}
	else if ( flag & SF_SELECT_FRIEND )
	{
		entity = this;
	}
	else if ( flag & SF_SELECT_TOBJECT )
	{
		class FindTObjCallBack : public EntityResultCallBack
		{
		public:
			FindTObjCallBack()
			{  
				findObj = NULL;
				minDist2 = 1e18; 
			}
			bool processResult( TEntityBase* entity )
			{
				if ( TObject* obj = TObject::upCast( entity ) )
				{
					float dist2 = obj->getPosition().distance2( pos );
					if ( dist2 < minDist2 )
					{
						minDist2 = dist2;
						findObj = obj;
					}
				}
				return true;
			}
			Vec3D    pos;
			TObject* findObj;
			float    minDist2;
		};

		FindTObjCallBack callback;
		callback.pos = getPosition();
		float dist = m_curSkill->getInfo().attackRange;
		TEntityManager& manager = TEntityManager::instance();
		manager.findEntityInSphere( callback.pos , dist , &callback );

		entity = callback.findObj;
	}

	if ( flag & SF_SELECT_ENTITY )
	{
		m_curSkill->endBuff();
		m_curSkill->OnStart( entity );

		setActivity( ACT_IDLE );
		setAnimation( ANIM_WAIT );

		int index = addThink( (fnThink)&ThisClass::processSustainedSkill , THINK_ONCE );
		setNextThink( index , g_GlobalVal.nextTime );
	}
	else
	{
		cancelSkill();
	}

}

void TActor::processSustainedSkill()
{
	if ( m_curSkill->sustainCast() )
	{
		m_skillbufTime += g_GlobalVal.frameTime;
		setCurContextNextThink( g_GlobalVal.nextTime );
	}
	else
	{
		OnSkillEnd();
	}
}

void TActor::castSkill( char const* name )
{
	if ( !canUseSkill( name ) )
		return;

	if ( m_curSkill )
		cancelSkill();

	TSkill* curSkill = TSkillLibrary::instance().createSkill( name );
	if ( curSkill != NULL )
	{
		if ( getMP() < curSkill->getInfo().castMP )
			return;

		curSkill->setCaster( this );
		m_cdManager->pushSkill( curSkill->getName() , curSkill->getInfo().cdTime );
		setActivity( ACT_BUF_SKILL );
		m_curSkill = curSkill;
		m_skillbufTime = 0;
		m_curSkill->startBuff();
		int index = addThink( (fnThink) &TActor::processCastSkill );
		setNextThink( index ,  g_GlobalVal.curtime );
	}
	else
	{
		DevMsg( 1 ,"cant find skill: %s" , name );
	}
}


void TActor::cancelSkill()
{
	addThink( (fnThink)&TActor::processCastSkill , THINK_NOTHINK );
	m_curSkill->OnCancel();
	sendEvent( EVT_SKILL_CANCEL , m_curSkill , true );

	m_curSkill = NULL;

	setActivity( ACT_IDLE );
	setAnimation( ANIM_WAIT );
	
}

void TActor::OnSkillEnd()
{
	sendEvent( EVT_SKILL_END , m_curSkill , true );
	m_curSkill = NULL;
}

bool TActor::canDoAction()
{
	return m_actType == ACT_IDLE ||
		   m_actType == ACT_MOVING ;
}

void TActor::addPropModify( PropModifyInfo& info )
{
	addStateBit( info.state );
	getAbilityProp().addPropModify( info );
}

bool TActor::isEmpty( TActor& actor )
{
	return m_team != actor.m_team && 
		   GetTeamRelation( m_team , actor.m_team ) ==  TEAM_EMPTY;
}

int TActor::getHP() const
{
	return m_abilityProp->getPropVal( PROP_HP );
}

int TActor::getMP() const
{
	return m_abilityProp->getPropVal( PROP_MP );
}

float TActor::getPropValue( PropType prop ) const
{
	return m_abilityProp->getPropVal( prop );
}

void TActor::setHP( int val )
{
	m_abilityProp->setHP( val );
}

void TActor::setMP( int val )
{
	m_abilityProp->setMP( val );
}

float TActor::getAttackRange()
{
	return m_abilityProp->getPropVal( PROP_AT_RANGE );
}

float TActor::getVisibleDistance()
{
	return m_abilityProp->getPropVal( PROP_VIEW_DIST );
}

float TActor::getMaxRotateAngle()
{
	return DEG2RAD( 12 );

}


bool TActor::canUseSkill( char const* name )
{
	return m_cdManager->canPlaySkill( name );
}

MoveType TActor::getMovementType()
{
	return getMovement().getMoveResult().type;
}

void TActor::jump( Vec3D const& vecticy )
{
	if ( getMovement().jump( vecticy ) )
	{
		setActivity( ACT_JUMP );
	}
}

void TActor::defense( bool beD )
{
	if ( beD )
	{
		setActivity( ACT_DEFENSE );
		setAnimation( ANIM_DEFENSE );
	}
	else if ( getActivityType() == ACT_DEFENSE )
	{
		setActivity( ACT_IDLE );
		setAnimation( ANIM_WAIT );
	}
}

void TActor::OnCollision( TPhyEntity* colObj , Float depth , Vec3D const& selfPos , Vec3D const& colObjPos , Vec3D const& normalOnColObj )
{
	if ( TObject* obj = TObject::upCast( colObj ) )
	{
		obj->getPhyBody()->applyCentralForce( - 1000 * normalOnColObj  );
		obj->getPhyBody()->activate();
	}
}

void TActor::OnSpawn()
{
	getFlyActor().ChangeScene( getLiveLevel()->getFlyScene().Object() );
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    

void TActor::OnDying()
{
	setAnimation( ANIM_DYING , true );
	addThink( THINK_FUN( processAttack ), THINK_NOTHINK );
}

void TActor::addMoney( unsigned much )
{
	m_money+= much;
}

bool TActor::spendMoney( unsigned much )
{
	if ( m_money < much )
		return false;

	m_money -= much;
	return true;
}


void TActor::inputWeaponDamage( TPhyEntity* dtEntity )
{
	DamageInfo* info = DamageInfo::create();

	info->typeBit  = DT_USE_WEAPON | DT_PHYSICAL;
	info->attacker = this;
	info->defender = dtEntity;
	info->val = getPropValue( PROP_MAT );

	TFxData* fxData = TFxPlayer::instance().play( "hit" );

	FnObject& obj = fxData->getBaseObj();
	obj.SetPosition( dtEntity->getPosition() - 10 *getFaceDir() );

	UG_InputDamage( *info );
}

void TActor::evalAttackMove()
{
	setAnimation( ANIM_ATTACK );
	int index = addThink( (fnThink)&TActor::processAttack );
	setNextThink( index , g_GlobalVal.curtime + 
		getAnimation().getAnimFrameLength(ANIM_ATTACK) * g_GlobalVal.frameTime / 2 );
}
