
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "TFWRGameState.h"
#include "TFWREntities.h"

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

#include "DebugDraw.h"


#include <memory>
#include "Async/Coroutines.h"
#include "CodeEditor.h"
#include "GameWidget.h"
#include "Asset.h"
#include <optional>

namespace TFWR
{
	typedef TVector2<int> Vec2i;

	using namespace Render;

	struct TestData
	{
		TestData()
		{
			a = 1;
			b = 2;
		}
		int a;
		float b;

		virtual ~TestData() = default;
		virtual void foo()
		{
			LogMsg("TestData : %d, %f", a, b);
		}

		REFLECT_STRUCT_BEGIN(TestData)
			REF_PROPERTY(a)
			REF_PROPERTY(b)
			REF_FUNCION(foo)
		REFLECT_STRUCT_END()
	};


	struct TestData2 : public TestData
	{
		void foo()
		{
			LogMsg("TestData2 : %d, %f", a, b);
		}

		REFLECT_STRUCT_BEGIN(TestData2)
			REF_BASE_CLASS(TestData)
		REFLECT_STRUCT_END()
	};


	class CodeFileAsset : public ScriptFile, public IAssetViewer
	{
	public:
		class CodeEditor* editor;
		int id;

		void getDependentFilePaths(TArray< std::wstring >& paths) override
		{
			std::wstring fullPath = FFileSystem::ConvertToFullPath(FCString::CharToWChar(getPath().c_str()).c_str());
			paths.push_back(fullPath);
		}

		void postFileModify(EFileAction action) override;
	};


	IViewModel* IViewModel::Instance = nullptr;

	class CodeEditor : public CodeEditorBase
	{
	public:
		using CodeEditorBase::CodeEditorBase;

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
			return CodeEditorBase::onChildEvent(event, id, ui);
		}

		void setAsset(CodeFileAsset& inCodeFile)
		{
			codeFile = &inCodeFile;
			loadFromCode(codeFile->code);
		}
	};

	void CodeFileAsset::postFileModify(EFileAction action)
	{
		if (action == EFileAction::Modify)
		{
			if (loadCode())
			{
				if (editor)
				{
					editor->loadFromCode(code);
				}
			}
		}
	}


#define P EPlant::Pumpkin
#define D EPlant::DeadPumpkin
	uint8 LevelData[] =
	{
		P, P, P, 0,
		P, P, P, D,
		P, P, P, 0,
		0, 0, 0, 0,
	};
