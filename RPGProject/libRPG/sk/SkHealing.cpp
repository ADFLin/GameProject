#include "common.h"

#include "TSkill.h"
#include "TActor.h"
#include "TResManager.h"
#include "TEntityManager.h"
#include "TPhySystem.h"
#include "TBullet.h"
#include "UtilityGlobal.h"
#include "TFxPlayer.h"


#define FX_Healing  "healing"

class SkHealing : public TSkill
{
public:
	SkHealing();
	void OnStart( TPhyEntity* select);
	SkillInfo const& querySkillInfo( int level );
};

LINK_SKILL_LIBRARY( SkHealing , "sk_healing" )

SkillInfo const& SkHealing::querySkillInfo( int level )
{
	static SkillInfo info[] =
	{//  lv   buf   castMP   cd    range     val    flag
		{ 1 ,   3  ,   10 ,  10 ,   1000 ,   200  , SF_SELECT_FRIEND ,  } ,
		{ 2 ,   3  ,   35 ,  12 ,   1000 ,   400  , SF_SELECT_FRIEND ,  } ,
		{ 3 ,   3  ,   60 ,  12 ,   1000 ,  1000  , SF_SELECT_FRIEND ,  } ,
	};

	return info[ TMin( level - 1 , int( sizeof(info) / sizeof( SkillInfo) ) ) ];
}

SkHealing::SkHealing()
{
	fullName = TSTR("¦^´_³N");
}

void SkHealing::OnStart( TPhyEntity *select )
{
	TActor* actor = TActor::upCast( select );

	FnAudio audio;
	if ( actor )
	{
		actor->setHP( actor->getHP() + getInfo().val );
		UG_PlaySound3D( actor->getFlyActor().GetBaseObject() , "healing" , 1 );

		TFxData* healingFx = TFxPlayer::instance().play( FX_Healing );

		FnObject obj = healingFx->getBaseObj();
		obj.SetParent( actor->getFlyActor().GetBaseObject() );
		obj.Translate( 0 ,0 , 50 ,REPLACE );

	}
}


