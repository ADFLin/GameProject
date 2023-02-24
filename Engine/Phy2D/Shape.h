#ifndef Shape_h__13F88FEE_FB5E_4052_A11C_1B95CC06E91B
#define Shape_h__13F88FEE_FB5E_4052_A11C_1B95CC06E91B

#include "Phy2D/Base.h"
#include "DataStructure/Array.h"

namespace Phy2D
{
	struct MassInfo
	{
		float m;
		float I;
		Vector2 center;
	};

	class Shape
	{
	public:
		enum Type
		{
			eBox ,
			ePolygon ,
			eCircle ,
			eCapsule ,

			NumShape ,
		};
		virtual Type  getType() const = 0;
		virtual void  calcAABB( XForm2D const& xform , AABB& aabb ) = 0;
		virtual void  calcAABB( AABB& aabb ) = 0;
		virtual void  calcMass( MassInfo& info ) = 0;
		virtual Vector2 getSupport(Vector2 const& dir ) = 0;
		virtual bool  isConvex() const = 0;
	};

	class BoxShape : public Shape
	{
	public:
		virtual Type  getType() const { return eBox; }
		virtual void  calcAABB( XForm2D const& xform , AABB& aabb );
		virtual void  calcAABB( AABB& aabb );
		virtual void  calcMass( MassInfo& info );
		virtual Vector2 getSupport(Vector2 const& dir );
		virtual bool isConvex() const{ return true; }
		Vector2 mHalfExt;
	};

	class CircleShape : public Shape
	{
	public:
		virtual Type  getType() const { return eCircle; }
		virtual void  calcAABB( XForm2D const& xform , AABB& aabb );
		virtual void  calcAABB( AABB& aabb );
		virtual void calcMass( MassInfo& info );
		virtual Vector2 getSupport(Vector2 const& dir);
		virtual bool isConvex() const{ return true; }

		float getRadius() const { return mRadius; }
		void  setRadius( float r ){ mRadius = r; }
	private:

		float mRadius;
	};

	class PolygonShape : public Shape
	{
	public:
		virtual Type getType() const { return ePolygon; }
		virtual void calcAABB( XForm2D const& xform , AABB& aabb );
		virtual void calcAABB( AABB& aabb );
		virtual void calcMass( MassInfo& info );
		virtual Vector2 getSupport(Vector2 const& dir );
		bool isConvex() const
		{
			//#TODO
			return true;
		}

		int getVertexNum() const { return (int)mVertices.size(); }
		TArray< Vector2 > mVertices;
	};


	class CapsuleShape : public Shape
	{
	public:
		virtual Type getType() const { return eCapsule; }
		virtual bool isConvex() const {  return true;  }
		virtual void calcAABB(XForm2D const& xform , AABB& aabb);
		virtual void calcAABB(AABB& aabb);
		virtual void calcMass(MassInfo& info);
		virtual Vector2 getSupport(Vector2 const& dir);

		float getRadius() const { return mHalfExt.x; }

		Vector2 mHalfExt;
	};

	



}//namespace Phy2D


#endif // Shape_h__13F88FEE_FB5E_4052_A11C_1B95CC06E91B
