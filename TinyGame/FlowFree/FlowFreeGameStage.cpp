#include "FlowFreeGameStage.h"
#include "RHI/RHIGraphics2D.h"


#include "DrawEngine.h"
#include "RHI/RHICommand.h"
#include "InputManager.h"

using namespace Render;

REGISTER_STAGE_ENTRY("Flow Free Game", FlowFree::LevelStage , EExecGroup::Test, "Game");


namespace FlowFree
{
	struct LevelData
	{
		int sizeX;
		int sizeY;
		int type;
		char const* mapData;
	};

	namespace EBoundType
	{
		enum 
		{
			Close,
			Open,
		};
	}
	LevelData Level15_0 = { 15 , 15 , EBoundType::Close ,
		"a - - - - - b - - - - - - - -"
		"- - - - - - - - - - - - d c -"
		"- - - - - - - - - - - - c - d"
		"- - j - - - k - - - - - - - e"
		"- - - - - - - - - - - - a - -"
		"- - - - - - - - - - - l - - -"
		"- f - - - - m - - - - - - - -"
		"- - - - g - - n p - - - - - -"
		"- - - - - h - - - - - - - - -"
		"- - g - - - i - - - k - - - -"
		"- - - - - - j o - - p - - b -"
		"- - - i - h - - - - - - - - -"
		"- - - - - - - - - o - - - - -"
		"- - - - - - - n - - - f - - -"
		"- - - - - - - - m - - - - l e"
	};

	LevelData Level15_1 = { 15 , 15 , EBoundType::Close ,
		"a - - - - - - - e - - - - - -"
		"- - - - - - - - - - - - - - -"
		"- - i - o - - - - - - - c d -"
		"- - - - - - - - - - c - e b -"
		"a - - - - g - - - - d - - - -"
		"- - - - - - - - - - - - i b -"
		"- - - - - f - - - - - n - - -"
		"- - - - - - - - - - - m - - -"
		"- - - - - - - - - - - - - - -"
		"- - - - h - i j - - k l - - -"
		"- - i - - - j - - f - - - - -"
		"- - g - - - - - - - - - - - -"
		"- - h - - - - - - o - - l - -"
		"- - - - - - - - - - - - - - -"
		"- - - - - - - k - - - - n m -"
	};

	LevelData Level1 = { 8 , 8 , EBoundType::Close ,
		"a - - a b - - -"
		"- - f g c - c -"
		"- - d - - - - -"
		"- - e - i h - -"
		"- - - - g - - -"
		"- - - f - - - -"
		"- d - e - - i b"
		"- - - - - - - h"
	};

	LevelData Level2 = { 8 , 8 , EBoundType::Close ,
		"- - - - - - d c"
		"- - - a f - - -"
		"- a b - - - e d"
		"- - - - - - - -"
		"- - - - - - - -"
		"- - - c - e f -"
		"- b - - - - - -"
		"- - - - - - - -"
	};

	LevelData Level3 = { 10 , 10 , EBoundType::Close ,
		"- - - - - - - - - -"
		"- - - - - - - - - -"
		"- - - - - - - e - -"
		"- d e - - - - - d -"
		"- i - - - h f - c -"
		"- - - - - g - - - -"
		"- - a f g - - - - -"
		"- - - - - - - - - b"
		"- h - i - - - - - c"
		"- b - - - - - - - a"
	};

	LevelData Level4 = { 9 , 9 , EBoundType::Close ,
		"- - - - - - - - -"
		"- a - - - - - - -"
		"- - - - - d e - -"
		"c b f - - e - - -"
		"h g - - - - - - -"
		"- - - - - - - d -"
		"- - - - - - - c -"
		"- - - g - - - - -"
		"- - - h f - - b a"
	};


	LevelData Level5 = { 4 , 3 , EBoundType::Close ,
		"a - a d"
		"b - b -"
		"c - c d"
	};

	LevelData Level6 = { 3 , 2 , EBoundType::Close ,
		"- - a"
		"a b b"
	};

