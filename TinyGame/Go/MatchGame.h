#include "GoCore.h"
#include "GoBot.h"

#include "DataStream.h"

#include "Core/FNV1a.h"
#include "Misc/Guid.h"


namespace Go
{
	enum class ControllerType
	{
		ePlayer = 0,
		eLeelaZero,
		eAQ,
		eZenV7,
		eZenV6,
		eZenV4,

		Count,
	};


	inline char const* GetControllerName(ControllerType type)
	{
		switch( type )
		{
		case ControllerType::ePlayer:
			return "Player";
		case ControllerType::eLeelaZero:
			return "LeelaZero";
		case ControllerType::eAQ:
			return "AQ";
		case ControllerType::eZenV7:
			return "Zen7";
		case ControllerType::eZenV6:
			return "Zen6";
		case ControllerType::eZenV4:
			return "Zen4";
		}

		return "Unknown";
	}


	class GameProxy;

	typedef std::function< void(GameProxy& game) > GameEventDelegate;

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
			if( onStateChanged )
				onStateChanged(*this);
		}
		bool canPlay(int x, int y) const { return mGame.canPlay(x, y); }
		bool playStone(int x, int y)
		{
			if( mGame.playStone(x, y) )
			{
				if( onStateChanged )
					onStateChanged(*this);
				return true;
			}
			return false;
		}
		bool addStone(int x, int y, int color)
		{
			if( mGame.addStone(x, y, color) )
			{
				if( onStateChanged )
					onStateChanged(*this);
				return true;
			}
			return false;
		}
		void    playPass()
		{
			mGame.playPass();
			if( onStateChanged )
				onStateChanged(*this);
		}
		bool undo()
		{
			if( mGame.undo() )
			{
				if( onStateChanged )
					onStateChanged(*this);
				return true;
			}
			return false;
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

		void setValue(std::string const&paramString)
		{
			mHash = FNV1a::MakeStringHash< uint64 >(paramString.c_str());
		}
	};

	struct MatchPlayer
	{
		ControllerType type;
		std::unique_ptr< IBotInterface > bot;

		std::string    paramString;
		MatchParamKey  paramKey;
		int winCount;

		FixString< 128 > getName() const;
		bool   isBot() const { return type != ControllerType::ePlayer; }
		bool   initialize(ControllerType inType, void* botSetting = nullptr, MatchPlayer const* otherPlayer = nullptr);


		static std::string GetParamString(ControllerType inType, void* botSetting);
	};



	class MatchResultMap
	{
	public:
		enum Version
		{

			DATA_LAST_VERSION_PLUS_ONE,
			DATA_LAST_VERSION = DATA_LAST_VERSION_PLUS_ONE - 1,
		};
		struct ResultData
		{
			std::string playerSetting[2];
			std::string gameSetting;
			uint32 winCounts[2];

			template< class OP >
			void serialize(OP op)
			{
				int version = DATA_LAST_VERSION;
				op & version;
				op & winCounts[0] & winCounts[1];
				op & gameSetting & playerSetting[0] & playerSetting[1];
			}

			friend DataSerializer& operator << (DataSerializer& serializer, ResultData const& data)
			{
				const_cast<ResultData&>(data).serialize(DataSerializer::WriteOp(serializer));
				return serializer;
			}
			friend DataSerializer& operator >> (DataSerializer& serializer, ResultData& data)
			{
				data.serialize(DataSerializer::ReadOp(serializer));
				return serializer;
			}
		};

		struct MatchKey
		{
			MatchParamKey playerParamKeys[2];
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

			bool operator < (MatchKey const& other) const
			{
				assert(!(playerParamKeys[1] < playerParamKeys[0]));
				assert(!(other.playerParamKeys[1] < other.playerParamKeys[0]));

				if( playerParamKeys[0] < other.playerParamKeys[0] )
					return true;
				if( !(other.playerParamKeys[0] < playerParamKeys[0]) )
					return playerParamKeys[1] < other.playerParamKeys[1];
				return false;
			}
		};

		ResultData* getMatchResult(MatchPlayer players[2], GameSetting const& gameSetting, bool& bSwap)
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
						  gameSetting.komi, gameSetting.bBlackFrist ? 'b' : 'w');
			return result.c_str();
		}

		void addMatchResult(MatchPlayer players[2], GameSetting const& gameSetting);

		bool save(char const* path);
		bool load(char const* path);

		std::map< MatchKey, ResultData > mDataMap;
	};

	struct MatchGameData
	{
		MatchPlayer players[2];
		int         idxPlayerTurn = 0;
		bool        bSwapColor = false;
		bool        bAutoRun = false;
		bool        bSaveSGF = false;
		int         historyWinCounts[2];


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
			for( int i = 0; i < 2; ++i )
			{
				auto& bot = players[i].bot;
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
		IBotInterface* getCurTurnBot()
		{
			return players[idxPlayerTurn].bot.get();
		}
	};


}//namespace Go