#ifndef TTrigger_h__
#define TTrigger_h__

#include "common.h"

#include <list>
#include "TPhyEntity.h"
#include "CHandle.h"
#include "TTrigger.h"
#include "TEventSystem.h"
#include "TSlotSingalSystem.h"


enum TriggerType
{
	TGR_LADDER ,

};

class TriggerLogicComponent : public EntityLogicComponent
{
public:
	TriggerLogicComponent( CollisionComponent* colComp )
	{

	}

	TriggerType getTriggerType() const { return mType; }
	void        setTriggerType( TriggerType type) { mType = type; }
protected:
	TriggerType mType;
};

class TTrigger : public TPhyEntity
{
	DESCRIBLE_ENTITY_CLASS( TTrigger , TPhyEntity );

protected:
	struct PEData
	{
		bool     isInside;
		PEHandle handle;
	};

	typedef std::list<PEData> PEDataList;
	typedef PEDataList::iterator iterator;

	iterator findEntity( TPhyEntity* entity );
public:
	TTrigger(TPhyBodyInfo& info);

	void OnCollision( TPhyEntity* colObj , Float depth ,
		              Vec3D const& selfPos , Vec3D const& colObjPos , 
					  Vec3D const& normalOnColObj );

	void updateFrame();

	virtual void InTrigger( TPhyEntity* entity ){}
	virtual void OnTough( TPhyEntity* entity ){}
	virtual void OnExit( TPhyEntity* entity ){}

	FnObject& getDebugObj(){ return m_dbgObj; }
protected:
	virtual void initDebugObj(){}
	FnObject   m_dbgObj;

	PEDataList m_handleVec;
	TString    name;

};

class TBoxTrigger : public TTrigger
{
public:
	TBoxTrigger( Vec3D const& len )
		:TTrigger( createPhyBodyInfo( 0 , new btBoxShape( 0.5 * len ) ) )
	{

	}

	void setBoxSize( Vec3D const& size );
};

class PlayerBoxTrigger : public TBoxTrigger
{
public:
	PlayerBoxTrigger( Vec3D const& len );
	~PlayerBoxTrigger(){}
	void OnTough( TPhyEntity* entity );
	void debugDraw( bool beDebug );

protected:
	SINGNAL_CLASS( PlayerBoxTrigger )
	SIGNAL( void , playerTough )
};

class TItemStorage;
class TChestTrigger : public TBoxTrigger
{
	DESCRIBLE_CLASS( TChestTrigger , TBoxTrigger )
public:
	TChestTrigger();

	~TChestTrigger();

	virtual void updateFrame();
	void addItem( unsigned itemID  , int num );
	TItemStorage& getItems(){ return *m_items; }

	void OnTough( TPhyEntity* entity );
	void OnExit( TPhyEntity* entity );

	void setVanishTime( float time );

	static int const MaxItemNum = 16;
	static float const VanishTime;

protected:
	void vanishThink();


	float m_DTime;
	FnObject modelObj;
	Holder< TItemStorage > m_items;
};

class TLadderTrigger : public TBoxTrigger
{
public:
	TLadderTrigger( Vec3D const& len )
		:TBoxTrigger( len ){}
	virtual void OnTough( TPhyEntity* entity );
	virtual void OnExit( TPhyEntity* entity );

};
class TLevelChangeTrigger : public TBoxTrigger
{
public:
	TLevelChangeTrigger();

};

#endif // TTrigger_h__
