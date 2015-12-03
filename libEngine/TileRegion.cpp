#include "TileRegion.h"

void RegionManager::splitRegionVertical( Region& oldRegion , Region* newRegion[] , rect_t const& outerRect , rect_t const& innerRect , bool beRemoveOld )
{
	if ( outerRect.getMinX() < innerRect.getMinX() )
	{
		Region* region = creatRegion();
		region->getRect().setValue(
			outerRect.getMinX() , innerRect.getMinX() ,
			outerRect.getMinY() , outerRect.getMaxY() );
		newRegion[eLinkLeft] = region;
	}
	else
	{
		newRegion[eLinkLeft] = NULL;
	}

	if ( outerRect.getMaxX() > innerRect.getMaxX() )
	{
		Region* region = creatRegion();
		region->getRect().setValue(
			innerRect.getMaxX() , outerRect.getMaxX() ,
		    outerRect.getMinY() , outerRect.getMaxY() );
		newRegion[eLinkRight] = region;
	}
	else
	{
		newRegion[eLinkRight] = NULL;
	}

	if ( outerRect.getMaxY() > innerRect.getMaxY() )
	{
		Region* region = creatRegion();
		region->getRect().setValue(
			innerRect.getMinX(), innerRect.getMaxX() ,
			innerRect.getMaxY(), outerRect.getMaxY() );
		newRegion[eLinkTop] = region;
	}
	else
	{
		newRegion[eLinkTop] = NULL;
	}

	if ( outerRect.getMinY() < innerRect.getMinY() )
	{
		Region* region = creatRegion();
		region->getRect().setValue(
			innerRect.getMinX() , innerRect.getMaxX() ,
			outerRect.getMinY() , innerRect.getMinY() );
		newRegion[eLinkBottom] = region;
	}
	else
	{
		newRegion[eLinkBottom] = NULL;
	}

	unsigned topBit = BIT( eLinkTop );
	unsigned downBit = BIT( eLinkBottom );

	if ( newRegion[eLinkLeft] )
	{
		updatePortal( oldRegion  , *newRegion[eLinkLeft] , 
			BIT(eLinkLeft)| BIT( eLinkTop )|BIT( eLinkBottom ) , beRemoveOld );

		if (  newRegion[eLinkTop] )
			buildPortal( *newRegion[ eLinkLeft ] , *newRegion[eLinkTop] , eLinkRight );
		if (  newRegion[eLinkBottom] )
			buildPortal( *newRegion[ eLinkLeft ] , *newRegion[eLinkBottom] , eLinkRight );
	}
	else
	{
		topBit |= BIT( eLinkLeft );
		downBit |= BIT( eLinkLeft );
	}

	if ( newRegion[eLinkRight ] )
	{
		updatePortal( oldRegion  , *newRegion[eLinkRight] ,
			BIT(eLinkRight)| BIT( eLinkTop )|BIT( eLinkBottom ) , beRemoveOld );

		if (  newRegion[eLinkTop]  )
			buildPortal( *newRegion[eLinkRight ] , *newRegion[eLinkTop]  , eLinkLeft );
		if (  newRegion[eLinkBottom]  )
			buildPortal( *newRegion[eLinkRight ] , *newRegion[eLinkBottom] , eLinkLeft );
	}
	else
	{
		topBit |= BIT( eLinkRight );
		downBit |= BIT( eLinkRight );
	}


	if ( newRegion[eLinkTop] )
		updatePortal( oldRegion  , *newRegion[eLinkTop] , topBit , beRemoveOld);

	if ( newRegion[ eLinkBottom ] )
		updatePortal( oldRegion  , *newRegion[eLinkBottom] , downBit , beRemoveOld );

}

