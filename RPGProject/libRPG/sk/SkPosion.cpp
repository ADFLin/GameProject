#include "common.h"

#include "TSkill.h"
#include "TActor.h"
#include "TEntityManager.h"
#include "TPhySystem.h"
#include "UtilityGlobal.h"
#include "TCombatSystem.h"
#include "TEffect.h"

#include "TFxPlayer.h"

#define FX_POSION "healing"

class SkPosion : public TSkill
{
public:
	SkPosion();
	void OnStart( TPhyEntity* select);
	SkillInfo const& querySkillInfo( int level );
};

LINK_SKILL_LIBRARY( SkPosion , "sk_posion" )


SkillInfo const& SkPosion::querySkillInfo( int level )
{
	static SkillInfo info[] =
	{//  lv   buf   castMP   cd    range    val   flag
		{ 1 ,   3  ,   10 ,  10 ,   1000 ,   20 , SF_SELECT_EMPTY ,  } ,
	};

	return info[ TMin( level - 1 , int( sizeof(info) / sizeof( SkillInfo) ) ) ];
}

SkPosion::SkPosion()
{
	fullName = TSTR("¬r³N");

}

void SkPosion::OnStart( TPhyEntity *select )
{
	TActor* actor = TActor::upCast( select );
	if ( actor )
	{
		DamageInfo* damage = DamageInfo::create();
		
		damage->attacker = caster;
		damage->defender = actor;
		damage->typeBit  = DT_MAGIC;
		damage->val      = getInfo().val;
		
		CycleDamgeEffect* effect = new CycleDamgeEffect( damage , AS_POISON , 30 , 2.0f );
		UG_AddEffect( select , effect );

		TFxData* healingFx = TFxPlayer::instance().play( FX_POSION );

		FnObject obj = healingFx->getBaseObj();
		obj.SetParent( actor->getFlyActor().GetBaseObject() );
		obj.Translate( 0 ,0 , 50 ,REPLACE );
	}
}

