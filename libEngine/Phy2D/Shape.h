#ifndef Shape_h__13F88FEE_FB5E_4052_A11C_1B95CC06E91B
#define Shape_h__13F88FEE_FB5E_4052_A11C_1B95CC06E91B

#include "Phy2D/Base.h"

#include <vector>

namespace Phy2D
{
	struct MassInfo
	{
		float m;
		float I;
		Vec2f center;
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
		virtual void  calcAABB( XForm const& xform , AABB& aabb ) = 0;
		virtual void  calcAABB( AABB& aabb ) = 0;
		virtual void  calcMass( MassInfo& info ) = 0;
		virtual Vec2f getSupport( Vec2f const& dir ) = 0;
		virtual bool  isConvex() const = 0;
	};

	class BoxShape : public Shape
	{
	public:
		virtual Type  getType() const { return eBox; }
		virtual void  calcAABB( XForm const& xform , AABB& aabb );
		virtual void  calcAABB( AABB& aabb );
		virtual void  calcMass( MassInfo& info );
		virtual Vec2f getSupport( Vec2f const& dir );
		virtual bool isConvex() const{ return true; }
		Vec2f mHalfExt;
	};

	class CircleShape : public Shape
	{
	public:
		virtual Type  getType() const { return eCircle; }
		virtual void  calcAABB( XForm const& xform , AABB& aabb );
		virtual void  calcAABB( AABB& aabb );
		virtual void calcMass( MassInfo& info );
		virtual Vec2f getSupport(Vec2f const& dir);
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
		virtual void calcAABB( XForm const& xform , AABB& aabb );
		virtual void calcAABB( AABB& aabb );
		virtual void calcMass( MassInfo& info );
		virtual Vec2f getSupport( Vec2f const& dir );
		bool isConvex() const
		{
			//TODO
			return true;
		}

		int getVertexNum() const { return (int)mVertices.size(); }
		std::vector< Vec2f > mVertices;
	};


	class CapsuleShape : public Shape
	{
	public:
		virtual Type getType() const { return eCapsule; }
		virtual bool isConvex() const {  return true;  }
		virtual void calcAABB(XForm const& xform , AABB& aabb);
		virtual void calcAABB(AABB& aabb);
		virtual void calcMass(MassInfo& info);
		virtual Vec2f getSupport(Vec2f const& dir);

		float getRadius() const { return mHalfExt.x; }

		Vec2f mHalfExt;
	};

	



}//namespace Phy2D


#endif // Shape_h__13F88FEE_FB5E_4052_A11C_1B95CC06E91B
