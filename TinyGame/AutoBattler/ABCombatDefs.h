#ifndef ABCombatDefs_h__
#define ABCombatDefs_h__

#include "Math/Vector2.h"
#include "DataStructure/Array.h"
#include "Core/IntegerType.h"

namespace AutoBattler
{
	using Math::Vector2;

	// Combat event type
	enum class CombatEventType : uint8
	{
		UnitAttack,
		UnitTakeDamage,
		UnitDeath,
		UnitMove,
		UnitCastSkill,
		UnitHeal,
		UnitBuffApplied,
		UnitManaGain,
	};

	// Combat event
	struct CombatEvent
	{
		CombatEventType type;
		float timestamp;      // Event timestamp
		int sourceUnitID;     // Source unit ID
		int targetUnitID;     // Target unit ID
		
		// Event-specific data (union to save memory)
		union EventData
		{
			float damageAmount;      // UnitTakeDamage
			Vec2i moveTo;    // UnitMove
			float healAmount;        // UnitHeal
			float manaGained;        // UnitManaGain
			int skillID;             // UnitCastSkill
			
			struct BuffData {
				int type;
				float value;
				float duration;
			} buff;                  // UnitBuffApplied
			
			EventData() { memset(this, 0, sizeof(EventData)); }
		} data;

		template<class OP>
		void serialize(OP& op)
		{
			op & type & timestamp & sourceUnitID & targetUnitID;
			
			// Serialize union based on type
			switch (type)
			{
			case CombatEventType::UnitTakeDamage:
				op & data.damageAmount;
				break;
			case CombatEventType::UnitMove:
				op & data.moveTo;
				break;
			case CombatEventType::UnitHeal:
				op & data.healAmount;
				break;
			case CombatEventType::UnitManaGain:
				op & data.manaGained;
				break;
			case CombatEventType::UnitCastSkill:
				op & data.skillID;
				break;
			case CombatEventType::UnitBuffApplied:
				op & data.buff.type & data.buff.value & data.buff.duration;
				break;
			default:
				break;
			}
		}
	};

	struct CombatEventBatch
	{
		uint32 combatID;
		uint32 batchIndex;
		TArray<CombatEvent> events;

		template<class OP>
		void serialize(OP& op)
		{
			op & combatID & batchIndex & events;
		}
	};
}

#endif // ABCombatDefs_h__
