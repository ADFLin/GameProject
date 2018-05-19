#include "ProjectPCH.h"
#include "CActor.h"

#include "ActorMovement.h"
#include "AbilityProp.h"
#include "SpatialComponent.h"

#include "PhysicsSystem.h"
#include "TCombatSystem.h"


#include "TRoleTemplate.h"
#include "TSkillBook.h"
#include "UtilityGlobal.h"
#include "UtilityMath.h"

#include "TResManager.h"
#include "CSceneLevel.h"

#include "TSkillBook.h"
#include "IScriptTable.h"

float gDefultAttackRange = 10;
float gDefultVisibleDistance = 200;

DEFINE_DOWNCAST( CActor , LOT_ACTOR )
DEFINE_HANDLE( CActor )

CActor::CActor()
{
	mObjectType |= LOT_ACTOR;
	m_roleID = 0;

	mModel   = nullptr;
	mSpatialComp = nullptr;

	mActState = ACT_IDLE; 

	setAlignDirectionAxis( CFly::CF_AXIS_Y_INV , CFly::CF_AXIS_Z );
}


CActor::~CActor()
{

}


bool CActor::init( GameObject* gameObject , GameObjectInitHelper const& helper )
{
	if ( !BaseClass::init( gameObject , helper ) )
		return false;

	m_money = 0;
	m_stateBit = 0;

	IScriptTable* scriptTable = gameObject->getEntity()->getScriptTable();
	if ( !scriptTable->getValue( "roleID" , m_roleID ) )
		return false;

	SRoleInfo* roleData = TRoleManager::Get().getRoleInfo( m_roleID );
	if ( !roleData )
		return false;

	CRenderComponent* renderComponent = getEntity()->getComponentT< CRenderComponent >( COMP_RENDER );
	if ( !renderComponent )
		return false;

	SRenderEntityCreateParams params;
	params.name    = roleData->modelName.c_str();
	params.resType = RES_ACTOR_MODEL;
	params.scene   = helper.sceneLevel->getRenderScene();
	mModel         = static_cast< CActorModel* >( renderComponent->createRenderEntity( RS_MODEL_SLOT , params ) );

	ActorModelRes* modelRes = TResManager::Get().getActorModel( roleData->modelName.c_str() );
	PhyCollision* phyicalComp = getEntity()->getComponentT< PhyCollision >( COMP_PHYSICAL ) ;

	SNSpatialComponent* spatialComp = new SNSpatialComponent( mModel->getCFActor() , phyicalComp );
	spatialComp->setPhyOffset( modelRes->modelOffset );
	getEntity()->addComponent( spatialComp );

	mAbilityProp.reset( new AbilityProp( roleData->propData ) );

	mItemStroage.reset( new TItemStorage( 30 ) );
	mEquipmentTable.reset( new TEquipTable );

	m_skillBook.reset( new TSkillBook( getRefID() ) );
	m_cdManager.reset( new TColdDownManager( getRefID() ) );

	mMovement.reset( new ActorMovement( phyicalComp , 25 , 2 ) );

	m_curSkill = NULL;

	mSpatialComp = getEntity()->getComponentT< SNSpatialComponent >( COMP_SPATIAL );
	assert( mSpatialComp );
	
	PhyCollision* physicalComp = getEntity()->getComponentT< PhyCollision >( COMP_PHYSICAL );
	assert( physicalComp );
	physicalComp->setCollisionCallback( this , &CActor::onCollision );

	//FIXME
	//m_movement->setJumpHeight( getPropValue(PROP_JUMP_HEIGHT) );

	mModel->changeAction( ANIM_WAIT );
	return true;
}


void CActor::release()
{
	PhyCollision* physicalComp = getEntity()->getComponentT< PhyCollision >( COMP_PHYSICAL );
	if ( physicalComp )
		physicalComp->clearCallback();

	BaseClass::release();
}

