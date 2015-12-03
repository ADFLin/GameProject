#include "TEditData.h"

#include <algorithm>
#include <sstream>

#include "TLevel.h"
#include "TObjectEditor.h"
#include "TActor.h"
#include "TNPCBase.h"
#include "TVillager.h"
#include "TObject.h"
#include "TResManager.h"
#include "TEntityManager.h"
#include "TTrigger.h"
#include "TLogicObj.h"
#include "TSkillBook.h"


void EmObjData::setPosition( Vec3D const& pos )
{
	obj.SetWorldPosition( (float*) & pos );
	this->pos = pos;
}

Vec3D EmObjData::getPosition()
{
	obj.GetWorldPosition( pos );
	return pos;
}

void EmObjData::setFrontDir( Vec3D const& dir )
{
	obj.SetWorldDirection( (float*)&dir ,  NULL );
	obj.GetWorldDirection( front , up );
}

void EmObjData::setUpDir( Vec3D const& dir )
{
	obj.SetWorldDirection( NULL , (float*)&dir );
	obj.GetWorldDirection( front , up );
}

Vec3D EmObjData::getFrontDir()
{
	obj.GetWorldDirection( front , NULL );
	return front;
}

Vec3D EmObjData::getUpDir()
{
	obj.GetWorldDirection( NULL , up );
	return up;
}

void EmObjData::show( bool beS )
{
	EmDataBase::show( beS );
	obj.Show( beS );
}

void EmGroupManager::setGroup( EmDataBase* data , unsigned group /*= 0*/ )
{
	if ( group >= groupVec.size() )
		return;

	if ( groupVec[group] == NULL )
		return;

	EmGroup* groupData = groupVec[ data->group ];
	assert( groupData );

	EmGroup::iterator iter = std::find( groupData->children.begin() ,
		groupData->children.end() , data );

	if ( iter != groupData->children.end() )
	{
		groupData->children.erase( iter );
	}
	
	data->group = group;
	groupVec[ group ]->children.push_back( data );
}

void EmDataBase::restore()
{
	if ( g_isEditMode )
	{
		setName( m_name );
		show( m_show );
	}
}

void EmSceneObj::restore()
{
	TLevel* level = getWorldEditor()->getCurLevel();
	FnScene& scene = level ->getFlyScene();
	OBJECTid id = scene.GetObjectByName( (char*)m_name.c_str() );
	assert ( id != FAILED_ID );
	
	obj.Object( id );
	obj.ChangeRenderGroup( renderGroup );

	EmObjData::restore();
}

EmSceneObj::EmSceneObj( OBJECTid id ) 
	:EmObjData(id)
{
	renderGroup = RG_TERRAIN;
	colShape    = NULL;
}

void EmSceneObj::setRenderGroup( RenderGroup rGroup )
{
	renderGroup = rGroup;
	obj.ChangeRenderGroup( rGroup );
}

void EmSceneObj::accept( EmVisitor& visitor )
{
	visitor.visit( this );
}
void EmObjData::OnSelect()
{
	show( true );
	getWorldEditor()->setControlObj( obj.Object() );
}


void EmObjData::restore()
{
	if ( obj.Object() != FAILED_ID && ( flag & EDF_SAVE_XFORM ) )
	{
		obj.SetWorldPosition( pos );
		obj.SetWorldDirection( front , up );
	}
	EmDataBase::restore();
}

void EmObjData::OnCreate()
{
	getWorldEditor()->addChoiceObjMap( this , obj.Object() );
}

void EmObjData::OnDestory()
{
	getWorldEditor()->removeChoiceObjMap( obj.Object() );
	EmDataBase::OnDestory();
}

EmObjData::EmObjData( OBJECTid id /*= FAILED_ID*/ ) 
	:pos( 0,0,0 )
	,front( 1,0,0)
	,up(0,0,1)
{
	flag = 0;
	obj.Object( id );
	if ( id != FAILED_ID )
		m_name = obj.GetName();
}


void EmGroup::restore()
{
	EmDataBase::restore();
}

void EmGroup::show( bool beS )
{
	EmDataBase::show( beS );
	for( iterator iter = children.begin(); 
		iter != children.end() ; ++iter)
	{
		EmDataBase* eData = *iter;
		eData->show( beS );		
	}
}

