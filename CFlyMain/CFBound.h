#ifndef CFBound_h__
#define CFBound_h__

#include "CFBase.h"

namespace CFly
{
	class AABBox
	{
	public:

		AABBox(){}
		static AABBox Empty(){ return AABBox( Vector3(0,0) , Vector3(0,0) ); }


		void translate( Vector3 const& offset )
		{
			mMax += offset;
			mMin += offset;
		}
		void expand( Vector3 const& dv )
		{
			mMax += dv;
			mMin -= dv;
		}


		bool makeInteration( AABBox const& other )
		{
			





		}

		bool rayTest( Vector3 const& p , Vector3 const& dir , float& )
		{


		}

		void makeUnion( AABBox const& other )
		{
			mMin.min( other );
			mMax.max( other );
		}
	private:
		Vector3 mMin;
		Vector3 mMax;
	};

	AABBox makeInteration( AABBox const& a , AABBox const& b )
	{
		AABBox out( a );
		if ( out.makeInteration( b ) )
			return out;

		return AABBox::empty();
	}

	AABBox makeUnion( AABBox const& a , AABBox const& b )
	{
		AABBox out( a );
		out.makeUnion( b );
		return out;
	}



}//namespace CFly

#endif // CFBound_h__