//void CActor::onCollision( TPhyEntity* colObj , float depth , Vec3D const& selfPos , Vec3D const& colObjPos , Vec3D const& normalOnColObj )
//{
//	if ( TObject* obj = TObject::downCast( colObj ) )
//	{
//		obj->getPhyBody()->applyCentralForce( - 1000 * normalOnColObj  );
//		obj->getPhyBody()->activate();
//	}
//}

void CActor::update( long time )
{
	BaseClass::update( time );

	//§ó·sÄÝ©Êª¬ºA­È
	class ModifyCallBack : public PropModifyCallBack
	{
	public:
		ModifyCallBack( CActor* _actor ) : actor(_actor){}
		void process( PropModifyInfo& info , bool beD )
		{
			if ( beD )
				actor->removeStateBit( info.state );
		}
		CActor* actor;
	} callback( this );

	mAbilityProp->updateFrame( callback );

	mMovement->update( time );

	processAction();
	//m_movement->updateAction(  g_GlobalVal.frameTime );
	//updateAnimation();

	mModel->updateAnim( time );
	m_cdManager->updateFrame();
}

//TActorModelData& ActorLogicComponent::getModelData( unsigned modelID )
//{
//	TObjModelData* data = TResManager::instance().getModelData(modelID);
//	assert( data != NULL );
//	return *( (TActorModelData*)data );
//}

void CActor::takeDamage( DamageInfo const& info )
{
	if ( isDead() )
	{

	}
	else
	{
		PropModifyInfo PMInfo;
		PMInfo.prop = PROP_HP;
		PMInfo.val  =  info.damageResult;
		PMInfo.flag = PF_VOP_SUB;

		mAbilityProp->addPropModify( PMInfo );

		switch ( mActState )
		{
		case ACT_BUF_SKILL:
			m_skillbufTime = TMax( m_skillbufTime - 1 , 0.0f );
			break;
		}

		if ( getHP() <= 0 )
		{
			onDying();
			if ( CActor* actor = CActor::downCast(info.attacker) )
				actor->onKillActor( this );
		}
		else
		{
			mModel->changeAction( ANIM_HURT );
			addAnimationEndThink( ANIM_HURT );
		}
	}
}


CActor* CActor::findEmpty( float dist )
{

	CSceneLevel::Cookie cookie;
	getSceneLevel()->startFindObject( cookie );


	Vec3D pos = getSpatialComponent()->getPosition();
	float dist2 = dist * dist;
	while( ILevelObject* object = getSceneLevel()->findObject( cookie , pos , dist2 ) )
	{
		CActor* actor = CActor::downCast( object );
		if ( !actor || actor == this )
			continue;
		if ( actor->isDead() || !isEmpty( actor ) )
			continue;
		return actor;
	}

	return NULL;
}


void CActor::attackRangeTest( Vec3D const& pos , Vec3D const& dir , float r , float angle )
{

	CSceneLevel::Cookie cookie;
	getSceneLevel()->startFindObject( cookie );

	float r2 = r * r;
	ILevelObject* pe = nullptr;

	while ( pe = getSceneLevel()->findObject( cookie , pos ,r2 )  )
	{
		if ( pe == this ) 
			continue;

		CActor* actor = CActor::downCast( pe );
		if ( actor )
		{
			if ( !isEmpty( actor ) )
				continue;

			Vec3D objPos = pe->getSpatialComponent()->getPosition();
			Vec3D objDir = objPos - pos;

			float dotAngle = UM_Angle( dir , objDir );

			if ( dotAngle < angle / 2  )
			{
				inputWeaponDamage( pe );
			}
		}
		//else
		//{
		//	TObject* obj = TObject::downCast( pe );
		//	if ( !obj )
		//		continue;

		//	Vec3D objPos = pe->getPosition();
		//	Vec3D objDir = objPos - pos;

		//	float dotAngle = UM_Angle( dir , objDir );

		//	if ( dotAngle < angle / 2  )
		//	{
		//		objDir .normalize();
		//		obj->getPhyBody()->applyCentralImpulse( 100 * dir );
		//		obj->getPhyBody()->activate();
		//	}
		//}
	}
}

