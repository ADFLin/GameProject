#pragma once
#ifndef RichActionCard_H_02E66B7A_C4D3_4DA8_95DE_EA1FE8DBB486
#define RichActionCard_H_02E66B7A_C4D3_4DA8_95DE_EA1FE8DBB486

#include "DataStructure/CycleQueue.h"

namespace Rich
{
	class ActionEvent;

	struct ActionCard
	{
		char const* desc;
		ActionEvent* action;
	};


	class ICardSet
	{
	public:
		virtual ~ICardSet() = default;
		virtual int         getChanceCardNum() = 0;
		virtual ActionCard& getChanceCard(int index) = 0;

		virtual int getCommunityCardNum() = 0;
		virtual ActionCard& getCommunityCard(int index) = 0;


		static ICardSet& Get();
	};
	

	class CardDeck
	{
	public:

		void initialize()
		{

		}

		ActionCard* peekCard() { return mQueue.front(); }

		void advanceCard(bool bRemove)
		{
			if (bRemove)
			{
				mQueue.pop_front();
			}
			else
			{
				mQueue.moveFrontToBack();
			}
		}


		TCycleQueue<ActionCard*> mQueue;
	};


}//namespace Rich

#endif // RichActionCard_H_02E66B7A_C4D3_4DA8_95DE_EA1FE8DBB486