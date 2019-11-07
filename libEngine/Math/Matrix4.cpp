#include "Matrix4.h"

namespace Math
{

	static Matrix4 const gIdentity( 
		1 , 0 , 0 , 0 ,
		0 , 1 , 0 , 0 ,
		0 , 0 , 1 , 0 ,
		0 , 0 , 0 , 1 );

	static Matrix4 const gZero( 
		0 , 0 , 0 , 0 ,
		0 , 0 , 0 , 0 ,
		0 , 0 , 0 , 0 ,
		0 , 0 , 0 , 0 );

	Matrix4 const& Matrix4::Identity(){ return gIdentity; }
	Matrix4 const& Matrix4::Zero(){ return gZero; }

	void Matrix4::setTransform( Vector3 const& pos  , Quaternion const& q )
	{
		modifyOrientation( q );
		modifyTranslation( pos );
		mValues[3] = mValues[7] = mValues[11]= 0;
		mValues[15] = 1.0f;
	}

	void Matrix4::modifyOrientation( Quaternion const& q )
	{
		MatrixUtility::modifyOrientation( *this , q );
	}

	void Matrix4::setQuaternion( Quaternion const& q )
	{
		setTransform( Vector3::Zero() , q );
	}

	bool  Matrix4::inverseAffine( Matrix4& m , float& det ) const
	{
		assert( isAffine() );

		float m00 = mM[0][0], m01 = mM[0][1], m02 = mM[0][2];
		float m10 = mM[1][0], m11 = mM[1][1], m12 = mM[1][2];
		float m20 = mM[2][0], m21 = mM[2][1], m22 = mM[2][2];
		float m30 = mM[3][0], m31 = mM[3][1], m32 = mM[3][2];

		float v0 = m20 * m31 - m21 * m30;
		float v1 = m20 * m32 - m22 * m30;
		float v2 = m21 * m32 - m22 * m31;

		float t00 = + ( m22 * m11 - m21 * m12 );
		float t10 = - ( m22 * m10 - m20 * m12 );
		float t20 = + ( m21 * m10 - m20 * m11 );
		float t30 = - ( v2 * m10 - v1 * m11 + v0 * m12 );

		det = ( t00 * m00 + t10 * m01 + t20 * m02 );

		if ( det == 0 )
			return false;

		float invDet = 1.0f / det;

		float d00 = t00 * invDet;
		float d10 = t10 * invDet;
		float d20 = t20 * invDet;
		float d30 = t30 * invDet;

		float d01 = - ( m22 * m01 - m21 * m02 ) * invDet;
		float d11 = + ( m22 * m00 - m20 * m02 ) * invDet;
		float d21 = - ( m21 * m00 - m20 * m01 ) * invDet;
		float d31 = + ( v2 * m00 - v1 * m01 + v0 * m02 ) * invDet;

		v0 = m10 * m31 - m11 * m30;
		v1 = m10 * m32 - m12 * m30;
		v2 = m11 * m32 - m12 * m31;

		float d02 = + ( m12 * m01 - m11 * m02 ) * invDet;
		float d12 = - ( m12 * m00 - m10 * m02 ) * invDet;
		float d22 = + ( m11 * m00 - m10 * m01 ) * invDet;
		float d32 = - ( v2 * m00 - v1 * m01 + v0 * m02 ) * invDet;

		m.setValue(
			d00, d01, d02,
			d10, d11, d12,
			d20, d21, d22,
			d30, d31, d32 );

		return true;
	}

	bool  Matrix4::inverse( Matrix4& m , float& det ) const
	{
		//inv(M) = cofactor / det(M)
		float m00 = mM[0][0], m01 = mM[0][1], m02 = mM[0][2], m03 = mM[0][3];
		float m10 = mM[1][0], m11 = mM[1][1], m12 = mM[1][2], m13 = mM[1][3];
		float m20 = mM[2][0], m21 = mM[2][1], m22 = mM[2][2], m23 = mM[2][3];
		float m30 = mM[3][0], m31 = mM[3][1], m32 = mM[3][2], m33 = mM[3][3];

		float v0 = m20 * m31 - m21 * m30;
		float v1 = m20 * m32 - m22 * m30;
		float v2 = m20 * m33 - m23 * m30;
		float v3 = m21 * m32 - m22 * m31;
		float v4 = m21 * m33 - m23 * m31;
		float v5 = m22 * m33 - m23 * m32;

		float t00 = + (v5 * m11 - v4 * m12 + v3 * m13);
		float t10 = - (v5 * m10 - v2 * m12 + v1 * m13);
		float t20 = + (v4 * m10 - v2 * m11 + v0 * m13);
		float t30 = - (v3 * m10 - v1 * m11 + v0 * m12);

		det = ( t00 * m00 + t10 * m01 + t20 * m02 + t30 * m03 );

		if ( det == 0 )
			return false;

		float invDet = 1.0f / det;

		float d00 = t00 * invDet;
		float d10 = t10 * invDet;
		float d20 = t20 * invDet;
		float d30 = t30 * invDet;

		float d01 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d11 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d21 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d31 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m10 * m31 - m11 * m30;
		v1 = m10 * m32 - m12 * m30;
		v2 = m10 * m33 - m13 * m30;
		v3 = m11 * m32 - m12 * m31;
		v4 = m11 * m33 - m13 * m31;
		v5 = m12 * m33 - m13 * m32;

		float d02 = + (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d12 = - (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d22 = + (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d32 = - (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		v0 = m21 * m10 - m20 * m11;
		v1 = m22 * m10 - m20 * m12;
		v2 = m23 * m10 - m20 * m13;
		v3 = m22 * m11 - m21 * m12;
		v4 = m23 * m11 - m21 * m13;
		v5 = m23 * m12 - m22 * m13;

		float d03 = - (v5 * m01 - v4 * m02 + v3 * m03) * invDet;
		float d13 = + (v5 * m00 - v2 * m02 + v1 * m03) * invDet;
		float d23 = - (v4 * m00 - v2 * m01 + v0 * m03) * invDet;
		float d33 = + (v3 * m00 - v1 * m01 + v0 * m02) * invDet;

		m.setValue(
			d00, d01, d02, d03,
			d10, d11, d12, d13,
			d20, d21, d22, d23,
			d30, d31, d32, d33 );

		return true;
	}

	Matrix4 Matrix4::leftMul( Matrix3 const& m ) const
	{
		//FIXME
		assert(0);
		return Matrix4(

			);
	}

	Matrix4 Matrix4::rightMul( Matrix3 const& m ) const
	{
		//FIXME
		assert(0);
		return Matrix4(


			);
	}

	bool Matrix4::isAffine() const
	{
		return mValues[3] == 0 && mValues[7] == 0 && mValues[11] == 0 && mValues[15] == 1.0;
	}

}