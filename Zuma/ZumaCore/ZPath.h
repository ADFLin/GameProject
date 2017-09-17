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
		Vec2f    pos;
		unsigned flag;
	};

	typedef std::vector< CVData > CVDataVec;
	bool savePathVertex( char const* path , CVDataVec& vtxVec );
	bool loadPathVertex( char const* path , CVDataVec& vtxVec );

	class ZPath
	{
	public:
		virtual float  getPathLength() const = 0;
		virtual Vec2f  getEndLocation() const = 0;
		virtual Vec2f  getLocation( float s ) const = 0;
		virtual bool   haveMask( float s ) const = 0;
	};

	class ZCurvePath : public ZPath
	{
	public:
		Vec2f  getEndLocation() const { return mEndPos; }
		float  getPathLength() const;
		Vec2f  getLocation( float s ) const;
		bool   haveMask( float s ) const;
		void   buildSpline( CVDataVec const& vtxVec , float step );
	protected:
		void   buildSplinePath( CRSpline2D& spline , Vec2f const& startPos , Vec2f& endPos , float delta );

		struct Range
		{
			float from ,  to;
		};
		typedef std::vector< Range > RangeVec;
		typedef std::vector< Vec2f > VertexVec;

		RangeVec  mMaskRanges;
		Vec2f     mEndPos;
		VertexVec mPath;
		VertexVec mDirs;
		float     mStepOffset;
	};

	class ZLinePath : public ZPath
	{
	public:
		ZLinePath( Vec2f const& from , Vec2f const& to);
		Vec2f  getEndLocation() const { return mTo; }
		Vec2f  getLocation( float s ) const;
		Vec2f  getDir() const { return mDir; }
		bool   haveMask( float s ) const  {  return false;  }
	protected:

		Vec2f calcDir();
		Vec2f mDir;
		Vec2f mFrom;
		Vec2f mTo;
	};

}//namespace Zuma

#endif // ZPath_h__
