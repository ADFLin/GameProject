#ifndef TCombatSystem_h__
#define TCombatSystem_h__

#include "common.h"

class ILevelObject;


enum DamageType
{
	DT_MELE       = BIT(0),
	DT_FAR        = BIT(1),
	DT_MAGIC      = BIT(2),
	DT_PHYSICAL   = BIT(3),
	DT_NO_DEFENCE = BIT(4),

	DT_USE_WEAPON = BIT(5) ,
	DT_RHAND      = BIT(6),
	DT_LHAND      = BIT(7),	
};

struct DamageInfo
{
	unsigned      typeBit;
	float         val;
	ILevelObject*  attacker;
	ILevelObject*  defender;
	float         damageResult;
	unsigned      resultBit;
};



#endif // TCombatSystem_h__