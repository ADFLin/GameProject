#include "rcPCH.h"
#include "rcLevelMap.h"

#include "rcMapLayerTag.h"

void rcMapData::init()
{
	std::fill_n( layer , ARRAY_SIZE( layer ) , 0 );

	texBase     = RC_ERROR_TEXTURE_ID;
	buildingID  = RC_ERROR_BUILDING_ID;
	flag        = 0;
	workerList  = NULL;
	renderFrame = 0;
}

bool rcMapData::haveRoad()
{
	return haveConnect( CT_ROAD );
}

bool rcLevelMap::checkCanBuild( rcBuildingInfo const& info ,Vec2i const pos , Vec2i const& size )
{

	for( int i = 0 ; i < size.x ; ++i )
	{
		for( int j = 0 ; j < size.y ; ++j )
		{
			Vec2i destPos = pos + Vec2i(i,j);

			if ( !isVaildRange( destPos ) )
				return false;

			if ( !getData( destPos ).canBuild() )
				return false;
		}
	}
	return true;
}

void rcLevelMap::markBuilding( unsigned tag ,  unsigned id , Vec2i const& pos , Vec2i const& size )
{

	for( int i = 0 ; i < size.x ; ++i )
	{
		for( int j = 0 ; j < size.y ; ++j )
		{
			rcMapData& data = getData( pos + Vec2i(i,j) );

			data.layer[ ML_BUILDING ] = tag;
			data.buildingID = id;
		}
	}
}

void rcLevelMap::unmarkBuilding( Vec2i const pos , Vec2i const& size )
{
	for( int i = 0 ; i < size.x ;++i )
	{
		for( int j = 0 ; j < size.y ; ++j )
		{
			Vec2i destPos = pos + Vec2i( i , j );

			rcMapData& data = getData( destPos );
			data.layer[ ML_BUILDING ] = 0;
			data.buildingID = RC_ERROR_BUILDING_ID;
		}
	}
}

void rcLevelMap::visitMapData( rcLevelMap::Visitor& visitor , Vec2i const& pos , Vec2i const& size  )
{
	Vec2i endPos;

	if ( size.x == -1 )
		endPos.x = getSizeX();
	else
		endPos.x = std::min( pos.x + size.x , getSizeX() );

	if ( endPos.y == -1 )
		endPos.y = getSizeY();
	else
		endPos.y = std::min( pos.y + size.y , getSizeY() );

	for( int i = std::max( pos.x , 0 ) ; i < endPos.x ; ++i )
	{
		for( int j = std::max( pos.y , 0 ); j < endPos.y ; ++j )
		{
			if ( !isVaildRange(i,j) )
				continue;

			if ( !visitor.visit( getData(i,j) ) )
				return;
		}
	}
}

void rcLevelMap::init( int xSize , int ySize )
{
	mMapData.resize( xSize , ySize );


	for( int j = 0 ; j < ySize ; ++j )
	{
		for( int i = 0 ; i < xSize ; ++i )
		{
			getData( i , j ).init();
		}
	}
}
void rcLevelMap::updateAquaductType( Vec2i const& pos , int forceDir )
{
	rcMapData& data = getData( pos );

	assert( ( data.layer[ ML_BUILDING ] == BT_ROAD || 
		      data.layer[ ML_BUILDING ] == BT_AQUADUCT ) &&
		      data.haveConnect( CT_WATER_WAY ) );

	int numLink = 0;
	int dir = 0;
	unsigned linkBit = 0;

	for( int i = 0 ; i < 4 ; ++i )
	{
		Vec2i destPos = pos + g_OffsetDir[i];
		if ( !isVaildRange( destPos ) )
			continue;

		if ( getData( destPos ).haveConnect( CT_WATER_WAY )  )
		{
			++numLink;
			if ( numLink == 1 )
				dir = i;
			linkBit |= BIT(i);
		}
	}

	rcMapData::ConnectInfo& info = data.conInfo[ CT_WATER_WAY ];

	switch ( numLink )
	{
	case 0:
	case 1:
	case 2:
		if ( data.haveConnect( CT_ROAD ) )
		{
			info.type = 3;
		}
		else
		{
			if ( dir == 0 && ( linkBit & BIT(3) ) )
				dir = 3;

			if ( linkBit & BIT( ( dir + 1 ) % 4 ) )
				info.type = 2;
			else
				info.type = 1;
		}
		break;

	case 4: info.type  = 5; break;
	case 3:
		if ( dir == 0 )
		{
			if ( ( linkBit & BIT(1) ) == 0 )
				dir = 2;
			else if ( ( linkBit & BIT(2) ) == 0 )
				dir = 3;
		}
		info.type = 4;
		break;
	}

	if ( forceDir == -1 )
	{
		if ( numLink != 0)
			info.dir = dir;
	}
	else
		info.dir = forceDir;

	info.numLink = numLink;
}