#undef P
#undef D


	class MyButton : public GButtonBase
	{
	public:
		enum Type
		{
			Plus,
			Open,
			Info,
			Expand,
		};

		MyButton(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent, Type type)
			: GButtonBase(id, pos, size, parent)
			, mType(type)
		{
		}

		void onRender() override
		{
			IGraphics2D& g = Global::GetIGraphics2D();

			Color3ub color = mColor.getValue();
			g.setBrush(color);
			g.setPen(Color3ub(0, 0, 0));
			g.drawRoundRect(getPos(), getSize(), Vec2i(5, 5));

			RenderUtility::SetBrush(g, EColor::White);
			RenderUtility::SetPen(g, EColor::White);

			Vec2i center = getPos() + getSize() / 2;

			switch (mType)
			{
			case Plus:
				{
					int len = 14;
					int thickness = 4;
					g.drawRect(center - Vec2i(len / 2, thickness / 2), Vec2i(len, thickness));
					g.drawRect(center - Vec2i(thickness / 2, len / 2), Vec2i(thickness, len));
				}
				break;
			case Open:
				{
					// Folder icon
					int w = 16;
					int h = 11;
					Vec2i folderPos = center - Vec2i(w / 2, h / 2);
					// Tab
					g.drawRect(folderPos + Vec2i(0, -2), Vec2i(6, 2));
					// Body
					g.drawRect(folderPos, Vec2i(w, h));
					// Line to separate flap
					g.setPen(color);
					g.drawLine(folderPos + Vec2i(0, 2), folderPos + Vec2i(w, 2));
				}
				break;
			case Info:
				{
					// 'i'
					int w = 4;
					// Dot
					g.drawRect(center + Vec2i(-w / 2, -7), Vec2i(w, 4));
					// Body
					g.drawRect(center + Vec2i(-w / 2, -1), Vec2i(w, 9));
				}
				break;
			case Expand:
				{
					// '^'
					int thickness = 3;
					Vec2i top = center - Vec2i(0, 4);
					Vec2i left = center + Vec2i(-6, 2);
					Vec2i right = center + Vec2i(6, 2);

					for (int i = 0; i < thickness; ++i)
					{
						g.drawLine(left + Vec2i(0, i), top + Vec2i(0, i));
						g.drawLine(top + Vec2i(0, i), right + Vec2i(0, i));
					}
				}
				break;
			}
		}
		Type mType;
	};

	class FileSelectDialog : public GFrame
	{
	public:
		enum
		{
			UI_FILE_LIST = GWidget::NEXT_UI_ID,
			UI_BTN_LOAD,
			UI_BTN_CANCEL,
		};

		FileSelectDialog(int id, Vec2i const& pos, Vec2i const& size, GWidget* parent)
			: GFrame(id, pos, size, parent)
		{
			setRenderType(GPanel::eRectType);

			mList = new GFileListCtrl(UI_FILE_LIST, Vec2i(10, 10), Vec2i(280, 340), this);
			mList->setDir(TFWR_SCRIPT_DIR);
			mList->refreshFiles();

			GButton* btnLoad = new GButton(UI_BTN_LOAD, Vec2i(10, 360), Vec2i(80, 30), this);
			btnLoad->setTitle("Load");

			GButton* btnCancel = new GButton(UI_BTN_CANCEL, Vec2i(100, 360), Vec2i(80, 30), this);
			btnCancel->setTitle("Cancel");
		}

		bool onChildEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			case UI_BTN_LOAD:
				if (event == EVT_BUTTON_CLICK)
				{
					String filePath = mList->getSelectedFilePath();
					if (!filePath.empty())
					{
						if (onFileSelect)
						{
							onFileSelect(filePath);
						}
						destroy();
					}
					return true;
				}
				break;
			case UI_BTN_CANCEL:
				if (event == EVT_BUTTON_CLICK)
				{
					destroy();
					return true;
				}
				break;
			}
			return GFrame::onChildEvent(event, id, ui);
		}

		std::function<void(String const&)> onFileSelect;
		GFileListCtrl* mList;
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
			UI_BTN_PLUS,
			UI_BTN_OPEN,
			UI_BTN_INFO,
			UI_BTN_EXPAND,

			UI_FILE_DIALOG,
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

			mGame.runExecution(*mGame.getDrones()[0], codeFile);

		}
		
		virtual void stopExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.getDrones())
			{
				if (drone->executionCode == &codeFile)
				{
					mGame.stopExecution(*drone);
				}
			}
		}

		virtual void pauseExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.getDrones())
			{
				if (drone->executionCode == &codeFile)
				{
					drone->stateFlags |= ExecutableObject::ePause;
				}
			}
		}

		virtual void runStepExecution(CodeFileAsset& codeFile) 
		{
			for (Drone* drone : mGame.getDrones())
			{
				if (drone->executionCode == &codeFile)
				{
					drone->stateFlags &= ~ExecutableObject::ePause;
					drone->stateFlags |= ExecutableObject::eStep;
				}
			}

		}

		virtual std::optional<std::string> getVariableValue(CodeFileAsset& codeFile, StringView varName)
		{
			return mGame.getVariableValue(&codeFile, varName);
		}

		virtual void onHookLine(StringView const& fileName, int line)
		{
			auto iter = mCodeAssetMap.find(fileName);
			if (iter != mCodeAssetMap.end())
			{
				CodeFileAsset* asset = iter->second;
				asset->editor->setExecuteLine(line);

				if (asset->editor->mCanvas->hasBreakPoint(line - 1))
				{
					pauseExecution(*asset);
				}
			}
		}

		virtual void onHookCall() 
		{

		}

		virtual void onHookReturn() 
		{

		}

		void loadLevel()
		{
			int mapSize = 4;
			mGame.resizeMap(mapSize);
			for (int i = 0; i < mapSize * mapSize; ++i)
			{
				if (LevelData[i])
				{
					auto entity = EPlant::GetEntity(EPlant::Type(LevelData[i]));
					mGame.getGameLogic().plantAndRipen(Vec2i(i % mapSize, i / mapSize), *entity);
				}
			}
		}

		template< typename T, typename TFunc >
		struct TMemberFuncCallable
		{
			TFunc funcPtr;
			T* obj;

			template< typename ...Ts >
			auto operator()(Ts&& ...ts)
			{
				CHECK(obj);
				return (obj->*funcPtr)(std::forward<Ts>(ts)...);
			}
		};
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;


			mLookPos = 0.5 *  Vector2(mGame.getGameLogic().mTiles.getSize());
			mViewScale = ::Global::GetScreenSize().x / 10.0f;
			mZoomScale = 1.0;

			updateTransform();


			IExecuteManager::Get().mHookListener = this;
			IViewModel::Instance = this;

			::Global::GUI().cleanupWidget();



			auto codeFile = createCodeFile("Test");

			mGame.init(8);
			LuaBindingCollector::Register<TestData>(mGame.getMainLuaState());
			LuaBindingCollector::Register<TestData2>(mGame.getMainLuaState());
			mGame.createDrone(*codeFile);

			mGame.getGameLogic().generateMaze(Vec2i(0, 0), Vec2i(8, 8));
			//loadLevel();


			auto frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Debug", bDebugPause);

			// Create UI Buttons
			{
				Vector2 screenSize = ::Global::GetScreenSize();
				Vector2 btnSize(30, 30);
				float spacing = 10;
				float startX = screenSize.x - (btnSize.x + spacing) * 4 - 20;
				float startY = 20;

				mBtnPlus = new MyButton(UI_BTN_PLUS, Vec2i(startX, startY), Vec2i(btnSize), nullptr, MyButton::Plus);
				mBtnPlus->setColor(Color3ub(85, 107, 47));
				::Global::GUI().addWidget(mBtnPlus);

				mBtnOpen = new MyButton(UI_BTN_OPEN, Vec2i(startX + btnSize.x + spacing, startY), Vec2i(btnSize), nullptr, MyButton::Open);
				mBtnOpen->setColor(Color3ub(85, 107, 47));
				::Global::GUI().addWidget(mBtnOpen);

				mBtnInfo = new MyButton(UI_BTN_INFO, Vec2i(startX + (btnSize.x + spacing) * 2, startY), Vec2i(btnSize), nullptr, MyButton::Info);
				mBtnInfo->setColor(Color3ub(85, 107, 47));
				::Global::GUI().addWidget(mBtnInfo);

				mBtnExpand = new MyButton(UI_BTN_EXPAND, Vec2i(startX + (btnSize.x + spacing) * 3, startY), Vec2i(btnSize), nullptr, MyButton::Expand);
				mBtnExpand->setColor(Color3ub(85, 107, 47));
				::Global::GUI().addWidget(mBtnExpand);
			}

			restart();
			return true;
		}

		void onEnd() override
		{

		}

		void restart() {}



		bool bDebugPause = false;
		Coroutines::ExecutionHandle hExec;

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
			if (bDebugPause)
			{
				if (!hExec.isValid())
				{
					hExec = Coroutines::Start([this, deltaTime]()
					{
						mGame.update(deltaTime);
						hExec.mPtr == 0;
					});
				}
			}
			else
			{
				mGame.update(deltaTime);
			}
		}

		TArray< CodeFileAsset* > mCodeFiles;
		int mNextCodeId = 0;

		ScriptFile* createCodeFile(char const* name)
		{
			CodeFileAsset* codeFile = new CodeFileAsset;
			codeFile->name = name;
			codeFile->id = mNextCodeId++;
			codeFile->loadCode();

			::Global::GetAssetManager().registerViewer(codeFile);

			CodeEditor* editor = new CodeEditor(UI_CODE_EDITOR + codeFile->id, Vec2i(100, 100), Vec2i(200, 400), nullptr);
			::Global::GUI().addWidget(editor, &mEditorLayer);
			codeFile->editor = editor;
			editor->codeFile = codeFile;
			mCodeFiles.push_back(codeFile);

			editor->setAsset(*codeFile);
			mCodeAssetMap.emplace(codeFile->getPath().c_str(), codeFile);

			return codeFile;
		}


		WidgetLayoutLayer mEditorLayer;

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		Vector2 mLookPos;
		float   mViewScale;
		float   mZoomScale;
		bool    bIsDragging = false;
		Vec2i   mLastMousePos;

		void updateTransform()
		{
			mWorldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), mLookPos, Vector2(0, -1), mViewScale * mZoomScale);
			mScreenToWorld = mWorldToScreen.inverse();

			mEditorLayer.worldToScreen = RenderTransform2D::LookAt(::Global::GetScreenSize(), mLookPos * mViewScale, Vector2(0, -1), mZoomScale);
			mEditorLayer.screenToWorld = mEditorLayer.worldToScreen.inverse();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onRightDown())
			{
				bIsDragging = true;
				mLastMousePos = msg.getPos();
				return MsgReply::Handled();
			}
			else if (msg.onRightUp())
			{
				bIsDragging = false;
				return MsgReply::Handled();
			}
			else if (msg.onMoving())
			{
				if (bIsDragging)
				{
					Vec2i delta = msg.getPos() - mLastMousePos;
					mLastMousePos = msg.getPos();

					Vector2 worldDelta = mScreenToWorld.transformVector(Vector2(delta));
					mLookPos -= worldDelta;

					updateTransform();
				}
			}
			else if (msg.onWheelFront())
			{
				float scale = 1.1f;
				mZoomScale *= scale;
				updateTransform();
				return MsgReply::Handled();
			}
			else if (msg.onWheelBack())
			{
				float scale = 1.0f / 1.1f;
				mZoomScale *= scale;
				updateTransform();
				return MsgReply::Handled();
			}
			return BaseClass::onMouse(msg);
		}

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

			auto& gameLogic = mGame.getGameLogic();

			for (int j = 0; j < gameLogic.mTiles.getSizeY(); ++j)
			{
				for (int i = 0; i < gameLogic.mTiles.getSizeX(); ++i)
				{
					Vec2i tilePos = Vec2i(i, j);
					auto const& tile = gameLogic.getTile(tilePos);
					RenderUtility::SetBrush(g, tile.ground == EGround::Grassland ? EColor::Green : EColor::Brown, COLOR_LIGHT);
					g.drawRect(Vector2(i,j), Vector2(1,1));
				}
			}

			for (int j = 0; j < gameLogic.mTiles.getSizeY(); ++j)
			{
				for (int i = 0; i < gameLogic.mTiles.getSizeX(); ++i)
				{
					Vec2i tilePos = Vec2i(i, j);
					auto const& tile = gameLogic.getTile(tilePos);
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
									auto const& area = gameLogic.mAreas[tile.meta - 1];
									if (area.min == tilePos)
									{
										Vector2 border(0.2, 0.2);
										RenderUtility::SetBrush(g, EColor::Gray);
										g.drawRect(Vector2(tilePos) + border, Vector2(area.getSize()) - 2 * border);
									}
								}
							}
							break;
						case EPlant::Hedge:
							{
								g.setBrush(Color3ub(34, 139, 34)); // Dark green foliage
								g.drawRect(Vector2(tilePos), Vector2(1, 1));

								Vector2 cPos = Vector2(tilePos) + Vector2(0.5, 0.5);
								g.setBrush(Color3ub(160, 82, 45)); // sienna - path color


								g.enablePen(false);
								int numLink = 0;
								for (int dir = 0; dir < 4; ++dir)
								{
									if (tile.linkMask & BIT(dir))
									{
										numLink++;
										Vec2i vDir = GDirectionOffset[dir];
										Vector2 rectPos;
										Vector2 rectSize;
										if (vDir.x != 0) // right or left
										{
											rectSize = Vector2(0.5, 0.4);
											rectPos.y = cPos.y - 0.2;
											if (vDir.x > 0)
												rectPos.x = cPos.x;
											else
												rectPos.x = cPos.x - 0.5;
										}
										else
										{
											rectSize = Vector2(0.4, 0.5);
											rectPos.x = cPos.x - 0.2;
											if (vDir.y > 0)
												rectPos.y = cPos.y;
											else
												rectPos.y = cPos.y - 0.5;
										}
										g.drawRect(rectPos, rectSize);
									}
								}

								if (numLink > 0)
								{
									g.drawRect(cPos - Vector2(0.2, 0.2), Vector2(0.4, 0.4));
								}
								g.enablePen(true);

								// Draw treasure marker if this is a maze
								if (tile.mazeID != INDEX_NONE && tile.mazeID < gameLogic.mMazes.size())
								{
									auto const& maze = gameLogic.mMazes[tile.mazeID];
									if (maze.treasurePos == tilePos)
									{
										RenderUtility::SetBrush(g, EColor::Yellow);
										g.drawCircle(cPos, 0.15f);
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
			for (auto drone : mGame.getDrones())
			{
				g.drawCircle( Vector2(drone->pos) + Vector2(0.5,0.5), 0.3 );
			}

			DrawDebugCommit(::Global::GetIGraphics2D());
			g.popXForm();


			g.endRender();
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::X:
					{
						DrawDebugClear();
						Coroutines::Resume(hExec);
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		void showFileSelectDialog()
		{
			Vec2i size(300, 400);
			Vec2i pos = (::Global::GetScreenSize() - Vector2(size)) / 2;
			FileSelectDialog* frame = new FileSelectDialog(UI_FILE_DIALOG, pos, size, nullptr);
			
			frame->onFileSelect = [this](String const& filePath)
			{
				String name = FFileUtility::GetBaseFileName(filePath.c_str()).toStdString();
				createCodeFile(name.c_str());
			};

			::Global::GUI().addWidget(frame);
			frame->doModal();
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			case UI_BTN_PLUS:
				if (event == EVT_BUTTON_CLICK)
				{
					String fileName = "Script" + std::to_string(mNextCodeId);
					createCodeFile(fileName.c_str());
					return true;
				}
				break;
			case UI_BTN_OPEN:
				if (event == EVT_BUTTON_CLICK)
				{
					showFileSelectDialog();
					return true;
				}
				break;

			case UI_BTN_INFO:
				if (event == EVT_BUTTON_CLICK)
				{
					LogMsg("Info Button Clicked");
					return true;
				}
				break;
			case UI_BTN_EXPAND:
				if (event == EVT_BUTTON_CLICK)
				{
					LogMsg("Expand Button Clicked");
					return true;
				}
				break;
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1080;
			systemConfigs.screenHeight = 768;
		}

	protected:
		MyButton* mBtnPlus = nullptr;
		MyButton* mBtnOpen = nullptr;
		MyButton* mBtnInfo = nullptr;
		MyButton* mBtnExpand = nullptr;
	};


	REGISTER_STAGE_ENTRY("TFWR Game", TestStage, EExecGroup::Dev, "Game|Script");

}
