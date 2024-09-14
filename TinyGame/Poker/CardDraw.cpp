#include "TinyGamePCH.h"
#include "CardDraw.h"

#include "resource.h"
#include "libcards.h"

#include "RenderUtility.h"
#include "GameGraphics2D.h"

#include "DrawEngine.h"

#include "RHI/RHICommand.h"
#include "RenderDebug.h"
#include "Core/ScopeGuard.h"
#include "RHI/RHIGraphics2D.h"
#include "Image/ImageData.h"

static CardLib gCardLib;

namespace Poker {

	using namespace Render;

	class CCardDraw : public ICardDraw
	{
	public:
		CCardDraw()
		{
			mBugFixDC.initialize( ::Global::GetGraphics2D().getTargetDC() , CardLib::CardSizeX , CardLib::CardSizeY );
		}
		Vec2i getSize() override
		{
			return Vec2i( CardLib::CardSizeX , CardLib::CardSizeY );
		}
		void draw(IGraphics2D& g, Vec2i const& pos , Card const& card ) override
		{
			if( card == Card::None() )
				return;

			Graphics2D& impl = g.getImpl< Graphics2D >();
			cdtDraw( mBugFixDC.getHandle() , 0  , 0 , card.getIndex() , mdFACES , RGB( 0,0,0 )  );
			mBugFixDC.bitBltTo( impl.getRenderDC() , pos.x , pos.y  );
		}
		void drawCardBack(IGraphics2D& g, Vec2i const& pos ) override
		{
			Graphics2D& impl = g.getImpl< Graphics2D >();
			cdtDraw( mBugFixDC.getHandle() , 0 , 0 , 58 , mdBACKS, RGB( 255 , 255 , 255 ) );
			mBugFixDC.bitBltTo( impl.getRenderDC() , pos.x , pos.y  );
		}
		void release() override
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


	class Win7CardDrawBase : public ICardDraw
	{
	public:
		enum
		{
			TYPE_A,
			TYPE_B,
			TYPE_C,
			TYPE_BIG,
		};
		Win7CardDrawBase()
			:CardSize(73, 98)
		{
			type = TYPE_B;
			calcCardPos();
		}

		Vec2i getSize() override
		{
			return CardSize;
		}

		Vec2i getImagePos(Card const& card)
		{
			Vec2i out;
			if (card.getFaceRank() < 10)
			{
				PosInfo const& info = CardNumPos[type == TYPE_BIG ? 1 : 0][card.getSuit()];
				int order = card.getFaceRank();
				out.x = info.x + card.getFaceRank() % info.numRow;
				out.y = info.y + card.getFaceRank() / info.numRow;
			}
			else
			{
				PosInfo const& info = CardPicPos[type];
				int const orderFactor[] = { 0 , 1 , 3 , 2 };
				int order = 3 * orderFactor[card.getSuit()] + card.getFaceRank() - 10;
				out.x = info.x + order % info.numRow;
				out.y = info.y + order / info.numRow;
			}
			return out;
		}

		void release() override
		{
			delete this;
		}
		void  calcCardPos()
		{
			for (int i = 0; i < 52; ++i)
			{
				mCardPos[i] = getImagePos(Card(i)).mul(CardSize);
			}

			PosInfo const& info = CardBackPos[type];
			mCardBackPos.x = info.x * CardSize.x;
			mCardBackPos.y = info.y * CardSize.y;
		}

		Vec2i    mCardPos[52];
		Vec2i    mCardBackPos;
		int      type;
		Vec2i    CardSize;
	};

	class Win7CardDraw : public Win7CardDrawBase
	{
	public:

		Win7CardDraw( HDC hDC )
			:mBmpDC( hDC , IDB_CARDS )
		{
	
		}

		void draw(IGraphics2D& g, Vec2i const& pos , Card const& card ) override
		{
			if( card == Card::None() )
				return;
			Graphics2D& impl = g.getImpl< Graphics2D >();
			Vec2i const& posImg = mCardPos[ card.getIndex() ];
			mBmpDC.bitBltTo(impl.getRenderDC() , pos.x , pos.y , posImg.x , posImg.y , CardSize.x , CardSize.y );
		}
		void drawCardBack(IGraphics2D& g, Vec2i const& pos ) override
		{
			Graphics2D& impl = g.getImpl< Graphics2D >();
			mBmpDC.bitBltTo( impl.getRenderDC() , pos.x , pos.y , mCardBackPos.x , mCardBackPos.y , CardSize.x , CardSize.y );
		}

		BitmapDC mBmpDC;
	};

	class Win7CardDrawRHI : public Win7CardDrawBase
		                  , public ICardResource
	{
	public:
		Win7CardDrawRHI()
		{
#if 1
			HANDLE hHandle = LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARDS), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
			HANDLE hMask = LoadImageA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_CARDS_MASK), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

			ON_SCOPE_EXIT
			{
				DeleteObject(hHandle);
				DeleteObject(hMask);
			};

