#include "CFlyPCH.h"
#include "CFTransformUtility.h"

namespace CFly
{

	void TransformUntility::transform( Matrix4& self , Matrix4 const& trans , TransOp op )
	{
		switch ( op )
		{
		case CFTO_REPLACE: self = trans; break;
		case CFTO_LOCAL:   mul( self , trans , self ); break;
		case CFTO_GLOBAL:  mul( self , self , trans ); break;
		}
	}

	void TransformUntility::rotate( Matrix4& self , Vector3 const& axis , float angle , TransOp op )
	{
		transform( self , Matrix4::Rotate( axis , angle ) , op );
	}

	void TransformUntility::rotate( Matrix4& self , AxisEnum axis , float angle , TransOp op )
	{
		Matrix4 mat;
		switch( axis )
		{
		case CF_AXIS_X: mat.setRotationX( angle ); break;
		case CF_AXIS_Y: mat.setRotationY( angle ); break;
		case CF_AXIS_Z: mat.setRotationZ( angle ); break;
		case CF_AXIS_X_INV: mat.setRotationX( -angle ); break;
		case CF_AXIS_Y_INV: mat.setRotationY( -angle ); break;
		case CF_AXIS_Z_INV: mat.setRotationZ( -angle ); break;
		}
		transform( self , mat , op );
	}

	void TransformUntility::rotate( Matrix4& self , Quaternion const& q , TransOp op )
	{
		Matrix4 mat;
		mat.setQuaternion( q );
		transform( self , mat , op );
	}

	void TransformUntility::scale( Matrix4& self , Vector3 const& s , TransOp op )
	{
		switch( op )
		{
		case CFTO_GLOBAL: scaleGlobal( self , s ); break;
		case CFTO_LOCAL:  scaleLocal( self , s );  break;
		case CFTO_REPLACE: self.setScale( s );   break;
		}
	}

	void TransformUntility::translate( Matrix4& self , Vector3 const& offset , TransOp op )
	{
		switch( op )
		{
		case CFTO_GLOBAL: translateGlobal( self , offset ); break;
		case CFTO_LOCAL:  translateLocal( self , offset );  break;
		case CFTO_REPLACE: self.modifyTranslation( offset );   break;
		}
	}

	void TransformUntility::transformInv( Matrix4& self , Matrix4 const& trans , TransOp op )
	{
		checkFormat( trans );

		Matrix4 inv;
		float det;
		trans.inverseAffine( inv , det );
		switch ( op )
		{
		//case CFTO_REPLACE: self = trans; break;
		case CFTO_LOCAL:   self = inv * self; break;
		case CFTO_GLOBAL:  self = inv * trans; break;
		default:
			assert(0);
			break;
		}
	}

	void TransformUntility::translateGlobal( Matrix4& self , Vector3 const& offset )
	{
		checkFormat( self );
		self.modifyTranslation( offset );
	}

	void TransformUntility::translateLocal( Matrix4& self , Vector3 const& offset )
	{
		checkFormat( self );
		self.translate( Math::TransformVector( offset , self )  );
	}

	void TransformUntility::scaleGlobal( Matrix4& self , Vector3 const& s )
	{
		checkFormat( self );

		self[0] *= s.x; self[1] *= s.y; self[2] *= s.z;
		self[4] *= s.x; self[5] *= s.y; self[6] *= s.z;
		self[8] *= s.x; self[9] *= s.y; self[10]*= s.z;
		self[12]*= s.x; self[13]*= s.y ;self[14]*= s.z;
	}

	void TransformUntility::scaleLocal( Matrix4& self , Vector3 const& s )
	{
		checkFormat( self );

		self[0] *= s.x; self[1] *= s.x; self[2] *= s.x;
		self[4] *= s.y; self[5] *= s.y; self[6] *= s.y;
		self[8] *= s.z; self[9] *= s.z; self[10] *= s.z;
	}

	void TransformUntility::mul( Matrix4& result , Matrix4 const& lhs , Matrix4 const& rhs )
	{
		checkFormat( lhs );
		checkFormat( rhs );
#define MUL3( i , j ) lhs[ 4*i+0 ] * rhs[ j+0*4 ] + lhs[ 4*i+1 ] * rhs[ j+1*4 ] + lhs[ 4*i+2 ] * rhs[ j+2*4 ]

		result.setValue(
			MUL3( 0 , 0 ) , MUL3( 0 , 1 ) , MUL3( 0 , 2 ) , 0 ,
			MUL3( 1 , 0 ) , MUL3( 1 , 1 ) , MUL3( 1 , 2 ) , 0 ,
			MUL3( 2 , 0 ) , MUL3( 2 , 1 ) , MUL3( 2 , 2 ) , 0 ,

			MUL3( 3 , 0 ) + rhs( 3 , 0 ) , 
			MUL3( 3 , 1 ) + rhs( 3 , 1 ) ,
			MUL3( 3 , 2 ) + rhs( 3 , 2 ) ,
			1 );
#undef MUL3
	}

	void TransformUntility::inverse( Matrix4 const& trans , Matrix4& result )
	{

	}

}//namespace CFly
