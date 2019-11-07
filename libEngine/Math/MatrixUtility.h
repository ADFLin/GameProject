#ifndef MatrixUtility_h__
#define MatrixUtility_h__

#include "Math/Base.h"
#include "Math/Vector3.h"
#include "Math/Quaternion.h"

namespace Math
{
	class Quaternion;
	class MatrixUtility
	{
	public:
		template< class TMatrix >
		static void toQuaternion(TMatrix const& m , Quaternion& q )
		{
			float tr = m(0,0)+m(1,1)+m(2,2);
			if (tr>0)	
			{
				float sw= Math::Sqrt( tr + float(1.0) );
				float s=float(0.5)/sw;
				q.setValue( ( m(1,2)-m(2,1))*s ,
					( m(2,0)-m(0,2))*s ,
					( m(0,1)-m(1,0))*s ,  
					sw*float(0.5) );
			}
			else
			{
				int i = 0;
				if( m(1,1) > m(0,0) ) 
					i=1;
				if( m(2,2) > m(i,i) ) 
					i=2;

				const int nxt[3]={1,2,0};
				int j = nxt[i];
				int k = nxt[j];

				float s = Math::Sqrt( m(i,i)-m(j,j)-m(k,k) + 1.0f );

				q[i]= s * 0.5f;
				s = 0.5f / s;
				q[j]=(m(j,i)+m(i,j))*s;
				q[k]=(m(k,i)+m(i,k))*s;
				q.w =(m(j,k)-m(k,j))*s;
			}
		}

		template< class TMatrix >
		FORCEINLINE static Vector3 Rotate( Vector3 const& v  , TMatrix const& m )
		{
			return Vector3(
				v[0] * m( 0 , 0 ) + v[1] * m( 1 , 0 ) + v[2] * m( 2 , 0 ) ,
				v[0] * m( 0 , 1 ) + v[1] * m( 1 , 1 ) + v[2] * m( 2 , 1 ) ,
				v[0] * m( 0 , 2 ) + v[1] * m( 1 , 2 ) + v[2] * m( 2 , 2 ) );
		}

		template< class TMatrix >
		FORCEINLINE static Vector3 RotateInverse( Vector3 const& v , TMatrix const& m )
		{
			return Vector3(
				m( 0 , 0 )*v[0] + m( 0 , 1 )*v[1] + m( 0 , 2 )*v[2] ,
				m( 1 , 0 )*v[0] + m( 1 , 1 )*v[1] + m( 1 , 2 )*v[2] ,
				m( 2 , 0 )*v[0] + m( 2 , 1 )*v[1] + m( 2 , 2 )*v[2] );
		}


		template< class TMatrix >
		static void setRotationX(TMatrix& m , float angle )
		{
			float c,s;
			Math::SinCos( angle , s , c );
			m.setValue( 1 , 0 ,  0 ,
				0 , c ,  s ,
				0 , -s , c );
		}

		template< class TMatrix >
		FORCEINLINE static void setRotationY(TMatrix& m ,float angle )
		{
			float c,s;
			Math::SinCos( angle , s , c );
			m.setValue(  c , 0 , -s ,
				0 , 1 ,  0 ,
				s , 0 ,  c );
		}

		template< class TMatrix >
		FORCEINLINE static void setRotationZ(TMatrix& m ,float angle )
		{
			float c,s;
			Math::SinCos( angle , s , c );
			m.setValue(  c , s ,  0 ,
				-s , c ,  0 ,
				0 , 0 ,  1 );
		}

		template< class TMatrix >
		static void modifyOrientation(TMatrix& m , Quaternion const& q )
		{
			float d = q.length2();
			assert( d > 1e-5 );
			float s = 2.0f / d;
			float xs = q.x*s , ys = q.y*s ,zs = q.z*s ;

			float xx = q.x*xs , xy = q.x*ys , xz = q.x*zs;
			float yy = q.y*ys , yz = q.y*zs , zz = q.z*zs;
			float wx = q.w*xs , wy = q.w*ys , wz = q.w*zs;


			m( 0 , 0 ) = 1.0f - (yy+zz); 
			m( 0 , 1 ) = xy + wz;
			m( 0 , 2 ) = xz - wy;

			m( 1 , 0 ) = xy - wz;
			m( 1 , 1 ) = 1.0f - (xx+zz);
			m( 1 , 2 ) = yz + wx ;

			m( 2 , 0 ) = xz + wy;
			m( 2 , 1 ) = yz - wx ;
			m( 2 , 2 ) = 1.0f - (xx+yy);
		}

		template< class TMatrix >
		static void setRotation(TMatrix& m , Vector3 const& axis , float angle )
		{
			// R = cos(theta ) I + sin(theta)[k]x + ( 1 - cos(theta) )[k]x[k]
			//           0    kz  -ky                  kx^2   kx*ky kx*kz
			// [k]x = [ -kz   0    kx  ] , [k]x[k] = [ kx*ky  ky^2  ky*kz ]
			//           ky  -kx   0                   kx*kz  ky*kz kz^2
			// v' = v R

			float c,s;
			Math::SinCos( 0.5f * angle , s , c );

			float c2= c*c - s*s;

			float len2 = axis.length2(); 

			float k2 = ( 1- c2 ) / len2;
			float wkx = axis.x*k2;
			float wky = axis.y*k2;
			float wkz = axis.z*k2;
			float xx  = axis.x*wkx, yy=axis.y*wky, zz=axis.z*wkz;
			float xy  = axis.x*wky, yz=axis.y*wkz, zx=axis.z*wkx;

			float k = 2.0f*s*c / Math::Sqrt( len2 );
			float kx = axis.x*k; 
			float ky = axis.y*k; 
			float kz = axis.z*k;

			m.setValue( 
				xx+c2  , xy+kz  , zx-ky ,
				xy-kz  , yy+c2  , yz+kx ,
				zx+ky  , yz-kx  , zz+c2 );
		}

	};
}//namespace Math


#endif // MatrixUtility_h__
