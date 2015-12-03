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

ConVar< bool > gPlayerNoDamage( false , "playerNoDamage" );


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


		//���m�̸˥ҭ�
		float WDT = dtProp->getPropValue( PROP_DT );  //�@�ҭ�
		float END = dtProp->getPropValue( PROP_END ); //�@�O

		float STR = atProp->getPropValue( PROP_STR ); //�O�q
		

		///////////////////�q�o�̶}�l///////////////////////
		float INT = atProp->getPropValue( PROP_INT ); //���O(�i�[�]�k����)
		float DEX = atProp->getPropValue( PROP_DEX ); //�ӱ�(�i�[���q����)
		float MAT = atProp->getPropValue( PROP_MAT ); //�D�����(�i�[���q����)
		float SAT = atProp->getPropValue( PROP_SAT ); //�Ƥ����(�i�[���q����)
		float AT_SPEED = atProp->getPropValue( PROP_AT_SPEED ); //�����t��()


		int atLevel = atActor->getPropValue(PROP_LEVEL); 
		int dtLevel = dtActor->getPropValue(PROP_LEVEL);

		
		float TOTAL_DT = WDT + END + 0.001 * WDT * END;
		float TOTAL_AT = info.val + ( (STR + DEX) * 0.5 + atLevel*2 );

		float DR = ( WDT + 15 * dtLevel ) / ( WDT + 1500 ); //����򥻴�˦ʤ���(�p����oNPC�����ˮ`���??)

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
	CombatSystem::getInstance().inputDamage( info );
}