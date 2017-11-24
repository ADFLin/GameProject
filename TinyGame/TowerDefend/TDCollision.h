#ifndef TDCollision_h__
#define TDCollision_h__

#include "TDDefine.h"
#include <list>
#include <vector>

namespace TowerDefend
{
	float const gMapCellLength  = 20;
	int   const gColCellScale   = 8;
	float const gColCellLength  = gColCellScale * gMapCellLength;

#define INVALID_HASH_VALUE unsigned(-1)

	bool testCollision( Vector2 const& tPos , float tRadius , Vector2 const& pos , float radius , Vector2& offset );
	bool testCollisionFrac( Vector2 const& tPos , float tR , Vector2 const& pos , float r , Vector2 const& offset , float frac[] );
	bool testCollisionOffset( Vector2 const& tPos , float tR , Vector2 const& pos , float r , Vector2 const& dir , float offset[] );

	class ColObject
	{
	public:
		ColObject( Entity& entity );
		Entity&   getOwner(){ return mOwner; }
		float     getBoundRadius() const { return mBoundRadius;  }
		void      setBoundRadius(float radius ) { mBoundRadius = radius; }
		void      render( Renderer& renderer );
		//private:
		friend class CollisionManager;
		bool       mHaveTested;
		unsigned   mColHashValue[4];
		Entity&    mOwner;
		float      mBoundRadius;
	};

	typedef fastdelegate::FastDelegate< bool ( ColObject& ) > CollisionCallback;

	class CollisionManager
	{
	public:

		CollisionManager();
		void    init( Vec2i const& size );

		void         testCollision( Vector2 const& min , Vector2 const& max , CollisionCallback const& callback );
		ColObject*   testCollision( Vector2 const& pos , float radius , ColObject* skip , Vector2& offset );
		ColObject*   testCollision( ColObject& obj , Vector2& offset );
		ColObject*   getObject( Vector2 const& pos );
		bool         tryPlaceObject( ColObject& obj , Vector2 const& offsetDir , Vector2& pos , float& totalOffset );

		void         addObject( ColObject& obj );
		void         removeObject( ColObject& obj );
		void         updateColObject( ColObject& obj );

	private:

		void         calcHashValue( Vector2 const& min , Vector2 const& max , unsigned value[] );
		unsigned     getHash( float x , float y );

		typedef std::list< ColObject* > ColObjList;
		struct Grid 
		{
			ColObjList objs;
		};

		typedef std::vector< Grid > GridVec;
		Vec2i   mGridSize;
		GridVec mGrids;
		int     mNumColObject;

		bool addTestedObj( ColObject* obj );
		void clearTestedObj();

		typedef std::vector< ColObject* > ColObjVec;
		ColObjVec mTestedUnit;
	};

}//namespace TowerDefend

#endif // TDCollision_h__
