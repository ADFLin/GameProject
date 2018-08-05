#include "ProjectPCH.h"
#include "TCombatSystem.h"

#include "CActor.h"
#include "CHero.h"

#include "AbilityProp.h"

#include "ConsoleSystem.h"
#include "UtilityGlobal.h"
#include "TItemSystem.h"
#include "EventType.h"

#include "Singleton.h"

TConsoleVariable< bool > gPlayerNoDamage( false , "playerNoDamage" );


class DamageComputer
{
public:

	static void computeDamageResult( DamageInfo& info )
	{
		CActor* atActor = CActor::downCast( info.attacker );
		CActor* dtActor = CActor::downCast( info.defender );

		if ( atActor == NULL || dtActor == NULL )
		{
			info.damageResult = info.val;
			return;
		}

		if ( CHero::downCast( dtActor ) && gPlayerNoDamage )
		{
			info.damageResult = 0;
			return;
		}


		AbilityProp* atProp = atActor->getAbilityProp();
		AbilityProp* dtProp = dtActor->getAbilityProp();

		TEquipment* equip = dtActor->getEquipTable().getEquipment( EQS_HAND_L );

		if ( equip && equip->isShield() )
		{


		}


		//防禦者裝甲值
		float WDT = dtProp->getPropValue( PROP_DT );  //護甲值
		float END = dtProp->getPropValue( PROP_END ); //耐力

		float STR = atProp->getPropValue( PROP_STR ); //力量
		

		///////////////////從這裡開始///////////////////////
		float INT = atProp->getPropValue( PROP_INT ); //智力(可加魔法攻擊)
		float DEX = atProp->getPropValue( PROP_DEX ); //敏捷(可加普通攻擊)
		float MAT = atProp->getPropValue( PROP_MAT ); //主手攻擊(可加普通攻擊)
		float SAT = atProp->getPropValue( PROP_SAT ); //副手攻擊(可加普通攻擊)
		float AT_SPEED = atProp->getPropValue( PROP_AT_SPEED ); //攻擊速度()


		int atLevel = atActor->getPropValue(PROP_LEVEL); 
		int dtLevel = dtActor->getPropValue(PROP_LEVEL);

		
		float TOTAL_DT = WDT + END + 0.001 * WDT * END;
		float TOTAL_AT = info.val + ( (STR + DEX) * 0.5 + atLevel*2 );

		float DR = ( WDT + 15 * dtLevel ) / ( WDT + 1500 ); //角色基本減傷百分比(如何取得NPC攻擊傷害資料??)

		info.damageResult = TOTAL_AT * ( 1 - DR ) ;

		info.damageResult += TRandom() % 5;

	}
};



class CombatSystem : public SingletonT< CombatSystem >
{
public:
	void inputDamage(DamageInfo& info);
	void applyDamage(DamageInfo& info);

};



void CombatSystem::applyDamage(DamageInfo& info)
{
	TEvent event;

	event.type   = EVT_COMBAT_DAMAGE;
	event.id     = info.defender->getRefID();
	event.sender = info.defender;
	event.data   = &info;

	info.defender->takeDamage( info );

	//FIXME
	UG_ProcessEvent( event );
}

void CombatSystem::inputDamage( DamageInfo& info )
{
	DamageComputer::computeDamageResult( info );
	applyDamage( info );
}


void UG_InputDamage( DamageInfo& info )
{
	CombatSystem::Get().inputDamage( info );
}