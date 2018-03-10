#include "ProjectPCH.h"
#include "TSkill.h"

#include "CActor.h"
#include "EventType.h"
#include "UtilityGlobal.h"

#include "serialization.h"

#define FX_BUF_SKILL "buf_skill"

typedef std::vector<ISkillFactory*> SkFactoryVec;

static SkFactoryVec& getSkFactoryVec()
{
	static SkFactoryVec s_RegFactoryVec;
	return s_RegFactoryVec;
}


void ISkillFactory::clientLibrary(ISkillFactory* factory)
{
	getSkFactoryVec().push_back( factory );
}

bool TSkillLibrary::saveIconName( char const* patch )
{
	try
	{
		std::ofstream fs( patch );
		boost::archive::text_oarchive ar(fs);

		for( SkFactoryVec::iterator iter = getSkFactoryVec().begin();
			iter != getSkFactoryVec().end(); ++iter )
		{
			ISkillFactory* factory = *iter;
			if ( factory->getIconName() != NULL )
			{
				ar & String(factory->getName());
				ar & String(factory->getIconName());
			}
		}
	}
	catch ( ... )
	{
		return false;
	}

	return true;
}


bool TSkillLibrary::loadIconName( char const* patch )
{
	try
	{
		std::ifstream fs( patch );
		boost::archive::text_iarchive ar(fs);

		String name , iconName;

		while ( fs)
		{
			ar & name ;
			ar & iconName;

			if ( ISkillFactory* factory = getFactory( name.c_str() ) )
			{
				factory->setIconName( iconName.c_str() );
			}

		}
	}
	catch ( ... )
	{
		return true;
	}

	return true;
}

void TSkillLibrary::init()
{
	for( SkFactoryVec::iterator iter = getSkFactoryVec().begin();
		iter != getSkFactoryVec().end(); ++iter )
	{
		initSkill( *iter );
	}

	if ( !loadIconName( SKILL_DATA_PATCH ) )
	{
		LogErrorF("Can't Load Skill Data : %s" , SKILL_DATA_PATCH );
	}
}

TSkill* TSkillLibrary::createSkill( char const* name , int level )
{
	ISkillFactory* factory = getFactory( name );

	if ( factory )
	{
		TSkill* skill = factory->create( level );
		skill->name = factory->getName();
		return skill;
	}

	return NULL;
}

void TSkillLibrary::initSkill( ISkillFactory* factory )
{
	m_skillMap.insert( std::make_pair( factory->getName() , factory ) );
}

SkillInfo const* TSkillLibrary::querySkill( char const* name , int level )
{
	ISkillFactory* factory = getFactory( name );

	if ( factory )
	{
		return &factory->querySkillInfo( level );
	}
	return NULL;
}

ISkillFactory* TSkillLibrary::getFactory( char const* name )
{
	SkillMap::iterator iter = m_skillMap.find( name );

	if ( iter != m_skillMap.end() )
		return iter->second;
	return NULL;
}

char const* TSkillLibrary::getIconName( char const* name )
{
	ISkillFactory* factory = getFactory( name );
	if ( factory )
		return factory->getIconName();

	return NULL;
}

bool TSkillLibrary::setIconName( char const* name , char const* iconName )
{
	if ( ISkillFactory* factory = getFactory( name ) )
	{
		factory->setIconName( iconName );
		return true;
	}
	return false;
}

int TSkillLibrary::getAllSkillName( char const** str , int maxNum )
{
	int num = 0;
	for( SkFactoryVec::iterator iter = getSkFactoryVec().begin();
		iter != getSkFactoryVec().end(); ++iter )
	{
		if ( num > maxNum )
			break;

		ISkillFactory* factory = *iter;
		str[num] = factory->getName();
		++num;
	}
	return num;
}

void TSkillLibrary::clear()
{
	m_skillMap.clear();
}

char const* TSkillLibrary::getFullName( char const* name )
{
	ISkillFactory* factory = getFactory( name );
	if ( factory )
		return factory->getFullName();

	return NULL;
}

void TSkill::onStart( ILevelObject* select )
{
	int selectFlag = SF_SELECT_EMPTY|SF_SELECT_FRIEND|SF_NO_SELECT;
	assert( ( getInfo().flag & selectFlag ) == 0 );
}