float CActor::getFaceOffest()
{
	PhyShapeParams info;
	PhysicsSystem::getShapeParamInfo( 
		mMovement->getCollisionComp()->getShape() , info );

	switch( info.type )
	{
	case CSHAPE_BOX: return 0.8f * info.box.y; 
	}
	return 0;
}

bool CActor::attack()
{
	if ( isDead() )
		return false;

	if ( getActivityType() == ACT_BUF_SKILL || 
		 getActivityType() == ACT_ATTACK )
		return false;

	evalAttackMove();

	return true;
}

float CActor::getMaxMoveSpeed()
{
	return mAbilityProp->getPropValue( PROP_MV_SPEED );
}

bool CActor::onAttackTest()
{
	if ( mActState != ACT_ATTACK )
		return false;

	Vec3D pos = mSpatialComp->getPosition() + getFaceOffest() * getFaceDir() ;
	attackRangeTest( pos , getFaceDir() , mAbilityProp->getPropValue( PROP_AT_RANGE ) , Math::Deg2Rad(120) );

	return true;
}

void CActor::onAnimationFinish( AnimationType finishPose )
{
	switch ( finishPose )
	{
	case ANIM_HURT:
		mModel->changeAction( ANIM_WAIT );
		break;
	case ANIM_ATTACK :
		setActivity( ACT_IDLE );
		mModel->changeAction( ANIM_WAIT );
		break;
	case ANIM_DYING :
		if ( ( getStateBit() & AS_DIE ) == 0 )
		{
			LogDevMsg( 5 , "Dying" );
			addStateBit( AS_DIE );
			onDead();
		}
		else LogDevMsg( 5 , "Double Dying" );
		break;
	}
}


void CActor::setActivity( ActivityType type )
{
	switch ( mActState )
	{
	case ACT_MOVING:
		mModel->changeAction( ANIM_WAIT );
		break;
	case ACT_BUF_SKILL:
		break;
	case ACT_IDLE:
		break;
	case ACT_LADDER:
		mMovement->setMoveType( AMT_NORMAL );
		break;
	}

	switch ( type )
	{
	case  ACT_LADDER:
		mMovement->setMoveType( AMT_LADDER );
		break;
	}

	mActState = type;
}



void CActor::turnRight( float angle )
{
	mSpatialComp->rotate( Quat::Rotate( g_UpAxisDir , angle ) );
}

void CActor::moveFront( float offset )
{
	mMovement->setVelocityForTimeInterval(  offset * getFaceDir() , g_GlobalVal.frameTime );
	mActState = ACT_MOVING;
	mModel->changeAction( ANIM_RUN );
}

void CActor::moveSpace( Vec3D const& offset  )
{
	mMovement->setVelocityForTimeInterval( offset ,g_GlobalVal.frameTime );
}


//float ActorLogicComponent::getBodyHalfHeigh()
//{
//	btBoxShape* shape = (btBoxShape*) getCollisionShape();
//	return shape->getHalfExtentsWithoutMargin().z();
//}

bool CActor::addItem( char const* itemName , int num )
{
	unsigned id = UG_GetItemID( itemName );
	if ( id == ITEM_ERROR_ID )
		return false;

	return addItem( id , num );
}

bool CActor::addItem( unsigned itemID , int num )
{
	assert( UG_GetItem( itemID ) );
	return mItemStroage->addItem( itemID , num );
}

Vec3D CActor::getFaceDir() const
{
	Vec3D frontDir;
	getFrontUpDirection( &frontDir , NULL );
	return frontDir;
}

void CActor::setFaceDir( Vec3D const& front )
{
	setFrontUpDirection( front , g_UpAxisDir );
}

