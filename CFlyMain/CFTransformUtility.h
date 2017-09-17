#ifndef CFTransformUtility_h__
#define CFTransformUtility_h__

#include "CFBase.h"

namespace CFly
{

	enum AxisEnum
	{
		CF_AXIS_X ,
		CF_AXIS_Y ,
		CF_AXIS_Z ,

		CF_AXIS_X_INV ,
		CF_AXIS_Y_INV ,
		CF_AXIS_Z_INV ,
	};

	inline Vector3 getAxisDirection( CFly::AxisEnum axis )
	{
		Vector3 const direction[] = 
		{
			Vector3(1,0,0) , Vector3(0,1,0) ,Vector3(0,0,1) ,
			Vector3(-1,0,0) , Vector3(0,-1,0) ,Vector3(0,0,-1) ,
		};
		return direction[ axis ];
	}

	enum TransOp
	{
		CFTO_REPLACE ,
		CFTO_GLOBAL ,
		CFTO_LOCAL ,
	};


	static void mul33( Matrix4& result , Matrix4& lhs , Matrix4& rhs )
	{

	}

	//assert matrix's format =
	//m0 , m1 , m2 , 0 , 
	//m4 , m5 , m6 , 0 ,
	//m8 , m9 , m10 , 0 ,
	//m12 , m13 , m14 , 1 ,
	class TransformUntility
	{
	public:
		static void transform( Matrix4& self , Matrix4 const& trans , TransOp op );
		static void transformInv( Matrix4& self , Matrix4 const& trans , TransOp op  );
		static void translate( Matrix4& self , Vector3 const& offset , TransOp op );
		static void scale( Matrix4& self , Vector3 const& s , TransOp op );
		static void rotate( Matrix4& self , AxisEnum axis , float angle , TransOp op);
		static void rotate( Matrix4& self , Vector3 const& axis , float angle  , TransOp op );
		static void rotate( Matrix4& self , Quaternion const& q , TransOp op );

		static void translateGlobal( Matrix4& self , Vector3 const& offset );
		static void translateLocal( Matrix4& self , Vector3 const& offset );
		static void scaleLocal( Matrix4& self , Vector3 const& s );
		static void scaleGlobal( Matrix4& self , Vector3 const& s );

		static void checkFormat( Matrix4 const& mat )
		{
			assert( mat.isAffine() );
		}

		static void mul( Matrix4& result , Matrix4 const& lhs , Matrix4 const& rhs );
		static void mul( Matrix4& result , Matrix3 const& lhs , Matrix4 const& rhs );

		static Vector3 mul( Vector3 const& v , Matrix4 const& m )
		{
			checkFormat( m );
			return Vector3(
				m[0] *v[0] + m[4] * v[1] + m[8] * v[2] + m[12] ,
				m[1] *v[0] + m[5] * v[1] + m[9] * v[2] + m[13] ,
				m[2] *v[0] + m[6] * v[1] + m[10] * v[2] + m[14] );
		}

	};

}//namespace CFly

#endif // CFTransformUtility_h__