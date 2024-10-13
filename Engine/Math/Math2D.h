#pragma once
#ifndef Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D
#define Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D

#include "Vector2.h"
#include "Matrix2.h"

namespace Math
{

	class Rotation2D
	{
	public:
		Rotation2D() = default;
		Rotation2D(float angle) { setAngle(angle); }

		static Rotation2D Angle(float angle) { return Rotation2D(angle); }
		static Rotation2D Identity() { return Rotation2D(1, 0); }
		static Rotation2D Make( Vector2 const& from , Vector2 const& to );

		void     setXDir(Vector2 const& dir)
		{
			CHECK(dir.isNormalized());
			c = dir.x;
			s = dir.y;
		}
		Vector2  getXDir() const { return Vector2(c, s); }
		Vector2  getYDir() const { return Vector2(-s, c); }
		void     setAngle(float angle) { Math::SinCos(angle, s, c); }
		float    getAngle() const { return Math::ATan2(s, c); }

		Vector2    rotate(Vector2 const& v) const { return leftMul(v); }
		Vector2    rotateInv(Vector2 const& v) const { return mul(v); }

		Rotation2D inverse() const { return Rotation2D(c, -s); }

		Rotation2D mul(Rotation2D const& rhs) const
		{
			// R * Rr = [  c  s ][  cr  sr ] = [  c*cr-s*sr    c*sr+s*cr ]
			//          [ -s  c ][ -sr  cr ]   [-(c*sr+s*cr)   c*cr-s*sr ]
			Rotation2D r;
			r.c = c * rhs.c - s * rhs.s;
			r.s = c * rhs.s + s * rhs.c;
			return r;
		}

		FORCEINLINE Rotation2D operator * (Rotation2D const& rhs) const
		{
			return mul(rhs);
		}

		Matrix2 toMatrix() const
		{
			return Matrix2(c, s, -s, c);
		}
	private:
		
		Vector2 mul(Vector2 const& v) const
		{
			// [  c  s ][ x ] = [ c * x + s * y ]
			// [ -s  c ][ y ]   [ -s * x + c * y]
			return Vector2(c * v.x + s * v.y, -s * v.x + c * v.y);
		}

		Vector2 leftMul(Vector2 const& v) const
		{
			//[ x  y ][  c  s ] = [ c * x - s * y ]
			//        [ -s  c ]   [ s * x + c * y ]
			return Vector2(c * v.x - s * v.y, s * v.x + c * v.y);
		}

		Rotation2D(float c, float s) :c(c), s(s) {}
		// [  c  s ]
		// [ -s  c ]
		float c, s;
	};

	class XForm2D
	{
	public:
		XForm2D()
			:mP(Vector2::Zero()), mR(Rotation2D::Identity())
		{

		}
		XForm2D(Vector2 const& p)
			:mP(p), mR(Rotation2D::Identity())
		{

		}
		XForm2D(Vector2 const& p, float angle)
			:mP(p), mR(angle)
		{

		}
		XForm2D(Vector2 const& p, Rotation2D const& r)
			:mP(p), mR(r)
		{
		}

		void setIdentity()
		{
			mP = Vector2::Zero();
			mR = Rotation2D::Identity();
		}

		Vector2 const& getPos() const { return mP; }
		Rotation2D const& getRotation() const { return mR; }

		XForm2D getRelativeTransform(XForm2D const& rhs) const
		{
			//return A.mul(B.inverse());
			//[ R  0][  Rr(-1)    0] = [ R*Rr(-1)        0]
			//[ P  1][ -Pr*Rr(-1) 1]   [ (P*-Pr)*Rr(-1)  1]
			return XForm2D(rhs.mR.rotateInv(mP - rhs.mP), mR * rhs.mR.inverse());
		}

		// A * B(-1)
		static XForm2D MakeRelative(XForm2D const& A, XForm2D const& B)
		{
			return A.getRelativeTransform(B);
		}

		XForm2D mul(XForm2D const& rhs) const
		{
			// [ R 0 ][ Rr  0 ]=[ R*Rr    1 ]
			// [ P 1 ][ Pr  1 ] [ P*Rr+Pr 1 ]
			return XForm2D(rhs.mR.rotate(mP) + rhs.mP, mR * rhs.mR);
		}

		FORCEINLINE XForm2D operator * (XForm2D const& rhs) const
		{
			return mul(rhs);
		}

		XForm2D inverse() const
		{
			// [ R 0 ][ R(-1)  0 ]=[ R*R(-1)       1 ] = [ I 0 ]
			// [ P 1 ][ P(-1)  1 ] [ P*R(-1)+P(-1) 1 ]   [ 0 1 ]
			return XForm2D(-mR.rotateInv(mP), mR.inverse());
		}

		Vector2 transformPosition(Vector2 const& v) const 
		{ 
			//  T = [ R  0 ]   v' = v * T = v * R + P;
			//      [ P  1 ]
			return mP + mR.rotate(v);
		}
		
		Vector2 transformPositionInv(Vector2 const& v) const 
		{ 
			//  T(-1) = [ R(-1)    0 ]   v' = v * T(-1) = ( v - P ) * R(-1);
			//          [-P*R(-1)  1 ]
			return mR.rotateInv(v - mP);
		}
		
		Vector2 transformVector(Vector2 const& v) const { return mR.rotate(v); }
		Vector2 transformVectorInv(Vector2 const& v) const { return mR.rotateInv(v); }

		void  translate(Vector2 const& offset) { mP += offset; }
		//void translateLocal( Vector2 const& offset ){ mP += offset; }
		void  rotate(float angle) { mR = mR.mul(Rotation2D(angle)); }
		void  setTranslation(Vector2 const& p) { mP = p; }
		void  setRoatation(float angle) { mR.setAngle(angle); }
		void  setBaseXDir(Vector2 const& dir)
		{
			mR.setXDir(dir);
		}
		float getRotateAngle() const { return mR.getAngle(); }

	private:
		Vector2 mP;
		Rotation2D mR;

	};
}

#endif // Math2D_H_F04CCD24_F01B_47D4_B29F_08BA7185C53D