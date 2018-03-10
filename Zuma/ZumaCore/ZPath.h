#ifndef ZPath_h__
#define ZPath_h__

#include "ZBase.h"
class CRSpline2D;

namespace Zuma
{
	struct CurveVertex
	{
		enum
		{
			eMask = BIT(1),
		};
		Vector2  pos;
		unsigned flag;
	};

	typedef std::vector< CurveVertex > CurveVertexVector;
	bool savePathVertex( char const* path , CurveVertexVector& vtxVec );
	bool loadPathVertex( char const* path , CurveVertexVector& vtxVec );

	class ZPath
	{
	public:
		virtual float  getPathLength() const = 0;
		virtual Vector2  getEndLocation() const = 0;
		virtual Vector2  getLocation( float s ) const = 0;
		virtual bool   haveMask( float s ) const = 0;
	};

	class ZCurvePath : public ZPath
	{
	public:
		Vector2  getEndLocation() const { return mEndPos; }
		float  getPathLength() const;
		Vector2  getLocation( float s ) const;
		bool   haveMask( float s ) const;
		void   buildSpline( CurveVertexVector const& vtxVec , float step );
	protected:
		void   buildSplinePath( CRSpline2D& spline , Vector2 const& startPos , Vector2& endPos , float delta );

		struct Range
		{
			float from ,  to;
		};
		typedef std::vector< Range > RangeVec;
		typedef std::vector< Vector2 > VertexVec;

		RangeVec  mMaskRanges;
		Vector2   mEndPos;
		VertexVec mPath;
		VertexVec mDirs;
		float     mStepOffset;
	};

	class ZLinePath : public ZPath
	{
	public:
		ZLinePath( Vector2 const& from , Vector2 const& to);
		Vector2  getEndLocation() const { return mTo; }
		Vector2  getLocation( float s ) const;
		Vector2  getDir() const { return mDir; }
		bool   haveMask( float s ) const  {  return false;  }
	protected:

		Vector2 calcDir();
		Vector2 mDir;
		Vector2 mFrom;
		Vector2 mTo;
	};

}//namespace Zuma

#endif // ZPath_h__
