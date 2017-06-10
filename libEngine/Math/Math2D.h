#pragma once
#ifndef Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D
#define Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D

#include "Vector2.h"

namespace Math2D
{
	using namespace Math;

	class Rotation
	{
	public:
		Rotation() {}
		Rotation(float angle) { setAngle(angle); }
		void  setAngle(float angle) { Math::SinCos(angle, s, c); }
		float getAngle() const { return Math::ATan2(s, c); }

		Vector2D mul(Vector2D const& v) const
		{
			return Vector2D(v.x * c - v.y * s, v.x * s + v.y * c);
		}

		Vector2D mulInv(Vector2D const& v) const
		{
			return Vector2D(v.x * c + v.y * s, -v.x * s + v.y * c);
		}

		// R * Rr = [ c -s ][ cr -sr ] = [ c*cr-s*sr -(c*sr+s*cr)]
		//          [ s  c ][ sr  cr ]   [ c*sr+s*cr   c*cr-s*sr ]
		Rotation mul(Rotation const& rhs) const
		{
			Rotation r;
			r.c = c*rhs.c - s*rhs.s;
			r.s = c*rhs.s + s*rhs.c;
			return r;
		}
		// Rt * Rr = [  c  s ][ cr -sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
		//           [ -s  c ][ sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
		Rotation mulInv(Rotation const& rhs) const
		{
			Rotation r;
			r.c = c*rhs.c + s*rhs.s;
			r.s = c*rhs.s - s*rhs.c;
			return r;
		}

		// R * Rr^t = [ c -s ][  cr  sr ] = [ c*cr+s*sr -(s*cr-c*sr)]
		//            [ s  c ][ -sr  cr ]   [(s*cr-c*sr)  c*cr+s*sr ]
		Rotation mulRightInv(Rotation const rhs) const
		{
			Rotation r;
			r.c = c*rhs.c + s*rhs.s;
			r.s = s*rhs.c - c*rhs.s;
			return r;

		}

		Vector2D getXDir() const { return Vector2D(c, s); }
		Vector2D getYDir() const { return Vector2D(-s, c); }
		static Rotation Identity() { return Rotation(1, 0); }

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
			:mP(Vector2D::Zero()), mR(Rotation::Identity())
		{

		}
		XForm(Vector2D const& p)
			:mP(p), mR(Rotation::Identity())
		{

		}
		XForm(Vector2D const& p, float angle)
			:mP(p), mR(angle)
		{

		}
		XForm(Vector2D const& p, Rotation const& r)
			:mP(p), mR(r)
		{
		}



		Vector2D const& getPos() const { return mP; }
		Rotation const& getRotation() const { return mR; }



		// [ R P ][ Rr Pr ]=[ R*Rr R*Pr+P ]  R * Rr = [ c -s ][ cr -sr ] = [ c*cr-s*sr -(c*sr+s*cr)]
		// [ 0 1 ][ 0  1  ] [  0     1    ]           [ s  c ][ sr  cr ]   [ c*sr+s*cr   c*cr-s*sr ]
		XForm mul(XForm const& rhs) const
		{
			return XForm(mR.mul(rhs.mP) + mP, mR.mul(rhs.mR));
		}

		// [ Rt -P ][ Rr Pr ]=[ Rt*Rr Rt*Pr-P ]  R^t * Rr = [  c  s ][ cr -sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
		// [ 0  1  ][ 0  1  ] [   0     1     ]             [ -s  c ][ sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
		XForm mulInv(XForm const rhs) const
		{
			return XForm(mR.mulInv(rhs.mP) - mP, mR.mulInv(rhs.mR));
		}
		// [ R  P ][ Rr^t -Pr ]=[ R*Rr^t -R*Pr+P ]  R * Rr^t = [ c -s ][  cr  sr ] = [ c*cr+s*sr -(c*sr-s*cr)]
		// [ 0  1 ][ 0     1  ] [   0       1    ]             [ s  c ][ -sr  cr ]   [ c*sr-s*cr   c*cr+s*sr ]
		XForm mulRightInv(XForm const rhs) const
		{
			return XForm(mP - mR.mulInv(rhs.mP), mR.mulRightInv(rhs.mR));
		}

		//  T = [ R  P ]   Vw = T v = R V + P;
		//      [ 0  1 ]
		Vector2D transformPosition(Vector2D const& v) const { return mP + mR.mul(v); }
		//  Tinv = [ Rt  -Rt * P ]   VL = Tinv V = Rt * ( V - P );
		//         [ 0    1      ]
		Vector2D transformPositionInv(Vector2D const& v) const { return mR.mulInv(v - mP); }
		Vector2D transformVector(Vector2D const& v) const { return mR.mul(v); }
		Vector2D transformVectorInv(Vector2D const& v) const { return mR.mulInv(v); }

		void  translate(Vector2D const& offset) { mP += offset; }
		//void translateLocal( Vec2f const& offset ){ mP += offset; }
		void  rotate(float angle) { mR = mR.mul(Rotation(angle)); }
		void  setTranslation(Vector2D const& p) { mP = p; }
		void  setRoatation(float angle) { mR.setAngle(angle); }
		float getRotateAngle() const { return mR.getAngle(); }
	private:

		Vector2D mP;
		Rotation mR;

	};
}

#endif // Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D