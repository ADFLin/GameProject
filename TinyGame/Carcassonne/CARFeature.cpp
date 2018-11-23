#include "CAR_PCH.h"
#include "CARFeature.h"

#include "CARPlayer.h"
#include "CARGameplaySetting.h"
#include "CARParamValue.h"
#include "CARLevelActor.h"
#include "CARWorldTileManager.h"
#include "CARDebug.h"

#include <algorithm>

namespace CAR
{
	int const DefaultActorMaskNum = 3;

	int FeatureBase::getActorPutInfoInternal(int playerId , ActorPos const& actorPos , MapTile& mapTile, unsigned actorMasks[] , int numMask , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorTypeMask = 0;
		unsigned actorTypeMaskOther = 0;
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];
			if ( actor->owner == nullptr )
				continue;

			if ( mActors[i]->owner->getId() != playerId )
				actorTypeMaskOther |= BIT(mActors[i]->type);
			else
				actorTypeMask |= BIT(mActors[i]->type);
		}

		if ( actorTypeMask == 0 && actorTypeMaskOther != 0 )
			return 0;

		ActorPosInfo info;
		info.actorTypeMask = 0;
		for( int i = 0 ; i < numMask ; ++i )
		{
			if ( actorMasks[i] == 0 )
				continue;
			if ( actorTypeMask & actorMasks[i] )
				continue;

			info.actorTypeMask = actorMasks[i];
			break;
		}

		if ( info.actorTypeMask == 0 )
			return 0;

		info.mapTile = &mapTile;
		info.pos   = actorPos;
		info.group = group;
		outInfo.push_back( info );
		return 1;
	}

	int FeatureBase::getDefaultActorPutInfo(int playerId , ActorPos const& actorPos , MapTile& mapTile, unsigned actorMasks[] , std::vector< ActorPosInfo >& outInfo )
	{
		actorMasks[0] |= BIT(ActorType::eMeeple) | BIT( ActorType::ePhantom ) | BIT( ActorType::eBigMeeple );
		return getActorPutInfoInternal( playerId , actorPos , mapTile , actorMasks , DefaultActorMaskNum , outInfo );
	}

	void FeatureBase::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		assert( other.type == type );
		assert( group != -1 && other.group != -1 );
		while( LevelActor* actor = other.popActor() )
		{
			actor->feature = nullptr;
			addActor( *actor );
		}
		MergeData( mapTiles , other.mapTiles );
	}

	void FeatureBase::addActor( LevelActor& actor )
	{
		if ( actor.feature != nullptr )
		{
			actor.feature->removeActor( actor );
		}
		actor.feature = this;
		mActors.push_back( &actor );
	}

	void FeatureBase::removeActor( LevelActor& actor )
	{
		assert( actor.feature == this );
		actor.feature = nullptr;
		mActors.erase( std::find( mActors.begin() , mActors.end() , &actor ) );
	}

	LevelActor* FeatureBase::removeActorByIndex(int index)
	{
		assert(  0 <= index && index < mActors.size() );
		LevelActor* actor = mActors[ index ];
		actor->feature = nullptr;
		mActors.erase( mActors.begin() + index );
		return actor;
	}

	int FeatureBase::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		initFeatureScoreInfo( scoreInfos );
		addMajority( scoreInfos );
		int numPlayer = evalMajorityControl( scoreInfos );
		for( int i = 0 ; i < numPlayer ; ++i )
		{
			scoreInfos[i].score = calcPlayerScore( scoreInfos[i].playerId );
		}
		return numPlayer;
	}

	int FeatureBase::evalMajorityControl(std::vector< FeatureScoreInfo >& featureControls)
	{
		struct CmpFun
		{
			CmpFun()
			{
				useHomeRule = false;
			}
			bool operator() ( FeatureScoreInfo const& f1 , FeatureScoreInfo const& f2 ) const
			{
				if ( f1.hillFollowerCount || f2.hillFollowerCount )
				{
					if ( useHomeRule )
						return f1.hillFollowerCount >= f2.hillFollowerCount;
					else
						return f1.hillFollowerCount != 0;
				}
				return f1.majority > f2.majority;
			}

			bool isEqual( FeatureScoreInfo const& f1 , FeatureScoreInfo const& f2 ) const
			{
				if ( useHomeRule )
				{
					if ( f1.hillFollowerCount != 0 && f1.hillFollowerCount == f2.hillFollowerCount )
						return true;
				}
				else
				{
					if ( f1.hillFollowerCount != 0 && f2.hillFollowerCount != 0 )
						return true;
				}

				return f1.majority == f2.majority;
			}

			bool useHomeRule;
		};


		CmpFun fun;
		std::sort( featureControls.begin() , featureControls.end() , fun );

		int numPlayer = 0;
		if ( featureControls[0].majority != 0 )
		{
			++numPlayer;
			for( int i = 1; i < featureControls.size() ; ++i )
			{
				if ( fun.isEqual( featureControls[0] , featureControls[i] ) == false )
					break;
				++numPlayer;
			}
		}
		return numPlayer;
	}

	int FeatureBase::getMajorityValue( ActorType actorType )
	{
		switch( actorType )
		{
		case ActorType::eMeeple:    return 1; break;
		case ActorType::eBigMeeple: return 2; break;
		case ActorType::eWagon:     return 1; break;
		case ActorType::eMayor:
			{
				assert( type == FeatureType::eCity );
				CityFeature* city = static_cast< CityFeature* >( this );
				return city->getSideContentNum( SideContent::ePennant );
			}
			break;
		}
		return 0;
	}

	void FeatureBase::addMajority( std::vector< FeatureScoreInfo >& scoreInfos )
	{
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];
			FeatureScoreInfo& info = scoreInfos[ actor->owner->getId() ];
			info.majority += getMajorityValue( actor->type );
			if ( mSetting->isFollower( actor->type ) && actor->mapTile->haveHill )
				info.hillFollowerCount += 1;
		}
	}

	void FeatureBase::initFeatureScoreInfo(std::vector< FeatureScoreInfo > &scoreInfos)
	{
		scoreInfos.resize( MaxPlayerNum );
		for( int i = 0 ; i < MaxPlayerNum ; ++i )
		{
			scoreInfos[i].playerId = i;
			scoreInfos[i].majority = 0;
			scoreInfos[i].hillFollowerCount = 0;
		}
	}

	bool FeatureBase::testInRange(Vec2i const& min , Vec2i const& max)
	{
		for( MapTileSet::iterator iter = mapTiles.begin() , itEnd = mapTiles.end() ;
			iter != itEnd ; ++iter )
		{
			MapTile* mapTile = *iter;
			if ( isInRange( mapTile->pos , min , max ) )
				return true;
		}
		return false;
	}

	SideFeature::SideFeature()
	{
		openCount = 0;
		halfSepareteCount = 0;
	}

	void SideFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		SideFeature& otherData = static_cast< SideFeature& >( other );

		openCount += otherData.openCount;
		halfSepareteCount += otherData.halfSepareteCount;

		unsigned mask = putData.getSideLinkMask( meta );
		for( auto iter = TBitMaskIterator<FDir::TotalNum>(mask); iter; ++iter )
		{
			SideNode* nodeCon = putData.sideNodes[iter.index].outConnect;
			if ( nodeCon && nodeCon->group == other.group )
			{
				openCount -= 2;
				assert( openCount >= 0 );
			}
		}
		int idx = nodes.size();
		MergeData( nodes , otherData.nodes );
		for( int i = idx ; i < nodes.size() ; ++i )
			nodes[i]->group = group;
	}

	bool SideFeature::checkNodesConnected() const
	{
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			if ( nodes[i]->outConnect == nullptr )
				return false;
		}
		return true;
	}

	void SideFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		addMapTile( mapData );
		for( auto iter = TBitMaskIterator<FDir::TotalNum>(dirMask ); iter ; ++iter  )
		{
			SideNode& node = mapData.sideNodes[iter.index];
			if( node.group != ERROR_GROUP_ID )
			{
				assert(group == node.group);
				continue;
			}

			node.group = group;
			nodes.push_back( &node );

			if ( node.outConnect )
			{
				int conGroup = node.outConnect->group;
				if ( conGroup == group )
				{
					--openCount;
					continue;
				}
				else if ( conGroup == ABBEY_GROUP_ID )
				{
					continue;
				}
			}
			++openCount;
		}
		
	}

	int SideFeature::calcOpenCount()
	{
		int result = 0;
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			for( int j = i + 1 ; j < nodes.size() ; ++j )
			{
				assert( nodes[i] != nodes[j]);
			}
			SideNode* node = nodes[i];
			if ( node->outConnect == nullptr )
				++result;
		}
		return result;
	}

	int SideFeature::getSideContentNum(unsigned contentMask)
	{
		int result = 0;
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			SideNode* node = nodes[i];

			MapTile const* mapTile = node->getMapTile();
			uint32 content = mapTile->getSideContnet( node->index ) & contentMask;
			while ( content )
			{
				content &= ~FBit::Extract( content );
				++result;
			}
		}
		return result;
	}

	void SideFeature::generateRoadLinkFeatures( GroupSet& outFeatures )
	{
		for( int i = 0 ; i < nodes.size() ; ++i )
		{
			SideNode* node = nodes[i];
			MapTile const* mapTile = node->getMapTile();
			unsigned roadMask = mapTile->getRoadLinkMask( node->index );

			CAR_LOG("road Mask = %u" , roadMask );

			if ( roadMask == 0 )
				continue;
			
			if ( roadMask & TilePiece::CenterMask )
			{
				//#TODO: use feature type check
				switch ( mapTile->getTileContent() & TileContent::FeatureMask )
				{
				case TileContent::eCloister:
					outFeatures.insert( mapTile->group );
					break;
				}
			}
			else
			{
				int dir;
				while( FBit::MaskIterator< FDir::TotalNum >(roadMask,dir) )
				{
					SideNode const& linkNode = mapTile->sideNodes[dir];
					if ( linkNode.group == group )
						continue;

					if ( linkNode.group == -1 )
					{
						CAR_LOG("Warnning: No Link Feature In Road Link");
					}
					else
					{
						outFeatures.insert( linkNode.group );
					}
				}
			}
		}
	}

	void SideFeature::addAbbeyNode(MapTile& mapData , int dir )
	{
		--openCount;
	}

	bool SideFeature::getActorPos(MapTile const& mapTile , ActorPos& actorPos)
	{
		for (int i = 0 ; i < FDir::TotalNum ;++i )
		{
			SideNode const& node = mapTile.sideNodes[i];
			if ( node.group == group )
			{
				actorPos.type = ActorPos::eSideNode;
				actorPos.meta = i;
				return true;
			}
		}
		return false;
	}


	RoadFeature::RoadFeature()
	{
		haveInn    = false;
	}

	int RoadFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( ActorType::eWagon ) , BIT( ActorType::eBuilder ) , 0 };
		return getDefaultActorPutInfo( playerId , ActorPos( ActorPos::eSideNode , posMeta ) , mapTile , actorMasks , outInfo );
	}

	void RoadFeature::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );
		RoadFeature& otherData = static_cast< RoadFeature& >( other );

	}

	void RoadFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		BaseClass::addNode( mapData , dirMask , linkNode );

		if ( mSetting->have( Rule::eInn ) )
		{
			for( auto iter = TBitMaskIterator< FDir::TotalNum >( dirMask ) ; iter ; ++iter )
			{
				if ( mapData.getSideContnet( iter.index ) & SideContent::eInn )
				{
					haveInn = true;
					break;
				}
			}
		}
	}

	bool RoadFeature::checkComplete()
	{
		return openCount == 0;
	}

	int RoadFeature::calcPlayerScore(int playerId)
	{
		int numTile = mapTiles.size();

		int factor = CAR_PARAM_VALUE(NonCompleteFactor);
		if ( getSetting().have( Rule::eInn ) && haveInn )
		{
			if ( checkComplete() )
				factor += CAR_PARAM_VALUE(InnAddtitionFactor);
			else
				factor = 0;
		}
		else if ( checkComplete() )
		{
			factor = CAR_PARAM_VALUE(RoadFactor);
		}
		return numTile * factor;
	}

	CityFeature::CityFeature()
	{
		haveCathedral = false;
		isCastle = false;
	}


	int CityFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( ActorType::eWagon ) | BIT( ActorType::eMayor ) , BIT( ActorType::eBuilder ) , 0 };
		return getDefaultActorPutInfo( playerId , ActorPos( ActorPos::eSideNode , posMeta ) , mapTile , actorMasks ,outInfo );
	}

	void CityFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		assert( other.type == type );
		CityFeature& otherData = static_cast< CityFeature& >( other );
		MergeData( linkedFarms , otherData.linkedFarms );
		for( std::set< FarmFeature* >::iterator iter = otherData.linkedFarms.begin() , itEnd = otherData.linkedFarms.end();
			iter != itEnd ; ++iter )
		{
			FarmFeature* farm = *iter;
			farm->linkedCities.erase( &otherData );
		}
	}

	void CityFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		BaseClass::addNode( mapData , dirMask , linkNode );

		if ( mSetting->have( Rule::eCathedral ) )
		{
			if ( mapData.have( TileContent::eCathedral ) )
			{
				haveCathedral = true;
			}
		}
	}

	bool CityFeature::checkComplete()
{
		return openCount == 0 && halfSepareteCount == 0;
	}

	int CityFeature::calcPlayerScore( int playerId )
	{
		int numTile = mapTiles.size();
		int factor = CAR_PARAM_VALUE(NonCompleteFactor);
		int pennatFactor = CAR_PARAM_VALUE(PennatNonCompletFactor);
		if ( getSetting().have(Rule::eCathedral) && haveCathedral )
		{
			if ( checkComplete() )
			{
				factor = CAR_PARAM_VALUE(CityFactor) + CAR_PARAM_VALUE(CathedralAdditionFactor);
				pennatFactor = CAR_PARAM_VALUE(PennatFactor) + CAR_PARAM_VALUE(CathedralAdditionFactor);
			}
			else
			{
				factor = 0;
				pennatFactor = 0;
			}
		}
		else if ( checkComplete() )
		{
			factor = CAR_PARAM_VALUE(CityFactor);
			pennatFactor = CAR_PARAM_VALUE(PennatFactor);
		}

		int numPennats = getSideContentNum( SideContent::ePennant );

		return numTile * factor + numPennats * pennatFactor;
	}

	bool CityFeature::isSamllCircular()
	{
		assert( checkComplete() );
		if ( nodes.size() != 2 )
			return false;
		SideNode* nodeA = nodes[0];
		if ( nodeA->getMapTile()->isSemiCircularCity( nodeA->index ) == false )
			return false;
		SideNode* nodeB = nodes[1];
		if ( nodeB->getMapTile()->isSemiCircularCity( nodeB->index ) == false )
			return false;
		return true;
	}

	int CityFeature::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		if ( isCastle )
			return -1;
		return BaseClass::calcScore( scoreInfos );
	}

	FarmFeature::FarmFeature()
	{
		haveBarn = false;
	}

	int FarmFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo)
	{
		if ( haveActorMask( BIT( ActorType::eBarn ) ) )
			return 0;
		unsigned actorMasks[DefaultActorMaskNum] = { 0 , BIT( ActorType::ePig ) , 0};
		return getDefaultActorPutInfo( playerId , ActorPos( ActorPos::eFarmNode , posMeta ) , mapTile , actorMasks , outInfo );
	}

	void FarmFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		FarmFeature& otherData = static_cast< FarmFeature& >( other );
		int idx = nodes.size();
		MergeData( nodes , otherData.nodes );
		MergeData( linkedCities , otherData.linkedCities );
		for( std::set< CityFeature* >::iterator iter = otherData.linkedCities.begin() , itEnd = otherData.linkedCities.end();
			iter != itEnd ; ++iter )
		{
			CityFeature* city = *iter;
			city->linkedFarms.erase( &otherData );
		}
		for( int i = idx ; i < nodes.size() ; ++i )
			nodes[i]->group = group;
	}

	void FarmFeature::addNode( MapTile& mapData , unsigned idxMask , FarmNode* linkNode)
	{
		addMapTile( mapData );

		for( auto iter = TBitMaskIterator< TilePiece::NumFarm >(idxMask); iter; ++iter )
		{
			FarmNode& node = mapData.farmNodes[iter.index];
			if( node.group != ERROR_GROUP_ID )
			{
				assert(group == node.group);
				continue;
			}
			node.group = group;
			nodes.push_back( &node );
		}
	}

	int FarmFeature::calcPlayerScore( int playerId )
	{
		int factor = CAR_PARAM_VALUE(FarmFactorV3);
		switch ( mSetting->getFarmScoreVersion() )
		{
		case 1: factor = CAR_PARAM_VALUE(FarmFactorV1); break;
		case 2: factor = CAR_PARAM_VALUE(FarmFactorV2); break;
		case 3: factor = CAR_PARAM_VALUE(FarmFactorV3); break;
		}

		if ( haveBarn )
			factor += CAR_PARAM_VALUE(BarnAddtionFactor);

		return calcPlayerScoreInternal(playerId, factor);

	}

	int FarmFeature::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		switch( mSetting->getFarmScoreVersion() )
		{
		case 1:
		case 2:
			return -1;
		case 3:
		default:
			return BaseClass::calcScore( scoreInfos );
			break;
		}

		return 0;
	}

	int FarmFeature::calcPlayerScoreInternal(int playerId, int farmFactor)
	{
		int factor = farmFactor;
		int numCityFinish = 0;
		int numCastle = 0;

		for( std::set< CityFeature* >::iterator iter = linkedCities.begin() , itEnd = linkedCities.end();
			iter != itEnd ; ++iter )
		{
			CityFeature* city = *iter;
			if ( city->checkComplete() )
			{
				++numCityFinish;
				if ( city->isCastle )
					++numCastle;
			}
		}

		if ( mSetting->have(Rule::ePig) )
		{
			if ( havePlayerActor( playerId , ActorType::ePig ) )
				factor += CAR_PARAM_VALUE(PigAdditionFactor);
		}

		return numCityFinish * factor + numCastle;
	}

	int FarmFeature::calcPlayerScoreByBarnRemoveFarmer(int playerId)
	{
		return calcPlayerScoreInternal( playerId , (haveBarn)?(CAR_PARAM_VALUE(BarnRemoveFarmerFactor) ):(CAR_PARAM_VALUE(FarmFactorV3) ) );
	}

	CloisterFeature::CloisterFeature()
	{
		
	}

	int CloisterFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( ActorType::eWagon ) , 0 , 0 };
		return getDefaultActorPutInfo( playerId , ActorPos( ActorPos::eTile , posMeta ) , mapTile, actorMasks , outInfo );
	}

	void CloisterFeature::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		return;
	}

	int CloisterFeature::calcPlayerScore( int playerId )
	{
		int result = ( neighborTiles.size() + 1 ) * CAR_PARAM_VALUE( CloisterFactor );
		if ( checkComplete() )
		{
			if ( mSetting->have( Rule::eUseVineyard) )
			{
				for( int i = 0; i < neighborTiles.size(); ++i )
				{
					MapTile* mapTile = neighborTiles[i];
					if( mapTile->have( TileContent::eVineyard ) )
					{
						result += CAR_PARAM_VALUE(VineyardAdditionScore);
					}
				}
			}
		}
		return result;
	}

	int CloisterFeature::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];

			int majority = getMajorityValue( actor->type );

			if ( majority > 0 )
			{
				scoreInfos.resize(1);
				scoreInfos[0].playerId = actor->owner->getId();
				scoreInfos[0].majority = majority;
				scoreInfos[0].score = calcPlayerScore(scoreInfos[0].playerId);
				return 1;
			}
		}
		return 0;
	}

	void CloisterFeature::generateRoadLinkFeatures( GroupSet& outFeatures )
	{
		assert( mapTiles.empty() == false );
		MapTile* mapTile = *mapTiles.begin();

		unsigned roadMask = mapTile->calcSideRoadLinkMeskToCenter() & ~TilePiece::CenterMask;
		for( auto iter = TBitMaskIterator< FDir::TotalNum >(roadMask); iter; ++iter )
		{
			int dir = iter.index;
			SideNode* linkNode = mapTile->sideNodes[dir].outConnect;
			if ( linkNode == nullptr )
				continue;

			if ( linkNode->group == -1 )
			{
				CAR_LOG("Warnning: No Link Feature In Road Link");
			}
			else
			{
				outFeatures.insert( linkNode->group );
			}
		}
	}

	bool CloisterFeature::updateForNeighborTile(MapTile& tile)
	{
		neighborTiles.push_back( &tile );
		return true;
	}

	bool CloisterFeature::getActorPos(MapTile const& mapTile , ActorPos& actorPos)
	{
		if ( mapTile.group == group )
		{
			actorPos.type = ActorPos::eTile;
			actorPos.meta = 0;
			return true;
		}
		return false;
	}

	GermanCastleFeature::GermanCastleFeature()
	{

	}

	int GermanCastleFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( ActorType::eWagon ) , 0 , 0 };
		return getActorPutInfoInternal( playerId , ActorPos( ActorPos::eSideNode , posMeta ) , mapTile, actorMasks , ARRAY_SIZE( actorMasks ) , outInfo );
	}

	void GermanCastleFeature::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		assert( 0 );
		return;
	}

	int GermanCastleFeature::calcPlayerScore( int playerId )
	{
		int result = ( neighborTiles.size() + 1 ) * CAR_PARAM_VALUE(CloisterFactor);
		if ( checkComplete() )
		{
			if( mSetting->have(Rule::eUseVineyard) )
			{
				for( int i = 0; i < neighborTiles.size(); ++i )
				{
					MapTile* mapTile = neighborTiles[i];
					if( mapTile->have( TileContent::eVineyard ) )
					{
						result += CAR_PARAM_VALUE(VineyardAdditionScore);
					}
				}
			}
		}
		return result;
	}

	int GermanCastleFeature::calcScore(std::vector< FeatureScoreInfo >& scoreInfos)
	{
		for( int i = 0 ; i < mActors.size() ; ++i )
		{
			LevelActor* actor = mActors[i];

			int majority = getMajorityValue( actor->type );

			if ( majority > 0 )
			{
				scoreInfos.resize(1);
				scoreInfos[0].playerId = actor->owner->getId();
				scoreInfos[0].majority = majority;
				scoreInfos[0].score = calcPlayerScore( actor->owner->getId() );
				return 1;
			}
		}
		return 0;
	}

	void GermanCastleFeature::generateRoadLinkFeatures( GroupSet& outFeatures )
	{
		assert( mapTiles.empty() == false );
		MapTile* mapTile = *mapTiles.begin();

		unsigned roadMask = mapTile->calcSideRoadLinkMeskToCenter() & ~TilePiece::CenterMask;
		for( auto iter = TBitMaskIterator< FDir::TotalNum >(roadMask); iter; ++iter )
		{
			int dir = iter.index;
			SideNode* linkNode = mapTile->sideNodes[dir].outConnect;
			if ( linkNode == nullptr )
				continue;

			if ( linkNode->group == -1 )
			{
				CAR_LOG("Warnning: No Link Feature In Road Link");
			}
			else
			{
				outFeatures.insert( linkNode->group );
			}
		}
	}


	bool GermanCastleFeature::updateForNeighborTile(MapTile& tile)
	{
		return AddUnique( neighborTiles , &tile );
	}

	bool GermanCastleFeature::getActorPos(MapTile const& mapTile , ActorPos& actorPos)
	{
		for( int i = 0 ; i < TilePiece::NumSide ; ++i )
		{
			if ( mapTile.getSideGroup( i ) == group )
			{
				actorPos.type = ActorPos::eSideNode;
				actorPos.meta = i;
				return true;
			}
		}
		return false;
	}
}//namespace CAR