#include "MatchGame.h"

#include "ZenBot.h"
#include "LeelaBot.h"

#include "FileSystem.h"
#include "StringParse.h"
#include "Core\StringConv.h"

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
		else if( type == ControllerType::eZen )
		{
			int version;
			bot->getMetaDataT(ZenMetaParam::eVersion, version);
			result += FStringConv::From(version);
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
			case ControllerType::eKata:
				bot.reset(new KataBot());
				break;
			case ControllerType::eAQ:
				bot.reset(new AQBot());
				break;
			case ControllerType::eZen:
				bot.reset(new ZenBot());
				break;
			}
		}
		if( bot )
		{
			if( !bot->initialize(botSetting) )
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
		case ControllerType::eKata:
			{
				auto mySetting = static_cast<KataAISetting*>(botSetting);
			}
			break;
		case ControllerType::eAQ:
			break;

		case ControllerType::eZen:
			{
				auto mySetting = static_cast<Zen::CoreSetting*>(botSetting);
				paramString += " ";
				paramString += FStringConv::From(mySetting->version);
			}
			break;
		default:
			NEVER_REACH("Error Controller Type");
		}

		return paramString;
	}



	void MatchResultMap::addMatchResult(MatchPlayer players[2], GameSetting const& gameSetting, uint32* savedWinCounts)
	{
		MatchKey key;
		key.setValue(players, gameSetting);

		auto& resultData = mDataMap[key];

		resultData.gameSetting = ToString(gameSetting);

		int idx0 = 0;
		int idx1 = 1;

		if( key.playerParamKeys[0] != players[0].paramKey )
		{
			std::swap(idx0, idx1);
		}

		if( savedWinCounts )
		{
			assert(players[0].winCount >= savedWinCounts[0] &&
				   players[1].winCount >= savedWinCounts[1]);
			resultData.winCounts[0] += (players[idx0].winCount - savedWinCounts[idx0]);
			resultData.winCounts[1] += (players[idx1].winCount - savedWinCounts[idx1]);

			savedWinCounts[0] = players[0].winCount;
			savedWinCounts[1] = players[1].winCount;
		}
		else
		{
			resultData.winCounts[0] += players[idx0].winCount;
			resultData.winCounts[1] += players[idx1].winCount;
		}
		resultData.playerSetting[0] = players[idx0].paramString;
		resultData.playerSetting[1] = players[idx1].paramString;
	}

	bool MatchResultMap::convertLeelaWeight(LeelaWeightTable const& table)
	{
		std::vector< MatchResultData > requireUpdatedData;
		StringView leelaZeroName{ GetControllerName(ControllerType::eLeelaZero) };

		std::string bestWeightName = LeelaAppRun::GetBestWeightName();

		for( auto iter = mDataMap.begin(); iter != mDataMap.end(); )
		{
			MatchResultData& data = iter->second;

			bool bModified = false;
			for( int i = 0; i < 2; ++i )
			{
				char const* playerSettingStr = data.playerSetting[i].c_str();

				if( FCString::CompareN( playerSettingStr, leelaZeroName.data() , leelaZeroName.length() ) != 0 )
					continue;

				char const* pFind = strstr(playerSettingStr, "-w");
				if( pFind )
				{
					pFind += 2;
					char const* pWeightStart = FStringParse::SkipChar(pFind, ' ');
					char const* pWeightEnd = FStringParse::FindChar(pWeightStart, ' ');

					StringView weightName { pWeightStart, size_t( pWeightEnd - pWeightStart ) };

					if( FCString::CompareN(bestWeightName.c_str() , weightName.data() , weightName.size()) == 0 )
						continue;

					if( LeelaWeightTable::IsOfficialFormat(weightName , StringView()) )
					{
						int offset = pWeightStart - data.playerSetting[i].data();

						auto weightInfo = table.find( StringView( weightName.data() , LeelaWeightInfo::DefaultNameLength).toCString() );
						if( weightInfo )
						{
							data.playerSetting[i].erase(offset, weightName.length());
							FixString<128> newWeightName;
							weightInfo->getFormattedName(newWeightName);
							data.playerSetting[i].insert(offset, newWeightName);
							bModified = true;
						}			
					}
				}
			}

			if( bModified )
			{
				requireUpdatedData.push_back(std::move(data));
				iter = mDataMap.erase(iter);
			}
			else
			{
				++iter;
			}
		}

		for( auto& data : requireUpdatedData )
		{
			MatchKey key;
			if( key.setValue(data) )
			{
				data.swapPlayerData();
			}
			mDataMap.insert({ key , std::move(data) });
		}
		return true;
	}

	bool MatchResultMap::save(char const* path)
	{
		OutputFileSerializer serializer;
		if( !serializer.open(path) )
			return false;

		int num = mDataMap.size();
		int version = DATA_LAST_VERSION;
		serializer << version;
		serializer << num;
		for( auto const& pair : mDataMap )
		{
			serializer << pair.second;
		}
		if( !serializer.isValid() )
		{
			return false;
		}
		return false;
	}

	bool MatchResultMap::load(char const* path)
	{
		InputFileSerializer serializer;
		if( !serializer.open(path) )
			return false;
		
		int version = DATA_LAST_VERSION;
		int num;
	
		serializer >> version;
		serializer >> num;
		for( int i = 0; i < num; ++i )
		{
			MatchResultData data;
			serializer >> data;
			MatchKey key;
			if( key.setValue(data) )
			{
				data.swapPlayerData();
			}
			mDataMap.insert({ key , std::move(data) });
		}

		if( !serializer.isValid() )
		{
			return false;
		}

		return true;
	}

}//namespace Go