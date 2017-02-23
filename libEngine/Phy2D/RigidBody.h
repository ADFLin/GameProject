#ifndef RigidBody_h__A43993CD_7DDA_413E_A692_9AD6619A297D
#define RigidBody_h__A43993CD_7DDA_413E_A692_9AD6619A297D

#include "Phy2D/Base.h"
#include "Phy2D/Collision.h"

namespace Phy2D
{
	struct BodyMotion
	{
		enum Type
		{
			eDynamic ,
			eStatic ,
			eKinematic ,
		};

	};
	struct BodyInfo
	{
		BodyInfo()
		{
			density = 1.0f;
			linearDamping = 0.0f;
			angularDamping = 0.0f;
			motionType = BodyMotion::eDynamic;
		}

		float density;
		float linearDamping;
		float angularDamping;
		BodyMotion::Type motionType;
	};


	class RigidBody : public CollideObject
	{
	public:
		RigidBody()
			:mLinearVel(0,0)
			,mAngularVel(0)
			,mLinearImpulse(0,0)
			,mAngularImpulse(0)
		{


		}

		void init( Shape* shape , BodyInfo const& info );

		virtual PhyType  getType() const { return PhyObject::eRigidType; }
		void             setMotionType( BodyMotion::Type type );
		BodyMotion::Type getMotionType() const { return mMotionType; }

	
		void intergedTramsform( float dt )
		{
			mXForm.translate( mLinearVel * dt );
			mRotationAngle += mAngularVel * dt;
			synTransform();
		}
		void synTransform()
		{
			mXForm.setRoatation( mRotationAngle );
			mPosCenter = mXForm.transformPosition( mPosCenterLocal );
		}

		Vec2f getVelFromLocalPos( Vec2f const& posLocal ) const
		{
			return mLinearVel + mXForm.transformVector( Vec2f::Cross( mAngularVel , ( posLocal - mPosCenterLocal ) ) );
		}
		Vec2f getVelFromWorldPos( Vec2f const& pos ) const
		{
			return mLinearVel + Vec2f::Cross( mAngularVel , ( pos - mPosCenter ) );
		}

		void setupDefaultMass();
		void saveState()
		{
			mSaveState.xform = mXForm;
			mSaveState.linearVel = mLinearVel;
			mSaveState.angularVel = mAngularVel;

			mRotationAngle = mXForm.getRotation().getAngle();
		}

		void addImpulse( Vec2f const& pos , Vec2f const& impulse )
		{
			mLinearImpulse += impulse;
			mAngularImpulse += ( pos - mPosCenter ).cross( impulse );
		}

		void addLinearImpulse( Vec2f const& impulse )
		{
			mLinearImpulse += impulse;
		}

		void addAngularImpulse( float impulse )
		{
			mAngularImpulse += impulse;
		}

		void applyImpulse()
		{
			if ( mMotionType != BodyMotion::eStatic )
			{
				mLinearVel += mInvMass * mLinearImpulse;
				mAngularVel += mInvI * mAngularVel;
			}
			mLinearImpulse = Vec2f::Zero();
			mAngularImpulse = 0;
		}

		struct State
		{
			XForm xform;
			Vec2f linearVel;
			float angularVel;
		};

		Vec2f  mPosCenter;
		float  mRotationAngle;

		Vec2f  mPosCenterLocal;

		State  mSaveState;

		Vec2f  mLinearImpulse;
		float  mAngularImpulse;

		float  mLinearDamping;
		float  mAngularDamping;


		BodyMotion::Type mMotionType;
		Vec2f  mLinearVel;
		float  mAngularVel;
		float  mDensity;
		float  mMass , mInvMass;
		float  mI , mInvI;


	};

}//namespace Phy2D

#endif // RigidBody_h__A43993CD_7DDA_413E_A692_9AD6619A297D
