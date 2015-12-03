#include "common.h"


struct MoveResult;
class NotMeConvexCallback : public ClosestConvexResultCallback
{
public:
	NotMeConvexCallback(btCollisionObject* me) 
		: ClosestConvexResultCallback(Vec3D(0.0, 0.0, 0.0), Vec3D(0.0, 0.0, 0.0))
		,m_me(me)
	{
		m_me = me;
	}
	virtual btScalar addSingleResult(btCollisionWorld::LocalConvexResult& convexResult,bool normalInWorldSpace)
	{
		if ( convexResult.m_hitCollisionObject == m_me )
			return 1.0;
		return ClosestConvexResultCallback::addSingleResult (convexResult, normalInWorldSpace);
	}
protected:
	btCollisionObject* m_me;
};

class TMoveProbe
{
	TEntityBase* checkStep( Vec3D const& startPos , Vec3D const& startPos , unsigned colMask );
	{

	}

	TPhyEntity*  m_phyEntity;
};


