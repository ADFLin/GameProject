#include "CARGameInput.h"

#include "CARFeature.h"

namespace CAR
{
	void GameFeatureTileSelectData::fill(TArray<FeatureBase*> const& linkFeatures, TArray<MapTile*>& mapTileStorage, bool bCheckCanDeployFollower)
	{
		for(auto linkFeature : linkFeatures)
		{
			Info info;
			info.feature = linkFeature;
			info.index = mapTileStorage.size();
			info.num = 0;
			for( MapTile* mapTile : info.feature->mapTiles )
			{
				if( bCheckCanDeployFollower && !mapTile->canDeployFollower() )
					continue;

				++info.num;
				mapTileStorage.push_back(mapTile);
			}

			infos.push_back(info);
		}

		options = MakeView(mapTileStorage);
	}

	FeatureBase* GameFeatureTileSelectData::getResultFeature()
	{
#if 0
		for( int i = 0; i < infos.size(); ++i )
		{
			Info& info = infos[i];
			if( resultIndex < (unsigned)(info.index + info.num) )
				return info.feature;
		}
#else

		int startIndex = 0;
		int endIndex = infos.size() - 1;
		while( startIndex <= endIndex )
		{
			int curIndex = (startIndex + endIndex) / 2;
			Info& info = infos[curIndex];
			if( resultIndex < (unsigned)(info.index) )
			{
				endIndex = curIndex - 1;
			}
			else if( resultIndex < (unsigned)(info.index + info.num) )
			{
				return info.feature;
			}
			else
			{
				startIndex = curIndex + 1;
			}
		}
#endif
		return nullptr;
	}

}//namespace CAR

