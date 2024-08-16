#include "Matrix3.h"

#include "Quaternion.h"

namespace Math
{
	static Matrix3 const gIdentity( 
		1 , 0 , 0 ,
		0 , 1 , 0 ,
		0 , 0 , 1  );

	static Matrix3 const gZero( 
		0 , 0 , 0 ,
		0 , 0 , 0 ,
		0 , 0 , 0 );

	Matrix3 const& Matrix3::Identity(){ return gIdentity; }
	Matrix3 const& Matrix3::Zero(){ return gZero; }

	void Matrix3::transpose()
	{
		using namespace std;
#define SWAP_ROW_COL(i , j) swap(mM[i][j] , mM[j][i])

		SWAP_ROW_COL(0, 1);
		SWAP_ROW_COL(0, 2);
		SWAP_ROW_COL(1, 2);

#undef SWAP_ROW_COL
	}

	bool Matrix3::inverse(Matrix3& m, float& det) const
	{
		float m00 = mM[0][0], m01 = mM[0][1], m02 = mM[0][2];
		float m10 = mM[1][0], m11 = mM[1][1], m12 = mM[1][2];
		float m20 = mM[2][0], m21 = mM[2][1], m22 = mM[2][2];

		float t00 = + ( m11 * m22 - m12 * m21 );
		float t10 = - ( m10 * m22 - m12 * m20 );
		float t20 = + ( m10 * m21 - m11 * m20 );

		det = t00 * m00 + t10 * m01 + t20 * m02;

		if ( det == 0 )
			return false;

		float invDet = 1.0f / det;

		float d00 = t00 * invDet;
		float d10 = t10 * invDet;
		float d20 = t20 * invDet;

		float d01 = - ( m01 * m22 - m21 * m11 ) * invDet;
		float d11 = + ( m00 * m22 - m02 * m20 ) * invDet;
		float d21 = - ( m00 * m21 - m01 * m20 ) * invDet;

		float d02 = + ( m01 * m12 - m02 * m11 ) * invDet;
		float d12 = - ( m00 * m12 - m02 * m10 ) * invDet;
		float d22 = + ( m00 * m11 - m01 * m10 ) * invDet;

		m.setValue(
			d00 , d01 , d02 ,
			d10 , d11 , d12 ,
			d20 , d21 , d22 );

		return false;
	}



}//namespace CFly

