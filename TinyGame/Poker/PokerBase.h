#ifndef PokerBase_h__
#define PokerBase_h__

#include <iostream>
#include <cassert>

#include "Core/IntegerType.h"

namespace Poker
{

	class IRandom
	{
	public:
		virtual int getInt() = 0;
		virtual int getInt( int form , int to )
		{
			return form + getInt() % ( to - form );
		}
	};

	class Card
	{
	public:
		enum Face : int8
		{
			eINVALID = -1,
			eACE = 0,
			eN2 , eN3 , eN4 , eN5 , eN6 , 
			eN7 , eN8 , eN9 , eN10 ,
			eJACK , eQUEEN , eKING ,
			eJOKER,
		};

		enum Suit : int8
		{
			eNONE     = -1,
			eCLUBS    = 0,
			eDIAMONDS = 1,
			eHEARTS   = 2,
			eSPADES   = 3,
		};

		enum
		{
			StandardCardNum = 52,
			TarotCardNum = 22,
			TotalCardNum = StandardCardNum + TarotCardNum,
		};

		Card() = default;

		// index = 0 ~ 51 for standard cards, 52 ~ 73 for tarot cards.
		explicit Card(int index)
		{
			assert(0 <= index && index < TotalCardNum);
			if (index < StandardCardNum)
			{
				mSuit = Suit(index % 4);
				mFace = Face(index / 4);
			}
			else
			{
				mSuit = eNONE;
				mFace = Face(index - StandardCardNum);
			}
		}
		Card(Suit suit , int faceRank )
			:mSuit(suit)
			,mFace(Face(faceRank))
		{

		}

		static Card const None() { return Card(Suit::eNONE, Face::eINVALID); }
		static Card const Joker(int suit = 0) { return Card(Suit(suit), Face::eJOKER); }
		static Card const Tarot(int index) { return Card(Suit::eNONE, index); }

		Face   getFace()     const { return mFace; }
		Suit   getSuit()     const { return mSuit; }
		int    getFaceRank() const { return ToRank( mFace ); }
		int    getIndex()    const 
		{ 
			if (isTarot())
				return StandardCardNum + getTarotIndex();
			if (isNone())
				return -1;
			return ToIndex(getSuit(), getFace());
		}
		int    getTarotIndex() const { return int(mFace); }
		bool   isNone() const { return mSuit == eNONE && mFace == eINVALID; }
		bool   isTarot() const { return mSuit == eNONE && mFace >= 0; }
		bool   isStandard() const { return eCLUBS <= mSuit && mSuit <= eSPADES && eACE <= mFace && mFace <= eKING; }

		bool operator == (Card const& card) const;

		static int         ToRank( Face face ){ return int( face ); }
		static int         ToIndex( Suit suit , Face face ){ return face * 4 + suit; }
		static char const* ToString( Face face );
		static char const* ToTarotString(int index);

		static bool isRedSuit( Card const& c )
		{ 
			return c.getSuit() == Card::eDIAMONDS ||  
				   c.getSuit() == Card::eHEARTS;
		}

		static bool isBlackSuit( Card const& c)
		{ 
			return c.getSuit() == Card::eCLUBS ||  
				   c.getSuit() == Card::eSPADES;
		}

		static bool isSameColorSuit(Card const& c1 , Card const& c2)
		{
			bool isRed1 = isRedSuit(c1);
			bool isRed2 = isRedSuit(c2);
			return  ( isRed1 == isRed2 );
		}

	private:
		union 
		{
			struct  
			{
				Suit mSuit;
				Face mFace;
			};

			uint16 mValue;
		};

	};

	std::ostream& operator << ( std::ostream& o , Card const& card );


	inline std::ostream& operator << (std::ostream& o,Card const& card)
	{
		if (card.isNone())
		{
			o << "None";
			return o;
		}
		if (card.isTarot())
		{
			o << "T" << card.getTarotIndex();
			return o;
		}
		static const char suit[]={ 0x05,0x04,0x03,0x06 };
		o << suit[card.getSuit()] << Card::ToString( card.getFace() );
		return o;
	}

	inline char const* Card::ToString( Face face )
	{
		char const* faceStr[]={"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
		if (face < eACE || face > eKING)
			return "?";
		return faceStr[ face ];
	}

	inline char const* Card::ToTarotString(int index)
	{
		static char const* tarotStr[] =
		{
			"Fool", "Magician", "Priestess", "Empress", "Emperor", "Hierophant",
			"Lovers", "Chariot", "Strength", "Hermit", "Wheel", "Justice",
			"Hanged", "Death", "Temperance", "Devil", "Tower", "Star",
			"Moon", "Sun", "Judgement", "World"
		};
		if (index < 0 || index >= TarotCardNum)
			return "?";
		return tarotStr[index];
	}

	inline bool Card::operator==( Card const& card ) const
	{
		return mValue == card.mValue;
	}

}

#endif // PokerBase_h__
