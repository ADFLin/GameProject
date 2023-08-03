#include "RichAI.h"

namespace Rich
{

	void AIController::startTurn(PlayerTurn& turn)
	{
		turn.goMoveByPower(mPlayer->getMovePower());
	}

	void AIController::queryAction(ActionRequestID id, PlayerTurn& turn, ActionReqestData const& data)
	{
		switch (id)
		{
		case REQ_ROMATE_DICE_VALUE:
			break;
		case REQ_UPGRADE_LAND:
		case REQ_BUY_LAND:
		case REQ_BUY_STATION:
			{
				ActionReplyData data;
				data.addParam(1);
				turn.replyAction(data);
			}
			break;
		case REQ_MOVE_DIR:
			break;
		case REQ_DISPOSE_ASSET:
			{
				struct AssetValue
				{
					int index;
					int value;
				};

				TArray<AssetValue> assetValues;
				int index = 0;
				for (auto asset : mPlayer->mAssets)
				{
					AssetValue av;
					av.index = index;
					av.value = asset->getAssetValue();

					assetValues.push_back(av);
					++index;
				}

				std::sort(assetValues.begin(), assetValues.end(), [](AssetValue const& a, AssetValue const& b)
				{
					return a.value < b.value;
				});
				int totalValue = mPlayer->getTotalMoney();
				TArray<int> disposeAssetIndices;
				for (auto const& av : assetValues)
				{
					disposeAssetIndices.push_back(av.index);
					totalValue += av.value;
					if (totalValue > data.debt)
					{
						break;
					}
				}

				ActionReplyData data;
				data.addParam(disposeAssetIndices.data());
				data.addParam((int)disposeAssetIndices.size());
				turn.replyAction(data);
			}
			break;
		}
	}

}