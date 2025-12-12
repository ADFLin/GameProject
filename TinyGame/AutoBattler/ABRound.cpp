#include "ABPCH.h"
#include "ABRound.h"

namespace AutoBattler
{

	void RoundManager::init()
	{
		mDefaultPVP.type = RoundType::PVP;
		mDefaultPVE.type = RoundType::PVE;

		// Setup 20 Rounds?
		mRounds.resize(20);

		// Round 1: Minions (Melee)
		{
			RoundConfig& r = mRounds[0];
			r.type = RoundType::PVP;
			
			EnemySpawnInfo enemy;
			enemy.unitId = 1;
			
			enemy.stats.hp = 100;
			enemy.stats.maxHp = 100;
			enemy.stats.attackDamage = 5;
			enemy.stats.attackSpeed = 0.6f;
			enemy.stats.range = 50;
			enemy.stats.maxMana = 100;
			enemy.stats.mana = 0;

			enemy.gridPos = Vec2i(4, 3);
			r.enemies.push_back(enemy);

			enemy.gridPos = Vec2i(5, 4);
			r.enemies.push_back(enemy);
		}

		// Round 2: Minions (Melee)
		{
			RoundConfig& r = mRounds[1];
			r.type = RoundType::PVE;
			// ...
			EnemySpawnInfo enemy;
			enemy.unitId = 1;
			enemy.stats.hp = 120;
			enemy.stats.maxHp = 120;
			enemy.stats.attackDamage = 8;
			enemy.stats.attackSpeed = 0.6f;
			enemy.stats.range = 50;
			enemy.stats.maxMana = 100;
			enemy.stats.mana = 0;

			enemy.gridPos = Vec2i(4, 2);
			r.enemies.push_back(enemy);

			enemy.gridPos = Vec2i(5, 3);
			r.enemies.push_back(enemy);

			enemy.gridPos = Vec2i(4, 4);
			r.enemies.push_back(enemy);
		}

		// Round 3: Minions
		{
			RoundConfig& r = mRounds[2];
			r.type = RoundType::PVE;
			// 3 Minions stronger
			EnemySpawnInfo enemy;
			enemy.unitId = 1;
			enemy.stats.hp = 150;
			enemy.stats.maxHp = 150;
			enemy.stats.attackDamage = 10;
			enemy.stats.attackSpeed = 0.6f;
			enemy.stats.range = 50;
			enemy.stats.maxMana = 100;
			enemy.stats.mana = 0;

			enemy.gridPos = Vec2i(3, 2); r.enemies.push_back(enemy);
			enemy.gridPos = Vec2i(5, 5); r.enemies.push_back(enemy);
			enemy.gridPos = Vec2i(4, 6); r.enemies.push_back(enemy);
		}

		// Round 4: PVP
		mRounds[3].type = RoundType::PVP;
		// Round 5: PVE (Boss?)
		{
			RoundConfig& r = mRounds[4];
			r.type = RoundType::PVE;
			
			EnemySpawnInfo enemy;
			enemy.unitId = 2; // Boss
			enemy.stats.hp = 600;
			enemy.stats.maxHp = 600;
			enemy.stats.attackDamage = 40;
			enemy.stats.attackSpeed = 0.5f;
			enemy.stats.range = 50;
			enemy.stats.maxMana = 100;
			enemy.stats.mana = 0;
			enemy.gridPos = Vec2i(5, 3); 
			r.enemies.push_back(enemy);
		}

		// Rest
		for (int i = 5; i < mRounds.size(); ++i)
		{
			if ((i + 1) % 5 == 0) // PVE every 5th round (1-based: 5, 10, 15...) -> Index 4, 9, 14
			{
				mRounds[i].type = RoundType::PVE;
				EnemySpawnInfo enemy;
				enemy.unitId = 2; // Big Guy
				enemy.stats.hp = 300 + i * 50.0f;
				enemy.stats.maxHp = enemy.stats.hp;
				enemy.stats.attackDamage = 20 + i * 5.0f;
				enemy.stats.attackSpeed = 0.6f;
				enemy.stats.range = 50;
				enemy.gridPos = Vec2i(5, 3);
				mRounds[i].enemies.push_back(enemy);
				
				enemy.gridPos = Vec2i(5, 4);
				mRounds[i].enemies.push_back(enemy);
			}
			else
			{
				mRounds[i].type = RoundType::PVP;
			}
		}
	}

	RoundConfig const& RoundManager::getRoundConfig(int roundIndex) const
	{
		if (mRounds.isValidIndex(roundIndex))
		{
			return mRounds[roundIndex];
		}
		
		// Fallback for infinite rounds
		if ((roundIndex + 1) % 5 == 0) 
			return mDefaultPVE;
		
		return mDefaultPVP;
	}

}