	LevelData Level7 = { 9 , 9 , EBoundType::Close ,
		"- - - a - - - - -"
		"- - b - - a - - -"
		"- - - - # # b - c"
		"- - - d - - - - -"
		"- - - - - - - - -"
		"- - # # - - - - -"
		"- - # # - c d - -"
		"- - - - - - - - -"
		"- - - - - - - - -"
	};

	LevelData Level_Unsolvable2 = { 12 , 15 , EBoundType::Close ,
		"- - - a - - - - - - - -"
		"- b - - - c - - - - - -"
		"- a - - - - - - - - - -"
		"- d - - - e - - f - - -"
		"- - - - - - - - - - - -"
		"- - - - f - - - - - - -"
		"- - - - - - g - - - - -"
		"- - - - h - - - g - - -"
		"- - - - - - - - - - - -"
		"- b - - - - - - - - - -"
		"- c - - i - - - - - - -"
		"- - - - - - - e - - - -"
		"- - j - - - - - k h - -"
		"- - k - - - j - - - - -"
		"d - - - - i - - - - - -"
	};

#define DEFAULT_LEVEL Level_Unsolvable2

	std::string ExportLevel(Level const& level, bool bStringLiteral = false)
	{
		std::string result;
		Vec2i size = level.getSize();

		for (int j = 0; j < size.y; ++j )
		{
			if (bStringLiteral)
			{
				result += '\"';
			}
			for (int i = 0; i < size.x; ++i)
			{
				Cell const& cell = level.getCellChecked(Vec2i(i, j));
				switch (cell.func)
				{
				case CellFunc::Empty:
					result += '-';
					break;
				case CellFunc::Source:
					{
						char c = 'a' + cell.funcMeta - 1;
						result += c;
					}
					break;
				case CellFunc::Bridge:
					result += '+';
					break;
				case CellFunc::Tunnel:
					break;
				case CellFunc::Block:
					result += '#';
					break;
				}

				if (i != size.x - 1)
				{
					result += ' ';
				}
			}

			if (bStringLiteral)
			{
				result += '\"';
			}
			result += '\n';
		}

		return result;
	}

	void ImportLevel(Level& level, char const* mapData )
	{
		int index = 0;
		while (*mapData != 0)
		{
			char c = *mapData;
			switch (c)
			{
			case ' ':
			case '\n':
			case '\r':
				{
					--index;
				}
				break;
			case '-':
				{

				}
				break;
			case '+':
				{
					level.setCellFunc(Vec2i(index % level.getSize().x, index / level.getSize().x), CellFunc::Bridge);
				}
				break;
			case '#':
				{
					level.setCellFunc(Vec2i(index % level.getSize().x, index / level.getSize().x), CellFunc::Block);
				}
				break;
			default:
				{
					int color = c - 'a' + 1;
					level.addSource(Vec2i(index % level.getSize().x, index / level.getSize().x), color);
				}
			}
			++index;
			++mapData;
		}
	}

	void LoadLevel(Level& level, LevelData& data)
	{
		level.setup(Vec2i(data.sizeX, data.sizeY));
		if (data.type == EBoundType::Close)
		{
			level.addMapBoundBlock();
		}
		ImportLevel(level, data.mapData);
	}


	int const CellLength = 30;
	Vec2i ScreenOffset = Vec2i(20, 20);
	int const FlowWidth = 12;
	int const FlowGap = (CellLength - FlowWidth) / 2;
	int const FlowSourceRadius = 10;

	class LoadParamFrame : public DevFrame
	{
		using BaseClass = DevFrame;
	public:
		LoadParamFrame(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(id, pos, size, parent)
		{
			FWidgetProperty::Bind(addSlider("fliterThreshold"), mParams.fliterThreshold, 0.0f, 1.0f);
			FWidgetProperty::Bind(addSlider("houghThreshold"), mParams.houghThreshold, 0.0f, 1.0f);
			FWidgetProperty::Bind(addTextCtrl("removeHeadHLineCount"), mParams.removeHeadHLineCount, 0 , 5);

			addButton("BuildAndSolve", [this](int event, GWidget *)
			{
				if (stage->mReader.loadLevel(stage->mLevel, imageData, mParams) == IRR_OK)
				{
					stage->mSolver2.solve(stage->mLevel);
				}
				return false;
			});

			addButton("Close", [this](int event, GWidget *)
			{
				destroy();
				return false;
			});
		}

		LevelStage* stage;
		ImageReader::TProcImage< Color3ub > imageData;
		ImageReader::LoadParams mParams;
	};