bool CActor::useItem( unsigned itemID )
{
	TItemBase* item = UG_GetItem(itemID);

	if ( !item )
	{
		LogDevMsg( 1 , "Use unknown ItemID = %d" , itemID );
		return false;
	}


	int itemNum = mItemStroage->queryItemNum( itemID );

	int slotPos = getItemStorage().findItemSoltPos( itemID );

	if ( slotPos < 0 )
		return false;

	return playItemBySlotPos( slotPos );
}

bool CActor::useItem( char const* itemName )
{
	unsigned itemID = UG_GetItemID( itemName );
	if ( itemID == ITEM_ERROR_ID  )
	{
		LogDevMsg( 1 , "use unknown Item = %s" , itemName );
		return false;
	}
	return useItem( itemID );
}

void CActor::doUseItem( unsigned itemID )
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
		onDying();
	}

	item->use( this );
}

bool CActor::equip( char const* itemName )
{
	unsigned itemID = UG_GetItemID( itemName );
	if ( itemID == ITEM_ERROR_ID  )
	{
		LogDevMsg( 1 , "equip unknown Item = %s" , itemName );
		return false;
	}

	return equip( itemID );
}

bool CActor::equip( unsigned itemID )
{
	TItemBase* equipItem = UG_GetItem( itemID );

	if ( !equipItem )
	{
		LogDevMsg( 1 , "equip unknown ItemID = %d" , itemID );
		return false;
	}

	if ( !equipItem->isEquipment() )
		return false;

	int slotPos  = getItemStorage().findItemSoltPos(itemID);

	if ( slotPos < 0 )
		return false;

	return playItemBySlotPos( slotPos );
}



bool CActor::doEquip( unsigned itemID )
{
	TItemBase* equipItem = UG_GetItem( itemID );

	if ( !equipItem->isEquipment() )
		return false;

	EquipSlot removeSlot[10];
	int numRemove = mEquipmentTable->queryRemoveEquip( itemID , removeSlot );

	if ( !mItemStroage->haveEmptySolt(numRemove) )
		return false;

	unsigned removeID[10];
	for( int i = 0 ; i < numRemove ; ++ i )
	{
		unsigned rmItemID = getEquipTable().removeEquip( removeSlot[i]);

		mItemStroage->addItem( rmItemID );

		removeID[i] = rmItemID;
	}

	EquipSlot slot = mEquipmentTable->equip( itemID );

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

	computeAbilityProp();
	onEquipItem( slot , false );

	return true;
}


void CActor::computeAbilityProp()
{
	int HP = getHP();
	int MP = getMP();

	mAbilityProp->resetAllPropVal();

	for ( int i = 0 ; i < EQS_EQUIP_TABLE_SLOT_NUM ; ++i )
	{
		TEquipment* equi = mEquipmentTable->getEquipment( EquipSlot(i) );

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

		mAbilityProp->addPropModify( info );

		TItemBase::PMInfoVec& PMVec = equi->getPropModifyVec();
		for( int n = 0; n < PMVec.size() ; ++n )
		{
			mAbilityProp->addPropModify( PMVec[n] );
		}
	}
	for( int i = 0 ; i < PROP_TYPE_NUM ; ++ i )
	{
		mAbilityProp->updatePropValue( PropType(i) );
	}

	mAbilityProp->setHP( HP );
	mAbilityProp->setMP( MP );
}

void CActor::removeEquipment( EquipSlot slot )
{
	unsigned itemID = getEquipTable().removeEquip( slot );
	if ( itemID != ITEM_ERROR_ID )
	{
		getItemStorage().addItem( itemID , 1 );
		onEquipItem( slot , true );
		computeAbilityProp();
	}
}
bool CActor::playItemBySlotPos( int slotPos )
{
	TItemBase* item = mItemStroage->getItem( slotPos );
	if ( !item || mItemStroage->getItemNum( slotPos ) <= 0 )
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

		int num = mItemStroage->removeItemInSlotPos( slotPos );
		assert( num == 1 );

		m_cdManager->pushItem( item->getName() , item->getCDTime() );
		onUseItem( item->getID() );

		return true;
	}

	return false;
}

