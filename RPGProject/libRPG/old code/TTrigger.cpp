#include "TTrigger.h"

#include "TMovement.h"
#include "shareDef.h"
#include "TActor.h"
#include "THero.h"
#include "TItemSystem.h"
#include "TResManager.h"
#include "TPlayerControl.h"
#include "UtilityGlobal.h"
#include "UtilityFlyFun.h"

#include "TGame.h"
#include "TLevel.h"

DEFINE_UPCAST( TTrigger , ET_TRIGGER );

TTrigger::TTrigger( TPhyBodyInfo& info ) 
	: TPhyEntity( PHY_SENSOR_BODY, info )
{
	m_entityType |= ET_TRIGGER;

	int flag = getPhyBody()->getCollisionFlags();
	getPhyBody()->setCollisionFlags( flag | btCollisionObject::CF_NO_CONTACT_RESPONSE );

	m_dbgObj.Object( g_Game->getCurLevel()->getFlyScene().CreateObject() );

	TObjMotionState* motion = new TObjMotionState( getTransform() , m_dbgObj.Object() );
	setMotionState( motion );
}

void TTrigger::updateFrame()
{
	for ( PEDataList::iterator iter = m_handleVec.begin();
		iter != m_handleVec.end() ; )
	{
		PEData& data = *iter;
		if ( !data.isInside ) 
		{
			TPhyEntity* entity = data.handle;
			if ( entity ) 
			{
				OnExit( entity );
			}

			iter = m_handleVec.erase( iter );
		}
		else
		{
			TPhyEntity* entity = data.handle;
			if ( entity != NULL )
			{
				InTrigger( entity );
				data.isInside = false;
				++iter;
				continue;
			}
			else
			{
				iter = m_handleVec.erase( iter );
			}
		}
	}
}



void TTrigger::OnCollision( TPhyEntity* colObj , Float depth , Vec3D const& selfPos , Vec3D const& colObjPos , Vec3D const& normalOnColObj )
{
	iterator iter = findEntity( colObj );
	if ( iter != m_handleVec.end() )
	{
		iter->isInside = true;
	}
	else
	{
		PEData data;
		data.isInside = true;
		data.handle  = colObj;

		m_handleVec.push_back( data );

		OnTough( colObj );
	}
}

TTrigger::iterator TTrigger::findEntity( TPhyEntity* entity )
{
	for ( PEDataList::iterator iter = m_handleVec.begin();
		iter != m_handleVec.end() ; ++iter)
	{
		if ( iter->handle == entity )
			return iter;
	}
	return m_handleVec.end();
}

void TLadderTrigger::OnTough( TPhyEntity* entity )
{
	if ( TActor* actor = TActor::upCast(entity) )
	{
		if ( actor->getMovementType() == MT_NORMAL_MOVE )
		{
			actor->setActivity( ACT_LADDER );
			Vec3D dir = getTransform().getBasis() * Vec3D(-1,0,0);
			actor->setFaceDir( dir );
		}
	}
}

void TLadderTrigger::OnExit( TPhyEntity* entity )
{
	if ( TActor* actor = TActor::upCast(entity) )
	{

	}
}



TChestTrigger::TChestTrigger() :TBoxTrigger( Vec3D( 100 , 100 , 100 ) )
	,m_items( new TItemStorage(MaxItemNum) )
{
	m_DTime = 0.0f;

	OBJECTid objID = TResManager::instance().cloneModel( "bag" , true );

	modelObj.Object( objID );
	float scale = 20;
	modelObj.Scale( scale , scale , scale , LOCAL  );
	modelObj.Rotate( X_AXIS , 90 , LOCAL );
	modelObj.XForm();

	XForm trans;
	trans.setIdentity();
	TObjMotionState* motionState = new TObjMotionState( trans , objID );
	setMotionState( motionState );
}

TChestTrigger::~TChestTrigger()
{
	UF_DestoryObject( modelObj );
}
void TChestTrigger::addItem( unsigned itemID , int num )
{
	if ( !UG_GetItem( itemID ) )
		return;
	m_items->addItem( itemID , num );
}

void TChestTrigger::updateFrame()
{
	BaseClass::updateFrame();
	if ( m_DTime == 0 || m_DTime - g_GlobalVal.curtime > VanishTime + 1.0 )
	{
		if ( m_items ->haveEmptySolt( MaxItemNum ) )
		{	
			setVanishTime( 1.0 );
		}
	}

}

void TChestTrigger::OnTough( TPhyEntity* entity )
{

	if ( TActor* actor = TActor::upCast(entity) )
	{
		if ( THero::upCast( actor ) )
		{
			g_PlayerControl->changeChest( this );
		}
		
		actor->addStateBit( AS_TOUCH_BAG );
	}
}

void TChestTrigger::OnExit( TPhyEntity* entity )
{
	if ( TActor* actor = TActor::upCast(entity) )
	{
		if ( THero::upCast( actor ) )
		{
			g_PlayerControl->removeChest( this );
		}

		actor->removeStateBit( AS_TOUCH_BAG );
	}
}

BEGIN_SIGNAL_MAP_NOBASE( PlayerBoxTrigger )
	DEF_SIGNAL( playerTough )
END_SIGNAL_MAP( PlayerBoxTrigger )

void TChestTrigger::vanishThink()
{
	if ( m_DTime < g_GlobalVal.curtime )
	{
		setEntityState( EF_DESTORY );
	}
	else
	{
		modelObj.SetOpacity( modelObj.GetOpacity() - 0.01 );
		setCurContextNextThink( g_GlobalVal.nextTime );
	}
}

void TChestTrigger::setVanishTime( float time )
{
	m_DTime = g_GlobalVal.curtime + time + VanishTime;

	int index = addThink( ( fnThink)&TChestTrigger::vanishThink );
	setNextThink( index , g_GlobalVal.curtime + time );
}

float const TChestTrigger::VanishTime = 5.0f;


void PlayerBoxTrigger::OnTough( TPhyEntity* entity )
{
	if ( THero::upCast( entity ) )
	{
		SEND_SIGNAL_VOID( playerTough )
	}
}

PlayerBoxTrigger::PlayerBoxTrigger( Vec3D const& len ) 
	:TBoxTrigger( len )
{
	INIT_SIGNAL( playerTough );
}


void TBoxTrigger::setBoxSize( Vec3D const& size )
{
	PhyShape* shape = getPhyBody()->getCollisionShape();
	getPhyBody()->setCollisionShape( new btBoxShape( 0.5 * size ) );
	delete shape;
}