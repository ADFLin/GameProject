#ifndef Collision_h__
#define Collision_h__

#include "Base.h"
#include "ColBody.h"
#include "Block.h"

#include "DataStructure/IntrList.h"
#include "DataStructure/Array.h"

struct ColInfo
{
	bool  isTerrain;
	union
	{
		Tile*    tile;
		ColBody* body;
	};
};

using ColBodyVec = TArray< ColBody* >;

class CollisionManager
{
public:
	CollisionManager();

	void  setTerrain( TileMap& terrain ){  mTerrain = &terrain; }
	void  setup( float width , float height  , float cellLength );
	void  addBody( LevelObject& obj , ColBody& body );
	void  removeBody( ColBody& body );
	bool  updateBodySize( ColBody& body );

	void  update();
	bool  testCollision( ColInfo& info , Vec2f const& offset , ColBody& body , unsigned colMaskReplace = 0 );
	Tile* rayTerrainTest( Vec2f const& from , Vec2f const& to , unsigned typeMask );
	Tile* testTerrainCollision( Rect const& bBox , unsigned typeMask );
	
	void  findBody( Rect const& bBox , unsigned colMask , ColBodyVec& out );
	float getCellLength(){ return mCellLength; }

private:

	bool checkCollision( ColBody& body );
	void updateBody( ColBody& body );
	void calcCellPos( Vec2f const& pos , int& cx , int& cy );

	Tile* rayBlockTest( Vec2i const& tPos , Vec2f const& from , Vec2f const& to , unsigned typeMask );
	
	template< typename TFunc >
	bool visitBodies( Vec2f const& boxMin , Vec2f const& boxMax, TFunc&& func )
	{
		for (ColBody& bodyTest : mGlobalBodies)
		{
			if (func(bodyTest))
				return true;
		}

		int xMin, xMax, yMin, yMax;
		xMin = Math::Clamp(Math::FloorToInt(boxMin.x / mCellLength) - 1, 0, mCellMap.getSizeX() - 1);
		xMax = Math::Clamp(Math::FloorToInt(boxMax.x / mCellLength) + 1, 0, mCellMap.getSizeX() - 1);
		yMin = Math::Clamp(Math::FloorToInt(boxMin.y / mCellLength) - 1, 0, mCellMap.getSizeY() - 1);
		yMax = Math::Clamp(Math::FloorToInt(boxMax.y / mCellLength) + 1, 0, mCellMap.getSizeY() - 1);

		for (int i = xMin; i <= xMax; ++i)
		{
			for (int j = yMin; j <= yMax; ++j)
			{
				Cell& cell = mCellMap.getData(i, j);

				for (ColBody& bodyTest : cell.bodies)
				{
					if (func(bodyTest))
						return true;
				}
			}
		}
		return false;
	}
	
	typedef TIntrList< ColBody , MemberHook< ColBody , &ColBody::cellHook > >    CellBodyList;
	typedef TIntrList< ColBody , MemberHook< ColBody , &ColBody::managerHook > > BodyList;

	static int const IdxGlobalCell = -5;
	struct Cell
	{
		CellBodyList bodies;
	};
	float            mCellLength;
	TGrid2D< Cell >  mCellMap;
	CellBodyList     mGlobalBodies;
	BodyList         mBodies;
	TileMap*         mTerrain;

};

#endif // Collision_h__
