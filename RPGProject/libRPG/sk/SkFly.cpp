#include "common.h"

#include "TSkill.h"
#include "TActor.h"
#include "TResManager.h"
#include "TEntityManager.h"
#include "TPhySystem.h"
#include "TBullet.h"
#include "TEffect.h"
#include "TMovement.h"
#include "UtilityGlobal.h"

class SkFly : public TSkill
{
public:
	SkFly();
	void OnStart( TPhyEntity* select);
	SkillInfo const& querySkillInfo( int level );
};

LINK_SKILL_LIBRARY( SkFly , "sk_fly" )

class FlyEffect : public TEffectBase
				, public EfTimer 
{
public:
	FlyEffect( float time )
		:EfTimer( time )
	{

	}

	virtual void OnStart(TEntityBase* entity)
	{
		TActor* actor = TActor::upCast( entity );
		if ( actor )
		{
			actor->setMovementType( MT_FLY );
		}

	}
	virtual void OnEnd(TEntityBase* entity)
	{
		TActor* actor = TActor::upCast( entity );
		if ( actor )
		{
			actor->setMovementType( MT_NORMAL_MOVE );
		}
	}

	bool needRemove( TEntityBase* entity ){	return OnTimer();  }
};

SkillInfo const& SkFly::querySkillInfo( int level )
{
	static SkillInfo info[] =
	{//  lv   buf   castMP   cd    range     val    flag
		{ 1 ,   3  ,   10 ,  10 ,   1000 ,   100  , SF_NO_SELECT ,  } ,
		{ 2 ,   3  ,   35 ,  12 ,   1000 ,   400  , SF_NO_SELECT ,  } ,
		{ 3 ,   3  ,   60 ,  12 ,   1000 ,  1000  , SF_NO_SELECT ,  } ,
	};

	return info[ TMin( level - 1 , int( sizeof(info) / sizeof( SkillInfo) ) ) ];
}

SkFly::SkFly()
{

}

void SkFly::OnStart( TPhyEntity *select )
{
	UG_AddEffect( select , new FlyEffect( getInfo().val) );
}

