#ifndef Matrix3_h__DDE84802_25A4_44E2_8D23_9D8734197C82
#define Matrix3_h__DDE84802_25A4_44E2_8D23_9D8734197C82

#include "Math/Base.h"
#include "Math/Vector3.h"
#include "Math/MatrixUtility.h"

namespace Math
{
	class Quaternion;

	class Matrix3
	{
	public:
		Matrix3() = default;

		FORCEINLINE
		Matrix3( float const values[] )
		{
			for( int i = 0 ; i < 9 ; ++i )
				mValues[i] = values[i];
		}
		FORCEINLINE
		Matrix3( float a0 , float a1 , float a2 , 
			     float a3 , float a4 , float a5 ,
				 float a6 , float a7 , float a8 )
		{
			setValue( a0 , a1 , a2 , a3 ,a4 , a5 , a6 , a7 , a8 );
		}

		FORCEINLINE
		Matrix3(Vector3 const& axisX, Vector3 const& axisY, Vector3 const& axisZ)
		{
			setBasis(axisX, axisY, axisZ);
		}

		FORCEINLINE
		void setBasis(Vector3 const& axisX, Vector3 const& axisY, Vector3 const& axisZ)
		{
			mValues[0] = axisX.x; mValues[1] = axisX.y; mValues[2] = axisX.z;
			mValues[3] = axisY.x; mValues[4] = axisY.y; mValues[5] = axisY.z;
			mValues[6] = axisZ.x; mValues[7] = axisZ.y; mValues[8] = axisZ.z;
		}

		FORCEINLINE
		void setValue( float a0 , float a1 , float a2 , 
			           float a3 , float a4 , float a5 ,
			           float a6 , float a7 , float a8 )
		{
			mValues[0] = a0; mValues[1] = a1; mValues[2] = a2; 
			mValues[3] = a3; mValues[4] = a4; mValues[5] = a5; 
			mValues[6] = a6; mValues[7] = a7; mValues[8] = a8;
		}


		static Matrix3 const& Identity();
		static Matrix3 const& Zero();

		FORCEINLINE static Matrix3 Rotate(Vector3 const& axis, float angle)
		{
			Matrix3 m; m.setRotation(axis, angle); return m;
		}

		operator       float* ()       { return mValues; }
		operator const float* () const { return mValues; }

		void setScale( Vector3 const& factor )
		{
			setValue( factor.x , 0, 0,
				      0, factor.y , 0,
			          0, 0, factor.z );
		}

		void setRotation( Vector3 const& axis , float angle ){ MatrixUtility::setRotation( *this , axis , angle ); }
		void setRotationX( float angle ){  MatrixUtility::setRotationX( *this , angle );  }
		void setRotationY( float angle ){  MatrixUtility::setRotationY( *this , angle );  }
		void setRotationZ( float angle ){  MatrixUtility::setRotationZ( *this , angle );  }

		void setQuaternion( Quaternion const& q ){ MatrixUtility::modifyOrientation( *this , q ); }

		Quaternion toQuatNoScale() const
		{
			return Quaternion(*this);
		}

		void setIdentity(){  setValue( 1, 0, 0,  0, 1, 0,  0, 0, 1);  }

		Vector3 leftMul( Vector3 const& v ) const
		{
#define MAT_MUL( m , index )\
	( v.x * m[ index ] + v.y * m[ index + 3 ] + v.z * m[ index + 6 ] )
			return Vector3( MAT_MUL( mValues , 0 ) , MAT_MUL( mValues , 1 ) , MAT_MUL( mValues , 2 ) );
#undef MAT_MUL
		}

		Vector3 mul( Vector3 const& v ) const
		{
#define MAT_MUL( m , index )\
	( v.x * m[ 3 * index ] + v.y * m[ 3 * index + 1 ] + v.z * m[ 3 * index + 2 ] )
			return Vector3( MAT_MUL( mValues , 0 ) , MAT_MUL( mValues , 1 ) , MAT_MUL( mValues , 2 ) );
#undef MAT_MUL
		}

		float      deter() const
		{
			return mValues[0] * ( mM[1][1] * mM[2][2] - mM[1][2] * mM[2][1] )
				-  mValues[1] * ( mM[1][0] * mM[2][2] - mM[2][0] * mM[1][2] )
				+  mValues[2] * ( mM[1][0] * mM[2][1] - mM[2][0] * mM[1][2] );
		}

		bool  inverse( Matrix3& m , float& det ) const;

		Matrix3  operator * ( Matrix3 const& rhs ) const;

		float& operator()( int idx )       { return mValues[idx]; }
		float  operator()( int idx ) const { return mValues[idx]; }

		float& operator()( int i , int j )       { return mM[ i ][ j ]; }
		float  operator()( int i , int j ) const { return mM[ i ][ j ]; } 

	private:
		union
		{
			float mValues[9];
			float mM[3][3];
		};
	};

	FORCEINLINE Matrix3 Matrix3::operator * ( Matrix3 const& rhs ) const
	{
#define MAT_MUL( v1 , v2 , idx1 , idx2 )\
	( v1[3*idx1]*v2[idx2] + v1[3*idx1+1]*v2[idx2+3] + v1[3*idx1+2]*v2[idx2+6] )

		return Matrix3(
			MAT_MUL( mValues , rhs.mValues , 0 , 0 ) ,
			MAT_MUL( mValues , rhs.mValues , 0 , 1 ) ,
			MAT_MUL( mValues , rhs.mValues , 0 , 2 ) ,

			MAT_MUL( mValues , rhs.mValues , 1 , 0 ) ,
			MAT_MUL( mValues , rhs.mValues , 1 , 1 ) ,
			MAT_MUL( mValues , rhs.mValues , 1 , 2 ) ,

			MAT_MUL( mValues , rhs.mValues , 2 , 0 ) ,
			MAT_MUL( mValues , rhs.mValues , 2 , 1 ) ,
			MAT_MUL( mValues , rhs.mValues , 2 , 2 ) );
#undef MAT_MUL

	}

	FORCEINLINE Vector3 TransformVector( Vector3 const& v  , Matrix3 const& m )
	{
		return MatrixUtility::Rotate( v , m );
	}


	FORCEINLINE Vector3 TransformVectorInverse( Vector3 const& v , Matrix3 const& m )
	{
		return MatrixUtility::RotateInverse( v , m );
	}

}//namespace Math


#endif // Matrix3_h__DDE84802_25A4_44E2_8D23_9D8734197C82