			BITMAP bmp , bmpMask;
			if (GetObject(hHandle, sizeof(bmp), &bmp) && GetObject(hMask, sizeof(bmpMask), &bmpMask))
			{
				textSizeInv.x = 1.0f / bmp.bmWidth;
				textSizeInv.y = 1.0f / bmp.bmHeight;

				cardUVSize = Vector2(CardSize).mul(textSizeInv);

				TArray< Color4ub > pixelData;
				pixelData.resize(bmp.bmWidth * bmp.bmHeight);

				int compCount = bmp.bmBitsPixel / 8;
				Color4ub* pPixel = pixelData.data();
				for (int j = 0; j < bmp.bmHeight; ++j)
				{
					uint8 const* pSrc = static_cast<uint8 const*>(bmp.bmBits) + ( bmp.bmHeight - 1 - j ) * bmp.bmWidthBytes;
					uint8 const* pSrcMask = static_cast<uint8 const*>(bmpMask.bmBits) + (bmp.bmHeight - 1 - j) * bmpMask.bmWidthBytes;
					for (int i = 0; i < bmp.bmWidth; ++i)
					{
						uint8 a = *pSrcMask;
						a = 0xff;
						*pPixel = Color4ub(pSrc[2], pSrc[1], pSrc[0], a);

						++pPixel;
						pSrc += compCount;
						pSrcMask += 1;
					}
				}

				mTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, bmp.bmWidth, bmp.bmHeight), pixelData.data());

				GTextureShowManager.registerTexture("Card", mTexture);
			}
#else
			HRSRC hRes = FindResource(GetModuleHandle(NULL), MAKEINTRESOURCE(IDR_DATA1), "DATA");
			HGLOBAL hMem = LoadResource(GetModuleHandle(NULL), hRes);
			int size = SizeofResource(GetModuleHandle(NULL), hRes);
			void* pData = LockResource(hMem);
			ImageData imageData;
			if (imageData.loadFromMemory(pData, size, ImageLoadOption().UpThreeComponentToFour()))
			{
				mTexture = RHICreateTexture2D(ETexture::RGBA8, imageData.width, imageData.height, 0, 1, TCF_DefalutValue, imageData.data);
				GTextureShowManager.registerTexture("Card", mTexture);
			}
#endif

		}

		void draw(IGraphics2D& g, Vec2i const& pos, Card const& card) override
		{
			if (card == Card::None())
				return;
			drawCard(g, pos, mCardPos[card.getIndex()]);
		}

		void drawCardBack(IGraphics2D& g, Vec2i const& pos) override
		{
			drawCard(g, pos, mCardBackPos);
		}

		void drawCard(IGraphics2D& g, Vec2i const& pos, Vector2 const& posUV)
		{
			RHIGraphics2D& impl = g.getImpl< RHIGraphics2D >();
			impl.setBlendState(ESimpleBlendMode::Translucent);
			impl.drawTexture(*mTexture, pos, CardSize, posUV.mul(textSizeInv), cardUVSize);
		}

		Vector2 cardUVSize;
		RHITexture2DRef mTexture;
		Vector2 textSizeInv;

		Render::RHITexture2D& getTexture() override
		{
			return *mTexture;
		}

		TRect<float> getUVRect(Card const& card) const override
		{
			return getCardUVRect( mCardPos[card.getIndex()]);
		}
		TRect<float> getBackUVRect(Card const& card) const override
		{
			return getCardUVRect(mCardBackPos);
		}

		TRect<float> getCardUVRect(Vector2 const& pos) const
		{
			Vector2 min = Vector2(pos).mul(textSizeInv);
			TRect<float> result;
			result.pos = min;
			result.size = cardUVSize;
			return result;
		}
		ICardResource* getResource() override
		{
			return this;
		}
	};

	class DebugCardDraw : public ICardDraw
	{
	public:
		static Vec2i CardSize;
		Vec2i getSize() override
		{
			return CardSize;
		}
		static int const RoundSize = 12;
		void draw(IGraphics2D& g, Vec2i const& pos , Card const& card ) override
		{
			if( card == Card::None() )
				return;
			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::White);
			g.drawRoundRect( pos , CardSize , Vec2i(RoundSize, RoundSize));
			if( Card::isBlackSuit(card) )
				g.setTextColor(Color3ub(0, 0, 0));
			else
				g.setTextColor(Color3ub(255, 0, 0));
			char const* suitStr[] = { "C" , "D" , "H" , "S" };
			InlineString< 128 > str;
			str.format("%s%s", suitStr[card.getSuit()], Card::ToString(card.getFace()) );
			g.drawText( pos, Vec2i(25,20), str.c_str());
			g.drawText( pos + CardSize - Vec2i(25, 20), Vec2i(25, 20), str.c_str());
		}
		void drawCardBack(IGraphics2D& g, Vec2i const& pos ) override
		{
			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::White);
			g.drawRoundRect( pos , CardSize , Vec2i(RoundSize, RoundSize));
			RenderUtility::SetBrush(g, EColor::Red);
			Vec2i border = Vec2i(4, 4);
			g.drawRoundRect(pos +border , CardSize - 2 * border , Vec2i(RoundSize, RoundSize));
		}
		void release() override
		{
			delete this;
		}
	};

	Vec2i DebugCardDraw::CardSize( 73 , 98 );

	ICardDraw* ICardDraw::Create( StyleType type )
	{
		switch( type )
		{
		case eWin7:		
			if (Global::GetDrawEngine().isRHIEnabled())
			{
				return new Win7CardDrawRHI();
			}
			else
			{
				return new Win7CardDraw(Global::GetDrawEngine().getWindow().getHDC());
			}
		case eWinXP:
			if ( CardLib::isInitialized() )
				return new CCardDraw;
		}
		return new DebugCardDraw;
	}



}//namespace Poker