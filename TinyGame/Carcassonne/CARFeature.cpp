#include "CAR_PCH.h"
#include "CARFeature.h"

#include "CARGameplaySetting.h"
#include "CARParamValue.h"
#include "CARLevelActor.h"
#include "CARPlayer.h"
#include "CARWorldTileManager.h"
#include "CARDebug.h"

#include <algorithm>

namespace CAR
{
	int const DefaultActorMaskNum = 3;

	GameParamCollection& FeatureBase::GetParamCollection(FeatureBase& t)
	{
		return t.getSetting();
	}

	int FeatureBase::getActorPutInfoInternal(int playerId , ActorPos const& actorPos , MapTile& mapTile, unsigned actorMasks[] , int numMask , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorTypeMask = 0;
		unsigned actorTypeMaskOther = 0;
		for(LevelActor * actor : mActors)
		{
			if ( actor->ownerId == CAR_ERROR_PLAYER_ID )
				continue;

			if ( actor->ownerId != playerId )
				actorTypeMaskOther |= BIT(actor->type);
			else
				actorTypeMask |= BIT(actor->type);
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

	int FeatureBase::getDefaultActorPutInfo(int playerId, ActorPos const& actorPos, MapTile& mapTile, unsigned actorMasks[], std::vector< ActorPosInfo >& outInfo)
	{
		actorMasks[0] |= BIT(EActor::Meeple) | BIT( EActor::Phantom ) | BIT( EActor::BigMeeple );
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
		if (mSetting->isFollower(actor.type))
		{
			onAddFollower(actor);
		}
	}

	void FeatureBase::removeActor( LevelActor& actor )
	{
		assert( actor.feature == this );
		actor.feature = nullptr;
		actor.className = EFollowerClassName::Undefined;
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

	int FeatureBase::doCalcScore(GamePlayerManager& playerManager , FeatureScoreInfo& outResult)
	{
		int numController = generateController(outResult.controllerScores);
		for( int i = 0 ; i < numController ; ++i )
		{
			auto& controllerScore = outResult.controllerScores[i];
			controllerScore.score = calcPlayerScore( playerManager.getPlayer( controllerScore.playerId ) );
		}
		return numController;
	}

	int FeatureBase::calcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult)
	{
		outResult.feature = this;
		outResult.numController = doCalcScore(playerManager, outResult);
		return outResult.numController;
	}

	int FeatureBase::evalMajorityControl(std::vector< FeatureControllerScoreInfo >& controllerScores)
	{
		assert(!controllerScores.empty());

		struct CmpFunc
		{
			CmpFunc()
			{
				useHomeRule = false;
			}
			bool operator() ( FeatureControllerScoreInfo const& f1 , FeatureControllerScoreInfo const& f2 ) const
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

			bool isEqual( FeatureControllerScoreInfo const& f1 , FeatureControllerScoreInfo const& f2 ) const
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



		CmpFunc func;
		std::sort( controllerScores.begin() , controllerScores.end() , func );

		int numController = 0;
		if ( controllerScores[0].majority != 0 )
		{
			++numController;
			for( int i = 1; i < controllerScores.size(); ++i )
			{
				if( func.isEqual(controllerScores[0], controllerScores[i]) == false )
					break;
				++numController;
			}
		}
		return numController;
	}

	bool FeatureBase::haveTileContent(TileContentMask contentMask) const
	{
		for (auto mapTile : mapTiles)
		{
			if (mapTile->getTileContent() & contentMask)
				return true;
		}
		return false;
	}

	int FeatureBase::generateController(std::vector< FeatureControllerScoreInfo >& controllerScores)
	{
		generateMajority(controllerScores);
		if( controllerScores.empty() )
			return 0;
		return evalMajorityControl(controllerScores);
	}

	int FeatureBase::getMajorityValue(EActor::Type actorType)
	{
		switch( actorType )
		{
		case EActor::Meeple:    return 1; 
		case EActor::BigMeeple: return 2; 
		case EActor::Wagon:     return 1; 
		case EActor::Phantom:   return 1;
		case EActor::Mayor:
			{
				assert( type == FeatureType::eCity );
				CityFeature* city = static_cast< CityFeature* >( this );
				return city->getSideContentNum( SideContent::ePennant );
			}
			break;
		}
		return 0;
	}

	void FeatureBase::generateMajority( std::vector< FeatureControllerScoreInfo >& controllerScores )
	{
		auto FindController = [&controllerScores](PlayerId id)-> FeatureControllerScoreInfo*
		{
			for( auto& info : controllerScores )
			{
				if( info.playerId == id )
					return &info;
			}
			return nullptr;
		};

		for( LevelActor* actor : mActors )
		{
			int majorityValue = getMajorityValue(actor->type);
			if( majorityValue <= 0 )
				continue;

			FeatureControllerScoreInfo* pInfo = FindController(actor->ownerId);
			if ( pInfo == nullptr )
			{
				FeatureControllerScoreInfo newInfo;
				newInfo.playerId = actor->ownerId;
				newInfo.majority = 0;
				newInfo.hillFollowerCount = 0;
				newInfo.score = 0;
				controllerScores.push_back(newInfo);
				pInfo = &controllerScores.back();
			}

			pInfo->majority += majorityValue;
			if ( mSetting->isFollower( actor->type ) && actor->mapTile->haveHill )
				pInfo->hillFollowerCount += 1;
		}
	}

	bool FeatureBase::testInRectArea(Vec2i const& min , Vec2i const& max)
	{
		for(MapTile* mapTile : mapTiles)
		{
			if ( IsInRect( mapTile->pos , min , max ) )
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
		for(auto node : nodes)
		{
			if ( node->outConnect == nullptr )
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

	int SideFeature::getSideContentNum(SideContentMask contentMask)
	{
		int result = 0;
		for(SideNode* node : nodes)
		{
			MapTile const* mapTile = node->getMapTile();
			SideContentMask content = mapTile->getSideContnet( node->index ) & contentMask;
			while ( content )
			{
				content &= ~FBitUtility::ExtractTrailingBit( content );
				++result;
			}
		}
		return result;
	}

	static void GenerateSideFeatureRoadLinkFeatures_R(WorldTileManager& worldTileManager, MapTile const* mapTile , int skipGroup , int dir , GroupSet& outFeatures)
	{
		unsigned roadMask = mapTile->getRoadLinkMask(dir);
		CAR_LOG("road Mask = %u", roadMask);

		if( roadMask == 0 )
			return;

		if( roadMask & TilePiece::CenterMask )
		{
			//#TODO: use feature type check
			switch( mapTile->getTileContent() & TileContent::FeatureMask )
			{
			case BIT(TileContent::eCloister):
				outFeatures.insert(mapTile->group);
				break;
			}
		}

		int dir1;
		while( FBitUtility::MaskIterator< FDir::TotalNum >(roadMask, dir1) )
		{
			MapTile::SideNode const& linkNode = mapTile->sideNodes[dir1];
			if( linkNode.group == skipGroup )
				continue;

			if( linkNode.group == -1 )
			{
				if( linkNode.type == ESide::InsideLink )
				{
					switch( mapTile->getSideContnet(dir1) & SideContent::InternalLinkTypeMask )
					{
					case BIT(SideContent::eSchool):
						{
							Vec2i linkPos = FDir::LinkPos(mapTile->pos, dir1);
							MapTile* mapTileLink = worldTileManager.findMapTile(linkPos);

							if( mapTileLink )
							{
								int invDir = FDir::Inverse(dir1);
								assert(mapTileLink->sideNodes[invDir].type == ESide::InsideLink);
								assert((mapTileLink->getSideContnet(invDir) & SideContent::InternalLinkTypeMask) == BIT(SideContent::eSchool));

								unsigned roadMaskLink = mapTileLink->getRoadLinkMask(invDir);
								int dir2;
								while( FBitUtility::MaskIterator< FDir::TotalNum >(roadMaskLink, dir2) )
								{
									GenerateSideFeatureRoadLinkFeatures_R(worldTileManager, mapTile, skipGroup, dir2, outFeatures);
								}
							}
						}
						break;
					}
				}
				CAR_LOG("Waning: No Link Feature In Road Link");
			}
			else
			{
				outFeatures.insert(linkNode.group);
			}
		}
	}

	void SideFeature::generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures)
	{
		for(FeatureBase::SideNode* node : nodes)
		{
			MapTile const* mapTile = node->getMapTile();
			GenerateSideFeatureRoadLinkFeatures_R(worldTileManager, mapTile, group, node->index, outFeatures);
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
				actorPos = ActorPos::SideNode(i);
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
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( EActor::Wagon ) , BIT( EActor::Builder ) , 0 };
		return getDefaultActorPutInfo( playerId , ActorPos::SideNode( posMeta ) , mapTile , actorMasks , outInfo );
	}

	void RoadFeature::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );
		RoadFeature& otherData = static_cast< RoadFeature& >( other );

	}

	void RoadFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		BaseClass::addNode( mapData , dirMask , linkNode );

		if ( mSetting->have( ERule::Inn ) )
		{
			for( auto iter = TBitMaskIterator< FDir::TotalNum >( dirMask ) ; iter ; ++iter )
			{
				if ( mapData.getSideContnet( iter.index ) & BIT(SideContent::eInn) )
				{
					haveInn = true;
					break;
				}
			}
		}
	}

	bool RoadFeature::checkComplete() const
	{
		return openCount == 0;
	}

	int RoadFeature::calcPlayerScore(PlayerBase* player)
	{
		int numTile = mapTiles.size();

		int factor = CAR_PARAM_VALUE(NonCompleteFactor);
		if ( getSetting().have( ERule::Inn ) && haveInn )
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
		haveLaPorxada = false;
		isCastle = false;
	}


	int CityFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( EActor::Wagon ) | BIT( EActor::Mayor ) , BIT( EActor::Builder ) , 0 };
		return getDefaultActorPutInfo( playerId , ActorPos::SideNode( posMeta ) , mapTile , actorMasks ,outInfo );
	}

