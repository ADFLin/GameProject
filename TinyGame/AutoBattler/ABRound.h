#ifndef ABRound_h__
#define ABRound_h__

#include "DataStructure/Array.h"
#include "ABUnit.h"
#include "Math/Vector2.h"

namespace AutoBattler
{
	using Math::Vector2;

	enum class RoundType
	{
		PVE,
		PVP,
	};

	struct EnemySpawnInfo
	{
		int unitId; 
		Vec2i gridPos; 
		UnitStats stats;
	};

	struct RoundConfig
	{
		RoundType type;
		TArray<EnemySpawnInfo> enemies;
	};

	class RoundManager
	{
	public:
		void init();
		RoundConfig const& getRoundConfig(int roundIndex) const;

	private:
		TArray<RoundConfig> mRounds;
		RoundConfig         mDefaultPVP;
		RoundConfig         mDefaultPVE; // Fallback
	};
}

#endif // ABRound_h__