void EmGroup::OnDestory()
{
	for( EDList::iterator iter = children.begin(); 
		iter != children.end() ; ++iter )
	{
		getWorldEditor()->destoryEditData( *iter );
	}
	EmGroupManager::instance().removeGroup( groupID );
}

void EmGroup::accept( EmVisitor& visitor )
{
	visitor.visit( this );
}

EmGroup::EmGroup()
{

}

EmGroup* EmGroupManager::createGroup( char const* name , unsigned parentGroup )
{
	EmGroup* data = new EmGroup;

	unsigned findID = 0;
	for( ; findID < groupVec.size() ; ++findID )
	{
		if ( groupVec[findID] == NULL )
			break;
	}
	assert( findID <= groupVec.size() );

	data->groupID = findID;
	data->setName( name );
	data->setGroup( parentGroup );
	groupVec.push_back( data );
	return data;
}

EmGroupManager::EmGroupManager()
{
	m_root.group = 0;
	groupVec.push_back( &m_root );
}

void EmGroupManager::clear()
{
	m_root.children.clear();
	groupVec.clear();
	groupVec.push_back( &m_root );
}

void EmDataBase::setGroup( unsigned gID )
{
	EmGroupManager::instance().setGroup( this , gID );
}

void EmDataBase::OnDestory()
{
	if ( logicGroup )
		logicGroup->removeEditData( this );
}

EmDataBase::EmDataBase()
{
	m_name = "unnamed";
	group = ROOT_GROUP;
	m_show = true;
	logicGroup = NULL;
}

void parseActorProp( TActor* actor , char const* prop )
{
	std::stringstream ss( prop );
	TVillager* villager;
	char buffer[ 128 ];
	TString token;

	while ( ss )
	{
		ss.getline( buffer , ARRAY_SIZE(buffer)  );
		std::stringstream sline( buffer );
		sline >> token;
		if ( token == "Item" )
		{
			if ( villager = dynamic_cast< TVillager* >( actor ) )
			{
				while ( sline.good() )
				{
					unsigned id;
					sline >> id;
					villager->addSellItem( id );
				}
			}
		}
		else if ( token == "Skill" )
		{
			while( sline.good() )
			{
				int level;
				sline >> token;
				sline >> level;
				actor->getSkillBook().learnSkill( token.c_str() , level );
			}
		}
	}
}

EmActor::EmActor( unsigned roleID ) 
	:roleID( roleID )
{
	actorType = AT_NPCBASE;

	setActor( createActor() );
	getActor()->setTransform( getActor()->getTransform() );
}


void EmActor::show( bool beS )
{
	EmDataBase::show( beS );
	getActor()->getFlyActor().Show( beS , beS , beS , false );
}


TActor* EmActor::createActor()
{
	TActor* result = NULL;
	XForm trans;
	trans.setIdentity();
	switch (  actorType )
	{
	case AT_NPCBASE:
		result = new TNPCBase( roleID ,  trans );
		break;
	case AT_VILLAGER:
		result = new TVillager( roleID , trans );
		break;
	}
	getWorldEditor()->addEntity( result );
	return result;
}

void EmActor::setActorType( ActorType type )
{
	if ( actorType == type )
		return;

	actorType = type;
	setActor( createActor() );
}

void EmActor::restore()
{
	setActor( createActor() );
	parseActorProp( getActor() , propStr.c_str() );
	BaseClass::restore();
}

void EmActor::OnUpdateData()
{
	getActor()->updateTransformFromFlyData();
}

void EmActor::OnDestory()
{
	getWorldEditor()->destoryEntity( getActor() );
	EmDataBase::OnDestory();
}

void EmActor::setActor( TActor* actor )
{
	setPhyEntity( actor );
	obj.Object( actor->getFlyActor().GetBaseObject() );
}

void EmActor::accept( EmVisitor& visitor )
{
	visitor.visit( this );
}

void EmPlayerBoxTGR::restore()
{
	setTrigger( createTrigger() );
	BaseClass::restore();
}

PlayerBoxTrigger* EmPlayerBoxTGR::createTrigger()
{
	PlayerBoxTrigger* result = new PlayerBoxTrigger( boxSize );
	getWorldEditor()->addEntity( result );
	return result;
}

