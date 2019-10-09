#ifndef TileRegion_h__
#define TileRegion_h__


#include "Math/TVector2.h"
typedef TVector2< int >  Vec2i;

#ifndef BIT
#	define BIT(i) ( 1 << (i) )
#endif


#include <list>
#include <cassert>
#include <algorithm>


namespace TileRangion
{
	class Portal
	{




	};

}

struct range_t
{
	union
	{
		struct
		{
			int min , max;
		};
		int value[ 2 ];
	};
	bool operator == ( range_t const& rh ) const
	{
		return min == rh.min && max == rh.max;
	}

	bool operator != ( range_t const& rh ) const
	{
		return !(*this == rh );
	}
	int length() const { return max - min; }
};


inline bool calcCrossRange( range_t const& a , range_t const& b , range_t& out )
{
	out.max = std::min( a.max , b.max );
	out.min = std::max( a.min , b.min );
	return out.max > out.min; 
}


struct rect_t
{

	union 
	{
		struct
		{
			range_t yRange;
			range_t xRange;
		};

		range_t axisRange[2];
	};


	void           setRangeX( range_t const& range ){ xRange = range; }
	void           setRangeY( range_t const& range ){ yRange = range; }


	void           setRange( int idx , range_t const& range ){ axisRange[idx] = range; }
	range_t const& getRangeX() const { return xRange; }
	range_t const& getRangeY() const { return yRange; }
	range_t const& getRange( int idx ) const { return axisRange[idx]; }
	range_t&       getRange( int idx )       { return axisRange[idx]; }


	void  setValue( Vec2i const& min , Vec2i const& max )
	{
		xRange.min = min.x;
		xRange.max = max.x;
		yRange.min = min.y;
		yRange.max = max.y;
	}
	void  setValue( int xMin , int xMax , int yMin  , int yMax )
	{
		xRange.min = xMin;
		xRange.max = xMax;
		yRange.min = yMin;
		yRange.max = yMax;
	}

	int  getMinX() const { return xRange.min; }
	int  getMaxX() const { return xRange.max; }
	int  getMinY() const { return yRange.min; }
	int  getMaxY() const { return yRange.max; }

	Vec2i getMin() const { return Vec2i( xRange.min , yRange.min ); }
	Vec2i getMax() const { return Vec2i( xRange.max , yRange.max ); }

	Vec2i getSize(){ return Vec2i( xRange.length() , yRange.length() ); }

	bool  isInRange( Vec2i const& pos )
	{
		return xRange.min <= pos.x && pos.x < xRange.max &&
			   yRange.min <= pos.y && pos.y < yRange.max ;
	}



};



class Portal;

class Region
{
public:
	Region()
	{
		type = eNormal;
		shareSideBit = 0;
		//portalList = NULL;
	}

	enum Type
	{
		eNormal   ,
		eTempBlock,
		eBlock    ,
		eHole     ,
	};

	typedef std::list<Portal*> PortalList;
	typedef PortalList::iterator PortalHandle;

	rect_t&      getRect(){ return rect; }
	int          getArea(){ return rect.xRange.length() * rect.yRange.length(); }
	PortalHandle add( Portal& p ){ portals.push_front( &p ); return portals.begin(); }
	void         remove( PortalHandle handle ){ portals.erase( handle ); }
	rect_t   rect;
	Type     type;
	unsigned shareSideBit;

	friend class Portal;
	
	PortalList portals;
};







typedef int DirType;
class  Portal
{
public:


	Portal()
	{

	}

	int     value;
	range_t range;
	DirType dir;
	Region* con[2];
	Region::PortalHandle conPos[2];

	Region* getOtherRegion( Region& region )
	{
		assert( &region == con[0] || &region == con[1] );
		return ( &region == con[0] ) ? con[1] : con[0];
	}
	DirType getConnectDir( Region& region )
	{
		assert( &region == con[0] || &region == con[1] );
		return ( &region == con[0] ) ? dir : DirType( ( dir + 2 ) % 4 );
	}

	int replace( Region& rt , Region& to )
	{
		assert( con[0] == &rt ||  con[1] == &rt );
		int idx = int( con[1] == &rt );

		con[ idx ]->remove( conPos[idx] );
		con[ idx ] = &to;
		conPos[ idx ] = to.add( *this );

		return idx;
	}

	void connect( Region& r0, Region& r1 , DirType _dir )
	{
		assert( &r0 != &r1 );
		con[0] = &r0;
		conPos[0] = r0.add( *this );
		con[1] = &r1;
		conPos[1] = r1.add( *this );
		dir = _dir;
	}

	friend class Region;
};

