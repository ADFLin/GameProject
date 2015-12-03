#ifndef TEffect_h__
#define TEffect_h__

#include "common.h"
#include "TAblilityProp.h"

class TEntityBase;
struct DamageInfo;

enum ActorState;

class TEffectBase 
{
public:
	virtual ~TEffectBase(){}
	virtual void update(TEntityBase* entity){}
	virtual void OnStart(TEntityBase* entity){}
	virtual void OnEnd(TEntityBase* entity){}
	virtual bool needRemove(TEntityBase* entity) = 0;
};

class EfCycle
{
public:
	EfCycle( int totalCount )
		:m_totalCount( totalCount )
		,count(0)
	{}
	void increase(){	++count; }
	bool isCycleEnd(){	return count >= m_totalCount;	}

	int m_totalCount;
	int count;
};

class EfTimer
{
public:
	EfTimer( float ctime )
		:cycleTime( ctime )
	{
		prevTime = g_GlobalVal.curtime;
	}
	bool OnTimer()
	{
		if ( g_GlobalVal.curtime - prevTime > cycleTime )
		{
			prevTime = g_GlobalVal.curtime;
			return true;
		}
		return false;
	}
	float cycleTime;
	float prevTime;
};


class PropMotifyEffect : public TEffectBase
	                   , public EfTimer
{
public:

	PropMotifyEffect( unsigned stateBit , float ctime )
		:EfTimer( ctime )
	{
		num = 0;
	}


	void update( TEntityBase* entity )
	{

	}
	void onStart( TEntityBase* entity );


	void onEnd( TEntityBase* entity )
	{
	}

	bool needRemove( TEntityBase* entity )
	{
		return OnTimer();
	}

	struct PropEffectInfo : public PropModifyInfo
	{
		bool isF;
	};

	static int const MaxModifyNum = 10;
	PropEffectInfo data[ MaxModifyNum ];
	int num;


};


class CycleDamgeEffect : public TEffectBase
	                   , public EfCycle 
				       , public EfTimer 
{
public:
	CycleDamgeEffect( DamageInfo* damage , ActorState state , int totalCount , float ctime );

	~CycleDamgeEffect();
	void update( TEntityBase* entity );
	void OnStart( TEntityBase* entity );

	void OnEnd( TEntityBase* entity );
	bool needRemove( TEntityBase* entity );

protected:
	ActorState   m_state;
	DamageInfo*  m_damage;

};



#endif // TEffect_h__