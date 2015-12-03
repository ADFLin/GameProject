#ifndef PokerBase_h__
#define PokerBase_h__

#include <iostream>

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
		enum Face
		{
			eACE = 0,
			eN2 , eN3 , eN4 , eN5 , eN6 , 
			eN7 , eN8 , eN9 , eN10 ,
			eJACK , eQUEEN , eKING ,
			eJOKER ,
		};

		enum Suit
		{
			eCLUBS    = 0,
			eDIAMONDS = 1,
			eHEARTS   = 2,
			eSPADES   = 3,
		};

		Card(){}
		explicit Card(int index); // index = 0 ~ 51
		Card(Suit suit , int faceRank );

		Face   getFace()     const { return mFace; }
		Suit   getSuit()     const { return mSuit; }
		int    getFaceRank() const { return toRank( mFace ); }
		int    getIndex()    const { return toIndex( getSuit() , getFace() ); }

		bool operator == (Card const& card) const;

		static int         toRank( Face face ){ return int( face ); }
		static int         toIndex( Suit suit , Face face ){ return face * 4 + suit; }
		static char const* toString( Face face );

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
		Suit mSuit;
		Face mFace;
	};

	std::ostream& operator << ( std::ostream& o , Card const& card );



	inline std::ostream& operator << (std::ostream& o,Card const& card)
	{
		static const char suit[]={ 0x05,0x04,0x03,0x06 };
		o << suit[card.getSuit()] << Card::toString( card.getFace() );
		return o;
	}

	inline Card::Card( int index )
	{
		mSuit     = Suit( index % 4 );
		mFace     = Face( index / 4 );
	}

	inline Card::Card( Suit suit,int faceRank ) 
		:mSuit(suit) ,mFace( Face( faceRank) )
	{

	}

	inline char const* Card::toString( Face face )
	{
		char const* faceStr[]={"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
		return faceStr[ face ];
	}

	inline bool Card::operator==( Card const& card ) const
	{
		return mSuit == card.getSuit() && 
			   mFace == card.getFace();
	}

}

#endif // PokerBase_h__
