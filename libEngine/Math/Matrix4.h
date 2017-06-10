#ifndef Matrix4_h__D6EC7C56_939F_4B27_81B6_42F5B02E8637
#define Matrix4_h__D6EC7C56_939F_4B27_81B6_42F5B02E8637

#include "Math/Base.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"
#include "Math/MatrixUtility.h"

namespace Math
{
	class Quaternion;
	class Matrix3;

	class Matrix4
	{
	public:
		Matrix4(){}
		Matrix4( float* val )
		{
			for( int i = 0 ; i < 16 ; ++i )
				m_val[i] = val[i];
		}

		Matrix4( float a0 , float a1 , float a2 , float a3 ,
			float a4 , float a5 , float a6 , float a7 ,
			float a8 , float a9 , float a10 , float a11 ,
			float a12 , float a13 , float a14 , float a15 )
		{
			setValue( a0 , a1 , a2 , a3 , a4 , a5 , a6 , a7 , a8 , a9 , a10 , a11 , a12 , a13 , a14 , a15 );
		}

		Matrix4( float a0 , float a1 , float a2 , 
			float a3 , float a4 , float a5 , 
			float a6 , float a7 , float a8 , 
			float a9 , float a10 , float a11 )
		{
			setValue( a0 , a1 , a2 ,a3 , a4 , a5 ,a6 , a7 , a8 ,a9 , a10 , a11 );
		}

		Matrix4( Vector3 const& pos , Quaternion const& q )
		{
			setTransform( pos , q );
		}

		void setValue( float a0 , float a1 , float a2 , 
			float a3 , float a4 , float a5 , 
			float a6 , float a7 , float a8  )
		{
			setValue( a0 , a1 , a2 , 0 , 
				a3 , a4 , a5 , 0 ,
				a6 , a7 , a8 , 0 ,
				0 , 0 , 0 , 1.0f );
		}

		void setValue( float a0 , float a1 , float a2 , float a3 ,
			float a4 , float a5 , float a6 , float a7 ,
			float a8 , float a9 , float a10 , float a11 ,
			float a12 , float a13 , float a14 , float a15 )
		{
			m_val[0] = a0; m_val[1] = a1; m_val[2] = a2; m_val[3] = a3;
			m_val[4] = a4; m_val[5] = a5; m_val[6] = a6; m_val[7] = a7;
			m_val[8] = a8; m_val[9] = a9; m_val[10] = a10;m_val[11] = a11;
			m_val[12] = a12; m_val[13] = a13; m_val[14] = a14; m_val[15] = a15;
		}

		void setValue( float a0 , float a1 , float a2 , 
			float a3 , float a4 , float a5 , 
			float a6 , float a7 , float a8 , 
			float a9 , float a10 , float a11 )
		{
			setValue( 
				a0 , a1 , a2 , 0 ,
				a3 , a4 , a5 , 0 ,
				a6 , a7 , a8 , 0 ,
				a9 , a10 , a11 , 1.0 );
		}

		static Matrix4 const& Identity();
		static Matrix4 const& Zero();

		static Matrix4 Translate( Vector3 const& pos )
		{
			Matrix4 m; m.setTranslation( pos ); return m;
		}
		static Matrix4 Translate(float x, float y, float z) 
		{ 
			return Translate(Vector3(x, y, z)); 
		}
		static Matrix4 Rotate( Quaternion const& q )
		{
			Matrix4 m; m.setQuaternion( q ); return m;
		}
		static Matrix4 Rotate( Vector3 const& axis , float angle )
		{
			Matrix4 m; m.setRotation( axis , angle ); return m;
		}
		static Matrix4 Scale( float scale )
		{
			Matrix4 m; m.setScale( Vector3( scale , scale , scale ) ); return m;
		}
		static Matrix4 Scale( Vector3 const& scale )
		{
			Matrix4 m; m.setScale( scale ); return m;
		}

		operator       float* ()       { return m_val; }
		operator const float* () const { return m_val; }

		void setTransform( Vector3 const& pos , Quaternion const& q );
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

		void setQuaternion( Quaternion const& q );
		void setTranslation( Vector3 const& pos )
		{
			setValue( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
				pos.x , pos.y , pos.z , 1 );
		}

