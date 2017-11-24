#pragma once
#ifndef Transform_H_C440317D_2374_4961_8391_CEF50341EF73
#define Transform_H_C440317D_2374_4961_8391_CEF50341EF73

#include "Math/Vector3.h"
#include "Math/Quaternion.h"
#include "Math/Matrix4.h"
#include "Math/Matrix3.h"

namespace Math
{
	class Transform
	{
	public:
		Transform(){}
		Transform(EForceInit) :location(0,0,0), rotation(ForceInit), scale(1,1,1) {}
		Transform( Vector3 const& inT , Quaternion const& inR = Quaternion::Identity() , Vector3 const& inS = Vector3(1,1,1) )
			:location( inT ) , rotation( inR ) , scale( inS ){ }

		Vector3    location;
		Quaternion rotation;

		//NOTE: scale always effect in local first
		Vector3    scale;

		static Transform Scale(float s) { return Transform(Vector3(0, 0, 0), Quaternion::Identity(), Vector3(s,s,s)); }

		Transform operator * (Transform const& rhs) const
		{
			// P M(A) = Q(A)S(A)*P * Q-(A) + T(A)
			// P M(A)M(B) = Q(B)[S(B)*(Q(A)[S(A)*P]Q-(A)+T(A)]Q-(B) + T(B) 
			//             = Q(B)[S(B)*(Q(A)[S(A)*P]Q-(A))]Q-(B) + Q(B)(S(B)*T(A))Q-(B) + T(B)
			//             = [Q(B)Q(A)][S(B)S(A)*P][Q(B)Q(A)]-1 + Q(B)(S(B)*T(A))Q-(B) + T(B)
			Transform result;
			result.scale = scale * rhs.scale;
			result.rotation = rhs.rotation * rotation;
			result.location = rhs.rotation.rotate(rhs.scale * location) + rhs.location;

			return result;
		}

		Vector3   transformPosition( Vector3 const& v ) const
		{
			return location + rotation.rotate(scale * v);
		}

		Vector3   transformVector(Vector3 const& v) const
		{
			return rotation.rotate(scale * v);
		}

		Vector3   transformVectorNoScale(Vector3 const& v) const
		{
			return rotation.rotate(v);
		}

		Vector3   transformPositionInverse(Vector3 const& v) const
		{
			return rotation.rotateInverse( v - location ) / scale;
		}

		Vector3   transformVectorInverse(Vector3 const& v) const
		{
			return rotation.rotateInverse( v ) / scale;
		}

		Vector3   transformVectorInverseNoScale(Vector3 const& v) const
		{
			return rotation.rotateInverse(v);
		}

		Transform inverse() const;

		// this * rel = rhs
		Transform getRelTransform(Transform const& rhs) const
		{
			//#TODO : improve
			return inverse() * rhs;
		}

		// rel * this = rhs
		Transform getLocalRelTransform(Transform const& rhs) const
		{
			//#TODO : improve
			return rhs * inverse();
		}

		Matrix4 toMatrix();
		Matrix4 toMatrixNoScale();
	};

}//namespace Math

#endif // Transform_H_C440317D_2374_4961_8391_CEF50341EF73