void rcLevelMap::updateRoadType( Vec2i const& pos )
{
	rcMapData& data = getData( pos );

	assert( ( data.layer[ ML_BUILDING ] == BT_ROAD || 
		      data.layer[ ML_BUILDING ] == BT_AQUADUCT ) &&
		      data.haveConnect( CT_ROAD ) );

	int numLink = 0;
	int dir = 0;
	unsigned linkBit = 0;

	for( int i = 0 ; i < 4 ; ++i )
	{
		Vec2i destPos = pos + g_OffsetDir[i];
		if ( !isVaildRange( destPos ) )
			continue;

		if ( getData( destPos ).haveRoad()  )
		{
			++numLink;
			if ( numLink == 1 )
				dir = i;
			linkBit |= BIT(i);
		}
	}

	rcMapData::ConnectInfo& info = data.conInfo[ CT_ROAD ];

	switch ( numLink )
	{
	case 0: info.type = 0; break;
	case 4: info.type = 5; break;
	case 1: info.type = 3; break;
	case 2:
		if ( dir == 0 && ( linkBit & BIT(3) ) )
			dir = 3;

		if ( linkBit & BIT( ( dir + 1 ) % 4 ) )
			info.type = 2;
		else
			info.type = 1;
		break;

	case 3:
		if ( dir == 0 )
		{
			if ( ( linkBit & BIT(1) ) == 0 )
				dir = 2;
			else if ( ( linkBit & BIT(2) ) == 0 )
				dir = 3;
		}
		info.type = 4;
		break;
	}

	info.dir = dir;
	info.numLink = numLink;
}

bool rcLevelMap::calcBuildingEntry( Vec2i const& pos , Vec2i const& size , Vec2i& entryPos )
{
	for( int n = 0 ; n < size.x ; ++n )
	{
		entryPos = pos + Vec2i( n , -1 );
		if ( !isVaildRange( entryPos ) )
			continue;
		if ( getData( entryPos ).haveRoad() )
			return true;
	}

	for( int n = 0 ; n < size.y ; ++n )
	{
		entryPos = pos + Vec2i( size.x , n );
		if ( !isVaildRange( entryPos ) )
			continue;
		if ( getData( entryPos ).haveRoad()  )
			return true;
	}

	for( int n = 0 ; n < size.y ; ++n )
	{
		entryPos = pos + Vec2i( -1 , n );
		if ( !isVaildRange( entryPos ) )
			continue;
		if ( getData( entryPos ).haveRoad()  )
			return true;
	}

	for( int n = 0 ; n < size.x ; ++n )
	{
		entryPos = pos + Vec2i( n , size.y );
		if ( !isVaildRange( entryPos ) )
			continue;
		if ( getData( entryPos ).haveRoad()  )
			return true;
	}

	return false;
}

rcMapData* rcLevelMap::getDataSafely( Vec2i const& pos )
{
	if ( !isVaildRange( pos ) )
		return NULL;
	return &mMapData.getData( pos.x , pos.y );
}

rcMapData const* rcLevelMap::getDataSafely( Vec2i const& pos ) const
{
	return const_cast<rcLevelMap*>( this )->getDataSafely( pos );
}
