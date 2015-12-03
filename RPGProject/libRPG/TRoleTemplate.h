#ifndef TRoleTemplate_h__
#define TRoleTemplate_h__

#include "common.h"
#include "AbilityProp.h"

#define ROLE_DATA_PATH "Data/role.dat"

#define ROLE_HERO 1

enum TeamFlag
{
	TEAM_PLAYER  = 0,
	TEAM_VILLAGER   ,
	TEAM_MONSTER    ,
	TEAM_ANIMAL     ,


	TEAM_RELATIONSHIP_BIT_NUM = 4   ,
	TEAM_RELATIONSHIP_MASK    = 0xf ,

	TEAM_FRIEND  = 1,
	TEAM_NEUTRAL = 2,
	TEAM_EMPTY   = 3,
};

unsigned GetTeamRelation( unsigned selfTeam , unsigned otherTeam );

enum RaceType
{
	RACE_HUMAN ,
	RACE_UNKONOWN ,
};

enum CareerType
{
	CRT_UNKNOW  ,
	CRT_WARRIOR ,
	CRT_MAGE    ,
};

struct  SRoleInfo
{
	String           roleName;
	String           modelName;
	TeamFlag         team;
	RaceType         race;
	CareerType       career;
	bool             useWeapon;
	SAbilityPropData propData;

	unsigned         roleID;

	void levelUp();


	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version );
};


class TRoleManager : public SingletonT< TRoleManager >
{
public:
	TRoleManager();

	void       initDefultRole();
	SRoleInfo* getRoleInfo( unsigned roleID );
	unsigned   getModelID( unsigned roleID );

	unsigned   createNewRole();

	void       destoryRole( unsigned roleID );

	unsigned   addRoleData( SRoleInfo& roleData );
	bool       saveData( char const* path );
	bool       loadData( char const* path );

	unsigned   getMaxRoleID(){ return roleTable.size() - 1; }

	std::vector< SRoleInfo* > roleTable;
	std::list< SRoleInfo >    roleList;

};

#endif // TRoleTemplate_h__