void TSkill::onStart(Vec3D const& pos )
{
	assert( ( getInfo().flag & SF_SELECT_POSITION ) == 0 );
}


//class TObjEntity : public TLevelEntity
//{
//public:
//	TObjEntity( FnObject& _obj )
//		:obj(_obj)
//		,pos(0,0,0)
//		,v(0,0,0)
//	{
//
//	}
//	~TObjEntity()
//	{
//		UF_DestoryObject( obj );
//	}
//
//	Vec3D const& getPosition() const { return pos; }
//	Vec3D const& getVelocity() const { return v; }
//
//	void  setPosition(Vec3D const& val) { pos = val; }
//	void  setVelocity(Vec3D const& vel ){ v = vel; }
//	void  updateFrame()
//	{
//		pos += v * g_GlobalVal.frameTime;
//		obj.SetWorldPosition( pos );
//	}
//
//	void destoryThink()
//	{
//		setEntityState( EF_DESTORY );
//	}
//
//	void setDestoryTime( float time )
//	{
//		int index = addThink( (fnThink) &TObjEntity::destoryThink );
//		setNextThink( index , time );
//	}
//protected:
//	FnObject obj;
//	Vec3D v;
//	Vec3D pos;
//};


void TSkill::startBuff()
{
	TEvent event( EVT_SKILL_BUFF_START , caster->getRefID() , caster , this  );
	UG_SendEvent( event );

	//FnWorld world = TResManager().instance().getWorld();

	//FnScene scene;
	//scene.Object( caster->getFlyActor().GetScene() );
	//
	//world.SetTexturePath("Data/UI");

	//char* texName = "magic2";
	//FnMaterial mat = UF_CreateMaterial();
	//TEXTUREid tID = mat.AddTexture( 0 , 0 , texName , FALSE );

	//int a = scene.GetCurrentRenderGroup();
	//scene.SetCurrentRenderGroup( 1 );
	//int b = scene.GetCurrentRenderGroup();
	//FnObject obj = UF_CreateObject( scene );

	////caster->getOwner()->getComponent( );
	//float dz = caster->getBodyHalfHeigh();
	//UF_CreateSquare3D( &obj , Vec3D(-60,-60,-dz)  , 120 , 120 , Vec3D(1,1,1) , mat.Object()  );


	//obj.SetOpacity( 0.30f );
	//obj.SetRenderOption( Z_BUFFER , FALSE );
	//obj.SetRenderOption( SOURCE_BLEND_MODE , BLEND_SRC_ALPHA );
	//obj.SetRenderOption( DESTINATION_BLEND_MODE , BLEND_ONE );

	//obj.ChangeRenderGroup( RG_SHADOW );


	//TObjEntity* objEntity = new TObjEntity( obj );

	//objEntity->setDestoryTime( g_GlobalVal.curtime + getInfo().bufTime );
	//objEntity->setPosition( caster->getPosition() );
	//objEntity->listenEvent( EVT_SKILL_CANCEL , caster->getRefID() , (FnMemEvent)&TObjEntity::destoryThink );

	//caster->getLiveLevel()->addEntity( objEntity );


	//{
	//	fxData = TFxPlayer::instance().play( FX_BUF_SKILL );
	//	fxData->playForever();
	//	FnObject obj; obj.Object( fxData->getFx()->GetBaseObject() );
	//	obj.SetParent( caster->getFlyActor().GetBaseObject() );
	//	obj.SetPosition( Vec3D(0,0,0) );
	//}

	
}

void TSkill::onCancel()
{
	//TEvent event( EVT_FX_DELETE , EVENT_ANY_ID , NULL , fxData );
	//UG_SendEvent( event );
}

void TSkill::endBuff()
{
	{
		//TEvent event( EVT_FX_DELETE , EVENT_ANY_ID , NULL , fxData );
		//UG_SendEvent( event );
	}

	{
		TEvent event( EVT_SKILL_BUFF_END , caster->getRefID() , this );
		UG_SendEvent( event );
	}
}

TSkill::TSkill()
{
	caster = NULL; fullName = NULL;
}


SkillInfo const* UG_QuerySkill( char const* name , int level )
{
	return TSkillLibrary::Get().querySkill( name , level );
}