void EmPlayerBoxTGR::setBoxSize( Vec3D const& size )
{
	boxSize = size;
	getTrigger()->setBoxSize( size );
	getTrigger()->debugDraw( false );
	getTrigger()->debugDraw( true );
}



EmPlayerBoxTGR::EmPlayerBoxTGR( Vec3D const& size ) 
	:boxSize( size )
{
	setTrigger( createTrigger() );
}

void EmPlayerBoxTGR::show( bool beS )
{
	BaseClass::show( beS );
	getTrigger()->debugDraw( beS );
}

TRefObj* EmPlayerBoxTGR::getLogicObj()
{
	return getTrigger();
}

void EmPlayerBoxTGR::accept( EmVisitor& visitor )
{
	visitor.visit( this );
}

void EmPlayerBoxTGR::setTrigger( PlayerBoxTrigger* trigger )
{
	setPhyEntity( trigger );
	obj.Object( trigger->getDebugObj().Object() );
}

EmChangeLevel::EmChangeLevel(TChangeLevel* logic)
	:logic(logic)
{

}

TRefObj* EmChangeLevel::getLogicObj()
{
	return logic;
}

void EmChangeLevel::accept( EmVisitor& visitor )
{
	visitor.visit( this );
}

unsigned EmLogicGroup::addEditData( EmDataBase* data )
{
	TRefObj* obj = data->getLogicObj();

	if ( !obj || data->logicGroup != NULL)
		return NO_LOGIC_GROUP;

	int emptyIndex  = -1;

	data->logicGroup = this;
	for ( unsigned i = 0 ; i < eDataVec.size() ; ++i )
	{
		if ( eDataVec[i] == data )
		{
			return i;
		}
		else if ( eDataVec[i] == NULL )
		{
			emptyIndex = i;
		}
	}

	if ( emptyIndex != -1 )
	{
		eDataVec[emptyIndex] = data;
		return emptyIndex;
	}

	eDataVec.push_back( data );
	return eDataVec.size() - 1;
}

void EmLogicGroup::addConnect( unsigned sID, char const* signalName , unsigned hID , char const* slotName )
{
	if ( eDataVec[ sID ] == NULL || 
		 eDataVec[ hID ] == NULL )
		 return;

	TRefObj* sender = eDataVec[ sID ]->getLogicObj();
	TRefObj* holder = eDataVec[ hID ]->getLogicObj();
	assert ( sender && holder );

	int result = SignalManager::instance().connect( 
		sender  ,  signalName   ,  holder  ,  slotName  );

	if ( result >= 0 )
	{
		ConnectInfo info;
		info.sID = sID;
		info.hID = hID;
		info.signalName = signalName;
		info.slotName = slotName;

		conList.push_back( info );
	}
}

void EmLogicGroup::restore()
{
	for ( ConInfoList::iterator iter = conList.begin(); 
		iter != conList.end() ; ++iter )
	{
		connect( *iter );
	}
	EmDataBase::restore();
}

void EmLogicGroup::connect( ConnectInfo& info )
{
	TRefObj* sender = eDataVec[ info.sID ]->getLogicObj();
	TRefObj* holder = eDataVec[ info.hID ]->getLogicObj();
	assert ( sender && holder );

	int result = SignalManager::instance().connect( 
		sender  ,  info.signalName.c_str()   ,  holder  ,  info.slotName.c_str()  );
}

void EmLogicGroup::removeEditData( EmDataBase* data )
{
	unsigned index = 0;
	for ( ; index < eDataVec.size() ; ++index )
	{
		if ( eDataVec[index] == data )
		{
			eDataVec[index] = NULL;
			break;
		}
	}

	for ( ConInfoList::iterator iter = conList.begin() ; 
		iter != conList.end() ; )
	{
		if ( iter->hID == index ||
			 iter->sID == index )
		{
			iter = conList.erase( iter );
		}
		else ++iter;
	}
}

void EmLogicGroup::accept( EmVisitor& visitor )
{
	visitor.visit( this );
}

EmLogicGroup::EmLogicGroup()
{

}

void EmLogicGroup::removeConnect( ConnectInfo* info )
{
	if ( !info )
		return;

	for ( ConInfoList::iterator iter = conList.begin() ; 
		  iter != conList.end() ; ++iter )
	{
		if ( info == &(*iter) )
		{
			disconnect( *info );
			iter = conList.erase( iter );
			return;
		}
	}
}