void RegionManager::splitRegionHorizontal( Region& oldRegion , Region* newRegion[] , rect_t const& outerRect , rect_t const& innerRect , bool beRemoveOld )
{
	if ( outerRect.getMaxY() > innerRect.getMaxY() )
	{
		Region* region = creatRegion();
		region->getRect().setValue(
			outerRect.getMinX(), outerRect.getMaxX() ,
			innerRect.getMaxY(), outerRect.getMaxY() );
		newRegion[eLinkTop] = region;
	}
	else
	{
		newRegion[eLinkTop] = NULL;
	}

	if ( outerRect.getMinY() < innerRect.getMinY() )
	{
		Region* region = creatRegion();
		region->getRect().setValue(
			outerRect.getMinX() , outerRect.getMaxX() ,
			outerRect.getMinY() , innerRect.getMinY() );
		newRegion[eLinkBottom] = region;
	}
	else
	{
		newRegion[eLinkBottom] = NULL;
	}

	if ( outerRect.getMinX() < innerRect.getMinX() )
	{
		Region* region = creatRegion();
		region->getRect().setValue(
			outerRect.getMinX() ,innerRect.getMinX() ,
			innerRect.getMinY() ,innerRect.getMaxY() );
		newRegion[eLinkLeft] = region;
	}
	else
	{
		newRegion[eLinkLeft] = NULL;
	}

	if ( outerRect.getMaxX() > innerRect.getMaxX() )
	{
		Region* region = creatRegion();
		region->getRect().setValue( 
			innerRect.getMaxX() ,outerRect.getMaxX() ,
			innerRect.getMinY() ,innerRect.getMaxY() );
		newRegion[eLinkRight] = region;
	}
	else
	{
		newRegion[eLinkRight] = NULL;
	}

	unsigned leftBit = BIT( eLinkLeft );
	unsigned rightBit = BIT( eLinkRight );

	if ( newRegion[eLinkTop] )
	{
		updatePortal( oldRegion  , *newRegion[eLinkTop] , 
			BIT( eLinkTop )|BIT( eLinkLeft )|BIT( eLinkRight ) , beRemoveOld );

		if (  newRegion[eLinkLeft]  )
			buildPortal( *newRegion[eLinkTop] , *newRegion[eLinkLeft] , eLinkBottom );
		if (  newRegion[eLinkRight] )
			buildPortal( *newRegion[eLinkTop] , *newRegion[eLinkRight] , eLinkBottom );
	}
	else
	{
		leftBit |= BIT( eLinkTop );
		rightBit |= BIT( eLinkTop );
	}

	if ( newRegion[eLinkBottom] )
	{
		updatePortal( oldRegion  , *newRegion[eLinkBottom] , 
			BIT( eLinkBottom )|BIT( eLinkLeft )|BIT( eLinkRight ) , beRemoveOld );

		if (  newRegion[eLinkLeft] )
			buildPortal( *newRegion[eLinkBottom] , *newRegion[eLinkLeft]  , eLinkTop );
		if (  newRegion[eLinkRight] )
			buildPortal( *newRegion[eLinkBottom] , *newRegion[eLinkRight] , eLinkTop );
	}
	else
	{
		leftBit |= BIT( eLinkBottom );
		rightBit |= BIT( eLinkBottom );
	}


	if ( newRegion[eLinkLeft] )
	{
		updatePortal( oldRegion  , *newRegion[eLinkLeft] , leftBit , beRemoveOld );
	}

	if ( newRegion[ eLinkRight ] )
	{
		updatePortal( oldRegion  , *newRegion[eLinkRight] , rightBit , beRemoveOld );
	}

}

bool RegionManager::split( Region& oldRegion , rect_t& splitRect , Region* newRegion[] , bool beRemoveOld )
{
	rect_t& outerRect = oldRegion.getRect();

	rect_t innerRect;
	if ( !getInteractionRect( outerRect , splitRect , innerRect ) )
		return false;

	int dx = outerRect.xRange.length() - innerRect.xRange.length();
	int dy = outerRect.yRange.length() - innerRect.yRange.length();


	if ( dx > dy )
		splitRegionVertical( oldRegion , newRegion ,  outerRect , innerRect , beRemoveOld );
	else
		splitRegionHorizontal( oldRegion , newRegion ,  outerRect , innerRect , beRemoveOld );

	oldRegion.rect = innerRect;
	oldRegion.type = Region::eTempBlock;

	return true;
}


void RegionManager::mergeRegion( Region& region , RegionList* rlist , int maxRepeat )
{

	int count = 0;
	while(1)
	{
		Region* mergeRegion = doMergePosibleRegion( region );

		if ( mergeRegion == NULL )
			break;

		if ( rlist )
			rlist->remove( mergeRegion );

		destoryRegion( mergeRegion );

		++count;
		if ( count >= maxRepeat )
			break;
	};
	

	for( Region::PortalList::iterator iter = region.portals.begin();
		iter != region.portals.end();  )
	{
		Portal* portal = *iter;

		if ( portal->con[0] == portal->con[1] )
		{
			destoryPortal( portal );
			iter = region.portals.erase( iter );
		}
		else
		{
			++iter;
		}
	}

	checkPortalVaild( region );
}

