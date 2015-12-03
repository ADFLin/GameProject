
#include "ZumaPCH.h"
#include "CRSpline.h"

void CRSpline2D::getValue( int numPoint,float* pXVal ,float* pYVal ,int numData )
{
	Vec2D p1 = getPoint(0.0f);

	float const dx = ( getPoint(1.0f) - p1 ).x/(numData);
	float const dt = 1.0f / numPoint;

	float x = p1.x;
	float t = 0.0f;
	int n=0; 

	for( int i = 0; i< numPoint; ++ i)
	{
		t+=dt;
		Vec2D p2 = getPoint(t);
		Vec2D dp = p2 - p1;

		float slop = dp.y / dp.x;

		while ( x < p2.x )
		{
			if (pXVal) 
				pXVal[n] =  x;
			if (pYVal) 
				pYVal[n] =  p1.y + ( x - p1.x ) * slop;

			++n;
			x+=dx;
		}
		p1 = p2;
	}
}
