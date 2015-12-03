#ifndef ZPath_h__
#define ZPath_h__

#include "ZBase.h"
class CRSpline2D;

namespace Zuma
{
	struct CVData
	{
		enum
		{
			eMask = BIT(1),
		};
		Vec2D    pos;
		unsigned flag;
	};

	typedef std::vector< CVData > CVDataVec;
	bool savePathVertex( char const* path , CVDataVec& vtxVec );
	bool loadPathVertex( char const* path , CVDataVec& vtxVec );

	class ZPath
	{
	public:
		virtual float  getPathLength() const = 0;
		virtual Vec2D  getEndLocation() const = 0;
		virtual Vec2D  getLocation( float s ) const = 0;
		virtual bool   haveMask( float s ) const = 0;
	};

	class ZCurvePath : public ZPath
	{
	public:
		Vec2D  getEndLocation() const { return mEndPos; }
		float  getPathLength() const;
		Vec2D  getLocation( float s ) const;
		bool   haveMask( float s ) const;
		void   buildSpline( CVDataVec const& vtxVec , float step );
	protected:
		void   buildSplinePath( CRSpline2D& spline , Vec2D const& startPos , Vec2D& endPos , float delta );

		struct Range
		{
			float from ,  to;
		};
		typedef std::vector< Range > RangeVec;
		typedef std::vector< Vec2D > VertexVec;

		RangeVec  mMaskRanges;
		Vec2D     mEndPos;
		VertexVec mPath;
		VertexVec mDirs;
		float     mStepOffset;
	};

	class ZLinePath : public ZPath
	{
	public:
		ZLinePath( Vec2D const& from , Vec2D const& to);
		Vec2D  getEndLocation() const { return mTo; }
		Vec2D  getLocation( float s ) const;
		Vec2D  getDir() const { return mDir; }
		bool   haveMask( float s ) const  {  return false;  }
	protected:

		Vec2D calcDir();
		Vec2D mDir;
		Vec2D mFrom;
		Vec2D mTo;
	};

}//namespace Zuma

#endif // ZPath_h__
