#ifndef Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510
#define Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510

#include "Math/Base.h"
#include "TVector2.h"
#include "IntegerType.h"
#include <limits>

#define PHY2D_DEBUG 1

#ifndef PHY2D_DEBUG
#define PHY2D_DEBUG 0
#endif//PHY2D_DEBUG

namespace Phy2D
{
	float const FLT_DIV_ZERO_EPSILON = 1e-6f;

	typedef void(*DebugJumpFun)();
	extern DebugJumpFun GDebugJumpFun;

	class Vec2f : public TVector2< float >
	{
	public:
		Vec2f(){}
		Vec2f( TVector2< float > const& rhs ):TVector2<float>(rhs){}
		Vec2f( float x , float y ):TVector2<float>(x,y){}
		float normalize()
		{
			float len = Math::Sqrt( length2() );
			if ( len < FLT_DIV_ZERO_EPSILON )
				return 0.0;
			*this *= ( 1 / len );
			return len;
		}
		float length() const 
		{
			return Math::Sqrt( length2() );
		}
		bool isNormalize() const
		{
			return std::abs( 1 - length2() ) < 1e-5;
		}


		static inline Vec2f Cross( float value , Vec2f const& v )
		{
			return Vec2f( -value * v.y , value * v.x );
		}


	};


	inline Vec2f normalize( Vec2f const& v )
	{
		Vec2f result = v;
		result.normalize();
		return result;
	}

	// ( a x b ) x c = (a.c)b - (b.c) a
	inline Vec2f tripleProduct( Vec2f const& a , Vec2f const& b , Vec2f const& c )
	{
		return a.dot( c ) * b - b.dot( c ) * a;
	}


	class AABB
	{
	public:
		Vec2f min;
		Vec2f max;


		void  expend( Vec2f const& offset )
		{
			if ( offset.x > 0 )
				max.x += offset.x;
			else
				min.x += offset.x;

			if ( offset.y > 0 )
				max.y += offset.y;
			else
				min.y += offset.y;
		}
		bool isInterect( AABB const& other )
		{
			if ( min.x > other.max.x || min.y > other.max.y)
				return false;
			if ( max.x < other.min.x || max.y < other.min.y )
				return false;
			return true;
		}
	};

	class Rotation
	{
	public:
		Rotation(){}
		Rotation( float angle ){ setAngle( angle ); }
		void  setAngle( float angle ){  Math::SinCos( angle , s , c );  }
		float getAngle() const { return Math::ATan2( s , c ); }

		Vec2f mul( Vec2f const& v ) const 
		{
			return Vec2f( v.x * c - v.y * s  , v.x * s + v.y * c );
		}

		Vec2f mulInv( Vec2f const& v ) const 
		{
			return Vec2f( v.x * c + v.y * s , -v.x * s + v.y * c );
		}

		// R * Rr = [ c -s ][ cr -sr ] = [ c*cr-s*sr -(c*sr+s*cr)]
		//          [ s  c ][ sr  cr ]   [ c*sr+s*cr   c*cr-s*sr ]
		Rotation mul( Rotation const& rhs ) const 
		{
			Rotation r;
			r.c = c*rhs.c - s*rhs.s;
			r.s = c*rhs.s + s*rhs.c;
			return r;
		}
		// Rt * Rr = [  c  s ][ cr -sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
		//           [ -s  c ][ sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
		Rotation mulInv( Rotation const& rhs ) const 
		{
			Rotation r;
			r.c = c*rhs.c + s*rhs.s;
			r.s = c*rhs.s - s*rhs.c;
			return r;
		}

		// R * Rr^t = [ c -s ][  cr  sr ] = [ c*cr+s*sr -(s*cr-c*sr)]
		//            [ s  c ][ -sr  cr ]   [(s*cr-c*sr)  c*cr+s*sr ]
		Rotation mulRightInv( Rotation const rhs ) const 
		{
			Rotation r;
			r.c = c*rhs.c + s*rhs.s;
			r.s = s*rhs.c - c*rhs.s;
			return r;

		}

		Vec2f getXDir() const {  return Vec2f(c,s);  }
		Vec2f getYDir() const {  return Vec2f(-s,c);  }
		static Rotation Identity(){ return Rotation(1,0); }

	private:
		Rotation( float c , float s ):c(c),s(s){}
		// [ c -s ]
		// [ s  c ]
		float c , s;
	};
	class XForm
	{
	public:
		XForm()
			:mP( Vec2f::Zero() ),mR( Rotation::Identity() )
		{

		}
		XForm( Vec2f const& p )
			:mP(p),mR( Rotation::Identity() )
		{

		}
		XForm( Vec2f const& p , float angle )
			:mP(p),mR( angle )
		{

		}
		XForm( Vec2f const& p , Rotation const& r )
			:mP(p),mR(r)
		{
		}



		Vec2f const& getPos() const { return mP; }
		Rotation const& getRotation() const { return mR; }



		// [ R P ][ Rr Pr ]=[ R*Rr R*Pr+P ]  R * Rr = [ c -s ][ cr -sr ] = [ c*cr-s*sr -(c*sr+s*cr)]
		// [ 0 1 ][ 0  1  ] [  0     1    ]           [ s  c ][ sr  cr ]   [ c*sr+s*cr   c*cr-s*sr ]
		XForm mul( XForm const& rhs ) const 
		{
			return XForm( mR.mul( rhs.mP ) + mP , mR.mul( rhs.mR ) );
		}

		// [ Rt -P ][ Rr Pr ]=[ Rt*Rr Rt*Pr-P ]  R^t * Rr = [  c  s ][ cr -sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
		// [ 0  1  ][ 0  1  ] [   0     1     ]             [ -s  c ][ sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
		XForm mulInv( XForm const rhs ) const 
		{
			return XForm( mR.mulInv( rhs.mP ) - mP , mR.mulInv( rhs.mR ) );
		}
		// [ R  P ][ Rr^t -Pr ]=[ R*Rr^t -R*Pr+P ]  R * Rr^t = [ c -s ][  cr  sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
		// [ 0  1 ][ 0     1  ] [   0       1    ]             [ s  c ][ -sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
		XForm mulRightInv( XForm const rhs ) const 
		{
			return XForm( mP - mR.mulInv( rhs.mP )  , mR.mulRightInv( rhs.mR ) );
		}

		//  T = [ R  P ]   Vw = T v = R V + P;
		//      [ 0  1 ]
		Vec2f transformPosition(Vec2f const& v) const { return mP + mR.mul(v); }
		//  Tinv = [ Rt  -Rt * P ]   VL = Tinv V = Rt * ( V - P );
		//         [ 0    1      ]
		Vec2f transformPositionInv(Vec2f const& v) const { return mR.mulInv(v - mP); }
		Vec2f transformVector( Vec2f const& v ) const { return mR.mul( v ); }
		Vec2f transformVectorInv( Vec2f const& v ) const { return mR.mulInv( v ); }

		void  translate( Vec2f const& offset ){ mP += offset; }
		//void translateLocal( Vec2f const& offset ){ mP += offset; }
		void  rotate( float angle ){  mR = mR.mul( Rotation( angle ) );  }
		void  setTranslation( Vec2f const& p ){ mP = p; }
		void  setRoatation( float angle ){  mR.setAngle( angle );  }
		float getRotateAngle() const { return mR.getAngle(); }
	private:

		Vec2f    mP;
		Rotation mR;

	};


	class Shape;
	class CollideObject;
	class RigidBody;




}//namespace Phy2D

#endif // Base_h__C8B87C85_21EC_4C14_B747_F8B15EE26510