bool RegionManager::tryMergeBarRegion( Region& region1 , Region& region2 , Portal& portal , int idxRange )
{
	int idxOther = ( idxRange + 1 ) % 2;
	range_t const& rang1 = region1.getRect().getRange( idxRange );
	range_t const& rang2 = region2.getRect().getRange( idxRange );

	int len1 = rang1.length();
	if ( len1 <  region1.getRect().getRange( idxOther ).length() )
		return false;
	int len2 = rang2.length();
	if ( len2 <  region2.getRect().getRange( idxOther ).length() )
		return false;

	Region* rs;
	Region* rm;

	if ( len1 > len2 )
	{
		rs = &region1;
		rm = &region2;	
	}
	else
	{
		rs = &region2;
		rm = &region1;	
	}

	return false;
}



Region* RegionManager::doMergePosibleRegion( Region& region )
{
	for( Region::PortalList::iterator iter = region.portals.begin();
		iter != region.portals.end(); )
	{
		Portal* portal = *iter;
		Region* other = ( portal->con[0] == &region ) ? portal->con[1] : portal->con[0];

		assert( other != &region );
		if (  other->type != region.type )
		{
			++iter;
			continue;
		}

		int idxRange = portal->dir % 2;

		range_t const& rang1 = region.getRect().getRange( idxRange );
		range_t const& rang2 = other->getRect().getRange( idxRange );

		if ( rang1.max == rang2.max )
		{
			if ( rang1.min == rang2.min )
			{
				doMergeRegion( region , other , portal , idxRange );
				iter = region.portals.erase( iter );
				return other;
			}
			else
			{
				if ( tryMergeBarRegion( region , *other , *portal , idxRange ) )
				{
					return NULL;
				}
			}
		}
		else if ( rang1.min == rang2.min )
		{
			if ( tryMergeBarRegion( region , *other , *portal , idxRange ) )
			{
				return NULL;
			}
		}

		++iter;
	}

	return NULL;
}

void RegionManager::doMergeRegion( Region& region , Region* other , Portal* portal , int idxRange )
{
	assert( region.getRect().getRange( idxRange ) == other->getRect().getRange( idxRange ) );
	
	Portal* nPortal[2] = { NULL , NULL };
	Region* nRegin[2] = { NULL , NULL };

	//find n-portal 
	int count = 0;
	for( Region::PortalList::iterator iter3 = region.portals.begin();
		iter3 != region.portals.end(); ++iter3 )
	{
		Portal* portal3 = *iter3;
		if ( ( portal3->dir % 2) != idxRange &&
			( portal3->range.min == portal->value ||
			  portal3->range.max == portal->value ) )
		{
			int idx = portal3->getConnectDir( region ) / 2;
			nPortal[ idx ] = portal3;
			nRegin[ idx ] = portal3->getOtherRegion( region );
			++ count;
			if ( count == 2 )
				break;
		}
	}


	//remove con portal and merge n-portal
	for( Region::PortalList::iterator iter2 = other->portals.begin();
		 iter2 != other->portals.end();)
	{
		Portal* portal2 = *iter2;

		if ( portal2 == portal )
		{
			iter2 = other->portals.erase( iter2 );
		}
		else 
		{
			if (  ( portal2->dir % 2) != idxRange &&
				( portal2->range.min == portal->value ||
				  portal2->range.max == portal->value ) )
			{
				int idx = portal2->getConnectDir( *other ) / 2;
				if ( portal2->getOtherRegion( *other ) == nRegin[ idx ] )
				{
					if ( nPortal[idx]->range.max == portal2->range.min )
						nPortal[idx]->range.max = portal2->range.max;
					else
						nPortal[idx]->range.min = portal2->range.min;

					iter2 = other->portals.erase( iter2 );
					nRegin[idx]->portals.remove( portal2 ); 
					destoryPortal( portal2 );
					continue;
				}
			}

			++iter2;
			portal2->replace( *other , region );
		}
	}

	region.portals.splice( region.portals.begin() , other->portals );

	int idxMerge = ( idxRange + 1 ) % 2;

	range_t& range1 = region.getRect().getRange( idxMerge ); 
	range_t& range2 = other->getRect().getRange( idxMerge ); 

	assert( range1.max == range2.min || range1.min == range2.max );
	if ( range1.max == range2.min )
		range1.max = range2.max;
	else
		range1.min = range2.min;

	destoryPortal( portal );
}

