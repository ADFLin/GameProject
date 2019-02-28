#include "MatchGame.h"

#include "ZenBot.h"
#include "LeelaBot.h"

#include "FileSystem.h"

namespace Go
{
	FixString< 128 > MatchPlayer::getName() const
	{
		FixString< 128 > result = GetControllerName(type);
		if( type == ControllerType::eLeelaZero )
		{
			std::string weightName;
			bot->getMetaDataT(LeelaBot::eWeightName, weightName);
			result += " ";
			char const* extension = FileUtility::GetExtension(weightName.c_str());
			if( extension )
			{
				weightName.resize(extension - weightName.data() - 1);
			}

			if( weightName.length() > 8 )
				weightName.resize(8);

			result += weightName;
		}
		return result;
	}

	bool MatchPlayer::initialize(ControllerType inType, void* botSetting, MatchPlayer const* otherPlayer)
	{
		type = inType;
		winCount = 0;

		bot.release();

		paramString = GetParamString(inType, botSetting);
		paramKey.setValue(paramString);

		if( otherPlayer && otherPlayer->bot && otherPlayer->paramKey == paramKey )
		{
			bot.reset(new PorxyBot(*otherPlayer->bot, true));
		}
		else
		{
			switch( type )
			{
			case ControllerType::ePlayer:
				break;
			case ControllerType::eLeelaZero:
				bot.reset(new LeelaBot());
				break;
			case ControllerType::eAQ:
				bot.reset(new AQBot());
				break;
			case ControllerType::eZenV7:
				bot.reset(new ZenBot(7));
				break;
			case ControllerType::eZenV6:
				bot.reset(new ZenBot(6));
				break;
			case ControllerType::eZenV4:
				bot.reset(new ZenBot(4));
				break;
			}
		}
		if( bot )
		{
			if( !bot->initilize(botSetting) )
			{
				bot.release();
				return false;
			}
		}
		return true;
	}

	std::string MatchPlayer::GetParamString(ControllerType inType, void* botSetting)
	{
		std::string paramString = GetControllerName(inType);
		switch( inType )
		{
		case ControllerType::ePlayer:
			break;
		case ControllerType::eLeelaZero:
			{
				auto mySetting = static_cast<LeelaAISetting*>(botSetting);
				paramString += " ";
				paramString += mySetting->toParamString();
			}
			break;
		case ControllerType::eAQ:
			break;
		case ControllerType::eZenV7:
		case ControllerType::eZenV6:
		case ControllerType::eZenV4:
			break;
		default:
			NEVER_REACH("Error Controller Type");
		}

		return paramString;
	}



	void MatchResultMap::addMatchResult(MatchPlayer players[2], GameSetting const& gameSetting)
	{
		MatchKey key;
		key.setValue(players, gameSetting);

		auto& resultData = mDataMap[key];

		resultData.gameSetting = ToString(gameSetting);

		if( key.playerParamKeys[0] == players[0].paramKey )
		{
			resultData.winCounts[0] += players[0].winCount;
			resultData.winCounts[1] += players[1].winCount;
			resultData.playerSetting[0] = players[0].paramString;
			resultData.playerSetting[1] = players[1].paramString;
		}
		else
		{
			resultData.winCounts[1] += players[0].winCount;
			resultData.winCounts[0] += players[1].winCount;
			resultData.playerSetting[1] = players[0].paramString;
			resultData.playerSetting[0] = players[1].paramString;
		}
	}

	bool MatchResultMap::save(char const* path)
	{
		OutputFileStream stream;
		if( !stream.open(path) )
			return false;

		DataSerializer serializer(stream);
		serializer << mDataMap;

		return false;
	}

	bool MatchResultMap::load(char const* path)
	{
		InputFileStream stream;
		if( !stream.open(path) )
			return false;

		DataSerializer serializer(stream);
		serializer >> mDataMap;

		return false;
	}

}//namespace Go