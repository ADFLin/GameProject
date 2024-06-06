#include "stage/TestStageHeader.h"

#include "Chess/ChessCore.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIUtility.h"

#include "RHI/RHIGraphics2D.h"
#include "GameRenderSetup.h"
#include "Renderer/MeshImportor.h"
#include "Renderer/SimpleCamera.h"
#include "Renderer/SceneView.h"
#include "Renderer/IBLResource.h"


#include "TestRenderStageBase.h"

#include <algorithm>

namespace Chess
{
	using namespace Render;


	class ChessShderProgram : public GlobalShaderProgram
	{
		using BaseClass = GlobalShaderProgram;
		DECLARE_SHADER_PROGRAM(ChessShderProgram, Global);

		static char const* GetShaderFileName()
		{
			return "Shader/Game/Chess";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}


		void bindParameters(ShaderParameterMap const& parameterMap) override
		{
			mParamIBL.bindParameters(parameterMap);
		}

		void setParameters(RHICommandList& commandList, IBLResource& IBL)
		{
			mParamIBL.setParameters(commandList, *this, IBL);
		}
		IBLShaderParameters mParamIBL;
	};


	class ChessBoardShderProgram : public ChessShderProgram
	{
		using BaseClass = ChessShderProgram;
		DECLARE_SHADER_PROGRAM(ChessShderProgram, Global);

		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(DRAW_BOARD), 1);
		}
	};

	class MeshUVCoordProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(MeshUVCoordProgram, Global);
	public:
		static char const* GetShaderFileName()
		{
			return "Shader/MeshUVCoord";
		}
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}
	};

	IMPLEMENT_SHADER_PROGRAM(ChessShderProgram);
	IMPLEMENT_SHADER_PROGRAM(ChessBoardShderProgram);
	IMPLEMENT_SHADER_PROGRAM(MeshUVCoordProgram);

	struct ScopedBlendState
	{
		ScopedBlendState(RHIGraphics2D& g, float alpha)
			:g(g)
		{
			g.setBlendState(ESimpleBlendMode::Translucent);
			g.setBlendAlpha(alpha);
		}

		~ScopedBlendState()
		{
			g.endBlend();
		}
		RHIGraphics2D& g;
	};

	class SceneRenderer
	{
	public:

		void render(Game& game, ViewInfo& view)
		{

			RHICommandList& commandList = RHICommandList::GetImmediateList();


			RHISetShaderProgram(commandList, mProgBoard->getRHI());

			mProgBoard->setTexture(commandList, SHADER_PARAM(NormalTexture), mTextures[1]);
			mProgBoard->setTexture(commandList, SHADER_PARAM(RoughnessTexture), mTextures[2]);

			LinearColor difColor = LinearColor(Color3ub(0xff, 0xfd, 0xd1), 0xff);
			LinearColor speColor = LinearColor::Black();
			mProgBoard->setParam(commandList, SHADER_PARAM(DiffuseColor), Vector4(difColor));
			mProgBoard->setParam(commandList, SHADER_PARAM(SpecularColor), Vector4(speColor));

			mProgBoard->setParameters(commandList, mIBLResource);

			mProgBoard->setParam(commandList, SHADER_PARAM(XForm), Matrix4::Identity());
			mBoardMesh.draw(commandList);

			RHISetShaderProgram(commandList, mProgChess->getRHI());

			mProgChess->setTexture(commandList, SHADER_PARAM(NormalTexture), mTextures[1]);
			mProgChess->setTexture(commandList, SHADER_PARAM(RoughnessTexture), mTextures[2]);

			mProgChess->setParam(commandList, SHADER_PARAM(DiffuseColor), Vector4(difColor));
			mProgChess->setParam(commandList, SHADER_PARAM(SpecularColor), Vector4(speColor));

			mProgChess->setParameters(commandList, mIBLResource);
			mProgChess->setParam(commandList, SHADER_PARAM(XForm), Matrix4::Identity());


			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				for (int i = 0; i < BOARD_SIZE; ++i)
				{
					Game::TileData const& tile = game.mBoard.getData(i, j);
					Vector3 tilePos = getTilePos(Vec2i(i, j));

					if (tile.chess)
					{
						Matrix4 rotation = Matrix4::Rotate(Quaternion::Rotate(Vector3(0,0,1), Math::DegToRad((tile.chess->color == EChessColor::White) ? -90 : 90)));
						//RHISetFixedShaderPipelineState(commandList, rotation * Matrix4::Translate(tilePos) * view.worldToClip);

						if (tile.chess->color == EChessColor::White)
						{
							LinearColor difColor = LinearColor( Color3ub(0xff, 0xfd, 0xd1) , 0xff );
							LinearColor speColor = LinearColor::Black();

							mProgChess->setTexture(commandList, SHADER_PARAM(BaseTexture), mTextures[0]);
							mProgChess->setParam(commandList, SHADER_PARAM(DiffuseColor), Vector4(difColor));
							mProgChess->setParam(commandList, SHADER_PARAM(SpecularColor), Vector4(speColor));
						}
						else
						{
							LinearColor difColor = LinearColor::Black();
							LinearColor speColor = LinearColor::White();

							mProgChess->setTexture(commandList, SHADER_PARAM(BaseTexture), mTextures[3]);
							mProgChess->setParam(commandList, SHADER_PARAM(DiffuseColor), Vector4(difColor));
							mProgChess->setParam(commandList, SHADER_PARAM(SpecularColor), Vector4(speColor));
						}

						mProgChess->setParam(commandList, SHADER_PARAM(XForm), rotation * Matrix4::Translate(tilePos));

						view.setupShader(commandList, *mProgChess);
						mChessMeshs[tile.chess->type].draw(commandList);
					}
				}
			}

			Vec2i screenSize = ::Global::GetScreenSize();

			{
				IntVector2 pos = { 120,120 };
				IntVector2 size = { 300, 300 };
				
				Matrix4 transform = Matrix4::Scale( size.x, size.y, 1 ) * Matrix4::Translate(pos.x, pos.y, 0) * Matrix4::Scale(2.0 / screenSize.x, 2.0 / screenSize.y, 1) * Matrix4::Translate(-1, -1, 0);

				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None, EFillMode::Wireframe>::GetRHI());

				RHISetShaderProgram(commandList, mProgMeshUV->getRHI());
				mProgMeshUV->setParam(commandList, SHADER_PARAM(XForm), transform);
				mChessMeshs[1].draw(commandList);

				Vector2 vertices[] =
				{
					Vector2( 0 , 0 ),
					Vector2( 1 , 0 ),
					Vector2( 1 , 1 ),
					Vector2( 0 , 1 ),
				};
				RHISetFixedShaderPipelineState(commandList, transform, LinearColor(1,1,0,1));
				TRenderRT< RTVF_XY >::Draw(commandList, EPrimitive::LineLoop, vertices, ARRAY_SIZE(vertices));
			}
		}

		Vector3 getTilePos(Vec2i const& pos)
		{
			Vector2 pos2D = 30.0f * (Vector2(pos - Vec2i(BOARD_SIZE / 2, BOARD_SIZE / 2) ) + Vector2(0.5,0.5));
			return Vector3(pos2D.x, pos2D.y, 0);
		}

		bool loadAsset()
		{

			char const* ChessNames[] =
			{
				"King",
				"Queen",
				"Bishop",
				"Knight",
				"Rook",
				"Pawn",
			};

			IMeshImporterPtr importer = MeshImporterRegistry::Get().getMeshImproter("FBX");
			if (importer)
			{
				for (int i = 0; i < ARRAY_SIZE(ChessNames); ++i)
				{
					InlineString<256> filePath;
					filePath.format("Model/Chess/%s.fbx", ChessNames[i]);

					VERIFY_RETURN_FALSE(importer->importFromFile(filePath, mChessMeshs[i], nullptr));
				}
			}
			else
			{
				LogWarning(0, "Cant Get FBX Mesh Importer");
				return false;
			}

			VERIFY_RETURN_FALSE(importer->importFromFile("Model/Chess/Board.fbx", mBoardMesh, nullptr));

			VERIFY_RETURN_FALSE(mProgChess = ShaderManager::Get().getGlobalShaderT< ChessShderProgram >());
			VERIFY_RETURN_FALSE(mProgBoard = ShaderManager::Get().getGlobalShaderT< ChessBoardShderProgram >());
			VERIFY_RETURN_FALSE(mProgMeshUV = ShaderManager::Get().getGlobalShaderT< MeshUVCoordProgram >());

			char const* textureNames[] = 
			{
				"Chess_Wood_Base.png",
				"Chess_Wood_Normal.png" ,
				"Chess_Wood_Roughtness.png",
				"Chess_Wood_Dark_Base.png",
			};
			for (int i = 0; i < 4; ++i)
			{
				InlineString<256> path;
				path.format("Model/Chess/textures/%s", textureNames[i]);
				VERIFY_RETURN_FALSE( mTextures[i] = RHIUtility::LoadTexture2DFromFile(path) );
			}

			{

				char const* HDRImagePath = "Texture/HDR/A.hdr";
				VERIFY_RETURN_FALSE(mHDRImage = RHIUtility::LoadTexture2DFromFile(::Global::DataCache(), HDRImagePath, TextureLoadOption().HDR()));
				VERIFY_RETURN_FALSE(mBuilder.loadOrBuildResource(::Global::DataCache(), HDRImagePath, *mHDRImage, mIBLResource));
			}
			
			return true;
		}

		Mesh mChessMeshs[EChess::COUNT];
		Mesh mBoardMesh;
		ChessShderProgram* mProgChess;
		ChessBoardShderProgram* mProgBoard;
		MeshUVCoordProgram* mProgMeshUV;

		RHITexture2DRef mTextures[4];

		IBLResourceBuilder mBuilder;
		IBLResource mIBLResource;

		RHITexture2DRef mHDRImage;
	};

	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		Game mGame;

		ViewInfo      mView;
		ViewFrustum   mViewFrustum;
		RHITexture2DRef mChessTex;
		SimpleCamera    mCamera;


		SceneRenderer mRenderer;
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();

			mView.gameTime = 0;
			mView.realTime = 0;
			mView.frameCount = 0;

			Vec2i screenSize = ::Global::GetScreenSize();
			mViewFrustum.mNear = 0.01;
			mViewFrustum.mFar = 800.0;
			mViewFrustum.mAspect = float(screenSize.x) / screenSize.y;
			mViewFrustum.mYFov = Math::DegToRad(60 / mViewFrustum.mAspect);

			mCamera.lookAt(Vector3(200, 200, 200), Vector3(0, 0, 0), Vector3(0, 0, 1));

			restart();
			return true;
		}


		Mesh mChessMeshs[EChess::COUNT];

		bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(mChessTex = RHIUtility::LoadTexture2DFromFile("texture/chess.png"));

			VERIFY_RETURN_FALSE(mRenderer.loadAsset());

			return true;
		}


		void preShutdownRenderSystem(bool bReInit = false) override
		{
			mChessTex.release();
		}


		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			mGame.restart();
		}
		void tick() {}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			mCamera.updatePosition(float(time) / 1000);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}


		float TileLength = 70;

		struct ChessImageTileSet 
		{
			int width;
			int height;
		};

		void drawChess(RHIGraphics2D& g, Vector2 const& centerPos, EChess::Type type, EChessColor color)
		{
			struct ImageTileInfo
			{
				Vec2i pos;
				Vec2i size;
			};

#if 1
			int const w = 64;
			int const h = 64;
			static const ImageTileInfo ChessTiles[] =
			{
				{Vec2i(w * 1,0),Vec2i(w,h)},
				{Vec2i(w * 0,0),Vec2i(w,h)},
				{Vec2i(w * 4,0),Vec2i(w,h)},
				{Vec2i(w * 3,0),Vec2i(w,h)},
				{Vec2i(w * 2,0),Vec2i(w,h)},
				{Vec2i(w * 5,0),Vec2i(w,h)},
			};
			Vector2 pivot = Vector2(0.5, 0.5);
			float sizeScale = 0.90f;
			
#else
			int const w = 200;
			int const h = 326;
			static const ImageTileInfo ChessTiles[] =
			{
				{Vec2i(0,0),Vec2i(w,h)},
				{Vec2i(w * 1,0),Vec2i(w,h)},
				{Vec2i(w * 4,0),Vec2i(w,h)},
				{Vec2i(w * 3,0),Vec2i(w,h)},
				{Vec2i(w * 2,0),Vec2i(w,h)},
				{Vec2i(w * 5,0),Vec2i(w,h)},
			};
			Vector2 pivot = Vector2(0.5, 0.7);
			float sizeScale = 0.8;
#endif
			ImageTileInfo const& tileInfo = ChessTiles[type];
			Vector2 size;
			size.x = sizeScale * tileInfo.size.x * TileLength / w;
			size.y = size.x * tileInfo.size.y / tileInfo.size.x;
			Vector2 texSize;
			texSize.x = mChessTex->getSizeX();
			texSize.y = mChessTex->getSizeY();
			g.drawTexture(centerPos - pivot * size , size , Vector2( tileInfo.pos + (( color == EChessColor::Black ) ? Vec2i(0,0) : Vec2i(0,h)) ) / texSize , Vector2( tileInfo.size ) / texSize );

		}

		bool bShow2D = true;

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			Vec2i screenSize = ::Global::GetScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);


			mView.frameCount += 1;

			mView.rectOffset = IntVector2(0, 0);
			mView.rectSize = screenSize;

			mView.setupTransform(mCamera.getPos(), mCamera.getRotation(), mViewFrustum.getPerspectiveMatrix());

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			RHISetViewport(commandList, mView.rectOffset.x, mView.rectOffset.y, mView.rectSize.x, mView.rectSize.y);
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

			RHISetFixedShaderPipelineState(commandList, mView.worldToClip);
			DrawUtility::AixsLine(commandList);

			mRenderer.render(mGame, mView);

			if ( bShow2D )
			{
				g.beginRender();
				render2D(g);
				g.endRender();
			}
		}

		void render2D(RHIGraphics2D& g)
		{
			RenderUtility::SetPen(g, EColor::Black);
			for (int j = 0; j < BOARD_SIZE; ++j)
			{
				for (int i = 0; i < BOARD_SIZE; ++i)
				{
					Game::TileData const& tile = mGame.mBoard.getData(i, j);
					Vector2 tilePos = toScreenPos(Vec2i(i, j));

					RenderUtility::SetBrush(g, (i + j) % 2 ? EColor::White : EColor::Black);
					g.drawRect(tilePos, Vector2(TileLength, TileLength));
				}
			}


			if (bShowAttackTerritory)
			{
				g.setTextColor(Color3ub(0, 255, 0));

				for (int j = 0; j < BOARD_SIZE; ++j)
				{
					for (int i = 0; i < BOARD_SIZE; ++i)
					{
						Game::TileData const& tile = mGame.mBoard.getData(i, j);
						Vector2 tilePos = toScreenPos(Vec2i(i, j));
						Vector2 tileCenterPos = tilePos + 0.5 * Vector2(TileLength, TileLength);

#if 0
						InlineString<256> str;
						str.format("%d %d", tile.whiteAttackCount, tile.blackAttackCount);
						g.drawText(tileCenterPos, str);
#endif


#if 1
#define BLEND_SCOPE( g , alpha ) 
#else
#define BLEND_SCOPE( g , alpha )  BlendScope scope( g , alpha);
#endif
						if (tile.blackAttackCount || tile.whiteAttackCount)
						{
							int border = 0.1 * TileLength;
							float alpha = 0.6;
							Vector2 renderPos = tilePos + Vector2(border, border);
							Vector2 renderSize = Vector2(TileLength, TileLength) - 2 * Vector2(border, border);

							EColor::Name const BlackAttackColor = EColor::Blue;
							EColor::Name const WhiteAttackColor = EColor::Red;

							BLEND_SCOPE(g, alpha);
							if (tile.blackAttackCount == 0)
							{
								RenderUtility::SetBrush(g, WhiteAttackColor);
								g.drawRect(renderPos, renderSize);
							}
							else if (tile.whiteAttackCount == 0)
							{
								RenderUtility::SetBrush(g, BlackAttackColor);
								g.drawRect(renderPos, renderSize);
							}
							else
							{
								if (tile.blackAttackCount != tile.whiteAttackCount)
								{
									float ratio = float(tile.blackAttackCount) / (tile.blackAttackCount + tile.whiteAttackCount);
									RenderUtility::SetBrush(g, BlackAttackColor);
									g.drawRect(renderPos, Vector2(renderSize.x, renderSize.y * ratio));
									RenderUtility::SetBrush(g, WhiteAttackColor);
									g.drawRect(renderPos + Vector2(0, renderSize.y * ratio), Vector2(renderSize.x, renderSize.y * (1 - ratio)));
								}
								else
								{
									RenderUtility::SetBrush(g, EColor::Yellow);
									g.drawRect(renderPos, renderSize);
								}
							}
						}
					}
				}
			}


			{
				ScopedBlendState scope(g, 1.0f);
				RenderUtility::SetBrush(g, EColor::White);

				g.setTexture(*mChessTex);
				for (int j = 0; j < BOARD_SIZE; ++j)
				{
					for (int i = 0; i < BOARD_SIZE; ++i)
					{
						Game::TileData const& tile = mGame.mBoard.getData(i, j);
						Vector2 tileCenterPos = toScreenPos(Vec2i(i, j)) + 0.5 * Vector2(TileLength, TileLength);
						if (tile.chess)
						{
							drawChess(g, tileCenterPos, tile.chess->type, tile.chess->color);
						}
					}
				}
			}


			bool bShowAttackList = true;
			if (bShowAttackList)
			{
				ScopedBlendState scope(g, 0.8f);
				if (mGame.isValidPos(mMouseTilePos))
				{
					auto& tileData = mGame.mBoard.getData(mMouseTilePos.x, mMouseTilePos.y);

					RenderUtility::SetBrush(g, EColor::Cyan);
					for (auto const& attack : tileData.attacks)
					{
						Vector2 tileCenterPos = toScreenPos(attack.pos) + 0.5 * Vector2(TileLength, TileLength);
						g.drawCircle(tileCenterPos, 12);
					}
				}
			}


			{
				ScopedBlendState scope(g, 0.8f);
				if (!mMovePosList.empty())
				{
					RenderUtility::SetBrush(g, EColor::Green);
					for (auto const& move : mMovePosList)
					{
						Vector2 tileCenterPos = toScreenPos(move.pos) + 0.5 * Vector2(TileLength, TileLength);
						g.drawCircle(tileCenterPos, 12);
					}
				}
			}
		}

		Vector2 toScreenPos(Vec2i const& tPos)
		{
			return Vector2(10, 10) + TileLength * Vector2( tPos.x , BOARD_SIZE - tPos.y - 1 );
		}

		Vec2i toTilePos(Vector2 const& pos)
		{
			if ( bShow2D )
			{
				Vector2 temp = (pos - Vector2(10, 10)) / TileLength;
				Vec2i result;
				result.x = Math::FloorToInt(temp.x);
				result.y = BOARD_SIZE - Math::FloorToInt(temp.y) - 1;
				if (result)
				{

				}
				return result;
			}

			return Vec2i(INDEX_NONE, INDEX_NONE);
		}

		void moveChess(Vec2i const& from, MoveInfo const& move)
		{
			mGame.moveChess(from, move);
			invalidateSelectedChess();
		}

		void invalidateSelectedChess()
		{
			mSelectedChessPos.x = INDEX_NONE;
			mSelectedChessPos.y = INDEX_NONE;
			mMovePosList.clear();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			Vec2i tPos = toTilePos(msg.getPos());

			mMouseTilePos = tPos;
			if (msg.onLeftDown())
			{
				if (mGame.isValidPos(mSelectedChessPos))
				{
					Game::ChessData const* chessSelected = mGame.getTile(mSelectedChessPos).chess;
					bool bCanMove = msg.isControlDown() || mGame.getCurTurnColor() == chessSelected->color;
					if ( bCanMove )
					{
						MoveInfo const* moveUsed = nullptr;
						for (auto const& move : mMovePosList)
						{
							if (move.pos == tPos)
							{
								moveUsed = &move;
								break;
							}
						}

						if (moveUsed)
						{
							if (moveUsed->bCapture)
							{
								Game::TileData const& removeChessTile = mGame.getTile(moveUsed->posEffect);
							}

							moveChess(mSelectedChessPos, *moveUsed);

							if (moveUsed->tag == EMoveTag::Promotion)
							{
								EChess::Type promotionType = EChess::Queen;
								//#TODO: user choice promoted chess

								mGame.promotePawn(*moveUsed, promotionType);
							}
						}
					}
				}
				else
				{
					mMovePosList.clear();
					if (mGame.getPossibleMove(tPos, mMovePosList))
					{
						mSelectedChessPos = tPos;
					}
				}
			}
			else if (msg.onRightDown())
			{
				invalidateSelectedChess();
			}

			static Vec2i oldPos = msg.getPos();

			if (msg.onLeftDown())
			{
				oldPos = msg.getPos();
			}
			if (msg.onMoving() && msg.isLeftDown())
			{
				float rotateSpeed = 0.01;
				Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
				mCamera.rotateByMouse(off.x, off.y);
				oldPos = msg.getPos();
			}

			return BaseClass::onMouse(msg);
		}

		bool bShowAttackTerritory = false;
		Vec2i mSelectedChessPos;
		Vec2i mMouseTilePos;
		TArray<MoveInfo> mMovePosList;

		MsgReply onKey(KeyMsg const& msg) override
		{
			float baseImpulse = 500;
			switch (msg.getCode())
			{
			case EKeyCode::W: mCamera.moveForwardImpulse = msg.isDown() ? baseImpulse : 0; break;
			case EKeyCode::S: mCamera.moveForwardImpulse = msg.isDown() ? -baseImpulse : 0; break;
			case EKeyCode::D: mCamera.moveRightImpulse = msg.isDown() ? baseImpulse : 0; break;
			case EKeyCode::A: mCamera.moveRightImpulse = msg.isDown() ? -baseImpulse : 0; break;
			case EKeyCode::Z: mCamera.moveUp(0.5); break;
			case EKeyCode::X: mCamera.moveUp(-0.5); break;
			}


			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::X: bShowAttackTerritory = !bShowAttackTerritory; break;
				case EKeyCode::V: bShow2D = !bShow2D; break;
				case EKeyCode::U: mGame.undo(); break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}


		ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::D3D11;
		}

	protected:
	};

	REGISTER_STAGE_ENTRY("Chess", TestStage, EExecGroup::Dev4, "Game");


}//namespace Chess
