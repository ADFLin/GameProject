#include "common.h"

#include "TSkill.h"
#include "TActor.h"
#include "TResManager.h"
#include "TEntityManager.h"
#include "TEventSystem.h"
#include "EventType.h"
#include "UtilityGlobal.h"
#include "TPhySystem.h"
#include "TBullet.h"
#include "TCombatSystem.h"
#include "TFxPlayer.h"
#include "TLevel.h"
#include "TAudioPlayer.h"


#define FX_TEST  "test"
#define FX_TEST2 "test2"

class SkFireBall : public TSkill
{
public:
	SkFireBall();
	void OnStart( TPhyEntity* select);
	SkillInfo const& querySkillInfo( int level );
};


LINK_SKILL_LIBRARY( SkFireBall , "sk_fireball" )


SkillInfo const& SkFireBall::querySkillInfo( int level )
{
	static SkillInfo info[] =
	{//  lv    buf    castMP   cd    range     val    flag
		{ 1 ,   1.5  ,   10 ,  2 ,   1000 ,    50  , SF_SELECT_EMPTY ,  } ,
	};

	return info[ TMin( level - 1 , int( sizeof(info) / sizeof( SkillInfo) ) ) ];
}


SkFireBall::SkFireBall()
{
	fullName = TSTR("¤õ²y³N");

}

class TFireBall : public TBullet
{

public:
	TFireBall( Float r ,  XForm const& trans );

	~TFireBall()
	{


	}

	void updateFrame();
	void collisionThink();
	void OnCollision( TPhyEntity* colObj , Float depth , Vec3D const& selfPos , Vec3D const& colObjPos , Vec3D const& normalOnColObj );
	void OnCollisionTerrain();

	void bow();

	bool        isCollision;
	TFxData*    fireBallFx;
	DamageInfo* damage;
};



void SkFireBall::OnStart( TPhyEntity* select )
{

	TFireBall* bullet = new TFireBall(  20 , caster->getTransform() );
	caster->getLiveLevel()->addEntity( bullet );

	DamageInfo* damInfo = DamageInfo::create();
	damInfo->typeBit = DT_MAGIC | DT_FAR;
	damInfo->attacker = caster;
	damInfo->val = getInfo().val;

	bullet->damage = damInfo;

	Vec3D dir;
	if ( select )
	{
		dir = select->getPosition() - caster->getPosition();
		dir.normalize();
	}
	else
	{
		dir = caster->getFaceDir();
	}
	bullet->getPhyBody()->setLinearVelocity( dir * 1600 );
}


TFireBall::TFireBall( Float r , XForm const& trans ) 
	:TBullet( createPhyBodyInfo( 0 ,  new btSphereShape(r) ) )
	,isCollision(false)
{

	fireBallFx = TFxPlayer::instance().play( FX_TEST );
	fireBallFx->playForever();

	setMotionState( new TObjMotionState( trans , fireBallFx->getFx()->GetBaseObject() ) );
}

void TFireBall::updateFrame()
{
	if ( !isCollision )
	{
		XForm trans = getTransform();
		trans.getOrigin() += getPhyBody()->getLinearVelocity() * g_GlobalVal.frameTime;
		setTransform(trans);
	}
}

void TFireBall::OnCollision( TPhyEntity* colObj , Float depth , Vec3D const& selfPos , Vec3D const& colObjPos , Vec3D const& normalOnColObj )
{
	if ( colObj != damage->attacker && !isCollision )
	{
		//TActor* actor = TActor::upCast( colObj );
		//if ( actor )
		{
			isCollision = true;
			damage->defender = colObj;
			UG_InputDamage( *damage );
			bow();
		}
	}
}

void TFireBall::OnCollisionTerrain()
{
	if ( !isCollision )
	{
		isCollision = true;
		bow();
	}
}

void TFireBall::bow()
{
	TEvent event( EVT_FX_DELETE , EVENT_ANY_ID , NULL , fireBallFx );
	UG_SendEvent( event );

	TFxData* bowFx = TFxPlayer::instance().play( FX_TEST2 );
	FnObject obj = bowFx->getBaseObj();

	obj.SetWorldPosition( (Float*)&getPosition() );

	setEntityState( EF_DESTORY );

	UG_PlaySound3D( getPosition() , "explosion" , 2 , ONCE );
	TEntityManager::instance().applyExplosion( getPosition() , 300 , 500 );
}