#include "RichActionCard.h"

#include "RichActionEvent.h"

namespace Rich
{
#define CARD( DESC , ACTION )\
	{ DESC , new ACTION },

	ActionCard GChanceCards[] =
	{
		CARD("Go directly to go", ActionEvent_GoDirectly)
		CARD("Give 150 from bank", ActionEvent_GetMoney(150))
		CARD("Give 50 from bank", ActionEvent_GetMoney(50))
		CARD("Go To Target Area", ActionEvent_GoTagArea(EAreaTag::Chance_MoveTo))

		CARD("Pay 15 to bank", ActionEvent_PayDebt(15))
	};

	ActionCard GCommunityCards[] =
	{
		CARD("Give 100 from bank", ActionEvent_GetMoney(100))
		CARD("Give 100 from bank", ActionEvent_GetMoney(100))


		CARD("Give 45 from bank", ActionEvent_GetMoney(45))
		CARD("Pay 150 to bank", ActionEvent_PayDebt(150))
		CARD("Give 25 from bank", ActionEvent_GetMoney(25))
		CARD("Pay 100 to bank", ActionEvent_PayDebt(100))

	};

#undef CARD

	class CardSet : public ICardSet
	{

	public:
		int getChanceCardNum() override
		{
			return ARRAY_SIZE(GChanceCards);
		}

		ActionCard& getChanceCard(int index) override
		{
			return GChanceCards[index];
		}

		int getCommunityCardNum() override
		{
			return ARRAY_SIZE(GCommunityCards);
		}

		ActionCard& getCommunityCard(int index) override
		{
			return GChanceCards[index];
		}
	};

	ICardSet& ICardSet::Get()
	{
		static CardSet StaticCardSet;
		return StaticCardSet;
	}

}//namespace Rich