	bool LevelStage::onInit()
	{
		if (!BaseClass::onInit())
			return false;

		::Global::GUI().cleanupWidget();

		for (int i = 0; i < ARRAY_SIZE(mColorMap); ++i)
		{
			int const ColorNum = 10;
			int color = (i) % ColorNum + 1; 
			int colorType = (i) / ColorNum;
			mColorMap[i] = RenderUtility::GetColor(color, colorType); 
		}

		auto frame = WidgetUtility::CreateDevFrame();

		auto LoadImageLevel = [this]() -> bool
		{
			InlineString< 512 > imagePath;
			if (SystemPlatform::OpenFileName(imagePath, imagePath.max_size(), {}, nullptr, nullptr, ::Global::GetDrawEngine().getWindowHandle()))
			{
				ImageReader::LoadParams params;
				params.colorMap = mColorMap;
				params.gridColor = &mGridColor;

				ImageReader::TProcImage< Color3ub > imageData;
				if (ImageReader::LoadImage(imagePath, imageData))
				{
					if (mReader.loadLevel(mLevel, imageData, params) == IRR_OK)
					{
						return true;
					}
					else
					{
						LoadParamFrame* frame = new LoadParamFrame(UI_ANY, Vec2i(100, 100), Vec2i(150, 200), nullptr);
						frame->stage = this;
						frame->mParams.colorMap = mColorMap;
						frame->imageData = std::move(imageData);
						::Global::GUI().addWidget(frame);
					}
				}
			}
			return false;
		};

		frame->addButton("Load/Solve Image Level", [this, LoadImageLevel](int event, GWidget* widget)
		{
			if (LoadImageLevel())
			{
				mSolver2.solve(mLevel);
			}
			return false;
		});
		frame->addButton("Load Image Level", [this, LoadImageLevel](int event, GWidget* widget)
		{
			LoadImageLevel();
			return false;
		});
		frame->addButton("Solve Level", [this](int event, GWidget* widget)
		{
			mSolver2.solve(mLevel);
			return false;
		});

		frame->addButton("Clear Flow", [this](int event, GWidget* widget)
		{
			mLevel.clearAllFlows();
			return false;
		});


		restart();
		return true;
	}

	void LevelStage::restart()
	{
		bStartFlowOut = false;

		mGridColor = Color3ub(255, 255, 255);

#if 0
		mLevel.setSize(Vec2i(5, 5));
		mLevel.addMapBoundBlock();

		mLevel.addSource(Vec2i(0, 0), Vec2i(1, 4), EColor::Red);
		mLevel.addSource(Vec2i(2, 0), Vec2i(1, 3), EColor::Green);
		mLevel.addSource(Vec2i(2, 1), Vec2i(2, 4), EColor::Blue);
		mLevel.addSource(Vec2i(3, 3), Vec2i(4, 0), EColor::Yellow);
		mLevel.addSource(Vec2i(3, 4), Vec2i(4, 1), EColor::Orange);
#else
		LoadLevel(mLevel, DEFAULT_LEVEL);
#endif

#if 0
		mSolver.setup(mLevel);
		mSolver.solve();
#else
		mSolver2.solve(mLevel);
#endif

		int i = 1;
	}

	Vec2i LevelStage::ToScreenPos(Vec2i const& cellPos)
	{
		return ScreenOffset + CellLength * cellPos;
	}

	Vec2i LevelStage::ToCellPos(Vec2i const& screenPos)
	{
		return (screenPos - ScreenOffset) / CellLength;
	}

	void LevelStage::onRender(float dFrame)
	{
		RHIGraphics2D& g = Global::GetRHIGraphics2D();
		RHICommandList& commandList = g.getCommandList();

		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 1), 1);

