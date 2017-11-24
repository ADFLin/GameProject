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
		Matrix3(){}
		Matrix3( float* val )
		{
			for( int i = 0 ; i < 9 ; ++i )
				m_val[i] = val[i];
		}
		Matrix3( float a0 , float a1 , float a2 , 
			       float a3 , float a4 , float a5 ,
				   float a6 , float a7 , float a8 )
		{
			setValue( a0 , a1 , a2 , a3 ,a4 , a5 , a6 , a7 , a8 );
		}

		Matrix3(Vector3 const& axisX, Vector3 const& axisY, Vector3 const& axisZ)
		{
			setBasis(axisX, axisY, axisZ);
		}

		void setBasis(Vector3 const& axisX, Vector3 const& axisY, Vector3 const& axisZ)
		{
			m_val[0] = axisX.x; m_val[1] = axisX.y; m_val[2] = axisX.z;
			m_val[3] = axisY.x; m_val[4] = axisY.y; m_val[5] = axisY.z;
			m_val[6] = axisZ.x; m_val[7] = axisZ.y; m_val[8] = axisZ.z;
		}


		void setValue( float a0 , float a1 , float a2 , 
			           float a3 , float a4 , float a5 ,
			           float a6 , float a7 , float a8 )
		{
			m_val[0] = a0; m_val[1] = a1; m_val[2] = a2; 
			m_val[3] = a3; m_val[4] = a4; m_val[5] = a5; 
			m_val[6] = a6; m_val[7] = a7; m_val[8] = a8;
		}


		static Matrix3 const& Identity();
		static Matrix3 const& Zero();

		operator       float* ()       { return m_val; }
		operator const float* () const { return m_val; }

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
			return Vector3( MAT_MUL( m_val , 0 ) , MAT_MUL( m_val , 1 ) , MAT_MUL( m_val , 2 ) );
#undef MAT_MUL
		}

		Vector3 rightMul( Vector3 const& v ) const
		{
#define MAT_MUL( m , index )\
	( v.x * m[ 3 * index ] + v.y * m[ 3 * index + 1 ] + v.z * m[ 3 * index + 2 ] )
			return Vector3( MAT_MUL( m_val , 0 ) , MAT_MUL( m_val , 1 ) , MAT_MUL( m_val , 2 ) );
#undef MAT_MUL
		}

		float      deter() const
		{
			return m_val[0] * ( m_m[1][1] * m_m[2][2] - m_m[1][2] * m_m[2][1] )
				-  m_val[1] * ( m_m[1][0] * m_m[2][2] - m_m[2][0] * m_m[1][2] )
				+  m_val[2] * ( m_m[1][0] * m_m[2][1] - m_m[2][0] * m_m[1][2] );
		}

		bool  inverse( Matrix3& m , float& det ) const;

		Matrix3  operator * ( Matrix3 const& rhs ) const;

		float& operator()( int idx )       { return m_val[idx]; }
		float  operator()( int idx ) const { return m_val[idx]; }

		float& operator()( int i , int j )       { return m_m[ i ][ j ]; }
		float  operator()( int i , int j ) const { return m_m[ i ][ j ]; } 

	private:
		union
		{
			float m_val[9];
			float m_m[3][3];
		};
	};

	inline Matrix3 Matrix3::operator * ( Matrix3 const& rhs ) const
	{
#define MAT_MUL( v1 , v2 , idx1 , idx2 )\
	( v1[3*idx1]*v2[idx2] + v1[3*idx1+1]*v2[idx2+3] + v1[3*idx1+2]*v2[idx2+6] )

		return Matrix3(
			MAT_MUL( m_val , rhs.m_val , 0 , 0 ) ,
			MAT_MUL( m_val , rhs.m_val , 0 , 1 ) ,
			MAT_MUL( m_val , rhs.m_val , 0 , 2 ) ,

			MAT_MUL( m_val , rhs.m_val , 1 , 0 ) ,
			MAT_MUL( m_val , rhs.m_val , 1 , 1 ) ,
			MAT_MUL( m_val , rhs.m_val , 1 , 2 ) ,

			MAT_MUL( m_val , rhs.m_val , 2 , 0 ) ,
			MAT_MUL( m_val , rhs.m_val , 2 , 1 ) ,
			MAT_MUL( m_val , rhs.m_val , 2 , 2 ) );
#undef MAT_MUL

	}

	inline Vector3 TransformVector( Vector3 const& v  , Matrix3 const& m )
	{
		return MatrixUtility::rotate( v , m );
	}


	inline Vector3 TransformVectorInverse( Vector3 const& v , Matrix3 const& m )
	{
		return MatrixUtility::rotateInverse( v , m );
	}

}//namespace Math


#endif // Matrix3_h__DDE84802_25A4_44E2_8D23_9D8734197C82
