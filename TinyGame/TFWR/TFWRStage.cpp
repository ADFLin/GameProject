
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "TFWRGameState.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"

#include "Core/StringConv.h"
#include "LogSystem.h"


#include "Meta/MetaBase.h"
#include "RandomUtility.h"
#include "AssetViewer.h"
#include "FileSystem.h"
#include "StringParse.h"

#include <memory>


namespace TFWR
{
	typedef TVector2<int> Vec2i;

	using namespace Render;


	struct TestData
	{

		int a;
		float b;


		REFLECT_STRUCT_BEGIN(TestData)
			REF_PROPERTY(a)
			REF_PROPERTY(b)
		REFLECT_STRUCT_END()
	};

	class CodeEditor;

	class CodeFileAsset : public ScriptFile, public IAssetViewer
	{
	public:
		CodeEditor* editor;
		int id;
	};

	class ExecButton : public GButtonBase
	{
		typedef GButtonBase BaseClass;
	public:
		ExecButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:BaseClass(id, pos, size, parent)
		{
			mID = id;
		}

		void onRender()
		{
			Vec2i pos = getWorldPos();
			Vec2i size = getSize();
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			g.enablePen(false);
			g.setBrush(Color3ub(96, 115, 13));
			g.drawRoundRect(pos, size, Vector2(4, 4));

			g.pushXForm();
			g.translateXForm(pos.x, pos.y);
			Vector2 vertices[] =
			{
				Vector2(size.x / 4, size.y / 4),
				Vector2(3 * size.x / 4, size.y / 2),
				Vector2(size.x / 4, 3 * size.y / 4),
			};

			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::White);
			g.enablePen(true);
			g.enableBrush(!bStep);
			g.drawPolygon(vertices, ARRAY_SIZE(vertices));
			g.enableBrush(true);
			g.popXForm();
		}
		bool bStep = false;
	};


	class IViewModel
	{
	public:
		virtual bool isExecutingCode() { return false; }
		virtual void runExecution(CodeFileAsset& codeFile){}
		virtual void stopExecution(CodeFileAsset& codeFile) {}
		virtual void pauseExecution(CodeFileAsset& codeFile) {}
		virtual void runStepExecution(CodeFileAsset& codeFile) {}
		static IViewModel& Get()
		{
			return *Instance;
		}
		static IViewModel*Instance;
	};

	IViewModel* IViewModel::Instance = nullptr;

	class CodeEditorSettings
	{
	public:
		CodeEditorSettings()
		{
			Color3ub DefaultKeywordColor{ 255, 188, 66 };
			Color3ub FlowKeywordColor{ 255, 188, 66 };
			WordColorMap["if"] = FlowKeywordColor;
			WordColorMap["elseif"] = FlowKeywordColor;
			WordColorMap["else"] = FlowKeywordColor;
			WordColorMap["end"] = FlowKeywordColor;
			WordColorMap["goto"] = FlowKeywordColor;
			WordColorMap["repeat"] = FlowKeywordColor;
			WordColorMap["for"] = FlowKeywordColor;
			WordColorMap["while"] = FlowKeywordColor;
			WordColorMap["do"] = FlowKeywordColor;
			WordColorMap["until"] = FlowKeywordColor;
			WordColorMap["return"] = FlowKeywordColor;
			WordColorMap["then"] = FlowKeywordColor;
			WordColorMap["break"] = FlowKeywordColor;
			WordColorMap["in"] = FlowKeywordColor;

			WordColorMap["function"] = DefaultKeywordColor;
			WordColorMap["local"] = DefaultKeywordColor;

			WordColorMap["true"] = DefaultKeywordColor;
			WordColorMap["false"] = DefaultKeywordColor;
			WordColorMap["nil"] = DefaultKeywordColor;

			WordColorMap["and"] = DefaultKeywordColor;
			WordColorMap["or"] = DefaultKeywordColor;
			WordColorMap["not"] = DefaultKeywordColor;
		}

		Color4ub getColor(StringView word)
		{
			auto iter = WordColorMap.find(word);
			if (iter != WordColorMap.end())
				return iter->second;

			return Color4ub(255, 255, 255, 255);
		}

		std::unordered_map<HashString, Color4ub > WordColorMap;
	};

	CodeEditorSettings GEditorSettings;
	class CodeEditor : public GFrame
	{
	public:
		using BaseClass = GFrame;
		enum 
		{
			UI_EXEC = BaseClass::NEXT_UI_ID,
			UI_EXEC_STEP,

			NEXT_UI_ID,
		};
		CodeEditor(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:GFrame(id, pos, size, parent)
		{
			ExecButton* button;
			button = new ExecButton(UI_EXEC, Vec2i(5, 5), Vec2i(25, 25), this);
			button = new ExecButton(UI_EXEC_STEP, Vec2i(5 + 30, 5), Vec2i(25, 25), this);
			button->bStep = true;
		}

		CodeFileAsset* codeFile;

		virtual bool onChildEvent(int event, int id, GWidget* ui) 
		{
			switch (id)
			{
			case UI_EXEC:
				if (IViewModel::Get().isExecutingCode())
				{
					IViewModel::Get().stopExecution(*codeFile);
				}
				else
				{
					IViewModel::Get().runExecution(*codeFile);
				}

				break;
			case UI_EXEC_STEP:
				if (IViewModel::Get().isExecutingCode())
				{
					IViewModel::Get().pauseExecution(*codeFile);
				}
				else
				{
					IViewModel::Get().runStepExecution(*codeFile);
				}
				break;
			}
			return true; 
		}


		void parseCode()
		{
			char const* pCode = codeFile->code.c_str();

			while (*pCode != 0)
			{
				auto line = FStringParse::StringTokenLine(pCode);

				mCodeLines.push_back(CodeLine());

				auto& codeLine = mCodeLines.back();
				if (line.size() == 0)
					continue;
				
				mFont->generateLineVertices(Vector2::Zero(), line , 1.0, codeLine.vertices);
				codeLine.colors.resize(codeLine.vertices.size() / 4);

				char const* pStr = line.data();
				char const* pStrEnd = line.data() + line.size();
				auto* pColor = codeLine.colors.data();
				int numWord = 0;
				while (pStr < pStrEnd)
				{
					char const* dropDelims = " \t\r\n";
					pStr = FStringParse::SkipChar(pStr, pStrEnd, dropDelims);
					if (pStr >= pStrEnd)
						break;

					char const* ptr = pStr;
					pStr = FStringParse::FindChar(pStr, pStrEnd, dropDelims);
					StringView token = StringView(ptr, pStr - ptr);
					
					Color4ub color = GEditorSettings.getColor(token);
					pColor = std::fill_n(pColor, token.size(), color);
					numWord += token.size();
				}

				CHECK(numWord == codeLine.colors.size());
			}
		}
		void onRender()
		{
			Vec2i pos = getWorldPos();
			Vec2i size = getSize();
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			g.enablePen(false);
			g.setBrush(Color3ub(86, 86, 86));
			g.drawRoundRect(pos, size, Vector2(4, 4));


			g.setBrush(Color3ub(46, 46, 46));
			int border = 3;
			Vector2 textRectPos = pos + Vector2(border, border + 30);
			Vector2 textRectSize = size - Vector2(2 * border, 2 * border + 30);
			g.drawRoundRect(textRectPos, textRectSize, Vector2(8,8));


			g.beginClip(textRectPos, textRectSize);
			g.pushXForm();

			++showFrame;
			if (showFrame > 4)
			{
				if (!mLineShowQueue.empty())
				{
					mShowExecuteLine = mLineShowQueue.front();
					mLineShowQueue.removeIndex(0);
					showFrame = 0;
				}
			}
			g.setBrush(Color3ub(255, 255, 255));
			g.drawTextF(pos, "%d", mShowExecuteLine);
			g.translateXForm(textRectPos.x + 5, textRectPos.y + 5);

			g.setTexture(mFont->getTexture());
			g.setBlendState(ESimpleBlendMode::Translucent);

			int curline = 1;
			for (auto const& line : mCodeLines)
			{
				if (curline == mShowExecuteLine)
				{
					g.setBrush(Color3ub(32, 32, 32));
					g.setBlendState(ESimpleBlendMode::Add);
					g.drawRect(Vector2(-5, 0), Vector2(textRectSize.x, 16));
		

					g.setBrush(Color3ub(255, 255, 255));
					g.setTexture(mFont->getTexture());
					g.setBlendState(ESimpleBlendMode::Translucent);
				}
	
				g.drawTextQuad(line.vertices, line.colors);
				g.translateXForm(0, 16);
				curline += 1;
			}
			g.popXForm();
			g.endClip();

			g.enablePen(true);
		}

		void setExecuteLine(int line)
		{
			if (mLineShowQueue.empty() || mLineShowQueue.back() != line)
				mLineShowQueue.push_back(line);
		}

		TArray<int> mLineShowQueue;
		int showFrame = 0;

		struct CodeLine
		{
			int offset;
			TArray<GlyphVertex> vertices;
			TArray<RHIGraphics2D::Color4Type> colors;
		};
		TArray< CodeLine > mCodeLines;
		FontDrawer* mFont;
		int mShowExecuteLine = -1;

		struct Cursor
		{
			int line;
			int offset;
		};

		TArray<int> mBreakPoints;
	};


	class TestStage : public StageBase
		            , public IGameRenderSetup
		            , public IExecuteHookListener
					, public IViewModel
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		enum
		{

			UI_DUMP = BaseClass::NEXT_UI_ID,

			UI_CODE_EDITOR,
		};
		

		GameState mGame;
		std::unordered_map<HashString, CodeFileAsset*> mCodeAssetMap;
		virtual bool isExecutingCode() 
		{
			return mGame.isExecutingCode();
		}

		virtual void runExecution(CodeFileAsset& codeFile) 
		{
			if (isExecutingCode())
				return;

			mGame.runExecution(*mGame.mDrones[0], codeFile);

		}
		
		virtual void stopExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.mDrones)
			{
				if (drone->executionCode == &codeFile)
				{
					mGame.stopExecution(*drone);
				}
			}
		}

		virtual void pauseExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.mDrones)
			{
				if (drone->executionCode == &codeFile)
				{
					drone->stateFlags |= ExecutableObject::ePause;
				}
			}
		}

		virtual void runStepExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.mDrones)
			{
				if (drone->executionCode == &codeFile)
				{
					drone->stateFlags &= ~ExecutableObject::ePause;
					drone->stateFlags |= ExecutableObject::eStep;
				}
			}

		}
		virtual void onHookLine(StringView const& fileName, int line)
		{
			auto iter = mCodeAssetMap.find(fileName);
			if (iter != mCodeAssetMap.end())
			{
				CodeFileAsset* asset = iter->second;
				asset->editor->setExecuteLine(line);
			}
		}

		virtual void onHookCall() 
		{

		}

		virtual void onHookReturn() 
		{

		}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			IExecuteManager::Get().mHookListener = this;
			IViewModel::Instance = this;

			::Global::GUI().cleanupWidget();

			auto codeFile = createCodeFile("Test");

			mGame.init();
			mGame.createDrone(*codeFile);


			Vector2 lookPos = 0.5 *  Vector2(mGame.mTiles.getSize());
			mWorldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), lookPos, Vector2(0, -1), ::Global::GetScreenSize().x / 10.0f);
			mScreenToWorld = mWorldToScreen.inverse();

			restart();
			return true;
		}

		void onEnd() override
		{

		}

		void restart() {}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			mGame.update(deltaTime);
		}

		TArray< CodeFileAsset* > mCodeFiles;
		int mNextCodeId = 0;

		ScriptFile* createCodeFile(char const* name)
		{
			CodeFileAsset* codeFile = new CodeFileAsset;
			codeFile->name = name;
			codeFile->id = mNextCodeId;
			codeFile->loadCode();

			CodeEditor* editor = new CodeEditor(UI_CODE_EDITOR + codeFile->id, Vec2i(100, 100), Vec2i(200, 400), nullptr);
			::Global::GUI().addWidget(editor);
			codeFile->editor = editor;
			editor->codeFile = codeFile;
			editor->mFont = &RenderUtility::GetFontDrawer(FONT_S10);
			mCodeFiles.push_back(codeFile);

			editor->parseCode();
			mCodeAssetMap.emplace(codeFile->getPath().c_str(), codeFile);

			return codeFile;
		}


		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			auto screenSize = ::Global::GetScreenSize();

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 0.0), 1);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();

			g.pushXForm();
			g.transformXForm(mWorldToScreen, true);

			g.setTextRemoveRotation(true);
			g.setTextRemoveScale(true);

			RenderUtility::SetPen(g, EColor::Black);
			for (int j = 0; j < mGame.mTiles.getSizeY(); ++j)
			{
				for (int i = 0; i < mGame.mTiles.getSizeX(); ++i)
				{
					Vec2i tilePos = Vec2i(i, j);
					auto const& tile = mGame.getTile(tilePos);
					RenderUtility::SetBrush(g, tile.ground == EGround::Grassland ? EColor::Green : EColor::Brown, COLOR_LIGHT);
					g.drawRect(Vector2(i,j), Vector2(1,1));
				}
			}


			for (int j = 0; j < mGame.mTiles.getSizeY(); ++j)
			{
				for (int i = 0; i < mGame.mTiles.getSizeX(); ++i)
				{
					Vec2i tilePos = Vec2i(i, j);
					auto const& tile = mGame.getTile(tilePos);
					if (tile.plant)
					{
						switch (tile.plant->getPlantType())
						{
						case EPlant::Bush:
							break;
						case EPlant::Grass:
							break;
						case EPlant::Pumpkin:
							if (tile.meta >= 0)
							{
								Vector2 border(0.2, 0.2);
								if (tile.meta == 0)
								{
									RenderUtility::SetBrush(g, EColor::Gray);
									g.drawRect(Vector2(tilePos) + border, Vector2(1, 1) - 2 * border);
								}
								else
								{
									auto const& area = mGame.mAreas[tile.meta - 1];
									if (area.min == tilePos)
									{
										Vector2 border(0.2, 0.2);
										RenderUtility::SetBrush(g, EColor::Gray);
										g.drawRect(Vector2(tilePos) + border, Vector2(area.getSize()) - 2 * border);
									}
								}
							}
							break;
						}

						g.drawTextF(Vector2(i, j), Vector2(1, 1), tile.plant->getDebugInfo(tile).c_str());
					}
				}
			}

			RenderUtility::SetBrush(g, EColor::Yellow);
			for (auto drone : mGame.mDrones)
			{
				g.drawCircle( Vector2(drone->pos) + Vector2(0.5,0.5), 0.3 );
			}

			g.popXForm();


			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
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
	protected:
	};


	REGISTER_STAGE_ENTRY("TFWR Game", TestStage, EExecGroup::Dev, "Game|Script");

}