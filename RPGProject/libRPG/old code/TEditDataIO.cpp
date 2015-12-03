#include "common.h"
#include "TObjectEditor.h"
#include "serialization.h"
#include "TLevel.h"
#include "TLogicObj.h"


template < class Archive >
void TObjectEditor::registerType( Archive & ar )
{
	//ar.register_type( static_cast<EmDataBase*>(NULL));
	//ar.register_type( static_cast<EmObjData*>(NULL));
	ar.register_type( static_cast<EmGroup*>(NULL));
	ar.register_type( static_cast<EmSceneObj*>(NULL));
	ar.register_type( static_cast<EmActor*>(NULL));
	ar.register_type( static_cast<EmPlayerBoxTGR*>(NULL));
	ar.register_type( static_cast<EmChangeLevel*>(NULL));
	ar.register_type( static_cast<EmLogicGroup*>(NULL));
	ar.register_type( static_cast<EmPhyObj*>(NULL));
	ar.register_type( static_cast<EmSpawnZone*>(NULL));
}

BOOST_CLASS_VERSION( EmDataBase , 1 )
BOOST_CLASS_VERSION( TChangeLevel , 1 )

template < class Archive >
void EmDataBase::serialize( Archive & ar , const unsigned int file_version )
{
	ar & group  & m_name & m_show;
	if ( file_version >= 1 )
	{
		ar & logicGroup;
	}
}


template < class Archive >
void EmObjData::serialize( Archive & ar , const unsigned int file_version )
{
	ar & boost::serialization::base_object<EmDataBase>(*this);
	ar & flag;
	if ( flag & EDF_SAVE_XFORM )
	{
		ar & pos & front & up;
	}
}

template < class Archive >
void EmSceneObj::serialize( Archive & ar , const unsigned int file_version )
{
	ar & boost::serialization::base_object<EmObjData>(*this);
	ar & renderGroup ;
}

template < class Archive >
void EmGroup::serialize( Archive & ar , const unsigned int file_version )
{
	ar & boost::serialization::base_object<EmDataBase>(*this);
	ar & groupID & children;
}

BOOST_CLASS_VERSION( EmActor , 1 )
template < class Archive >
void EmActor::serialize( Archive & ar , const unsigned int  file_version )
{
	ar & boost::serialization::base_object<EmObjData>(*this);
	ar & roleID & actorType ;
	if ( file_version >= 1 )
	{
		ar & propStr;
	}
}

template < class Archive >
void EmPlayerBoxTGR::serialize( Archive & ar , const unsigned int  file_version )
{
	ar & boost::serialization::base_object<EmObjData>(*this);
	ar & boxSize;
}


template < class Archive >
void EmLogicGroup::serialize( Archive & ar , const unsigned int  file_version )
{
	ar & boost::serialization::base_object<EmDataBase>(*this);
	ar & conList & eDataVec ;
}

template < class Archive >
void EmGroupManager::serialize( Archive & ar , const unsigned int  file_version )
{
	ar & m_root & groupVec;
}

template < class Archive >
void EmChangeLevel::serialize( Archive & ar , const unsigned int  file_version )
{	
	ar & boost::serialization::base_object<EmDataBase>(*this);
	ar & logic;
}

template < class Archive >
void EmPhyObj::serialize( Archive & ar , const unsigned int  file_version )
{	
	ar & boost::serialization::base_object<BaseClass>(*this);
	ar & modelName;
}

template < class Archive >
void EmSpawnZone::serialize( Archive & ar , const unsigned int  file_version )
{	
	ar & boost::serialization::base_object<BaseClass>(*this);
	ar & spawnZone;
}

bool TObjectEditor::saveEditData( char const* name )
{
	try
	{
		std::ofstream fs(name);
		boost::archive::text_oarchive ar(fs);

		registerType( ar );

		ar &  g_EditData;
		ar &  EmGroupManager::instance();


	}
	catch ( ... )
	{
		return false;
	}

	return true;
}

bool TObjectEditor::loadEditData( char const* name , TLevel* level )
{
	EmGroupManager::instance().clear();

	getWorldEditor()->changeLevel( level );

	try
	{
		std::ifstream fs(name);
		boost::archive::text_iarchive ar(fs);
		g_EditData.clear();

		registerType( ar );

		EmGroup* gData  = EmGroupManager::instance().getRootGroupData();

		ar &  g_EditData;
		ar &  EmGroupManager::instance();

		for ( int i = 0 ; i < g_EditData.size();++i)
		{
			g_EditData[i]->restore();
			g_EditData[i]->OnCreate();
		}
	
	}
	catch ( ... )
	{
		return false;
	}

	return true;

}

