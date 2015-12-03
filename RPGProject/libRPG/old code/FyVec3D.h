#ifndef FyVec3D_h__
#define FyVec3D_h__

#include "common.h"

struct FyVec3D
{
	float x,y,z;

	explicit FyVec3D( float const* pv )
	{
		x = pv[0];
		y = pv[1];
		z = pv[2];
	}

	FyVec3D(){}
	FyVec3D( float vx , float vy , float vz )
	{
		x = vx ; y = vy ; z = vz;
	}

	operator float*(){ return &x; }

	FyVec3D&  operator=( FyVec3D const& v )
	{
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}
	FyVec3D&  operator=( Vec3D const& v )
	{
		x = v.x();
		y = v.y();
		z = v.z();
		return *this;
	}
	float dot( FyVec3D const& b ) const
	{
		return x * b.x + y*b.y + z*b.z;
	}
	FyVec3D cross( FyVec3D const& b) const
	{
		return FyVec3D( y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x );
	}
	FyVec3D& operator+=( FyVec3D const& v);
	FyVec3D& operator-=( FyVec3D const& v);
};

inline 
FyVec3D& FyVec3D::operator+=( FyVec3D const& v)
{
	this->x+=v.x;
	this->y+=v.y;
	this->z+=v.z;
	return *this;
}

inline 
FyVec3D& FyVec3D::operator-=( FyVec3D const& v)
{
	this->x-=v.x;
	this->y-=v.y;
	this->z-=v.z;
	return *this;
}

inline 
FyVec3D operator+(const FyVec3D &a,const FyVec3D &b)
{
	return FyVec3D(a.x+b.x,a.y+b.y,a.z+b.z);
}

inline 
FyVec3D operator-(const FyVec3D &a,const FyVec3D &b)
{
	return FyVec3D(a.x-b.x,a.y-b.y,a.z-b.z);
}

#endif // FyVec3D_h__