void CActor::castSkillThink( ThinkContent& content )
{
	if ( !m_curSkill )
		return;

	if ( m_skillbufTime < m_curSkill->getInfo().bufTime )
	{
		m_skillbufTime += g_GlobalVal.frameTime;
		content.nextTime = g_GlobalVal.nextTime;
		return;
	}

	bool successCast = false;
	unsigned flag = m_curSkill->getInfo().flag;

	ILevelObject* entity = NULL;
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
		CSceneLevel::Cookie cookie;
		getSceneLevel()->startFindObject( cookie );

		float dist2 = m_curSkill->getInfo().attackRange;
		dist2 *= dist2;

		ILevelObject* comp;

		float minDist2 = 1e18; 
		//while ( comp = getSceneLevel()->findEntity( cookie , mSpatialComp->getPosition() , dist2 ))
		//{
		//	if ( TObject* obj = TObject::downCast( entity ) )
		//	{
		//		float dist2 = obj->getPosition().distance2( pos );
		//		if ( dist2 < minDist2 )
		//		{
		//			minDist2 = dist2;
		//			entity = obj;
		//		}
		//	}
		//}
	}

	if ( flag & SF_SELECT_ENTITY )
	{
		m_curSkill->endBuff();
		m_curSkill->onStart( entity );

		setActivity( ACT_IDLE );
		mModel->changeAction( ANIM_WAIT );

		ThinkContent* tc = setupThink( this , &ThisClass::castSustainedSkillThink , THINK_ONCE );
		tc->nextTime = g_GlobalVal.nextTime;
	}
	else
	{
		cancelSkill();
	}

}

void CActor::castSustainedSkillThink( ThinkContent& content )
{
	if ( m_curSkill->sustainCast() )
	{
		m_skillbufTime += g_GlobalVal.frameTime;
		content.nextTime = g_GlobalVal.nextTime;
	}
	else
	{
		onSkillEnd();
	}
}

void CActor::castSkill( char const* name )
{
	if ( !canUseSkill( name ) )
		return;

	if ( m_curSkill )
		cancelSkill();

	TSkill* curSkill = TSkillLibrary::Get().createSkill( name );
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

		ThinkContent* tc = setupThink( this , &ThisClass::castSkillThink );
		tc->nextTime = g_GlobalVal.curtime;
	}
	else
	{
		LogDevMsg( 1 ,"cant find skill: %s" , name );
	}
}


void CActor::cancelSkill()
{
	setupThink( this , &ThisClass::castSkillThink , THINK_REMOVE );
	m_curSkill->onCancel();
	//sendEvent( EVT_SKILL_CANCEL , m_curSkill , true );

	m_curSkill = NULL;

	setActivity( ACT_IDLE );
	mModel->changeAction( ANIM_WAIT );

}

void CActor::onSkillEnd()
{
	//sendEvent( EVT_SKILL_END , m_curSkill , true );
	m_curSkill = NULL;
}

bool CActor::canDoAction()
{
	return mActState == ACT_IDLE ||  mActState == ACT_MOVING ;
}

void CActor::addPropModify( PropModifyInfo& info )
{
	addStateBit( info.state );
	mAbilityProp->addPropModify( info );
}

bool CActor::isEmpty( CActor* actor )
{
	return m_team != actor->m_team && 
		   GetTeamRelation( m_team , actor->m_team ) == TEAM_EMPTY;
}

int CActor::getHP() const
{
	return mAbilityProp->getPropValue( PROP_HP );
}

int CActor::getMP() const
{
	return mAbilityProp->getPropValue( PROP_MP );
}

float CActor::getPropValue( PropType prop ) const
{
	return mAbilityProp->getPropValue( prop );
}

void CActor::setHP( int val )
{
	mAbilityProp->setHP( val );
}

