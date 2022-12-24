#ifndef CardDraw_h__
#define CardDraw_h__

#include "PokerBase.h"
#include "Math/TVector2.h"
#include "Rect.h"

typedef TVector2< int > Vec2i;
class IGraphics2D;

namespace Render
{
	class RHITexture2D;
}

namespace Poker
{

	class ICardResource
	{
	public:
		virtual ~ICardResource(){}
		virtual Render::RHITexture2D& getTexture() = 0;
		virtual TRect<float> getUVRect(Card const& card) const = 0;
		virtual TRect<float> getBackUVRect(Card const& card) const = 0;
	};

	class ICardDraw
	{
	public:

		virtual void  draw(IGraphics2D& g, Vec2i const& pos, Card const& card ) = 0;
		virtual void  drawCardBack(IGraphics2D& g, Vec2i const& pos ) = 0;
		virtual Vec2i getSize() = 0;
		virtual void  release() = 0;
		virtual ICardResource* getResource() { return nullptr; }

		enum StyleType
		{
			eWinXP ,
			eWin7 ,
			eDebug  ,
		};
		static ICardDraw* Create(StyleType type);
	};

	class ICardResourceSetup
	{
	public:
		virtual ~ICardResourceSetup(){}
		virtual void setupCardDraw(ICardDraw* cardDraw) = 0;
	};

}//namespace Poker
#endif // CardDraw_h__
