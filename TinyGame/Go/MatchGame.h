#include "GoCore.h"
#include "GoBot.h"

#include "Serialize/FileStream.h"

#include "Core/FNV1a.h"
#include "Misc/Guid.h"
#include "SystemPlatform.h"

#include <unordered_map>
#include <algorithm>
#include <memory>
#include <functional>


namespace Go
{
	enum class ControllerType
	{
		ePlayer = 0,
		eLeelaZero,
		eKata,
		eAQ,
		eZen,

		Count,
	};

	namespace BotParam
	{
		constexpr uint32 LeelaBase = 0;
		constexpr uint32 KataBase = 200;
		constexpr uint32 ZenBase = 400;
		constexpr uint32 AQBase = 600;
	}

	inline char const* GetControllerName(ControllerType type)
	{
		switch( type )
		{
		case ControllerType::ePlayer:
			return "Player";
		case ControllerType::eLeelaZero:
			return "LeelaZero";
		case ControllerType::eKata:
			return "Kata";
		case ControllerType::eAQ:
			return "AQ";
		case ControllerType::eZen:
			return "Zen";
		}

		return "Unknown";
	}


	class GameProxy;

	using GameEventDelegate = std::function< void(GameProxy& game) >;

	class GameProxy
	{

	public:
		GameProxy()
		{
			guid = Guid::New();
		}

		Guid guid;
		GameEventDelegate onStateChanged;

		void setup(int size) { mGame.setup(size); }
		void setSetting(GameSetting const& setting) { mGame.setSetting(setting); }
		void restart()
		{
			guid = Guid::New();
			mGame.restart();
			notifyStateChanged();
		}

		bool canPlay(int x, int y) const { return mGame.canPlay(x, y); }
		bool playStone(int x, int y)
		{
			if( mGame.playStone(x, y) )
			{
				notifyStateChanged();
				return true;
			}
			return false;
		}
		bool addStone(int x, int y, int color)
		{
			if( mGame.addStone(x, y, color) )
			{
				notifyStateChanged();
				return true;
			}
			return false;
		}
		void    playPass()
		{
			mGame.playPass();
			notifyStateChanged();
		}
		bool undo()
		{
			if( mGame.undo() )
			{
				notifyStateChanged();
				return true;
			}
			return false;
		}

		bool load(char const* path)
		{
			guid = Guid::New();
			if (mGame.loadSGF(path))
			{
				notifyStateChanged();
				return true;
			}

			return false;
		}