void CActor::setMP( int val )
{
	mAbilityProp->setMP( val );
}


float CActor::getAttackRange()
{
	if ( mAbilityProp )
		mAbilityProp->getPropValue( PROP_AT_RANGE );
	return gDefultAttackRange;
}

float CActor::getVisibleDistance()
{
	if ( mAbilityProp )
		mAbilityProp->getPropValue( PROP_VIEW_DIST );

	return gDefultVisibleDistance;
}

float CActor::getMaxRotateAngle()
{
	return Math::Deg2Rad( 12 );

}


bool CActor::canUseSkill( char const* name )
{
	return m_cdManager->canPlaySkill( name );
}


void CActor::jump( Vec3D const& vecticy )
{
	if ( mMovement->canJump() )
	{
		mMovement->setJumpSpeed( vecticy.z );
		mMovement->jump();
		setActivity( ACT_JUMP );
	}
}

bool CActor::isDead()
{
	return getHP() <= 0;
}

void CActor::defense( bool beD )
{
	if ( beD )
	{
		setActivity( ACT_DEFENSE );
		mModel->changeAction( ANIM_DEFENSE );
	}
	else if ( getActivityType() == ACT_DEFENSE )
	{
		setActivity( ACT_IDLE );
		mModel->changeAction( ANIM_WAIT );
	}
}

Vec3D CActor::getEyePos()
{
	return mSpatialComp->getPosition();
}

Vec3D CActor::getHandPos( bool isRight )
{
	return mSpatialComp->getPosition();
}

void CActor::attackThink( ThinkContent& content )
{
	onAttackTest();
}

void CActor::evalAttackMove()
{
	mModel->changeAction( ANIM_ATTACK , false );
	addAnimationEndThink( ANIM_ATTACK );
	setActivity( ACT_ATTACK );

	ThinkContent* tc = setupThink( this , &ThisClass::attackThink );
	tc->nextTime = g_GlobalVal.curtime + 
		mModel->getAnimTimeLength( ANIM_ATTACK ) / 2;
}

void CActor::animationEndThink( ThinkContent& content )
{
	onAnimationFinish( AnimationType( content.uintData)  );
}

void CActor::addAnimationEndThink( AnimationType anim )
{
	ThinkContent* tc = setupThink( this , &ThisClass::animationEndThink );
	tc->nextTime = g_GlobalVal.curtime + mModel->getAnimTimeLength( anim );
	tc->uintData = anim;
}

void CActor::inputWeaponDamage( ILevelObject* dtEntity )
{
	DamageInfo info;
	info.typeBit  = DT_USE_WEAPON | DT_PHYSICAL;
	info.attacker = this;
	info.defender = dtEntity;
	info.val      = getPropValue( PROP_MAT );

	//TFxData* fxData = TFxPlayer::instance().play( "hit" );
	//FnObject& obj = fxData->getBaseObj();
	//obj.SetPosition( dtEntity->getPosition() - 10 *getFaceDir() );

	UG_InputDamage( info );
}

#include "IScriptTable.h"

void CActorScript::createComponent( CEntity& entity , EntitySpawnParams& params )
{
	GameObjectInitHelper const& helper = *params.helper;

	unsigned roleID = 0;
	IScriptTable* scriptTable = entity.getScriptTable();
	scriptTable->getValue( "roleID" , roleID );
	SRoleInfo* roleData = TRoleManager::Get().getRoleInfo( roleID );

	ActorModelRes* modelRes = TResManager::Get().getActorModel( roleData->modelName.c_str() );

	CRenderComponent* renderComp = new CRenderComponent;
	entity.addComponent( renderComp );

	PhyCollision* phyicalComp = helper.sceneLevel->getPhysicsWorld()->createCollisionComponent( 
		modelRes->phyData.shape , PHY_ACTOR_OBJ , COF_ACTOR , COF_ALL );

	entity.addComponent( phyicalComp );
}
