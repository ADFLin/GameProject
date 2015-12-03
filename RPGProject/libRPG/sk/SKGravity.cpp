#include "common.h"

#include "TSkill.h"
#include "TActor.h"
#include "TResManager.h"
#include "TEntityManager.h"
#include "TPhySystem.h"
#include "TCombatSystem.h"
#include "TObject.h"
#include "ConsoleSystem.h"

DEFINE_CON_VAR( float , sk_gravity_impulse , 1000 )

class SkGravity : public TSkill
{
public:
	SkGravity();
	void OnStart( TPhyEntity* select);
	bool sustainCast();

	void OnCancel()
	{
		freeObj();
	}
	void freeObj();
	SkillInfo const& querySkillInfo( int level );

protected:
	bool     catchObj;
	TObject* obj;
	Vec3D    putPos;
};


LINK_SKILL_LIBRARY( SkGravity , "sk_gravity" )

SkillInfo const& SkGravity::querySkillInfo( int level )
{
	static SkillInfo info[] =
	{//  lv   buf   castMP   cd    range   val     flag
		{ 1 ,   0  ,   10 ,   5 ,   1000 ,   1   , SF_SELECT_TOBJECT ,  } ,
	};

	return info[ TMin( level - 1 , int( sizeof(info) / sizeof( SkillInfo) ) ) ];
}

SkGravity::SkGravity()
{

	obj = NULL;
	catchObj = false;
}

void SkGravity::OnStart( TPhyEntity* select )
{
	obj = TObject::upCast( select );
	assert( obj );

	PhyRigidBody* body = obj->getPhyBody();
	body->setGravity( Vec3D(0,0,0) );

	caster->addStateBit( AS_CATCH_OBJ );
	
}

bool SkGravity::sustainCast()
{
	PhyRigidBody* body = obj->getPhyBody();

	if ( caster->getStateBit() & AS_CATCH_OBJ )
	{
		putPos = caster->getPosition() + Vec3D( 0,0, 2 * caster->getBodyHalfHeigh()  + 5 );
		

		Vec3D const& pos  = obj->getPosition();
		Vec3D dir = putPos - pos;
		float len2 = dir.length2();

		body->activate(true);
		if ( len2 > 50 * 50  )
		{
			Vec3D impulse = ( sk_gravity_impulse / len2 ) * dir;
			body->applyCentralImpulse( impulse );
			catchObj = false;
		}
		else
		{
			body->setLinearVelocity( 0.95f * body->getLinearVelocity() );
			catchObj = true;
		}

	}
	else
	{
		freeObj();
		body->applyCentralImpulse( 1000 *caster->getFaceDir() );
		return false;
	}

	return true;
}

void SkGravity::freeObj()
{
	if ( !obj )
		return;

	PhyRigidBody* body = obj->getPhyBody();

	body->setGravity( Vec3D(0,0,-g_GlobalVal.gravity) );
	body->setAngularFactor( 1 );
	body->activate( true );
}