		g.beginRender();

		Vec2i size = mLevel.getSize();


		auto SetColor = [this , &g](int colorMeta)
		{
			g.setPen(mColorMap[colorMeta - 1]);
			g.setBrush(mColorMap[colorMeta - 1]);
		};

		auto DrawConnection = [&](Vec2i const& posLT, int dir, int width = FlowWidth)
		{
			int const gap = (CellLength - width) / 2;
			switch (dir)
			{
			case 0: g.drawRect(posLT + Vec2i(gap, gap), Vec2i(CellLength - gap, width)); break;
			case 1: g.drawRect(posLT + Vec2i(gap, gap), Vec2i(width, CellLength - gap)); break;
			case 2: g.drawRect(posLT + Vec2i(0, gap), Vec2i(CellLength - gap, width)); break;
			case 3: g.drawRect(posLT + Vec2i(gap, 0), Vec2i(width, CellLength - gap)); break;
			}
		};

		bool bDrawSolveDebug = true;
		if ( bDrawSolveDebug && 
			 mSolver2.mSolution.getSizeX() == mLevel.getSize().x && 
			 mSolver2.mSolution.getSizeY() == mLevel.getSize().y)
		{
			for (int i = 0; i < size.x; ++i)
			{
				for (int j = 0; j < size.y; ++j)
				{
					Vec2i cellPos = Vec2i(i, j);
					int cellIndex = mLevel.mCellMap.toIndex(i, j);
					Cell const& cell = mLevel.getCellChecked(Vec2i(i, j));

					if (cell.func == CellFunc::Block )
						continue;

					Vec2i posCellLT = ScreenOffset + CellLength * Vec2i(i, j);
#if 0
					Solver::CellState& cellState = mSolver.mState.cells[cellIndex];
					uint8 connectMask = cellState.connectMask;
#else

					auto const& solvedCell = mSolver2.getSolvedCell(cellPos);
#endif

					if (cell.func == CellFunc::Bridge)
					{
						if (solvedCell.color2)
						{
							SetColor(solvedCell.color2);
						}
						else
						{
							RenderUtility::SetPen(g, EColor::Gray);
							RenderUtility::SetBrush(g, EColor::Null);
						}

						DrawConnection(posCellLT, EDir::Top);
						DrawConnection(posCellLT, EDir::Bottom);

						RenderUtility::SetPen(g, EColor::Black);
						RenderUtility::SetBrush(g, EColor::Black);

						DrawConnection(posCellLT, EDir::Top, FlowWidth - 4);
						DrawConnection(posCellLT, EDir::Bottom, FlowWidth - 4);

						if (solvedCell.color)
						{
							SetColor(solvedCell.color);
						}
						else
						{
							RenderUtility::SetPen(g, EColor::Gray);
							RenderUtility::SetBrush(g, EColor::Null);
						}

						DrawConnection(posCellLT, EDir::Left);
						DrawConnection(posCellLT, EDir::Right);

						RenderUtility::SetPen(g, EColor::Black);
						RenderUtility::SetBrush(g, EColor::Black);
						DrawConnection(posCellLT, EDir::Left, FlowWidth - 4);
						DrawConnection(posCellLT, EDir::Right, FlowWidth - 4);

					}
					else
					{
						if (solvedCell.color)
						{
							SetColor(solvedCell.color);
						}
						else
						{
							RenderUtility::SetPen(g, EColor::Gray);
							RenderUtility::SetBrush(g, EColor::Null);
						}
						for (int dir = 0; dir < DirCount; ++dir)
						{
							if (solvedCell.mask & BIT(dir))
							{
								DrawConnection(posCellLT, dir);
							}
						}

						RenderUtility::SetPen(g, EColor::Black);
						RenderUtility::SetBrush(g, EColor::Black);

						for (int dir = 0; dir < DirCount; ++dir)
						{
							if (solvedCell.mask & BIT(dir))
							{
								DrawConnection(posCellLT, dir, FlowWidth - 4);
							}
						}
					}

				}
			}
		}
		