		void setIdentity(){  setValue( 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 );  }

		void translate( Vector3 const& offset )
		{
			m_val[12] += offset.x;
			m_val[13] += offset.y;
			m_val[14] += offset.z;
		}
		void modifyTranslation( Vector3 const& pos )
		{
			m_val[12] = pos.x;
			m_val[13] = pos.y;
			m_val[14] = pos.z;
		}
		void modifyOrientation( Quaternion const& q );

		Vector3 getTranslation() const { return Vector3( m_val[12] , m_val[13] , m_val[14] ); }

		// result = this * m
		Matrix4 leftMul ( Matrix3 const& m ) const;
		// result = m * this
		Matrix4 rightMul( Matrix3 const& m ) const;
		Vector3   leftMul ( Vector3 const& v ) const;
		Vector3   rightMul( Vector3 const& v ) const;
		Vector4   leftMul ( Vector4 const& v ) const;
		Vector4   rightMul( Vector4 const& v ) const;

		float     deter() const
		{
			return m_val[0] * mimor( *this , 1 , 2 , 3 , 1 , 2 , 3 ) 
				-  m_val[1] * mimor( *this , 1 , 2 , 3 , 0 , 2 , 3 )
				+  m_val[2] * mimor( *this , 1 , 2 , 3 , 0 , 1 , 3 ) 
				-  m_val[3] * mimor( *this , 1 , 2 , 3 , 0 , 1 , 2 ); 

		}
	private:
		inline static float  mimor( Matrix4 const& m , int r1 , int r2 , int r3 , int c1 , int c2 , int c3 )
		{
			return m( r1 , c1 ) * (  m( r2 , c2 ) * m( r3 , c3 ) - m( r2 , c3 ) * m( r3 , c2 ) ) 
				- m( r1 , c2 ) * (  m( r2 , c1 ) * m( r3 , c3 ) - m( r2 , c3 ) * m( r3 , c1 ) ) 
				+ m( r1 , c3 ) * (  m( r2 , c2 ) * m( r3 , c1 ) - m( r2 , c1 ) * m( r2 , c2 ) ) ;	   
		}

	public:
		bool  inverse( Matrix4& m , float& det ) const;
		bool  inverseAffine( Matrix4& m , float& det ) const;
		bool  isAffine() const;
		Matrix4  operator * ( Matrix4 const& rhs ) const;

		float& operator()( int idx )       { return m_val[idx]; }
		float  operator()( int idx ) const { return m_val[idx]; }

		float& operator()( int i , int j )       { return m_m[ i ][ j ]; }
		float  operator()( int i , int j ) const { return m_m[ i ][ j ]; }

	private:
		union
		{
			float m_val[16];
			float m_m[ 4 ][ 4 ];
		};

	};

	inline Matrix4  Matrix4::operator * ( Matrix4 const& rhs ) const
	{

#define MAT_MUL( v1 , v2 , idx1 , idx2 )\
	v1[4*idx1]*v2[idx2] + v1[4*idx1+1]*v2[idx2+4] + v1[4*idx1+2]*v2[idx2+8] + v1[4*idx1+3] * v2[idx2+12]

		return Matrix4(
			MAT_MUL( m_val , rhs.m_val , 0 , 0 ) ,
			MAT_MUL( m_val , rhs.m_val , 0 , 1 ) ,
			MAT_MUL( m_val , rhs.m_val , 0 , 2 ) ,
			MAT_MUL( m_val , rhs.m_val , 0 , 3 ) ,

			MAT_MUL( m_val , rhs.m_val , 1 , 0 ) ,
			MAT_MUL( m_val , rhs.m_val , 1 , 1 ) ,
			MAT_MUL( m_val , rhs.m_val , 1 , 2 ) ,
			MAT_MUL( m_val , rhs.m_val , 1 , 3 ) ,

			MAT_MUL( m_val , rhs.m_val , 2 , 0 ) ,
			MAT_MUL( m_val , rhs.m_val , 2 , 1 ) ,
			MAT_MUL( m_val , rhs.m_val , 2 , 2 ) ,
			MAT_MUL( m_val , rhs.m_val , 2 , 3 ) ,

			MAT_MUL( m_val , rhs.m_val , 3 , 0 ) ,
			MAT_MUL( m_val , rhs.m_val , 3 , 1 ) ,
			MAT_MUL( m_val , rhs.m_val , 3 , 2 ) ,
			MAT_MUL( m_val , rhs.m_val , 3 , 3 ) );
#undef MAT_MUL
	}

