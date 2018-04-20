#include "TinyGamePCH.h"
#include "CardDraw.h"

#include "resource.h"
#include "libcards.h"

#include "DrawEngine.h"
#include "RenderUtility.h"

static CardLib gCardLib;

namespace Poker {

	class CCardDraw : public ICardDraw
	{
	public:
		CCardDraw()
		{
			mBugFixDC.Initialize( ::Global::getGraphics2D().getTargetDC() , CardLib::CardSizeX , CardLib::CardSizeY );
		}
		Vec2i getSize()
		{
			return Vec2i( CardLib::CardSizeX , CardLib::CardSizeY );
		}
		void draw( Graphics2D& g , Vec2i const& pos , Card const& card )
		{
			if( card == Card::None() )
				return;
			cdtDraw( mBugFixDC.getDC() , 0  , 0 , card.getIndex() , mdFACES , RGB( 0,0,0 )  );
			mBugFixDC.bitBlt( g.getRenderDC() , pos.x , pos.y  );
		}
		void drawCardBack( Graphics2D& g , Vec2i const& pos )
		{
			cdtDraw( mBugFixDC.getDC() , 0 , 0 , 58 , mdBACKS, RGB( 255 , 255 , 255 ) );
			mBugFixDC.bitBlt( g.getRenderDC() , pos.x , pos.y  );
		}
		void release()
		{
			delete this;
		}
		BitmapDC mBugFixDC;
	};


	struct PosInfo
	{
		int x , y;
		int numRow;
	};
	PosInfo const CardNumPos[2][4]  = 
	{
		{{ 8 , 0 , 2 } , { 10 , 0 , 2 } ,{ 12 , 0 , 2 } , { 4 , 7 , 4 } },
		{{ 8 , 5 , 2 } , { 10 , 5 , 2 } ,{ 12 , 5 , 2 } , { 0 , 7 , 4 } }
	};
	PosInfo const CardPicPos[4] = {{ 0 , 0 , 2 } , { 4 , 0 , 2 } ,{ 6 , 0 , 2 } ,{ 2 , 0 , 2 } };
	PosInfo const CardBackPos[4] = {{ 0 , 6 , 1 } , { 4 , 6 , 1 } ,{ 6 , 6 , 1 } ,{ 2 , 6 , 1 } };

	class Win7CardShow : public ICardDraw
	{
	public:
		enum
		{
			TYPE_A ,
			TYPE_B ,
			TYPE_C ,
			TYPE_BIG ,
		};
		Win7CardShow( HDC hDC )
			:mBmpDC( hDC , IDB_CARDS )
			,CardSize( 73 , 98 )
		{
			type = TYPE_B;
			calcCardPos();
		}

		Vec2i getSize()
		{
			return CardSize;
		}

		Vec2i getImagePos( Card const& card )
		{
			Vec2i out;
			if ( card.getFaceRank() < 10 )
			{
				PosInfo const& info = CardNumPos[ type == TYPE_BIG ? 1 : 0 ][ card.getSuit() ];
				int order = card.getFaceRank();
				out.x = info.x + card.getFaceRank() % info.numRow;
				out.y = info.y + card.getFaceRank() / info.numRow;
			}
			else
			{
				PosInfo const& info = CardPicPos[ type ];
				int const orderFactor[] = { 0 , 1 , 3 , 2 };
				int order = 3 * orderFactor[ card.getSuit() ] + card.getFaceRank() - 10;
				out.x = info.x + order % info.numRow;
				out.y = info.y + order / info.numRow;
			}

			return out;
		}
		void draw( Graphics2D& g , Vec2i const& pos , Card const& card )
		{
			if( card == Card::None() )
				return;

			Vec2i const& posImg = mCardPos[ card.getIndex() ];
			mBmpDC.bitBlt( g.getRenderDC() , pos.x , pos.y , posImg.x , posImg.y , CardSize.x , CardSize.y );
		}
		void drawCardBack( Graphics2D& g , Vec2i const& pos )
		{
			PosInfo const& info = CardBackPos[ type ];
			int x = info.x;
			int y = info.y;
			mBmpDC.bitBlt( g.getRenderDC() , pos.x , pos.y , x * CardSize.x , y * CardSize.y , CardSize.x , CardSize.y );
		}

		void release()
		{
			delete this;
		}
		void  calcCardPos()
		{
			for( int i = 0 ; i < 52 ; ++i )
			{
				mCardPos[i] = getImagePos( Card(i) ).mul( CardSize );
			}
		}

		Vec2i    mCardPos[ 52 ];
		int      type;
		Vec2i    CardSize;
		BitmapDC mBmpDC;
	};


	class DBGCardDraw : public ICardDraw
	{
	public:
		static Vec2i CardSize;
		Vec2i getSize()
		{
			return CardSize;
		}
		static int const RoundSize = 12;
		void draw( Graphics2D& g , Vec2i const& pos , Card const& card )
		{
			if( card == Card::None() )
				return;

			RenderUtility::SetPen(g, Color::eBlack);
			RenderUtility::SetBrush(g, Color::eWhite);
			g.drawRoundRect( pos , CardSize , Vec2i(RoundSize, RoundSize));
			if( Card::isBlackSuit(card) )
				g.setTextColor(0, 0, 0);
			else
				g.setTextColor(255, 0, 0);
			char const* suitStr[] = { "C" , "D" , "H" , "S" };
			FixString< 128 > str;
			str.format("%s%s", suitStr[card.getSuit()], Card::ToString(card.getFace()) );
			g.drawText( pos, Vec2i(25,20), str.c_str());
			g.drawText( pos + CardSize - Vec2i(25, 20), Vec2i(25, 20), str.c_str());
		}
		void drawCardBack( Graphics2D& g , Vec2i const& pos )
		{
			RenderUtility::SetPen(g, Color::eBlack);
			RenderUtility::SetBrush(g, Color::eWhite);
			g.drawRoundRect( pos , CardSize , Vec2i(RoundSize, RoundSize));
			RenderUtility::SetBrush(g, Color::eRed);
			Vec2i border = Vec2i(4, 4);
			g.drawRoundRect(pos +border , CardSize - 2 * border , Vec2i(RoundSize, RoundSize));
		}
		void release()
		{
			delete this;
		}
	};

	Vec2i DBGCardDraw::CardSize( 73 , 98 );

	ICardDraw* ICardDraw::Create( StyleType type )
	{
		switch( type )
		{
		case eWin7:		
			return new Win7CardShow( Global::getDrawEngine()->getWindow().getHDC() );
		case eWinXP:
			if ( CardLib::isInitilized() )
				return new CCardDraw;
		}
		return new DBGCardDraw;
	}



}//namespace Poker