inline bool getCrossRect( rect_t const & r1 , rect_t const & r2 , rect_t& rt )
{
	if ( !calcCrossRange( r1.xRange , r2.xRange , rt.xRange ) )
		return false;

	if ( !calcCrossRange( r1.yRange , r2.yRange , rt.yRange ) )
		return false;

	return true;
}


class RegionManager
{
public:
	enum LinkDir
	{
		eLinkRight  = 0,
		eLinkTop    = 1,
		eLinkLeft   = 2,
		eLinkBottom = 3,
	};

	typedef std::list<Region*>   RegionList;
	typedef RegionList::iterator RegionHandle;

	RegionManager( Vec2i const& size )
		:mSize( size )
	{
		Region* region = creatRegion();
		setRegionRect( region , Vec2i(0,0) , size );
	}

	void setRegionRect( Region* region , Vec2i const& min , Vec2i const& max )
	{
		region->getRect().setValue( min , max );
	}

	void updatePortal( Region& old , Region& to , unsigned dirBit , bool beRemoveOld );

	void splitRegionHorizontal( Region& oldRegion , Region* newRegion[] , 
		                        rect_t const& outerRect , rect_t const& innerRect, 
								bool beRemoveOld  );
	void splitRegionVertical( Region& oldRegion , Region* newRegion[] , 
								rect_t const& outerRect , rect_t const& innerRect , 
								bool beRemoveOld );

	bool split( Region& oldRegion , rect_t& splitRect , Region* newRegion[] , bool beRemoveOld );
	
	Portal*  buildPortal( Region& bP , Region& sP , DirType dir );


	Region* creatRegion()
	{
		Region* rt = new Region;
		mRegionList.push_front( rt );
		return rt;
	}

	Portal* createPortal()
	{
		Portal* portal = new Portal;
		mProtalList.push_back( portal );

		return portal;
	}

	
	void    mergeRegion( Region& region , RegionList* rlist , int maxRepeat );

	Region* doMergePosibleRegion( Region& region );
	void    doMergeRegion( Region& region , Region* other , Portal* portal , int idxRange );


	RegionHandle productBlock( Vec2i const& pos , Vec2i const& size )
	{
		rect_t rect;
		rect.setValue( pos , pos + size );
		return productBlock( rect );
	}
	RegionHandle productBlock( rect_t& splitRect );

	void removeBlock( RegionHandle handle )
	{
		Region* region = *handle;
		assert( region->type == Region::eBlock );

		region->type = Region::eNormal;

		mRegionList.push_back( region );
		mBlockList.erase( handle );

		mergeRegion( *region , NULL , 8 );
	}

	void removeBlock( Vec2i const& pos )
	{
		RegionList::iterator iter = mBlockList.begin();
		RegionList::iterator iterEnd = mBlockList.end();
		for( ; iter != iterEnd ; ++iter )
		{
			if ( (*iter)->getRect().isInRange( pos ) )
			{
				removeBlock( iter );
			}
		}
	}

	void destoryPortal(Portal* portal)
	{
		mProtalList.remove( portal );
		delete portal;
	}
	void destoryRegion(Region* region)
	{
		for( Region::PortalList::iterator iter = region->portals.begin(); 
			iter != region->portals.end(); ++iter )
		{
			Portal* portal = *iter;
			if ( portal->con[0] != region )
				portal->con[0]->portals.remove( portal );
			else if ( portal->con[1] != region )
				portal->con[1]->portals.remove( portal );
			destoryPortal( portal );
		}
		mRegionList.remove( region );
		delete region;
	}


	Region* getHole( Vec2i const& pos )
	{
		RegionList::iterator iter = mBlockList.begin();
		RegionList::iterator iterEnd = mBlockList.end();
		for( ; iter != iterEnd ; ++iter )
		{
			if ( (*iter)->getRect().isInRange( pos ) )
				return *iter;
		}
		return NULL;
	}

	Region* getRegion( Vec2i const& pos )
	{
		RegionList::iterator iter = mRegionList.begin();
		RegionList::iterator iterEnd = mRegionList.end();
		for( ; iter != iterEnd ; ++iter )
		{
			if ( (*iter)->getRect().isInRange( pos ) )
				return *iter;
		}
		return NULL;
	}
	void checkPortal( Region& region );
	bool tryMergeBarRegion( Region& region1 , Region& region2 , Portal& portal , int idxRange );
	//private:

	Vec2i  mSize;
	RegionList mRegionList;
	RegionList mBlockList;

	typedef std::list<Portal*> PortalList;
	PortalList mProtalList;
};


typedef RegionManager::RegionHandle RegionHandle;


#endif // TileRegion_h__