		for (int i = 0; i < size.x; ++i)
		{
			for (int j = 0; j < size.y; ++j)
			{
				Vec2i cellPos = Vec2i(i, j);
				int cellIndex = mLevel.mCellMap.toIndex(i, j);
				Cell const& cell = mLevel.getCellChecked(Vec2i(i, j));
				if (!cell.bCompleted)
					continue;

				for (int dir = 0; dir < DirCount; ++dir)
				{
					ColorType color = cell.colors[dir];
					if (!color)
						continue;

					g.beginBlend(0.3f);
					SetColor(color);

					Vec2i posCellLT = ScreenOffset + CellLength * Vec2i(i, j);
					g.drawRect(posCellLT, Vec2i(CellLength, CellLength));
					g.endBlend();
					break;
				}
			}
		}

#if 1
		//RenderUtility::SetPen(g, EColor::White);
		g.setPen(mGridColor);
		for( int i = 0; i <= size.x; ++i )
		{
			Vec2i start = ScreenOffset + Vec2i(i * CellLength, 0);
			g.drawLine(start, start + Vec2i(0, size.y * CellLength));
		}

		for( int j = 0; j <= size.y; ++j )
		{
			Vec2i start = ScreenOffset + Vec2i(0, j * CellLength);
			g.drawLine(start, start + Vec2i(size.x * CellLength, 0));
		}
#endif


		for( int i = 0; i < size.x; ++i )
		{
			for( int j = 0; j < size.y; ++j )
			{
				Vec2i cellPos = Vec2i(i, j);
				int cellIndex = mLevel.mCellMap.toIndex(i, j);
				Cell const& cell = mLevel.getCellChecked(Vec2i(i, j));

#if 0
				Solver::Cell const& cellData = mSolver.getCell(cellPos);
#endif

				Vec2i posCellLT = ScreenOffset + CellLength * Vec2i(i, j);
				switch( cell.func )
				{
				case CellFunc::Source:
					SetColor(cell.funcMeta);
					g.drawCircle( posCellLT + Vec2i(CellLength, CellLength) / 2, FlowSourceRadius );
					break;
				case CellFunc::Bridge:
				default:
					break;
				}

				for( int dir = 0; dir < DirCount; ++dir )
				{
					ColorType color = cell.colors[dir];
					if ( !color )
						continue;

					SetColor(color);
					DrawConnection(posCellLT, dir);
				}

				if (cell.blockMask)
				{
					for (int dir = 0; dir < DirCount; ++dir)
					{
						RenderUtility::SetPen(g, EColor::Red);

						bool bWarpPos = false;
						Vec2i linkPos = mLevel.getLinkPos(cellPos, dir, bWarpPos);
						if (cell.func == CellFunc::Block && ( bWarpPos || mLevel.getCellChecked(linkPos).func == CellFunc::Block) )
							continue;

						if ( cell.blockMask & BIT(dir) )
						{
							switch (dir)
							{
							case 0: g.drawLine(posCellLT + Vec2i(CellLength, 0), posCellLT + Vec2i(CellLength, CellLength)); break;
							case 1: g.drawLine(posCellLT + Vec2i(0, CellLength), posCellLT + Vec2i(CellLength, CellLength)); break;
							case 2: g.drawLine(posCellLT, posCellLT + Vec2i(0, CellLength)); break;
							case 3: g.drawLine(posCellLT, posCellLT + Vec2i(CellLength, 0)); break;
							}
						}
					}
				}

#if 0
				if (cell.func == CellFunc::Source)
				{
					g.drawText(posCellLT, FStringConv::From(cell.funcMeta));
				}
#endif
			}
		}
		if (IndexReaderTextureShow != ImageReader::DebugTextureCount)
		{
			RHITexture2DRef& tex = mReader.mDebugTextures[IndexReaderTextureShow];
			if (tex.isValid())
			{
				float scale = float(tex->getSizeY()) / tex->getSizeX();
				if (IndexReaderTextureShow == ImageReader::eLineHough)
				{
					scale = 1;
				}
				Vector2 size = Vector2(300, 300 * scale);
				Vector2 pos = ::Global::GetScreenSize() - size - Vector2(10, 10);
				g.setBrush(Color3f(1, 1, 1));
				g.drawTexture(*tex, pos, size);

				if (IndexReaderTextureShow == ImageReader::eOrigin ||
					IndexReaderTextureShow == ImageReader::eFliter)
				{
					for (auto line : mReader.debugLines)
					{
						Vector2 start = pos + size.mul(line.start);
						Vector2 end = pos + size.mul(line.end);
						g.setPen(line.color);
						g.drawLine(start, end);
					}
				}
			}
		}

