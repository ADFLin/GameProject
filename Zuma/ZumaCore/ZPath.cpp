#include "ZumaPCH.h"
#include "ZPath.h"

#include "CRSpline.h"

namespace Zuma
{
	float ZCurvePath::getPathLength() const
	{
		return mStepOffset * ( mPath.size() - 1 );
	}

	Vec2D ZCurvePath::getLocation( float s ) const
	{
		if ( s < 0 )
		{
			return mPath[0] + s * mDirs[0];
		}

		int idx = int( s / mStepOffset );
		if ( idx < mPath.size() - 1 )
		{
			float frac = s - idx * mStepOffset;
			return mPath[idx] + frac * mDirs[ idx ];
		}
		else
		{
			return mPath.back();
		}

	}

	void ZCurvePath::buildSpline( CVDataVec const& vtxVec , float step  )
	{
		mStepOffset = step;

		assert( vtxVec.size() >= 4 );

		Vec2D startPos = vtxVec[1].pos; 
		Vec2D endPos;

		bool  haveMask = false;
		Range range;

		for( int i = 3 ; i < vtxVec.size() ; ++i )
		{
			CVData const& data = vtxVec[i-2];
			if ( haveMask && ( data.flag & CVData::eMask )== 0 )
			{
				range.to = mPath.size() * step;
				mMaskRanges.push_back( range );
				haveMask = false;
			}
			else if ( !haveMask && ( data.flag & CVData::eMask )!= 0 )
			{
				range.from = mPath.size() * step;
				haveMask = true;
			}

			CRSpline2D spline( vtxVec[i-3].pos , vtxVec[i-2].pos , vtxVec[i-1].pos , vtxVec[i].pos );

			float delta = 0.5f * mStepOffset / sqrt( ( vtxVec[i-2].pos- vtxVec[i-1].pos).length2() );
			buildSplinePath( spline , startPos , endPos , delta );

			startPos = endPos;
		}

		mEndPos = mPath.back();

		int num = mPath.size() - 1;
		mDirs.resize( num );
		for( int i = 0 ; i < num ; ++i )
		{
			Vec2D offset = mPath[ i + 1 ] - mPath[ i ];
			mDirs[ i ] = offset / sqrt( offset.length2() );
		}
	}

	void ZCurvePath::buildSplinePath( CRSpline2D& spline , Vec2D const& startPos , Vec2D& endPos , float delta )
	{
		float t = 0;
		endPos = startPos;

		unsigned loop = 0;
		while( t < 1 )
		{
			Vec2D pos = spline.getPoint( t + delta );

			float len = sqrt( ( pos - endPos ).length2() );
			float err  = mStepOffset - len;

			if ( fabs(err) < 1e-3f * mStepOffset  || loop > 100 )
			{
				mPath.push_back(pos);
				endPos = pos;
				t += delta;
				loop = 0;
			}
			else
			{
				++loop;
				delta *= ( 1 + 0.8f * err / len );
			}
		}
	}

	bool ZCurvePath::haveMask( float s ) const
	{
		int i = 0;
		for ( ; i < mMaskRanges.size() ; ++i )
		{
			if ( mMaskRanges[i].from <= s ) 
				break;
		}

		if ( i == mMaskRanges.size() )
			return false;

		return s < mMaskRanges[i].to;
	}

	ZLinePath::ZLinePath( Vec2D const& from , Vec2D const& to ) 
		:mFrom(from),mTo(to)
	{
		mDir = calcDir();
	}

	Vec2D ZLinePath::getLocation( float s ) const
	{
		return mFrom + s * getDir();
	}

	Vec2D ZLinePath::calcDir()
	{
		Vec2D offset =  mTo - mFrom;
		float len2 = offset.length2();
		return offset / sqrt(len2);
	}

}//namespace Zuma