		void notifyStateChanged()
		{
			if (onStateChanged)
				onStateChanged(*this);
		}
		int reviewBeginStep()
		{
			int result = mGame.reviewBeginStep();
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		int reviewPrevSetp(int numStep = 1)
		{
			int result = mGame.reviewPrevSetp(numStep);
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		int reviewNextStep(int numStep = 1)
		{
			int result = mGame.reviewNextStep(numStep);
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		int reviewLastStep()
		{
			int result = mGame.reviewLastStep();
			if( result && onStateChanged )
				onStateChanged(*this);
			return result;
		}
		void copy(GameProxy& otherGame, bool bRemoveUnplayedHistory = false)
		{
			guid = otherGame.guid;
			mGame.copy(otherGame.mGame);
			if( bRemoveUnplayedHistory )
				mGame.removeUnplayedHistory();
		}
		void synchronizeHistory(GameProxy& other)
		{
			if( guid != other.guid )
			{
				LogWarning(0, "Can't synchronize step history");
				return;
			}
			mGame.synchronizeHistory(other.mGame);
		}
		Board const& getBoard() const { return mGame.getBoard(); }
		GameSetting const& getSetting() const { return mGame.getSetting(); }
		Game const& getInstance() { return mGame; }

	private:
		Game mGame;
	};

	struct MatchParamKey
	{
		uint64 mHash;
		bool operator < (MatchParamKey const& other) const
		{
			return mHash < other.mHash;
		}
		bool operator == (MatchParamKey const& other) const
		{
			return mHash == other.mHash;
		}
		bool operator != (MatchParamKey const& other) const
		{
			return mHash != other.mHash;
		}
		void setValue(std::string const&paramString)
		{
			mHash = FNV1a::MakeStringHash< uint64 >(paramString.c_str());
		}
	};

	struct MatchPlayer
	{
		ControllerType type;
		std::unique_ptr< IBot > bot;

		std::string    paramString;
		MatchParamKey  paramKey;
		uint32 winCount;
		uint32 botParamBase;

		FixString< 128 > getName() const;
		bool   isBot() const { return type != ControllerType::ePlayer; }
		bool   initialize(ControllerType inType, void* botSetting = nullptr, MatchPlayer const* otherPlayer = nullptr);


		static std::string GetParamString(ControllerType inType, void* botSetting);
	};


	struct LeelaWeightInfo
	{
		static int const DefaultNameLength = 8;
		int version;
		std::string date;
		std::string time;
		std::string name;
		std::string format;
		int elo;
		int games;
		int training;

		void getFormattedName(FixString< 128 >& outName)
		{
			outName.format("#%03d-%s-%s", version, format.c_str(), name.c_str());
		}

		static bool Load(char const* path, std::vector< LeelaWeightInfo >& outTable)
		{
			std::ifstream fs(path);
			if( !fs.is_open() )
				return false;

			while( fs.good() )
			{
				LeelaWeightInfo info;
				fs >> info.version >> info.date >> info.time >> info.name >> info.format >> info.elo >> info.games >> info.training;

				outTable.push_back(std::move(info));
			}
			return true;
		}

	};

	class LeelaWeightTable
	{
	public:
		static bool IsOfficialFormat( StringView const& weightName , StringView const& subName )
		{
			return weightName.length() - subName.length() == 64;
		}

		bool load(char const* path)
		{		
			table.clear();
			weightMap.clear();
			if( !LeelaWeightInfo::Load(path, table) )
			{
				LogError("Can't Load Table");
				return false;
			}

			for( auto& info : table )
			{
				weightMap.emplace(info.name, &info);
			}
			return true;
		}


		LeelaWeightInfo* find(char const* weightName) const
		{
			auto iter = weightMap.find(weightName);
			if( iter == weightMap.end() )
				return nullptr;
			return iter->second;
		}
		std::vector< LeelaWeightInfo > table;
		std::unordered_map< std::string, LeelaWeightInfo* > weightMap;
	};

	struct MatchResultData
	{
		std::string playerSetting[2];
		std::string gameSetting;
		uint32 winCounts[2];

		void swapPlayerData()
		{
			using std::swap;
			swap(playerSetting[0], playerSetting[1]);
			swap(winCounts[0], winCounts[1]);
		}

		template< class OP >
		void serialize(OP op)
		{
			op & winCounts[0] & winCounts[1];
			op & gameSetting & playerSetting[0] & playerSetting[1];
		}

	};

	TYPE_SUPPORT_SERIALIZE_FUNC(MatchResultData);

	class MatchResultMap
	{
	public:
		enum Version
		{
			DATA_SAVE_VALUE_ONLY ,
			DATA_LAST_VERSION_PLUS_ONE,
			DATA_LAST_VERSION = DATA_LAST_VERSION_PLUS_ONE - 1,
		};

		struct MatchKey
		{
			MatchParamKey playerParamKeys[2];
			bool setValue(MatchResultData const& data)
			{
				playerParamKeys[0].setValue(data.playerSetting[0]);
				playerParamKeys[1].setValue(data.playerSetting[1]);
				if( playerParamKeys[1] < playerParamKeys[0] )
				{
					std::swap(playerParamKeys[0], playerParamKeys[1]);
					return true;
				}
				return false;
			}
			bool setValue(MatchPlayer players[2], GameSetting const& gameSetting)
			{
				playerParamKeys[0] = players[0].paramKey;
				playerParamKeys[1] = players[1].paramKey;
				if( playerParamKeys[1] < playerParamKeys[0] )
				{
					std::swap(playerParamKeys[0], playerParamKeys[1]);
					return true;
				}
				return false;
			}

			bool operator == (MatchKey const& rhs ) const
			{
				return playerParamKeys[0] == rhs.playerParamKeys[0] &&
					playerParamKeys[1] == rhs.playerParamKeys[1];
			}

			bool operator != (MatchKey const& rhs) const
			{
				return !this->operator==(rhs);
			}

			bool operator < (MatchKey const& rhs) const
			{
				assert(!(playerParamKeys[1] < playerParamKeys[0]));
				assert(!(rhs.playerParamKeys[1] < rhs.playerParamKeys[0]));

				if( playerParamKeys[0] < rhs.playerParamKeys[0] )
					return true;
				if( !(rhs.playerParamKeys[0] < playerParamKeys[0]) )
					return playerParamKeys[1] < rhs.playerParamKeys[1];
				return false;
			}
		};

		MatchResultData* getMatchResult(MatchPlayer players[2], GameSetting const& gameSetting, bool& bSwap)
		{
			MatchKey key;
			bSwap = key.setValue(players, gameSetting);

			auto iter = mDataMap.find(key);
			if( iter == mDataMap.end() )
				return nullptr;

			return &iter->second;
		}

		static std::string ToString(GameSetting const& gameSetting)
		{
			FixString<128> result;
			result.format("size=%d handicap=%d komi=%f start=%c",
						  gameSetting.boardSize, gameSetting.numHandicap,
						  gameSetting.komi, gameSetting.bBlackFirst ? 'b' : 'w');
			return result.c_str();
		}

		void addMatchResult(MatchPlayer players[2], GameSetting const& gameSetting , uint32* savedWinCounts = nullptr);

		void checkMapValid()
		{
			for( auto& pair : mDataMap )
			{
				MatchKey key;
				key.setValue(pair.second);
				if( pair.first != key )
				{
					int i = 1;
				}
			}
		}
		bool convertLeelaWeight( LeelaWeightTable const& table );

		bool save(char const* path);
		bool load(char const* path);

		std::map< MatchKey, MatchResultData > mDataMap;
	};




	struct MatchGameData
	{
		MatchPlayer players[2];
		int         idxPlayerTurn = 0;
		bool        bSwapColor = false;
		bool        bAutoRun = false;
		bool        bSaveSGF = false;
		uint32      historyWinCounts[2];
		DateTime    startTime;

		MatchPlayer& getPlayer(int color)
		{
			int idx = (color == StoneColor::eBlack) ? 0 : 1;
			if( bSwapColor )
				idx = 1 - idx;
			return players[idx];
		}

		int  getPlayerColor(int idx)
		{
			if( bSwapColor )
			{
				return (idx == 1) ? StoneColor::eBlack : StoneColor::eWhite;
			}
			return (idx == 0) ? StoneColor::eBlack : StoneColor::eWhite;
		}

		void advanceStep()
		{
			idxPlayerTurn = 1 - idxPlayerTurn;
		}

		void cleanup()
		{
			for(auto & player : players)
			{
				auto& bot = player.bot;
				if( bot )
				{
					bot->destroy();
					bot.release();
				}
			}
		}

		MatchPlayer&    getCurTurnPlayer()
		{
			return players[idxPlayerTurn];
		}
		IBot* getCurTurnBot()
		{
			return players[idxPlayerTurn].bot.get();
		}
		IBot* getNextTurnBot()
		{
			return players[1 - idxPlayerTurn].bot.get();
		}
	};


}//namespace Go