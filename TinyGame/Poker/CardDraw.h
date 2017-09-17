#ifndef CardDraw_h__
#define CardDraw_h__

#include "PokerBase.h"
class Graphics2D;
#include "TVector2.h"
typedef TVector2< int > Vec2i;

namespace Poker
{
	class ICardDraw
	{
	public:

		virtual void  draw( Graphics2D& g , Vec2i const& pos , Card const& card ) = 0;
		virtual void  drawCardBack( Graphics2D& g , Vec2i const& pos ) = 0;
		virtual Vec2i getSize() = 0;
		virtual void  release() = 0;

		enum StyleType
		{
			eWinXP ,
			eWin7 ,
			eDebug  ,
		};
		static ICardDraw* Create( StyleType type );
	};

}//namespace Poker
#endif // CardDraw_h__
