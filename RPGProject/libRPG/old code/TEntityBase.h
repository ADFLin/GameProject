#ifndef TBaseEnity_h__
#define TBaseEnity_h__

#include <vector>
#include "common.h"
#include "CHandle.h"

class TEntityBase;
class TLevel;

typedef void (TEntityBase::*fnThink)();


enum EventType;
enum ThinkMode
{
	THINK_LOOP,
	THINK_ONCE,
	THINK_NOTHINK,
};

enum EntityFlag;

enum EntityFlag
{
	EF_DESTORY = 0x001, 
	EF_FREEZE  = 0x002,
	EF_WORKING = 0x004,
	EF_WORK_STATE_MASK = 0x00f,

	EF_GLOBAL = 0x010,
	EF_LEVEL  = 0x020,
	EF_TEMP   = 0x040,

	EF_ZONE_MAK = 0x0f0 ,
	EF_MANAGING = 0x100 ,
};

struct ThinkContent
{
	ThinkMode     mode;
	fnThink       fun;
	float         nextTime;
	float         lastTime;
	float         loopTime;
};

#define NEVER_THINK (-1)
typedef std::vector< ThinkContent > ThinkVec;


class TEntityBase : public TRefObj
{
public:
	TEntityBase();
	virtual ~TEntityBase();

	//for every cycle frame
	virtual void updateFrame(){};
	unsigned     getEntityType() const { return m_entityType; }
	bool         isKindOf( unsigned type ){  return ( m_entityType & type ) ? true : false; }
	

	//void         addEntityFlag( EntityFlag val ){ m_entityFlag |= val; }
	unsigned     getEntityState() const;
	void         setEntityState( EntityFlag val);
	void         setGlobal( bool beG );
	void         setTemp( bool beT );
	bool         isGlobal();
	bool         isTemp();

	// using in TEntityManager
	void         update();
	virtual void debugDraw( bool beDebug ){}
	unsigned     getEntityFlag(){ return m_entityFlag; }

private:
	
	void         addEntityFlag( unsigned val ){ m_entityFlag |= val; }
	void         removeEntityFlag( unsigned val ){ m_entityFlag &= ~val; }
	friend class TEntityManager;
	unsigned     m_entityFlag;

protected:
	unsigned     m_entityType;

public: 
	//Think
	ThinkContent& getCurThinkContent();
	void          processThink();
	void          setNextThink( int index , float time );
	size_t        addThink( fnThink fun , ThinkMode mode = THINK_ONCE , float loopTime = 0.0 );
	void          setCurContextNextThink(float time);

protected:
	int          m_curThinkIndex;
	ThinkVec     m_thinkFunVec;

public:
	void *operator new( size_t size );
	void  operator delete( void *pMem );

};


#define DESCRIBLE_CLASS( thisClass , baseClass )\
private:\
	typedef thisClass  ThisClass;\
	typedef baseClass  BaseClass;

#define THINK_FUN( FunName ) (fnThink)( &ThisClass::FunName )

#define DESCRIBLE_ENTITY_CLASS( thisClass , baseClass )\
	DESCRIBLE_CLASS( thisClass , baseClass )\
public:\
	static thisClass * upCast( TEntityBase* entity );

#define DEFINE_UPCAST( type , sceneID )\
type* type::upCast( TEntityBase* entity )\
{\
	if ( entity && entity->isKindOf( sceneID ) )\
		return ( type *) entity;\
	return NULL;\
}\


class TLevelEntity : public TEntityBase
{
	DESCRIBLE_ENTITY_CLASS( TLevelEntity  , TEntityBase );
public:
	TLevelEntity();
	virtual void  OnSpawn(){}
	TLevel*       getLiveLevel(){ return m_level; }
private:
	friend class  TLevel;
	void          changeLevel( TLevel* level ){ m_level = level ; OnSpawn(); }
	TLevel*       m_level;
};

#endif // TBaseEnity_h__