void RegionManager::checkPortalVaild( Region& region )
{
	for( Region::PortalList::iterator iter = region.portals.begin();
		iter != region.portals.end(); ++iter )
	{
		Portal* portal = *iter;
		if ( portal->con[0] == portal->con[1] )
		{
			int i = 1;
			assert(0);
		}
		if ( ( portal->con[0] != &region && portal->con[1] != &region ) ||
			   portal->con[0] == portal->con[1] )
		{
			int i = 1;
			assert(0);
		}
	}

}

void RegionManager::updatePortal( Region& origin , Region& to , unsigned dirBit , bool beRemoveOld )
{
	for( Region::PortalList::iterator iter = origin.portals.begin();
		iter != origin.portals.end() ; )
	{
		Portal* portal = *iter;
		int conDir = portal->getConnectDir( origin );

		if ( !( BIT( conDir ) & dirBit ) )
		{
			++iter;
			continue;
		}

		rect_t& rect = to.getRect();
		range_t& range = rect.getRange( conDir % 2 );

		range_t crossRange;
		if ( portal->range.max <= range.max && portal->range.min >= range.min )
		{
			++iter;
			portal->replace( origin , to );

		}
		else if ( calcCrossRange( range , portal->range , crossRange ) )
		{
			if ( beRemoveOld )
			{
				++iter;
				portal->replace( origin , to );
				portal->range  = crossRange;
			}
			else
			{   
				
				if ( crossRange.max == portal->range.max )
				{ // |-----------|xxxxxxxxxxx|==========|
					portal->range.max = crossRange.min;
				}
				else if ( crossRange.min == portal->range.min )
				{ // |===========|xxxxxxxxxxx|----------|
					portal->range.min = crossRange.max;
				}
				else  
				{ // |-----------|xxxxxxxxxxx|----------|
					portal->range.min = crossRange.max;

					Portal* newPortal = createPortal();

					newPortal->connect( *portal->con[0] , *portal->con[1] , portal->dir );
					newPortal->value  = portal->value;
					newPortal->range.max = portal->range.max;
					newPortal->range.min = crossRange.max;
				}

				{
					Portal* newPortal = createPortal();

					newPortal->value = portal->value;
					newPortal->range = crossRange;

					if ( portal->con[0] == &origin)
						newPortal->connect( to , *portal->con[1] , conDir );
					else
						newPortal->connect( to , *portal->con[0] , conDir );
				}
				++iter;
			}

		}
		else
		{
			++iter;
		}

	}

	//checkPortalVaild( to );
}

Portal* RegionManager::buildPortal( Region& bP , Region& sP , DirType dir )
{
	Portal* portal = createPortal();

	rect_t& rect = sP.getRect();
	portal->range = rect.getRange( dir % 2 );

	switch( dir )
	{
	case eLinkTop:
		portal->value = rect.yRange.min;
		break;
	case eLinkBottom:
		portal->value = rect.yRange.max;
		break;
	case eLinkRight:
		portal->value = rect.xRange.min;
		break;
	case eLinkLeft:
		portal->value = rect.xRange.max;
		break;
	}

	portal->connect( bP , sP , dir );

	return portal;
}

RegionHandle RegionManager::productBlock( rect_t& splitRect )
{
	RegionList deleteList;
	RegionList newList;

	RegionList::iterator iter = mRegionList.begin();
	RegionList::iterator iterEnd = mRegionList.end();

	int coutSplit = 0;

	while( iter != iterEnd )
	{
		Region* rt = *iter;
		Region* newRegion[4];


		if ( split( *rt , splitRect , newRegion , false ) )
		{
			++coutSplit;
			//mHoleList.push_back( rt );

			deleteList.push_back( rt );
			iter = mRegionList.erase( iter );

			for( int i = 0 ; i < 4 ; ++i )
			{
				if ( !newRegion[i] )
					continue;

				buildPortal( *newRegion[ i ] , *rt , ( i + 2 ) % 4 );
				newList.push_back( newRegion[i] );
			}
		}
		else
		{
			++iter;
		}
	}
	

	for ( iter = deleteList.begin() ; iter != deleteList.end(); ++iter )
	{
		Region* region = *iter;
		mergeRegion( *region , &deleteList , 4 );
	}

	for ( iter = deleteList.begin() ; iter != deleteList.end(); ++iter )
	{
		(*iter)->type = Region::eBlock;
	}
	mBlockList.splice( mBlockList.begin() , deleteList );

	for ( iter = newList.begin() ; iter != newList.end(); ++iter )
	{
		Region* region = *iter;
		mergeRegion( *region , &newList , 4 );
	}

	return mBlockList.begin();
}
