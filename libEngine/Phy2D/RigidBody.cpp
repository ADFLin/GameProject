#include "RigidBody.h"

namespace Phy2D
{
	void RigidBody::setMotionType( BodyMotion::Type type)
	{
		if ( mMotionType == type )
			return;

		mMotionType = type;
		switch( mMotionType )
		{
		case BodyMotion::eStatic:
		case BodyMotion::eKinematic:
			mI = mInvI = 0;
			mMass = mInvMass = 0;
			break;
		case BodyMotion::eDynamic:
			setupDefaultMass();
			break;
		}
	}

	void RigidBody::init(Shape* shape , BodyInfo const& info)
	{
		mShape = shape;
		mDensity = info.density;
		setMotionType( info.motionType );
		mLinearDamping = info.linearDamping;
		mAngularDamping = info.angularDamping;
	}

	void RigidBody::setupDefaultMass()
	{
		MassInfo info;
		mShape->calcMass( info );

		mPosCenterLocal = info.center;
		mPosCenter = mXForm.mul( mPosCenterLocal );
		if ( mDensity == 0 )
		{
			mMass = mInvMass = 0; 
			mI    = mInvI = 0;
		}
		else
		{
			mMass = mDensity * info.m;
			mInvMass = 1 / info.m; 
			mI    = mDensity * info.I;
			mInvI = 1 / info.I;
		}
	}

}//namespace Phy2D