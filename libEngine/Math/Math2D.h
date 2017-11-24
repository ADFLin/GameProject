#pragma once
#ifndef Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D
#define Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D

#include "Vector2.h"

namespace Math2D
{
	using ::Math::Vector2;

	class Rotation
	{
	public:
		Rotation() {}
		Rotation(float angle) { setAngle(angle); }


		static Rotation Identity() { return Rotation(1, 0); }
		static Rotation Make( Vector2 const& from , Vector2 const& to );

		Vector2  getXDir() const { return Vector2(c, s); }
		Vector2  getYDir() const { return Vector2(-s, c); }
		void     setAngle(float angle) { Math::SinCos(angle, s, c); }
		float    getAngle() const { return Math::ATan2(s, c); }

		Vector2  rotate(Vector2 const& v) { return mul(v); }
		Rotation inverse() const { return Rotation(c, -s); }

		Vector2 mul(Vector2 const& v) const
		{
			return Vector2(v.x * c - v.y * s, v.x * s + v.y * c);
		}

		Vector2 mulInv(Vector2 const& v) const
		{
			return Vector2(v.x * c + v.y * s, -v.x * s + v.y * c);
		}

		Rotation mul(Rotation const& rhs) const
		{
			// R * Rr = [ c -s ][ cr -sr ] = [ c*cr-s*sr -(c*sr+s*cr)]
			//          [ s  c ][ sr  cr ]   [ c*sr+s*cr   c*cr-s*sr ]
			Rotation r;
			r.c = c*rhs.c - s*rhs.s;
			r.s = c*rhs.s + s*rhs.c;
			return r;
		}

		Rotation mulInv(Rotation const& rhs) const
		{
			// Rt * Rr = [  c  s ][ cr -sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
			//           [ -s  c ][ sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
			Rotation r;
			r.c = c*rhs.c + s*rhs.s;
			r.s = c*rhs.s - s*rhs.c;
			return r;
		}

		Rotation mulRightInv(Rotation const rhs) const
		{
			// R * Rr^t = [ c -s ][  cr  sr ] = [ c*cr+s*sr -(s*cr-c*sr)]
			//            [ s  c ][ -sr  cr ]   [(s*cr-c*sr)  c*cr+s*sr ]
			Rotation r;
			r.c = c*rhs.c + s*rhs.s;
			r.s = s*rhs.c - c*rhs.s;
			return r;
		}

	private:
		Rotation(float c, float s) :c(c), s(s) {}
		// [ c -s ]
		// [ s  c ]
		float c, s;
	};

	class XForm
	{
	public:
		XForm()
			:mP(Vector2::Zero()), mR(Rotation::Identity())
		{

		}
		XForm(Vector2 const& p)
			:mP(p), mR(Rotation::Identity())
		{

		}
		XForm(Vector2 const& p, float angle)
			:mP(p), mR(angle)
		{

		}
		XForm(Vector2 const& p, Rotation const& r)
			:mP(p), mR(r)
		{
		}


		Vector2 const& getPos() const { return mP; }
		Rotation const& getRotation() const { return mR; }

		XForm mul(XForm const& rhs) const
		{
			// [ R P ][ Rr Pr ]=[ R*Rr R*Pr+P ]  R * Rr = [ c -s ][ cr -sr ] = [ c*cr-s*sr -(c*sr+s*cr)]
			// [ 0 1 ][ 0  1  ] [  0     1    ]           [ s  c ][ sr  cr ]   [ c*sr+s*cr   c*cr-s*sr ]
			return XForm(mR.mul(rhs.mP) + mP, mR.mul(rhs.mR));
		}

		XForm mulInv(XForm const rhs) const
		{
			// [ Rt -P ][ Rr Pr ]=[ Rt*Rr Rt*Pr-P ]  R^t * Rr = [  c  s ][ cr -sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
			// [ 0  1  ][ 0  1  ] [   0     1     ]             [ -s  c ][ sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
			return XForm(mR.mulInv(rhs.mP) - mP, mR.mulInv(rhs.mR));
		}

		XForm mulRightInv(XForm const rhs) const
		{
			// [ R  P ][ Rr^t -Pr ]=[ R*Rr^t -R*Pr+P ]  R * Rr^t = [ c -s ][  cr  sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
			// [ 0  1 ][ 0     1  ] [   0       1    ]             [ s  c ][ -sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
			return XForm(mP - mR.mulInv(rhs.mP), mR.mulRightInv(rhs.mR));
		}

		Vector2 transformPosition(Vector2 const& v) const 
		{ 
			//  T = [ R  P ]   Vw = T v = R V + P;
			//      [ 0  1 ]
			return mP + mR.mul(v); 
		}
		
		Vector2 transformPositionInv(Vector2 const& v) const 
		{ 
			//  Tinv = [ Rt  -Rt * P ]   VL = Tinv V = Rt * ( V - P );
			//         [ 0    1      ]
			return mR.mulInv(v - mP); 
		}
		
		Vector2 transformVector(Vector2 const& v) const { return mR.mul(v); }
		Vector2 transformVectorInv(Vector2 const& v) const { return mR.mulInv(v); }

		void  translate(Vector2 const& offset) { mP += offset; }
		//void translateLocal( Vector2 const& offset ){ mP += offset; }
		void  rotate(float angle) { mR = mR.mul(Rotation(angle)); }
		void  setTranslation(Vector2 const& p) { mP = p; }
		void  setRoatation(float angle) { mR.setAngle(angle); }
		float getRotateAngle() const { return mR.getAngle(); }

	private:
		Vector2 mP;
		Rotation mR;

	};
}

#endif // Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D