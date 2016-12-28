#ifndef rcBuildingManager_h__
#define rcBuildingManager_h__

#include "rcBase.h"
#include "rcBuilding.h"

#include "TQuadTree.h"

class  rcLevelCity;
class  rcCityInfo;

struct BaseData
{
	Vec2i centerPos;
};
struct LeafData
{
	typedef std::list< rcBuilding* > BuildingList;
	BuildingList buildList;

	template < class Fun >
	rcBuilding* visitBuilding( Fun& fun )
	{
		for ( BuildingList::iterator iter = buildList.begin();
			  iter != buildList.end(); ++iter )
		{
			rcBuilding* building = *iter;
			if ( !fun( building ) )
				return building;
		}
		return NULL;
	}
};


class rcQuadTreeMap : public TQuadTree< rcQuadTreeMap , LeafData , BaseData >
{

public:

	static LeafNode* getNode( Node* node , Vec2i& pos , int length )
	{
		if ( node->isLeaf() )
			return static_cast< LeafNode* >( node );

		GrayNode* grayNode = static_cast< GrayNode* >( node );

		int subLength = length / 2;

		if ( pos.x >= subLength )
		{
			pos.x -= subLength;
			if ( pos.y >= subLength )
			{
				pos.y -= subLength;
				return getNode( grayNode->child[ND_NE] , pos , subLength);
			}
			else
				return getNode( grayNode->child[ND_SE] , pos , subLength);
		}
		else
		{
			if ( pos.y >= subLength )
			{
				pos.y -= subLength;
				return getNode( grayNode->child[ND_NW] , pos , subLength);
			}
			else
				return getNode( grayNode->child[ND_SW] , pos , subLength);
		}
	}


	Node* addBuilding( rcBuilding* build )
	{

		return nullptr;
	}
	void  removeBuilding( rcBuilding* build )
	{

	}

};

class rcBuildingManager : public rcBuildingQuery
{
public:
	rcBuildingManager();

	void         update( rcCityInfo& cInfo , long time );
	rcBuilding*  createBuilding( rcCityInfo& cInfo , unsigned bTag , unsigned modelID , Vec2i const& pos , bool swapAxis );
	void         destoryBuilding( rcCityInfo& cInfo , rcBuilding* building );

	rcBuilding*  findStorage( FindCookie& cookie , unsigned pTag );
	rcHouse*     findEmptyHouse( FindCookie& cookie , int& numEmptySpace );

	void         updateBuildingList();
	
	typedef std::map< unsigned , BuildingList > BdgListMap;
	

	BuildingList& getBuildingList( rcBuilding* building );

private:
	BdgListMap       mBuildingTagMap;
	
};
#endif // rcBuildingManager_h__