void EmLogicGroup::disconnect( ConnectInfo& info )
{
	TRefObj* sender = eDataVec[ info.sID ]->getLogicObj();
	TRefObj* holder = eDataVec[ info.hID ]->getLogicObj();
	assert ( sender && holder );

	SignalManager::instance().disconnect(
		sender  ,  info.signalName.c_str()   ,  
		holder  ,  info.slotName.c_str()  );

}
void EmPhyEntityData::setPhyEntity( TPhyEntity* pe )
{
	if ( m_pe )
	{
		pe->setTransform( m_pe->getTransform() );
		getWorldEditor()->destoryEntity( m_pe );
	}
	m_pe = pe;
}

EmPhyEntityData::EmPhyEntityData() 
	:EmObjData( FAILED_ID )
	,m_pe( NULL )
{
	flag |= EDF_SAVE_XFORM;
}

void EmPhyEntityData::setPosition( Vec3D const& pos )
{
	EmObjData::setPosition( pos );
	m_pe->updateTransformFromFlyData();
}

void EmPhyEntityData::setFrontDir( Vec3D const& dir )
{
	EmObjData::setFrontDir( dir );
	m_pe->updateTransformFromFlyData();
}

void EmPhyEntityData::setUpDir( Vec3D const& dir )
{
	EmObjData::setUpDir( dir );
	getPhyEntity()->updateTransformFromFlyData();
}

void EmPhyEntityData::restore()
{
	assert( obj.Object() != FAILED_ID );
	BaseClass::restore();
	getPhyEntity()->updateTransformFromFlyData();
}

void EmPhyEntityData::OnUpdateData()
{
	getPhyEntity()->updateTransformFromFlyData();
}

void EmPhyEntityData::OnDestory()
{
	getWorldEditor()->destoryEntity( getPhyEntity() );
	BaseClass::OnDestory();
}

void EmPhyObj::accept( EmVisitor& visitor )
{
	visitor.visit(this);
}

void EmPhyObj::setPhyObj( TObject* pObj )
{
	setPhyEntity( pObj );
	obj.Object( pObj->getFlyObj().Object() );
}

TObject* EmPhyObj::createPhyObj()
{
	unsigned id = TResManager::instance().getObjectModelID( modelName.c_str() );
	assert( id != RES_ERROR_ID );
	TObject* pObj = new TObject( id );
	getWorldEditor()->addEntity( pObj );

	return pObj;
}

void EmPhyObj::restore()
{
	setPhyObj( createPhyObj() );
	BaseClass::restore();
}

EmPhyObj::EmPhyObj( unsigned modelID )
{
	modelName = TResManager::instance().getModelData( modelID )->resName;
	setPhyObj( createPhyObj() );
}

void EmPhyObj::OnUpdateData()
{
	BaseClass::OnUpdateData();
	getPhyObj()->getPhyBody()->activate( );
}


EmSpawnZone::EmSpawnZone( Vec3D const& pos )
{
	spawnZone = NULL;
	setSpawnZone( createSpawnZone( pos ) );
}
void EmSpawnZone::restore()
{
	getWorldEditor()->addEntity( spawnZone );
}

TSpawnZone* EmSpawnZone::createSpawnZone( Vec3D const& pos )
{
	TSpawnZone* result = new TSpawnZone( pos );
	getWorldEditor()->addEntity( result );
	return result;
}

void EmSpawnZone::setSpawnZone( TSpawnZone* zone )
{
	if ( spawnZone )
	{
		getWorldEditor()->destoryEntity( spawnZone );
	}
	spawnZone = zone;
}

void EmSpawnZone::OnDestory()
{
	getWorldEditor()->destoryEntity( spawnZone );
}

void EmSpawnZone::show( bool beS )
{
	BaseClass::show(beS);
	spawnZone->debugDraw( beS );
}

void EmSpawnZone::accept( EmVisitor& visitor )
{
	visitor.visit( this );
}

TRefObj* EmSpawnZone::getLogicObj()
{
	return spawnZone;
}

void EmSpawnZone::OnUpdateData()
{
	BaseClass::OnUpdateData();
	show( false );
	show( true );
}