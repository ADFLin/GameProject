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

	bool testCollision( Vec2f const& tPos , float tRadius , Vec2f const& pos , float radius , Vec2f& offset );
	bool testCollisionFrac( Vec2f const& tPos , float tR , Vec2f const& pos , float r , Vec2f const& offset , float frac[] );
	bool testCollisionOffset( Vec2f const& tPos , float tR , Vec2f const& pos , float r , Vec2f const& dir , float offset[] );

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

		void         testCollision( Vec2f const& min , Vec2f const& max , CollisionCallback const& callback );
		ColObject*   testCollision( Vec2f const& pos , float radius , ColObject* skip , Vec2f& offset );
		ColObject*   testCollision( ColObject& obj , Vec2f& offset );
		ColObject*   getObject( Vec2f const& pos );
		bool         tryPlaceObject( ColObject& obj , Vec2f const& offsetDir , Vec2f& pos , float& totalOffset );

		void         addObject( ColObject& obj );
		void         removeObject( ColObject& obj );
		void         updateColObject( ColObject& obj );

	private:

		void         calcHashValue( Vec2f const& min , Vec2f const& max , unsigned value[] );
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