		g.endRender();
	}

	MsgReply LevelStage::onMouse(MouseMsg const& msg)
	{
		if (msg.onLeftDown())
		{
			Vec2i cPos = ToCellPos(msg.getPos());

			if (mLevel.isValidCellPos(cPos))
			{
				bStartFlowOut = true;
				flowOutColor = mLevel.getCellChecked(cPos).getFuncFlowColor();
				flowOutCellPos = cPos;
				if (mLevel.breakFlow(cPos, 0, 0) != Level::BreakResult::NoBreak)
				{
					mLevel.updateGobalStatus();
				}
			}
		}
		else if (msg.onLeftUp())
		{
			bStartFlowOut = false;
		}


		if (bStartFlowOut && msg.onMoving())
		{
			Vec2i cPos = ToCellPos(msg.getPos());

			if (mLevel.isValidCellPos(cPos))
			{
				Vec2i cOffset = cPos - flowOutCellPos;
				Vec2i cOffsetAbs = Abs(cOffset);
				int d = cOffsetAbs.x + cOffsetAbs.y;
				if (d > 0)
				{
					if (d == 1)
					{
						int dir;
						if (cOffset.x == 1)
						{
							dir = 0;
						}
						else if (cOffset.y == 1)
						{
							dir = 1;
						}
						else if (cOffset.x == -1)
						{
							dir = 2;
						}
						else
						{
							dir = 3;
						}

						if (mLevel.breakFlow(cPos, dir, flowOutColor) == Level::BreakResult::HaveBreakSameColor)
						{
							mLevel.updateGobalStatus();
							flowOutCellPos = cPos;
						}
						else
						{
							ColorType linkColor = mLevel.linkFlow(flowOutCellPos, dir);
							if (linkColor)
							{
								flowOutCellPos = cPos;
								flowOutColor = linkColor;
							}
							mLevel.updateGobalStatus();
						}
					}
					else
					{



					}
				}
			}
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply LevelStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::X: mSolver.solveNext(); break;
			case EKeyCode::Z: for (int i = 0; i < 97; ++i) mSolver.solveNext(); break;
			case EKeyCode::P: ++IndexReaderTextureShow; if (IndexReaderTextureShow > ImageReader::DebugTextureCount) IndexReaderTextureShow = 0; break;
			case EKeyCode::O: --IndexReaderTextureShow; if (IndexReaderTextureShow < 0) IndexReaderTextureShow = ImageReader::DebugTextureCount; break;
			case EKeyCode::C:
				{
					if (InputManager::Get().isKeyDown(EKeyCode::Control))
					{
#ifdef SYS_PLATFORM_WIN
						if (::OpenClipboard(NULL))
						{
							EmptyClipboard();
							std::string mapData = ExportLevel(mLevel, true);
							if (!mapData.empty())
							{
								HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE,
									(mapData.length() + 1) * sizeof(char));

								if (hglbCopy)
								{
									char* lptstrCopy = (char*)GlobalLock(hglbCopy);
									if (lptstrCopy)
									{
										memcpy(lptstrCopy, mapData.data(), mapData.length());
										lptstrCopy[mapData.length()] = 0;
										GlobalUnlock(hglbCopy);

										SetClipboardData(CF_TEXT, hglbCopy);
									}
								}
							}
							::CloseClipboard();
						}
#endif //SYS_PLATFORM_WIN
					}
				}
				break;
			}
		}
		return BaseClass::onKey(msg);
	}

}//namespace Flow
