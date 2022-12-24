#pragma once
#ifndef SimpleCamera_H_DDDC41C0_D6F7_4F4B_8A84_2B91D572DA73
#define SimpleCamera_H_DDDC41C0_D6F7_4F4B_8A84_2B91D572DA73

#include "RHI/RHIType.h"

namespace Render
{

	class SimpleCamera
	{
	public:
		SimpleCamera()
			:mPos(0, 0, 0)
			, mYaw(0)
			, mPitch(0)
			, mRoll(0)
		{
			updateInternal();
		}
		void setPos(Vector3 const& pos) { mPos = pos; }
		void lookAt(Vector3 const& pos, Vector3 const& posTarget, Vector3 const& upDir)
		{
			mPos = pos;
			setViewDir(posTarget - pos, upDir);
		}
		void setViewDir(Vector3 const& forwardDir, Vector3 const& upDir)
		{
			LookAtMatrix mat(forwardDir, upDir);
			mRotation.setMatrix(mat);
			mRotation = mRotation.inverse();
			Vector3 angle = mRotation.getEulerZYX();
			mYaw = angle.z;
			mRoll = angle.y;
			mPitch = angle.x;
		}
		void setRotation(float yaw, float pitch, float roll)
		{
			mYaw = yaw;
			mPitch = pitch;
			mRoll = roll;
			updateInternal();
		}

		Matrix4 getViewMatrix() const { return LookAtMatrix(mPos, getViewDir(), getUpDir()); }
		Matrix4 getTransform() const { return Matrix4(mPos, mRotation); }
		Vector3 const& getPos() const { return mPos; }

		static Vector3 LocalViewDir() { return Vector3(0, 0, 1); }
		static Vector3 LocalUpDir() { return Vector3(0, 1, 0); }

		Vector3 getViewDir() const { return mRotation.rotate(LocalViewDir()); }
		Vector3 getUpDir() const { return mRotation.rotate(LocalUpDir()); }
		Vector3 getRightDir() const { return mRotation.rotate(Vector3(1, 0, 0)); }

		void    moveRight(float dist)
		{
			mPos += mRotation.rotate(Vector3(dist, 0, 0));
		}
		void    moveFront(float dist) { mPos += dist * getViewDir(); }
		void    moveUp(float dist) { mPos += dist * getUpDir(); }


		Vector3 toWorld(Vector3 const& p) const { return mPos + mRotation.rotate(p); }
		Vector3 toLocal(Vector3 const& p) const { return mRotation.rotateInverse(p - mPos); }

		void    rotateByMouse(float dx, float dy)
		{
			mYaw += dx;
			mPitch += dy;

			//float const f = 0.00001;
			//if ( mPitch < f )
			//	mPitch = f;
			//else if ( mPitch > Deg2Rad(180) - f )
			//	mPitch = Deg2Rad(180) - f;
			updateInternal();
		}

		void updatePosition(float deltaTime)
		{
			Vector3 acc = moveSpeedScale * ( moveForwardImpulse * getViewDir() + moveRightImpulse * getRightDir() );
			
			if (bPhysicsMovement)
			{
				mMoveVelocity += acc * deltaTime;
				mMoveVelocity *= ( 1 - (Math::Clamp<float>(deltaTime * velocityDamping, 0, 0.75)) );
			}
			else
			{
				mMoveVelocity = acc * deltaTime;
			}
	
			mPos += mMoveVelocity * deltaTime;
		}


		float  velocityDamping = 10;
		float  moveSpeedScale = 1;
		bool   bPhysicsMovement = true;
		float  moveForwardImpulse = 0;
		float  moveRightImpulse = 0;

	private:
		void updateInternal()
		{
			mRotation.setEulerZYX(mYaw, mRoll, mPitch);
		}

		Vector3 mMoveVelocity = Vector3::Zero();


		Quaternion mRotation;
		Vector3 mPos;
		float   mYaw;
		float   mPitch;
		float   mRoll;
	};

}

#endif // SimpleCamera_H_DDDC41C0_D6F7_4F4B_8A84_2B91D572DA73
