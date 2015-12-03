#ifndef TSpawnZone_h__
#define TSpawnZone_h__

#include "common.h"
#include "TPhyEntity.h"
#include "TSlotSingalSystem.h"
#include "TPorpLoader.h"


class TLogicObj : public TLevelEntity
{
	SLOT_CLASS( TLogicObj )
public:
	TLogicObj();
	void play();
	void active(){ m_active = true; }
	void deactive(){ m_active = false; }
	void setActive( bool beA ){}
protected:
	virtual void doPlay(){}
	bool m_active;
	
};


class TTimeTrigger
{

};

class TChangeLevel : public TLogicObj
{
public:
	ACCESS_PROP_COLLECT();

	TString  levelName;
	Vec3D    playerPos;
	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version )
	{
		ar & levelName ;
		if ( file_version >= 1 )
			ar & playerPos;
	}

protected:
	virtual void doPlay();
};


class TSpawnZone :  public TLogicObj
{
public:
	TSpawnZone( Vec3D const& pos );
	virtual void doPlay();


	void       setPosition( Vec3D const& p ){ pos = p; }
	void       setSpawnRoleID( unsigned id );
	unsigned   getSpawnRoleID(){ return spawnRoleID; }
	virtual void debugDraw( bool beDebug );

	Vec3D      pos;
	unsigned   spawnRoleID;
	int        spawnNum;

	template < class Archive >
	void serialize( Archive & ar , const unsigned int  file_version )
	{
		ar & pos & spawnRoleID;
	}
	TSpawnZone(){}
protected:

	ACCESS_PROP_COLLECT();
	FnObject m_dbgObj;
};



#endif // TSpawnZone_h__