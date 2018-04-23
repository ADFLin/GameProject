#ifndef CRSpline_H
#define CRSpline_H

// Catmull-Rom Splines
//   Desired Line|  
//   x.......x--------x.......x
//   P0      P1      P2      P3
//           t=0     t=1
//                               [  0  2  0  0 ]  [P0]
//                      2   3    [ -1  0  1  0 ]  [P1]
//q(t) = 0.5 * [ 1  t  t   t ] * [  2 -5  4 -1 ] *[P2]
//					             [ -1  3 -3  1 ]  [P3]
//q(t) = 0.5 *( a0 + a1*t + a2*t^2 + a3*t^3 ) 
//	a0 = 2* P1
//	a1 = -P0 + P2
//	a2 = 2*P0 - 5*P1 + 4*P2 - P3
//	a3 = -P0 + 3*P1- 3*P2 + P3

template < class T >
class CRSplineT
{
public:
	typedef T VertexType;

	CRSplineT(){}
	CRSplineT( VertexType const& P0,VertexType const& P1,
		       VertexType const& P2,VertexType const& P3 )
	{
		create(P0,P1,P2,P3);
	}

	void create( VertexType const& P0,VertexType const& P1,
		         VertexType const& P2,VertexType const& P3 )
	{
		//m_a1 = 0.5f * ( P2 - P0 );
		//m_a2 = P0 - 2.5f*P1 + 2.0f*P2 - 0.5f*P3;
		//m_a2 = -0.5f * ( P3 - P0 ) - 2.5f * ( P1 - P2 ) -0.5f * ( P2 - P0 );
		//m_a3 =  0.5f * ( P3 - P0 ) + 1.5f * ( P1 - P2 );

		VertexType t1 =  P1 - P2;
		VertexType t2 =  0.5f*( P2 - P0 );
		VertexType t3 = -0.5f*( P3 - P0 );

		m_a0 = P1;
		m_a1 = t2 ;
		m_a2 = t3 - 2.5f * t1 - t2;
		m_a3 = 1.5f * t1 - t3;
	}

	VertexType  getPoint(float t) const { return  m_a0+t*(m_a1+t*(m_a2 + t*m_a3 ));}
protected:
	VertexType m_a0,m_a1,m_a2,m_a3;
};


#include "TVector2.h"

class CRSpline2D : public CRSplineT< TVector2< float > >
{
	typedef TVector2< float > Vec2D;
public:
	CRSpline2D(){}
    CRSpline2D(Vec2D const& P0,Vec2D const& P1,
		       Vec2D const& P2,Vec2D const& P3)
	   :CRSplineT< TVector2< float > >( P0 , P1 , P2 ,P3 ){}
//  0 <= t <= 1
	void  getValue( int numPoint ,float* pXVal ,float* pYVal ,int numData );
};


#endif //CRSpline_H




