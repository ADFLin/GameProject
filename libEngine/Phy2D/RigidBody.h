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

		Vector2 getVelFromLocalPos( Vector2 const& posLocal ) const
		{
			return mLinearVel + mXForm.transformVector( Vector2::Cross( mAngularVel , ( posLocal - mPosCenterLocal ) ) );
		}
		Vector2 getVelFromWorldPos( Vector2 const& pos ) const
		{
			return mLinearVel + Vector2::Cross( mAngularVel , ( pos - mPosCenter ) );
		}

		void setupDefaultMass();
		void saveState()
		{
			mSaveState.xform = mXForm;
			mSaveState.linearVel = mLinearVel;
			mSaveState.angularVel = mAngularVel;

			mRotationAngle = mXForm.getRotation().getAngle();
		}

		void addImpulse( Vector2 const& pos , Vector2 const& impulse )
		{
			mLinearImpulse += impulse;
			mAngularImpulse += ( pos - mPosCenter ).cross( impulse );
		}

		void addLinearImpulse( Vector2 const& impulse )
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
			mLinearImpulse = Vector2::Zero();
			mAngularImpulse = 0;
		}

		struct State
		{
			XForm xform;
			Vector2 linearVel;
			float angularVel;
		};

		Vector2  mPosCenter;
		float  mRotationAngle;

		Vector2  mPosCenterLocal;

		State  mSaveState;

		Vector2  mLinearImpulse;
		float  mAngularImpulse;

		float  mLinearDamping;
		float  mAngularDamping;


		BodyMotion::Type mMotionType;
		Vector2  mLinearVel;
		float  mAngularVel;
		float  mDensity;
		float  mMass , mInvMass;
		float  mI , mInvI;


	};

}//namespace Phy2D

#endif // RigidBody_h__A43993CD_7DDA_413E_A692_9AD6619A297D
