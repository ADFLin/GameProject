#ifndef rcLevelMap_h__
#define rcLevelMap_h__

#include "rcBase.h"

#include "DataStructure/Grid2D.h"


struct rcBuildingInfo;

enum MapLayerType
{
	ML_BUILDING ,
	ML_TERRAIN  ,
	ML_CONNECT  ,
	ML_SUPPORT_WATER ,
	NUM_MAP_LAYER ,
};

enum ConnectType
{
	CT_ROAD      = 0,
	CT_WATER_WAY = 1,
	CT_LINK_WATER ,

	NUM_CONNECT_TYPE ,
};


class rcWorker;
struct rcMapData
{
	int layer[ NUM_MAP_LAYER ];
	
	unsigned texBase;
	unsigned buildingID;
	unsigned flag;

	struct ConnectInfo
	{
		short type    : 6;
		short numLink : 6;
		short dir     : 4;
	};
	ConnectInfo conInfo[ 2 ];
	
	unsigned  renderFrame;
	rcWorker* workerList;


	void init();
	void setConnect   ( ConnectType type , bool beE ){  ( beE ) ? addConnect( type ) : removeConnect( type ); }
	void addConnect   ( ConnectType type ){  layer[ ML_CONNECT ] |= BIT( type );  }
	void removeConnect( ConnectType type ){  layer[ ML_CONNECT ] &= ~BIT( type );  }
	bool haveConnect  ( ConnectType type ) const {  return ( layer[ ML_CONNECT ] & BIT( type ) ) != 0;  }
	
	bool haveRoad();
	bool canBuild()
	{ 
		return ( layer[ ML_BUILDING ] | 
			     layer[ ML_TERRAIN  ]  )== 0 &&
				workerList ==  NULL; 
	}
};



class rcLevelMap
{
public:

	class Visitor
	{
	public:
		virtual ~Visitor(){}
		virtual bool visit( rcMapData& data ) = 0;
	};

	void init( int xSize , int ySize );

	int   getSizeX() const { return mMapData.getSizeX(); }
	int   getSizeY() const { return mMapData.getSizeY(); }
	Vec2i getSize() const { return Vec2i( getSizeX() , getSizeY() ); }

	void  updateRoadType    ( Vec2i const& pos );
	void  updateAquaductType( Vec2i const& pos , int forceDir = -1 );
	bool  calcBuildingEntry( Vec2i const& pos , Vec2i const& size , Vec2i& entryPos );
	void  visitMapData( Visitor& visitor , Vec2i const& pos = Vec2i(0,0) , Vec2i const& size = Vec2i(-1,-1) );

	rcMapData const* getLinkData( Vec2i const& pos , int dir ) const
	{
		assert( 0 <= dir && dir < 4 );

		Vec2i linkPos = pos + g_OffsetDir[ dir ];
		if ( isRange( linkPos ) )
			return &getData( linkPos );
		return NULL;
	}


	bool isRange( int i , int j )    const { return mMapData.checkRange( i , j ); }
	bool isRange( Vec2i const& pos ) const { return mMapData.checkRange( pos.x , pos.y ); }

	rcMapData*       getDataSafely( Vec2i const& pos );
	rcMapData const* getDataSafely( Vec2i const& pos ) const;
	rcMapData const& getData( Vec2i const& pos ) const { return mMapData.getData( pos.x , pos.y );  }
	rcMapData&       getData( Vec2i const& pos )       { return mMapData.getData( pos.x , pos.y );  }
	rcMapData&       getData( int i , int j )          { return mMapData.getData( i , j  );  }

	bool checkCanBuild( rcBuildingInfo const& info ,Vec2i const pos , Vec2i const& size );
	void unmarkBuilding( Vec2i const pos , Vec2i const& size );
	void markBuilding( unsigned tag , unsigned id , Vec2i const& pos , Vec2i const& size );
private:
	TGrid2D< rcMapData > mMapData;
};


class MapLayerCountVisitor : public rcLevelMap::Visitor
{
public:
	virtual bool visit( rcMapData& data )
	{
		data.layer[ layerType ] += count; 
		return true;
	}
	MapLayerType layerType;
	int          count;
};


#endif // rcLevelMap_h__