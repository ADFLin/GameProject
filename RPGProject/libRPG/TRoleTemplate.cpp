#include "ProjectPCH.h"
#include "TRoleTemplate.h"

#include "TResManager.h"
#include "serialization.h"

SRoleInfo g_RoleData[] =
{	               //team          //race        //¾�~
	                 //level//MaxHP//MaxMP// STR// INT// DEX// END//KExp
	{ 
		"defultRole" , "unknow"  , TEAM_MONSTER , RACE_UNKONOWN ,   CRT_UNKNOW  ,  false ,
		SAbilityPropData(  1 ,  200 ,  100 ,  20 ,  10 ,  12 ,  13 ,  0 ) ,
	} ,

	{  
		"Player"   , "Hero"  , TEAM_PLAYER  , RACE_HUMAN ,  CRT_WARRIOR  ,  true ,
		SAbilityPropData(  1 ,  200 ,  100 ,  20 ,  10 ,  12 ,  13 ,  0 ) ,
	} ,

	{  
		"Angel"  , "Angel"  , TEAM_MONSTER , RACE_UNKONOWN ,   CRT_UNKNOW  , false ,
		SAbilityPropData(  1 ,  200 ,  100 ,  20 ,  10 ,  12 ,  13 ,  0 ) ,
	} ,

	{ 
		"Basilisk", "Basilisk", TEAM_MONSTER , RACE_UNKONOWN ,   CRT_UNKNOW  , false ,
		SAbilityPropData(  1 ,  200 ,  100 ,  20 ,  10 ,  12 ,  13 ,  0 ) ,
	} ,

	{  
		"Flower" , "Flower" , TEAM_MONSTER , RACE_UNKONOWN ,   CRT_UNKNOW  , false ,
		SAbilityPropData(  1 ,  200 ,  100 ,  20 ,  10 ,  12 ,  13 ,  0 ) ,
	} ,
};


#define TEAM_RELATION( team , relation )\
	relation << ( TEAM_RELATIONSHIP_BIT_NUM * team )

unsigned g_teamRelationship[] = 
{
	//TEAM_PLAYER
	TEAM_RELATION( TEAM_VILLAGER , TEAM_FRIEND  ) |
	TEAM_RELATION( TEAM_MONSTER  , TEAM_EMPTY   ) |
	TEAM_RELATION( TEAM_ANIMAL   , TEAM_NEUTRAL ) ,
	//TEAM_VILLAGER
	TEAM_RELATION( TEAM_PLAYER   , TEAM_FRIEND  ) |
	TEAM_RELATION( TEAM_MONSTER  , TEAM_EMPTY   ) |
	TEAM_RELATION( TEAM_ANIMAL   , TEAM_NEUTRAL ) ,
	//TEAM_MONSTER
	TEAM_RELATION( TEAM_PLAYER   , TEAM_EMPTY   ) |
	TEAM_RELATION( TEAM_VILLAGER , TEAM_EMPTY   ) |
	TEAM_RELATION( TEAM_ANIMAL   , TEAM_NEUTRAL ) ,
	//TEAM_ANIMAL
	TEAM_RELATION( TEAM_PLAYER   , TEAM_NEUTRAL ) |
	TEAM_RELATION( TEAM_VILLAGER , TEAM_NEUTRAL ) |
	TEAM_RELATION( TEAM_MONSTER  , TEAM_NEUTRAL ) ,
};

unsigned GetTeamRelation( unsigned selfTeam , unsigned otherTeam )
{
	return ( g_teamRelationship[ selfTeam ] >> ( TEAM_RELATIONSHIP_BIT_NUM * otherTeam ) ) & TEAM_RELATIONSHIP_MASK;
}

void SRoleInfo::levelUp()
{
	propData.level += 1;

	propData.MaxHP *= 1.05;
	propData.MaxMP *= 1.05;

	propData.STR += TRandom() % 2 + 1;
	propData.END += TRandom() % 2 + 1;
	propData.INT += TRandom() % 2 + 1;
	propData.DEX += TRandom() % 2 + 1;

}

template < class Archive >
void SRoleInfo::serialize( Archive & ar , const unsigned int file_version )
{
	ar & roleName & modelName & team & race & career ;
	ar & useWeapon & propData & roleID ;
}

template < class Archive >
void SAbilityPropData::serialize( Archive & ar , const unsigned int file_version )
{
	ar &   level;
	ar &   MaxHP;
	ar &   MaxMP;
	///////////////////////////
	ar &   STR; //�O�q
	ar &   INT; //���O
	ar &   DEX; //�ӱ�
	ar &   END; //�@�O
	////////////////////////////
	ar &  KExp;    //�i��o���g���
	//////////////////////////
	ar &   viewDist;
	ar &   ATRange; //�����d��(�ѪZ�����)
	ar &   ATSpeed; //�����t��
	ar &   MVSpeed; //�̤j���ʳt��
	ar &   JumpHeight;
}

SRoleInfo* TRoleManager::getRoleInfo( unsigned roleID )
{
	if ( roleID < roleTable.size() && 
		 roleTable[roleID] )
		return roleTable[roleID];

	if ( roleID < ARRAY_SIZE(g_RoleData) )
		return &g_RoleData[ roleID ];

	return NULL;
}

unsigned TRoleManager::getModelID( unsigned roleID )
{
	return TResManager::getInstance().getActorModelID( 
		getRoleInfo( roleID )->modelName.c_str() );
}

TRoleManager::TRoleManager()
{

}

void TRoleManager::initDefultRole()
{
	for ( int i = 0 ; i < ARRAY_SIZE( g_RoleData ) ; ++i )
	{
		addRoleData( g_RoleData[i] );
	}
}

unsigned TRoleManager::addRoleData( SRoleInfo& roleData )
{
	roleList.push_back( roleData );

	SRoleInfo& data = roleList.back();

	for ( int i = 0; i < roleTable.size() ; ++i )
	{
		if ( roleTable[i] == NULL )
		{
			data.roleID = i;
			roleTable[i] = &data;
			return data.roleID;
		}
	}

	data.roleID = roleTable.size();
	roleTable.push_back( &data );

	return data.roleID;
}

bool TRoleManager::saveData( char const* path )
{
	try
	{
		std::ofstream fs( path );
		boost::archive::text_oarchive ar(fs);

		ar & roleList ;
		ar & roleTable;
	}
	catch ( ... )
	{
		return false;
	}
	return true;
}

bool TRoleManager::loadData( char const* path )
{
	roleTable.clear();
	roleList.clear();
	try
	{
		std::ifstream fs( path );
		boost::archive::text_iarchive ar(fs);

		ar & roleList ;
		ar & roleTable;

	}
	catch ( ... )
	{
		roleTable.clear();
		roleList.clear();

		initDefultRole();

		return false;
	}
	return true;
}

unsigned TRoleManager::createNewRole()
{
	return addRoleData( g_RoleData[0] );
}

void TRoleManager::destoryRole( unsigned roleID )
{
	if ( !roleTable[roleID] )
		return;

	for( std::list< SRoleInfo >::iterator iter = roleList.begin();
		iter != roleList.end() ; ++iter )
	{
		if ( roleTable[roleID] == &(*iter ) )
		{
			roleTable[roleID] = NULL;
			roleList.erase( iter );
			return;
		}
	}
}