#include "ColorMap.h"

#include <cassert>
#include <memory.h>
#include "CRSpline.h"
#include <algorithm>

ColorMap::ColorMap(int iSize)
	:m_iRotion(0)
	,m_iMapSize(iSize)
	,m_bSmoothLine(true)
	,m_isDirty( true )
	,mColorData( m_iMapSize , Color3f(0,0,0) )
{

}

ColorMap::ColorMap(ColorMap& rhs)
	:m_iRotion(rhs.m_iRotion)
	,m_iMapSize(rhs.m_iMapSize)
	,m_bSmoothLine(rhs.m_bSmoothLine)
	,mSortedCPoints(rhs.mSortedCPoints)
	,mColorData( rhs.mColorData )
	,m_isDirty( rhs.m_isDirty )
{

}

ColorMap::~ColorMap()
{

}


void ColorMap::addPoint(int iPos, Color3f const& color)
{
	ColorPoint cp;
	cp.color = color;
	cp.pos = iPos;

	mSortedCPoints.push_back( cp );
	sortCPList();
	m_isDirty = true;
}

void ColorMap::addPoint(int iPos)
{
	calcColorMap(false);
	ColorPoint cp;
	int pos = int( iPos * getSizePosRatio() );

	cp.color = mColorData[pos];
	cp.pos = iPos;
	
	mSortedCPoints[getCPListSize()]= cp;

	sortCPList();
	m_isDirty = true;
}

bool ColorMap::removePoint(int index)
{
	if( getCPListSize() <= 2 ) 
		return false;

	if(index < 0 || index >= mSortedCPoints.size()) 
		return false;

	for(int i = index + 1;i<mSortedCPoints.size();++i)
		mSortedCPoints[i-1] = mSortedCPoints[i];

	mSortedCPoints.clear();
	sortCPList();
	m_isDirty = true;
	return true;
}

void ColorMap::sortCPList()
{
	std::sort( mSortedCPoints.begin() , mSortedCPoints.end() , 
		[](auto const& lhs, auto const& rhs)
		{
			return lhs.pos < rhs.pos;
		});
}


void ColorMap::getColor(float fVal,Color3f& outColor) const
{
	int rotion = m_iRotion*getSizePosRatio();
	int index = ( int(fVal*m_iMapSize) - rotion ) % m_iMapSize;
	
	if (index < 0) 
		index+=m_iMapSize;

	outColor = mColorData[index];
}



void ColorMap::setCPPos(int index,int Pos)
{
	assert(index >= 0 && index < mSortedCPoints.size());

	mSortedCPoints[index].pos = Pos - getRotion();
	if (mSortedCPoints[index].pos<0)
		mSortedCPoints[index].pos += CPPosRange;

	sortCPList();
	m_isDirty = true;
}


void ColorMap::setCPColor(int index,int idxComp,float val)
{
	if( index < 0 || index >= mSortedCPoints.size()) 
		return;

	mSortedCPoints[index].color[idxComp] = val;
	m_isDirty = true;
	//computeColorMap();
}

void ColorMap::computeColorMapSmooth()
{
	float pos[4];
	Color3f arColor[4];
	float* y = new float[m_iMapSize];

	for(int n=0;n<mSortedCPoints.size()+3;++n)
	{
		ColorPoint* cps[4];
		for(int i=0;i<4;++i)
		{
			int index=(i+n-2+ mSortedCPoints.size())% mSortedCPoints.size();
			pos[i] = mSortedCPoints[index].pos;
			arColor[i]= mSortedCPoints[index].color;
			if(  ( i + n - 2 ) < 0 ) 
				pos[i] -= ColorMap::CPPosRange;

			if( ( i + n- 2 ) >= mSortedCPoints.size() )
				pos[i] += ColorMap::CPPosRange;
		}

		int x1=(pos[1]*m_iMapSize)/ColorMap::CPPosRange;
		int x2=(pos[2]*m_iMapSize)/ColorMap::CPPosRange;
		int size = x2 - x1;

		for (int idxComp=0;idxComp<3;++idxComp)
		{
			CRSpline2D spline(Vector2(pos[0],arColor[0][idxComp]) ,
							  Vector2(pos[1],arColor[1][idxComp]) ,
							  Vector2(pos[2],arColor[2][idxComp]) ,
							  Vector2(pos[3],arColor[3][idxComp]) );

			spline.getValue( 20,nullptr,y,size);
			for(int j=0;j<size;++j)
			{
				float val = Math::Clamp<float>(y[j], 0, 1);
				mColorData[(x1+j+m_iMapSize)%m_iMapSize][idxComp] =val;
			}
		}
	}
	delete[] y;
}
void ColorMap::calcColorMap( bool beForce /*= false */)
{
	if ( !m_isDirty && !beForce )
		return;

	if (m_bSmoothLine && mSortedCPoints.size() > 2)
		computeColorMapSmooth();
	else
		computeColorMapLinear();

	m_isDirty = false;
}

void ColorMap::computeColorMapLinear()
{
	for (int n=0;n<mSortedCPoints.size();++n)
	{
		int now = n%mSortedCPoints.size();
		int next = (n+1)% mSortedCPoints.size();
		int start = mSortedCPoints[now].pos*m_iMapSize/CPPosRange;
		int end  = mSortedCPoints[next].pos*m_iMapSize/CPPosRange;

		int dL = end-start;
		dL = (dL>0) ? dL : (m_iMapSize+dL);

		float dR = float(mSortedCPoints[next].color[COLOR_R] - mSortedCPoints[now].color[COLOR_R])/dL;
		float dG = float(mSortedCPoints[next].color[COLOR_G] - mSortedCPoints[now].color[COLOR_G])/dL;
		float dB = float(mSortedCPoints[next].color[COLOR_B] - mSortedCPoints[now].color[COLOR_B])/dL;
		float R = float(mSortedCPoints[now].color[COLOR_R]);
		float G = float(mSortedCPoints[now].color[COLOR_G]);
		float B = float(mSortedCPoints[now].color[COLOR_B]);

		for (int i=0;i<dL;++i)
		{
			int index=(start+i)%m_iMapSize;

			mColorData[index][0]=R;		
			mColorData[index][1]=G;
			mColorData[index][2]=B;

			R+=dR;
			G+=dG;
			B+=dB;
		}
	}
}