	void CityFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		assert( other.type == type );
		CityFeature& otherData = static_cast< CityFeature& >( other );

		haveCathedral |= otherData.haveCathedral;
		haveLaPorxada |= otherData.haveLaPorxada;
		MergeData( linkedFarms , otherData.linkedFarms );
		for( FarmFeature* farm : otherData.linkedFarms )
		{
			farm->linkedCities.erase( &otherData );
		}
	}

	void CityFeature::addNode(MapTile& mapData , unsigned dirMask , SideNode* linkNode)
	{
		BaseClass::addNode( mapData , dirMask , linkNode );

		if ( mSetting->have( ERule::Cathedral ) )
		{
			if ( mapData.have( TileContent::eCathedral ) )
			{
				haveCathedral = true;
			}
		}

		if( mSetting->have(ERule::LaPorxada) )
		{
			if( mapData.have(TileContent::eLaPorxada) )
			{
				haveLaPorxada = true;
			}
		}
	}

	bool CityFeature::checkComplete() const
{
		return openCount == 0 && halfSepareteCount == 0;
	}

	int CityFeature::calcPlayerScore(PlayerBase* player)
	{
		int numTile = mapTiles.size();
		int factor = CAR_PARAM_VALUE(NonCompleteFactor);
		int pennatFactor = CAR_PARAM_VALUE(PennatNonCompletFactor);
		if ( getSetting().have(ERule::Cathedral) && haveCathedral )
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
		else if ( checkComplete() || (getSetting().have(ERule::LaPorxada) && player && player->getFieldValue(EField::LaPorxadaFinishScoring)) )
		{
			factor = CAR_PARAM_VALUE(CityFactor);
			pennatFactor = CAR_PARAM_VALUE(PennatFactor);
		}

		if (isBesieged())
		{
			factor = std::max(0, factor - 1);
		}

		int numPennats = getSideContentNum( SideContent::ePennant );

		return numTile * factor + numPennats * pennatFactor;
	}

	bool CityFeature::isSamllCircular() const
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

	bool CityFeature::isBesieged() const
	{
		return haveTileContent(BIT(TileContent::eBesieger));
	}

	bool CityFeature::haveAdjacentCloister() const
	{
		for (auto farm : linkedFarms)
		{
			for (auto mapTile : farm->mapTiles)
			{
				if (mapTile->have(TileContent::eCloister))
					return true;
			}
		}
		return false;
	}

	int CityFeature::doCalcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult)
	{
		if ( isCastle )
			return -1;
		return BaseClass::doCalcScore( playerManager , outResult);
	}

	FarmFeature::FarmFeature()
	{
		haveBarn = false;
	}

	int FarmFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo)
	{
		if ( haveActorFromType( BIT( EActor::Barn ) ) )
			return 0;
		unsigned actorMasks[DefaultActorMaskNum] = { 0 , BIT( EActor::Pig ) , 0};
		return getDefaultActorPutInfo( playerId , ActorPos::FarmNode( posMeta ) , mapTile , actorMasks , outInfo );
	}

	void FarmFeature::mergeData( FeatureBase& other , MapTile const& putData , int meta)
	{
		BaseClass::mergeData( other , putData , meta );

		FarmFeature& otherData = static_cast< FarmFeature& >( other );
		int idx = nodes.size();
		MergeData( nodes , otherData.nodes );
		MergeData( linkedCities , otherData.linkedCities );
		for( FeatureBase* feature : otherData.linkedCities )
		{
			if( feature->type == FeatureType::eCity )
			{
				static_cast< CityFeature* >( feature )->linkedFarms.erase(&otherData);
			}

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

	int FarmFeature::calcPlayerScore(PlayerBase* player)
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

		return calcPlayerScoreInternal(player, factor);

	}

	int FarmFeature::doCalcScore(GamePlayerManager& playerManager , FeatureScoreInfo& outResult)
	{
		switch( mSetting->getFarmScoreVersion() )
		{
		case 1:
		case 2:
			return -1;
		case 3:
		default:
			return BaseClass::doCalcScore(playerManager , outResult);
			break;
		}

		return 0;
	}

	int FarmFeature::calcPlayerScoreInternal( PlayerBase* player , int farmFactor)
	{
		int factor = farmFactor;
		int numCityFinish = 0;
		int numCastle = 0;

		for( FeatureBase* linkFeatrue : linkedCities )
		{
			if ( linkFeatrue->checkComplete() )
			{
				++numCityFinish;
				if ( linkFeatrue->type == FeatureType::eCity &&  
					 static_cast< CityFeature* >( linkFeatrue )->isCastle )
					++numCastle;
			}
		}

		if ( mSetting->have(ERule::Pig) )
		{
			if ( player && havePlayerActor(player->getId(), EActor::Pig ) )
				factor += CAR_PARAM_VALUE(PigAdditionFactor);
		}

		return numCityFinish * factor + numCastle;
	}

	int FarmFeature::calcPlayerScoreByBarnRemoveFarmer(PlayerBase* player)
	{
		return calcPlayerScoreInternal( player , (haveBarn)?(CAR_PARAM_VALUE(BarnRemoveFarmerFactor) ):(CAR_PARAM_VALUE(FarmFactorV3) ) );
	}

	int AdjacentTileScoringFeature::doCalcScore(GamePlayerManager& playerManager, FeatureScoreInfo& outResult)
	{
		for (LevelActor * actor : mActors)
		{
			int majority = getMajorityValue(actor->type);

			if (majority > 0)
			{
				outResult.controllerScores.resize(1);
				auto& controllerScore = outResult.controllerScores[0];
				controllerScore.playerId = actor->ownerId;
				controllerScore.majority = majority;
				controllerScore.score = calcPlayerScore(playerManager.getPlayer(controllerScore.playerId));
				return 1;
			}
		}
		return 0;
	}

	bool AdjacentTileScoringFeature::updateForAdjacentTile(MapTile& tile)
	{
		neighborTiles.push_back(&tile);
		return true;
	}

	bool AdjacentTileScoringFeature::getActorPos(MapTile const& mapTile, ActorPos& actorPos)
	{
		if (mapTile.group == group)
		{
			actorPos = ActorPos::Tile();
			return true;
		}
		return false;
	}

	CloisterFeature::CloisterFeature()
	{
		challenger = nullptr;
	}

	int CloisterFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile , std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( EActor::Wagon ) | BIT( EActor::Abbot ), 0 , 0 };
		return getDefaultActorPutInfo( playerId , ActorPos::Tile() , mapTile, actorMasks , outInfo );
	}


	int CloisterFeature::calcPlayerScore(PlayerBase* player)
	{
		int result = ( neighborTiles.size() + 1 ) * CAR_PARAM_VALUE( CloisterFactor );
		if ( checkComplete() )
		{
			if ( mSetting->have(ERule::UseVineyard) )
			{
				for(MapTile* mapTile : neighborTiles)
				{
					if( mapTile->have( TileContent::eVineyard ) )
					{
						result += CAR_PARAM_VALUE(VineyardAdditionScore);
					}
				}
			}
		}
		return result;
	}

	void CloisterFeature::generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures )
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
				CAR_LOG("Warning: No Link Feature In Road Link");
			}
			else
			{
				outFeatures.insert( linkNode->group );
			}
		}
	}

	GardenFeature::GardenFeature()
	{

	}

	int GardenFeature::getActorPutInfo(int playerId, int posMeta, MapTile& mapTile, std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT(EActor::Abbot) , 0 , 0 };
		return getActorPutInfoInternal(playerId, ActorPos::Tile(), mapTile, actorMasks, DefaultActorMaskNum, outInfo);
	}

	int GardenFeature::calcPlayerScore(PlayerBase* player)
	{
		int result = (neighborTiles.size() + 1) * CAR_PARAM_VALUE(CloisterFactor);

		return result;
	}

	void GardenFeature::generateRoadLinkFeatures(WorldTileManager& worldTileManager, GroupSet& outFeatures)
	{

	}

	GermanCastleFeature::GermanCastleFeature()
	{

	}

	int GermanCastleFeature::getActorPutInfo(int playerId , int posMeta , MapTile& mapTile, std::vector< ActorPosInfo >& outInfo)
	{
		unsigned actorMasks[DefaultActorMaskNum] = { BIT( EActor::Wagon ) , 0 , 0 };
		return getActorPutInfoInternal( playerId , ActorPos::SideNode( posMeta ) , mapTile, actorMasks , ARRAY_SIZE( actorMasks ) , outInfo );
	}

	void GermanCastleFeature::mergeData(FeatureBase& other , MapTile const& putData , int meta)
	{
		assert( 0 );
	}

	int GermanCastleFeature::calcPlayerScore(PlayerBase* player)
	{
		int result = ( neighborTiles.size() + 1 ) * CAR_PARAM_VALUE(CloisterFactor);
		if ( checkComplete() )
		{
			if ( mSetting->have(ERule::UseVineyard) )
			{
				for(MapTile* mapTile : neighborTiles)
				{
					if( mapTile->have( TileContent::eVineyard ) )
					{
						result += CAR_PARAM_VALUE(VineyardAdditionScore);
					}
				}
			}
		}
		return result;
	}

	int GermanCastleFeature::doCalcScore( GamePlayerManager& playerManager , FeatureScoreInfo& outResult)
	{
		for(LevelActor * actor : mActors)
		{
			int majority = getMajorityValue( actor->type );

			if ( majority > 0 )
			{
				outResult.controllerScores.resize(1);
				auto& constrollerScore = outResult.controllerScores[0];
				constrollerScore.playerId = actor->ownerId;
				constrollerScore.majority = majority;
				constrollerScore.score = calcPlayerScore( playerManager.getPlayer(actor->ownerId) );
				return 1;
			}
		}
		return 0;
	}

	void GermanCastleFeature::generateRoadLinkFeatures( WorldTileManager& worldTileManager, GroupSet& outFeatures )
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
				CAR_LOG("Warning: No Link Feature In Road Link");
			}
			else
			{
				outFeatures.insert( linkNode->group );
			}
		}
	}


	bool GermanCastleFeature::updateForAdjacentTile(MapTile& tile)
	{
		return AddUnique( neighborTiles , &tile );
	}

	bool GermanCastleFeature::getActorPos(MapTile const& mapTile , ActorPos& actorPos)
	{
		for( int i = 0 ; i < TilePiece::NumSide ; ++i )
		{
			if ( mapTile.getSideGroup( i ) == group )
			{
				actorPos = ActorPos::SideNode(i);
				return true;
			}
		}
		return false;
	}

}//namespace CAR