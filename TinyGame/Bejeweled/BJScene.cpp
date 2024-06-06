#include "TinyGamePCH.h"
#include "BJScene.h"

#include "RenderUtility.h"
#include "SystemMessage.h"
#include "EasingFunction.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIUtility.h"
#include "Image/ImageData.h"
#include "InlineString.h"
#include "FileSystem.h"

namespace Bejeweled
{
	struct TextureLoadInfo
	{
		char const* base;
		char const* mask;
	};

	TextureLoadInfo const GTexFiles[] =
	{
		{ "al_litgems.gif", nullptr },
		{ "bigstar.gif", nullptr },
		{ "selector.gif" , "selector_.gif"},
		{ "hypergem.jpg" , "hypergem_.gif" },
		{ "rock.gif" , "rock_.gif" },
		{ "FRAME.gif", "FRAME_.gif" },
		{ "SCOREPOD.jpg" , "scorePod_.gif"},
	};

	struct SpriteAnimDesc
	{
		int   numFrame;
		float duration;
	};

	struct SpriteData
	{
		float time;
	} GSprite{ 0 };

	void updateSpriteAnim(float deltaTime)
	{
		GSprite.time += deltaTime;
	}

	RHITexture2D* LoadImageWithMask(char const* path, char const* maskPath)
	{
		ImageData image;
		if (!image.load(path, ImageLoadOption().UpThreeComponentToFour()))
		{
			return nullptr;
		}

		ImageData imageMask;
		if (!imageMask.load(maskPath))
		{
			return nullptr;
		}

		if (image.width != imageMask.width || image.height != imageMask.height)
			return nullptr;

		if (image.numComponent != 4 || imageMask.numComponent != 4)
			return nullptr;

		Color4ub* pData = (Color4ub*)image.data;
		Color4ub* pMaskData = (Color4ub*)imageMask.data;
		for (int i = 0; i < image.width * image.height; ++i)
		{
			pData->a = pMaskData->r;
			pData += 1;
			pMaskData += 1;
		}

		return RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, image.width, image.height), image.data);
	}

	Scene::Scene() 
		:mLevel( this )
		,mBoardPos( 50 , 50 )
	{
		mGemMoveVec.reserve( Level::BoardStorageSize );
	}


	char const* ImageDir = "Bejeweled/images";

	bool Scene::loadBackground()
	{
		std::vector<std::string> bgFileList;
		InlineString<> path;
		path.format("%s/backdrops/%s", ImageDir, "backdrops.lst");
		if (!FFileUtility::LoadLines(path, bgFileList))
			return false;

		for (auto const& fileName : bgFileList)
		{
			path.format("%s/backdrops/%s.jpg", ImageDir, fileName.c_str());
			auto texture = RHIUtility::LoadTexture2DFromFile(path);
			mBGTexturs.push_back(texture);
		}
		return true;
	}


	bool Scene::initializeRHI()
	{

		for (int i = 0; i < GemImageCount; ++i)
		{
			InlineString<> path;
			path.format("%s/gem%d.gif", ImageDir, i);
			InlineString<> pathMask;
			pathMask.format("%s/gem%d_.gif", ImageDir, i);
			mTexGems[i] = LoadImageWithMask(path, pathMask);
		}

		for (int i = 0; i < ETextureId::COUNT; ++i)
		{
			auto const& loadFile = GTexFiles[i];
			if (loadFile.mask)
			{
				InlineString<> path;
				path.format("%s/%s", ImageDir, loadFile.base);
				InlineString<> pathMask;
				pathMask.format("%s/%s", ImageDir, loadFile.mask);
				mTexMap[i] = LoadImageWithMask(path, pathMask);
			}
			else
			{
				InlineString<> path;
				path.format("%s/%s", ImageDir, loadFile.base);
				mTexMap[i] = RHIUtility::LoadTexture2DFromFile(path);
			}
		}

		if (!loadBackground())
			return false;
		

		return true;
	}

	void Scene::drawGem( IGraphics2D& g , Vector2 const& pos , GemData type )
	{
		int const gemColor[] =
		{
			EColor::Yellow , EColor::White,EColor::Blue ,EColor::Red ,
			EColor::Purple , EColor::Orange ,EColor::Green ,
			EColor::Cyan   ,
		};

		if (type.type == GemData::eHypercube)
		{
			return;
		}

		switch (type.meta % 3)
		{
		case 1:
			{
				int len = CellLength - 15;
				RenderUtility::SetPen(g, EColor::Black);
				RenderUtility::SetBrush(g, gemColor[type.meta], COLOR_DEEP);
				g.drawRoundRect(pos - Vec2i(len / 2, len / 2),
					Vec2i(len, len), Vec2i(10, 10));
				RenderUtility::SetPen(g, EColor::White);
				RenderUtility::SetBrush(g, gemColor[type.meta]);
				len -= 8;
				g.drawRoundRect(pos - Vec2i(len / 2, len / 2),
					Vec2i(len, len), Vec2i(6, 6));
			}
			break;
		case 2:
			{
				int radius = (CellLength - 12) / 2;
				RenderUtility::SetPen(g, EColor::Black);
				RenderUtility::SetBrush(g, gemColor[type.meta], COLOR_DEEP);
				g.drawCircle(pos, radius);
				RenderUtility::SetPen(g, EColor::White);
				RenderUtility::SetBrush(g, gemColor[type.meta]);
				g.drawCircle(pos, radius - 4);
			}
			break;
		case 3:
			{




			}
			//break;
		default:
			{
				int len = (CellLength - 10) / 2;
				int factor = 12;
				Math::Vector2 const vtx[] =
				{
					Vec2i(factor , -1) , Vec2i(factor , 1) ,
					Vec2i(1 , factor) , Vec2i(-1 , factor) ,
					Vec2i(-factor , 1) , Vec2i(-factor , -1) ,
					Vec2i(-1 , -factor)  , Vec2i(1 , -factor)
				};
				Math::Vector2 rPos[8];

				RenderUtility::SetPen(g, EColor::Black);
				RenderUtility::SetBrush(g, gemColor[type.meta], COLOR_DEEP);
				for (int i = 0; i < 8; ++i)
					rPos[i] = pos + (len * vtx[i]) / factor;
				g.drawPolygon(rPos, 8);
				RenderUtility::SetPen(g, EColor::White);
				RenderUtility::SetBrush(g, gemColor[type.meta]);
				len -= 6;
				for (int i = 0; i < 8; ++i)
					rPos[i] = pos + (len * vtx[i]) / factor;
				g.drawPolygon(rPos, 8);
			}
		}
	}


	void Scene::drawGem(RHIGraphics2D& g, Vector2 const& pos, GemData type, int layer)
	{
		if (layer == 0)
		{

			Vector2 size = Vector2(CellLength, CellLength);
			if (type.type == GemData::eHypercube)
			{
				SpriteAnimDesc desc{ 40, 1 };
				auto& tex = mTexMap[ETextureId::HyperGem];
				int frame = 0;
				frame = Math::FloorToInt(GSprite.time * desc.numFrame / desc.duration) % desc.numFrame;
				g.drawTexture(*tex, pos - 0.5 * size, size, Vector2(frame / float(desc.numFrame), 0), Vector2(1.0f / float(desc.numFrame), 1));
			}
			else
			{
				SpriteAnimDesc desc{ 20, 1 };
				auto& tex = mTexGems[type.meta];
				//bool bUseFrame = false;
				int frame = 0;
				//frame = Math::FloorToInt(GSprite.time * desc.numFrame / desc.duration) % desc.numFrame;
				g.drawTexture(*tex, pos - 0.5 * size, size, Vector2(frame / float(desc.numFrame), 0), Vector2(1.0f / float(desc.numFrame), 1));
			}

		}
		else
		{
			if (type.type == GemData::ePower)
			{
				auto& tex = mTexMap[ETextureId::BigStar];
				Vector2 size = Vector2(tex->getSizeX(), tex->getSizeY());
				g.setBlendState(ESimpleBlendMode::Add);
				g.drawTexture(*tex, pos - 0.5 * size, size);
				g.setBlendState(ESimpleBlendMode::Translucent);
			}
		}
	}

	void Scene::render(IGraphics2D& g)
	{
		bool bDebugDraw = bDrawDebugForce || !g.isUseRHI();

		if (bDebugDraw)
		{
			renderDebug(g);
		}
		else
		{
			RHIGraphics2D& gImpl = g.getImpl<RHIGraphics2D>();
			renderRHI(gImpl);
		}


		RenderUtility::SetFont( g , FONT_S8 );
		InlineString< 128 > str;
		Vec2i texPos( 10 , 10 );
		str.format( "CtrlMode = %d" , (int)mCtrlMode );
		g.drawText( texPos , str );
		texPos.y += 15;
		str.format( "MoveCell = ( %d , %d )" , mMoveCell.x , mMoveCell.y );
		g.drawText( texPos , str );
		texPos.y += 15;
		str.format("Turn = %d", (int)mTurnCount);
		g.drawText(texPos, str);
		texPos.y += 15;
	}

	void Scene::renderDebug(IGraphics2D& g)
	{
		for (int i = 0; i < Level::BoardStorageSize; ++i)
		{
			Vec2i renderPos = mBoardPos + CellLength * Vec2i(i % BoardSize, i / BoardSize);

			RenderUtility::DrawBlock(g, renderPos, Vec2i(CellLength, CellLength), EColor::Gray);

			if (mState == eChoiceSwapGem && (mGemRenderFlag[i] & RF_POSIBLE_SWAP))
			{
				g.beginBlend(renderPos, Vec2i(CellLength, CellLength), mFadeOutAlpha);
				RenderUtility::DrawBlock(g, renderPos, Vec2i(CellLength, CellLength), EColor::Red);
				g.endBlend();
			}
		}

		if (mState == eChoiceSwapGem && mCtrlMode == CM_CHOICE_GEM2)
		{
			int idx = mIndexSwapCell[0];
			Vec2i renderPos = mBoardPos + CellLength * Vec2i(idx % BoardSize, idx / BoardSize);
			g.beginBlend(renderPos, Vec2i(CellLength, CellLength), mFadeOutAlpha);
			RenderUtility::DrawBlock(g, renderPos, Vec2i(CellLength, CellLength), EColor::White);
			g.endBlend();
		}

		for (int i = 0; i < Level::BoardStorageSize; ++i)
		{
			if (mGemRenderFlag[i] & RF_RENDER_ANIM)
				continue;

			GemData type = getLevel().getBoardGem(i);
			if (type == GemData::None())
				continue;

			Vector2 renderPos;
			renderPos = mBoardPos + CellLength * Vec2i(i % BoardSize, i / BoardSize);
			renderPos += Vec2i(CellLength, CellLength) / 2;

			drawGem(g, renderPos, type);
		}

		for (auto& move : mGemMoveVec)
		{
			Vector2 renderPos;
			renderPos = mBoardPos + move.pos;

			GemData type = getLevel().getBoardGem(move.index);
			renderPos += Vec2i(CellLength, CellLength) / 2;

			drawGem(g, renderPos, type);
		}

		for (auto& fadeOut : mGemFadeOutVec)
		{
			GemData type = fadeOut.type;
			int idx = fadeOut.index;

			Vec2i cellPos = mBoardPos + CellLength * Vec2i(idx % BoardSize, idx / BoardSize);
			Vector2 renderPos = cellPos + Vector2(CellLength, CellLength) / 2;

			g.beginBlend(cellPos, Vec2i(CellLength, CellLength), mFadeOutAlpha);
			drawGem(g, renderPos, type);
			g.endBlend();
		}
	}

	void Scene::renderRHI(RHIGraphics2D& g)
	{
		g.setBrush(Color3ub(128, 128, 128));
		g.enablePen(false);
		g.drawRect(Vector2(0, 0), ::Global::GetScreenSize());
		g.enablePen(true);

		g.drawTexture(*mBGTexturs[mIndexBG], Vector2(0,0));


		g.setBlendState(ESimpleBlendMode::Translucent);
		g.setBrush(Color3ub::White());
		g.drawTexture(*mTexMap[ETextureId::Frame], mBoardPos - Vector2(43, 30));

		g.setBlendAlpha(mFadeOutAlpha);
		if (mState == eChoiceSwapGem)
		{
			for (int i = 0; i < Level::BoardStorageSize; ++i)
			{
				if (mGemRenderFlag[i] & RF_POSIBLE_SWAP)
				{
					Vec2i renderPos = mBoardPos + CellLength * Vec2i(i % BoardSize, i / BoardSize);
					RenderUtility::DrawBlock(g, renderPos, Vec2i(CellLength, CellLength), EColor::Red);
				}
			}
		}
		g.setBrush(Color3ub::White());
		g.setBlendAlpha(1.0f);

		for( int layer = 0 ; layer < 2; ++layer )
		{
			for (int i = 0; i < Level::BoardStorageSize; ++i)
			{
				if (mGemRenderFlag[i] & RF_RENDER_ANIM)
					continue;

				GemData type = getLevel().getBoardGem(i);
				if (type == GemData::None())
					continue;

				Vector2 renderPos;
				renderPos = mBoardPos + CellLength * Vec2i(i % BoardSize, i / BoardSize);
				renderPos += Vec2i(CellLength, CellLength) / 2;

				drawGem(g, renderPos, type, layer);

			}

			for (auto& move : mGemMoveVec)
			{
				Vector2 renderPos;
				renderPos = mBoardPos + move.pos;

				GemData type = getLevel().getBoardGem(move.index);
				renderPos += Vec2i(CellLength, CellLength) / 2;

				drawGem(g, renderPos, type, layer);
			}
		}

		g.setBlendAlpha(mFadeOutAlpha);
		for (auto& fadeOut : mGemFadeOutVec)
		{
			GemData type = fadeOut.type;
			int idx = fadeOut.index;

			Vec2i cellPos = mBoardPos + CellLength * Vec2i(idx % BoardSize, idx / BoardSize);
			Vector2 renderPos = cellPos + Vector2(CellLength, CellLength) / 2;
			drawGem(g, renderPos, type, 0);
		}

		g.setBlendAlpha(1.0f);
		if (mState == eChoiceSwapGem && mCtrlMode == CM_CHOICE_GEM2)
		{
			int idx = mIndexSwapCell[0];
			Vec2i renderPos = mBoardPos + CellLength * Vec2i(idx % BoardSize, idx / BoardSize);
			g.drawTexture(*mTexMap[ETextureId::Selector], renderPos, Vector2(CellLength, CellLength));
			//RenderUtility::DrawBlock(g, renderPos, Vec2i(CellLength, CellLength), EColor::White);
		}

		if ( mHoveredIndex != INDEX_NONE && mState == eChoiceSwapGem)
		{
			GemData type = getLevel().getBoardGem(mHoveredIndex);

			Vec2i cellPos = Vec2i(mHoveredIndex % BoardSize, mHoveredIndex / BoardSize);
			Vector2 renderPos;
			renderPos = mBoardPos + CellLength * cellPos;
			renderPos += Vec2i(CellLength, CellLength) / 2;

			static constexpr int FrameKeys[] = { 3 ,7, 8 , 1 , 5 , 0 , 4, 8 , 2 , 6 };
			SpriteAnimDesc desc;
			desc.numFrame = ARRAY_SIZE(FrameKeys);
			desc.duration = 1.3;

			float frameF = Math::Fmod(GSprite.time * desc.numFrame / desc.duration, desc.numFrame);

			int frame = Math::FloorToInt(frameF);
			float frameFrac = frameF - frame;
			int frame2 = (frame + 1) % desc.numFrame;


			Vector2 size = Vec2i(CellLength, CellLength);
			g.setBlendState(ESimpleBlendMode::Add);
			g.setTexture(*mTexMap[ETextureId::GemAdd]);
			g.setBrush(Color3f(1 - frameFrac, 1 - frameFrac, 1 - frameFrac));
			g.drawTexture(renderPos - 0.5 * size, size, Vector2(FrameKeys[frame] / float(9), float(type.meta) / 9), Vector2(1.0f / float(9), float(1) / 9));
			g.setBrush(Color3f(frameFrac, frameFrac, frameFrac));
			g.drawTexture(renderPos - 0.5 * size, size, Vector2(FrameKeys[frame2] / float(9), float(type.meta) / 9), Vector2(1.0f / float(9), float(1) / 9));

			auto GetDirOffset = [](int key) -> Vec2i
			{
				switch (key)
				{
				case 0: return Vec2i(0, -1); 
				case 1: return Vec2i(-1, -1);
				case 2: return Vec2i(-1, 0); 
				case 3: return Vec2i(-1, 1); 
				case 4: return Vec2i(0, 1); 
				case 5: return Vec2i(1, 1); 
				case 6: return Vec2i(1, 0); 
				case 7: return Vec2i(1, -1);
				}
			};

			auto DrawNeighborLight = [&](int key, float scale)
			{
				if (key == 8)
					return;

				Vec2i pos = cellPos + GetDirOffset(key);

				if (Level::IsVaildRange(pos.x, pos.y))
				{
					GemData type = getLevel().getBoardGem(Level::ToIndex(pos.x, pos.y));
					Vector2 renderPos = mBoardPos + CellLength * pos;
					renderPos += Vec2i(CellLength, CellLength) / 2;
					g.setBrush(Color3f(scale, scale, scale));
					g.drawTexture(renderPos - 0.5 * size, size, Vector2(((key + 4 ) % 8 ) / float(9), float(type.meta) / 9), Vector2(1.0f / float(9), float(1) / 9));
				}
			};

			float RefLight = 0.3;

			DrawNeighborLight(FrameKeys[frame], (1 - frameFrac) * RefLight);
			DrawNeighborLight(FrameKeys[frame2], frameFrac * RefLight);

			g.setBrush(Color3ub::White());
			g.setBlendState(ESimpleBlendMode::Translucent);
		}

		g.setBlendState(ESimpleBlendMode::None);
		g.setBlendAlpha(1.0f);
	}

	void Scene::restart()
	{
		mTurnCount = 0;
		std::fill_n( mGemRenderFlag , Level::BoardStorageSize , 0 );
		mGemFadeOutVec.clear();
		mGemMoveVec.clear();
		getLevel().generateRandGems();
		mCtrlMode = CM_CHOICE_GEM1;
		changeState( eChoiceSwapGem );
		checkAllPosibleSwapPair();

		//mIndexBG = rand() % mBGTexturs.size();
	}

	void Scene::tick()
	{
		mStateTime += gDefaultTickTime;
		for (;;)
		{
			State prevState = mState;
			evoluteState();
			if (prevState == mState)
				break;
		}
	}


	void Scene::evoluteState()
	{
		switch (mState)
		{
		case eChoiceSwapGem:
			if (bAutoPlay)
			{
				if (!mPosibleSwapPairVec.empty())
				{
					bool bChioceBest = true;
					int indexChoice;
					if (bChioceBest)
					{
						struct Listener : public Level::Listener
						{

						};
						Listener listener;
						ActionContext context;
						int maxScore = 0;
						for (int index = 0; index < mPosibleSwapPairVec.size(); ++index)
						{
							Level level(getLevel(), &listener);

							context.swapIndices[0] = mPosibleSwapPairVec[index].idx1;
							context.swapIndices[1] = mPosibleSwapPairVec[index].idx2;
							level.swapSequence(context, false);

							int numPosibleSwapPair = 0;
							VisitPosibleSwapPair(level, [&](int, int)
							{
								++numPosibleSwapPair;
							});

							int score = numPosibleSwapPair;
							if (maxScore == 0 || maxScore < score)
							{
								indexChoice = index;
							}
						}
					}
					else
					{
						indexChoice = rand() % mPosibleSwapPairVec.size();
					}

					mIndexSwapCell[0] = mPosibleSwapPairVec[indexChoice].idx1;
					mIndexSwapCell[1] = mPosibleSwapPairVec[indexChoice].idx2;
					if (Level::isNeighboring(mIndexSwapCell[0], mIndexSwapCell[1]))
					{
						mActionContext.swapIndices[0] = mIndexSwapCell[0];
						mActionContext.swapIndices[1] = mIndexSwapCell[1];

						swapGem(mIndexSwapCell[0], mIndexSwapCell[1]);
						mCtrlMode = CM_CHOICE_GEM1;
						changeState(eSwapGem);
					}
				}
			}
			break;
		case eSwapGem:
			if (mStateTime > TimeSwapGem)
			{
				++mTurnCount;
				removeAllGemAnim();
				if (getLevel().stepCheckSwap(mActionContext))
				{
					getLevel().stepAction(mActionContext);
					changeState(eDestroyGem);
				}
				else
				{
					swapGem(mIndexSwapCell[0], mIndexSwapCell[1]);
					changeState(eRestoreGem);
				}
			}
			break;
		case eRestoreGem:
			if (mStateTime > TimeSwapGem)
			{
				removeAllGemAnim();
				changeState(eChoiceSwapGem);
			}
			break;
		case eDestroyGem:
			if (mStateTime > TimeDestroyGem)
			{
				mGemFadeOutVec.clear();
				for (int i = 0; i < BoardSize; ++i)
					mNumFill[i] = 0;
				getLevel().stepFill(mActionContext);
				changeState(eFillGem);
			}
			break;
		case eFillGem:
			if (mStateTime > TimeFillGem)
			{
				removeAllGemAnim();
				if (getLevel().stepCheckFill(mActionContext))
				{
					getLevel().stepAction(mActionContext);
					changeState(eDestroyGem);
				}
				else
				{
					changeState(eChoiceSwapGem);
					checkAllPosibleSwapPair();
				}
			}
			break;
		}
	}

	void Scene::updateFrame(int frame)
	{
		updateAnim( gDefaultTickTime * frame );

		switch( mState )
		{
		case eDestroyGem: 
			mFadeOutAlpha = float( TimeDestroyGem - mStateTime ) / TimeDestroyGem;
			break;
		case eChoiceSwapGem:
			mFadeOutAlpha = 0.5f + 0.25f * Math::Sin( 0.005f * (float)mStateTime ); 
			break;
		}

		updateSpriteAnim(float(gDefaultTickTime * frame) / 1000.0);
	}

	MsgReply Scene::procMouseMsg( MouseMsg const& msg )
	{
		Vec2i pos = getCellPos(msg.getPos());
		if (Level::IsVaildRange(pos.x,pos.y))
		{
			mHoveredIndex = Level::ToIndex(pos.x, pos.y);
		}
		else
		{
			mHoveredIndex = INDEX_NONE;
		}

		if ( mState == eChoiceSwapGem )
		{
			switch( mCtrlMode )
			{
			case CM_CHOICE_GEM1:
				if ( msg.onLeftDown() )
				{
					if (mHoveredIndex != INDEX_NONE)
					{
						mIndexSwapCell[0] = mHoveredIndex;
						mCtrlMode = CM_CHOICE_GEM2;
					}
				}
				break;
			case CM_CHOICE_GEM2:
				if ( msg.onLeftDown() )
				{
					if (mHoveredIndex != INDEX_NONE)
					{
						mIndexSwapCell[1] = mHoveredIndex;
						if ( Level::isNeighboring( mIndexSwapCell[0] , mIndexSwapCell[1] ) )
						{
							mActionContext.swapIndices[0] = mIndexSwapCell[0];
							mActionContext.swapIndices[1] = mIndexSwapCell[1];

							swapGem( mIndexSwapCell[0] , mIndexSwapCell[1] );
							mCtrlMode = CM_CHOICE_GEM1;
							changeState( eSwapGem );
						}
						else
						{
							mIndexSwapCell[0] = mIndexSwapCell[1];
						}
					}
				}
				else if ( msg.onRightDown() )
				{
					mCtrlMode = CM_CHOICE_GEM1;
				}
				break;
			}
		}
		return MsgReply::Handled();
	}

	void Scene::updateAnim( long dt )
	{
		mTweener.update( (float) dt );
	}

	void Scene::addGemAnim( int idx , Vec2i const& from , Vec2i const& to , long time )
	{
		assert( ( mGemRenderFlag[ idx ] & RF_RENDER_ANIM ) == 0 );
		mGemMoveVec.push_back( GemMove() );
		GemMove& move = mGemMoveVec.back();
		move.index = idx;
		mTweener.tweenValue< Easing::OCubic >( move.pos , Vector2( from ) , Vector2( to ) , (float)time );

		mGemRenderFlag[ idx ] |= RF_RENDER_ANIM;
	}

	void Scene::removeAllGemAnim()
	{
		for(auto& move : mGemMoveVec)
		{
			mGemRenderFlag[ move.index ] &= ~RF_RENDER_ANIM;
		}
		mGemMoveVec.clear();
		mTweener.clear();
	}

	void Scene::swapGem( int idx1 , int idx2 )
	{
		getLevel().swapGeom(idx1, idx2);
		Vec2i pos1 = CellLength * getCellPos( idx1 );
		Vec2i pos2 = CellLength * getCellPos( idx2 );
		addGemAnim( idx1 , pos2 , pos1 , TimeSwapGem );
		addGemAnim( idx2 , pos1 , pos2 , TimeSwapGem );
	}

	MsgReply Scene::procKey( KeyMsg const& msg )
	{
		if ( msg.isDown() )
		{
			if (mCtrlMode == CM_CHOICE_GEM2)
			{
				switch (msg.getCode())
				{
				case EKeyCode::Num1:
				case EKeyCode::Num2:
				case EKeyCode::Num3:
				case EKeyCode::Num4:
				case EKeyCode::Num5:
				case EKeyCode::Num6:
				case EKeyCode::Num7:
					getLevel().setBoardGem(mIndexSwapCell[0], GemData::Normal(msg.getCode() - EKeyCode::Num1));
					mCtrlMode = CM_CHOICE_GEM1;
					break;
				case EKeyCode::Z:
					{
						int index = mIndexSwapCell[0];
						GemData type;
						getLevel().setBoardGem(index, GemData::Hypercube());
						mCtrlMode = CM_CHOICE_GEM1;
					}
					break;
				case EKeyCode::X:
					{
						int index = mIndexSwapCell[0];
						GemData type = getLevel().getBoardGem(index);
						if (type.type == GemData::eNormal)
						{
							type.type = GemData::ePower;
							getLevel().setBoardGem(index, type);
							mCtrlMode = CM_CHOICE_GEM1;
						}
					}
					break;
				}
			}
		}

		return MsgReply::Unhandled();
	}

	void Scene::checkAllPosibleSwapPair()
	{
		removeAllPosibleSwapPair();

		VisitPosibleSwapPair(mLevel, [&](int indexA, int indexB)
		{
			addPosibleSwapPair(indexA, indexB);
		});
	}


	void Scene::addPosibleSwapPair( int idx1 , int idx2 )
	{
		IndexPair pair;
		pair.idx1 = idx1;
		pair.idx2 = idx2;
		mPosibleSwapPairVec.push_back( pair );

		mGemRenderFlag[ idx1 ] |= RF_POSIBLE_SWAP ;
		mGemRenderFlag[ idx2 ] |= RF_POSIBLE_SWAP ;
	}

	void Scene::removeAllPosibleSwapPair()
	{
		for(IndexPair& pair : mPosibleSwapPairVec)
		{
			mGemRenderFlag[ pair.idx1 ] &= ~RF_POSIBLE_SWAP ;
			mGemRenderFlag[ pair.idx2 ] &= ~RF_POSIBLE_SWAP ;
		}
		mPosibleSwapPairVec.clear();
	}

	void Scene::onFillGem( int idx , GemData type )
	{
		int x = idx % BoardSize;
		mNumFill[ x ] += 1;

		Vec2i to = CellLength * getCellPos( idx );
		Vec2i from;
		from.x = to.x;
		from.y = -CellLength * mNumFill[ x ];

		addGemAnim( idx , from , to , TimeFillGem );
	}

	void Scene::onDestroyGem( int idx , GemData type )
	{
		GemFadeOut fadeOut;
		fadeOut.index = idx;
		fadeOut.type  = type;
		mGemFadeOutVec.push_back( fadeOut );
	}

	void Scene::onMoveDownGem( int idxFrom , int idxTo )
	{
		addGemAnim( idxTo , CellLength * getCellPos( idxFrom ) , CellLength * getCellPos( idxTo ) , TimeFillGem );
	}

	void Scene::changeState( State state )
	{
		if ( mState == state )
			return;

		mState = state;
		mStateTime = 0;
	}

}//namespace Bejeweled