	inline Matrix4 operator * ( Matrix4 const m1 , Matrix3 const& m2 )
	{
		return m1.rightMul( m2 );
	}

	inline Matrix4 operator * ( Matrix3 const& m2 , Matrix4 const m1 )
	{
		return m1.leftMul( m2 );
	}

	// v' = ( v , 1 )
	inline Vector3 operator * ( Matrix4 const& m , Vector3 const& v )
	{
		return m.rightMul( v );
	}

	inline Vector3 operator * (  Vector3 const& v  , Matrix4 const& m )
	{
		return m.leftMul( v );
	}

	inline Vector4 operator * ( Matrix4 const m , Vector4 const& v )
	{
		return m.rightMul( v );
	}

	inline Vector4 operator * (  Vector4 const& v  , Matrix4 const& m )
	{
		return m.leftMul( v );
	}

	inline Vector3 TransformVector( Vector3 const& v  , Matrix4 const& m )
	{ 
		return MatrixUtility::rotate( v , m );
	}

	inline Vector3 TransformPosition( Vector3 const& v ,  Matrix4 const& m )
	{
		float const* mv = m;
#define MAT_MUL( m , index )\
	( v.x * m[ index ] + v.y * m[ index + 4 ] + v.z * m[ index + 8 ] + m[ index + 12 ] )
		float wf = 1.0f / MAT_MUL( mv , 3 );
		return Vector3( MAT_MUL( mv , 0 ) * wf , MAT_MUL( mv , 1 ) * wf , MAT_MUL( mv , 2 ) * wf );
#undef MAT_MUL
	}

	inline Vector3 TransformVectorInverse( Vector3 const& v , Matrix4 const& m )
	{
		return MatrixUtility::rotateInverse( v , m );
	}

	inline Vector3 Matrix4::leftMul( Vector3 const& v ) const
	{
#define MAT_MUL( m , index )\
	( v.x * m[ index ] + v.y * m[ index + 4 ] + v.z * m[ index + 8 ] + m[ index + 12 ] )
		return Vector3( MAT_MUL( m_val , 0 ) , MAT_MUL( m_val , 1 ) , MAT_MUL( m_val , 2 ) );
#undef MAT_MUL
	}

	inline Vector3 Matrix4::rightMul( Vector3 const& v ) const
	{
#define MAT_MUL( m , index )\
	( v.x * m[ 4 * index ] + v.y * m[ 4 * index + 1 ] + v.z * m[ 4 * index + 2 ] + m[ 4 * index + 3 ] )
		return Vector3( MAT_MUL( m_val , 0 ) , MAT_MUL( m_val , 1 ) , MAT_MUL( m_val , 2 ) );
#undef MAT_MUL
	}

	inline Vector4 Matrix4::leftMul( Vector4 const& v ) const
	{
#define MAT_MUL( m , index )\
	( v.x * m[ index ] + v.y * m[ index + 4 ] + v.z * m[ index + 8 ] + v.w * m[ index + 12 ] )
		return Vector4( MAT_MUL( m_val , 0 ) , MAT_MUL( m_val , 1 ) , MAT_MUL( m_val , 2 ) , MAT_MUL( m_val , 3 ) );
#undef MAT_MUL
	}

	inline Vector4 Matrix4::rightMul( Vector4 const& v ) const
	{
#define MAT_MUL( m , index )\
	( v.x * m[ 4 * index ] + v.y * m[ 4 * index + 1 ] + v.z * m[ 4 * index + 2 ] + v.w + m[ 4 * index + 3 ] )
		return Vector4( MAT_MUL( m_val , 0 ) , MAT_MUL( m_val , 1 ) , MAT_MUL( m_val , 2 ) , MAT_MUL( m_val , 3 ) );
#undef MAT_MUL
	}
}

#endif // Matrix4_h__D6EC7C56_939F_4B27_81B6_42F5B02E8637
