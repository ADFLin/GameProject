#include "Stage/TestStageHeader.h"

#include "StoreSimCore.h"
#include "StoreSimNav.h"

#include "GameRenderSetup.h"


#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"
#include "Async/Coroutines.h"
#include "DebugDraw.h"


#include "BehaviorTree/TreeBuilder.h"
#include "Serialize/FileStream.h"
#include "StoreSimSerialize.h"
#include "FileSystem.h"



namespace StoreSim
{
	using namespace Render;


	template< typename T >
	struct TClassReigsterHelper
	{
		TClassReigsterHelper()
		{
			T::StaticClass();
		}
	};

#define REGISTER_CLASS(CLASS) TClassReigsterHelper<CLASS> ANONYMOUS_VARIABLE(GRegister)

	class TestBlockClass : public TEquipmentClass<TestBlockClass, Equipment>
	{
	public:
		static char const* GetClassName()
		{
			return "TestBlock";
		}
		virtual TArrayView<Area const> getAreaList()
		{
			static Area const AreaList[] =
			{
				{ AABB(Vector2(-1,-1), Vector2(0,1)) , EAreaFlags::Block },
				{ AABB(Vector2( 0, -1), Vector2(1,1)) , EAreaFlags::Aisle },
			};
			return AreaList;
		}
	};
	REGISTER_CLASS(TestBlockClass);

	RenderTransform2D ToRender(XForm2D const& xform)
	{
		RenderTransform2D result;
		result.M = xform.getRotation().toMatrix();
		result.P = xform.getPos();
		return result;
	}


	struct DelegateHandle
	{
		explicit DelegateHandle(void* inPtr = nullptr) : ptr(inPtr){}
		void* ptr;
	};

	template< typename TFunc >
	class TMultiDelegate
	{
	public:
		using FuncType = std::function< TFunc >;
		template< typename F >
		DelegateHandle add(F&& func)
		{
			auto ptr = std::make_unique<FuncType>(func);
			auto handlePtr = ptr.get();
			mDelegates.push_back(std::move(ptr));
			return DelegateHandle(handlePtr);
		}

		template< typename ...TArgs >
		void broadcast(TArgs&& ...args)
		{
			for (auto const& func : mDelegates)
			{
				(*func)(args...);
			}
		}

		std::vector< std::unique_ptr<FuncType> > mDelegates;
	};


	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();

			updateView();

			auto frame = WidgetUtility::CreateDevFrame();
			frame->addButton("Restart", [this](int event, GWidget*) -> bool
			{
				restart();
				return false;
			});
			frame->addCheckBox("Show AABB", bShowAABB);
			frame->addCheckBox("Show BSP", bShowBSP);
			frame->addCheckBox("Show BSP Node", bShowBSPNode);
			frame->addCheckBox("Show Portal", bShowPortal);
			frame->addCheckBox("Show NavArea", bShowNavArea);
			frame->addCheckBox("Optimize Path", [this](int eventId, GWidget* widget) ->bool
			{
				mNavManager.bOptimizePath = widget->cast<GCheckBox>()->bChecked;
				updatePath();
				return true;
			});
			frame->addCheckBox("Portal Clip", [this](int eventId, GWidget* widget) ->bool
			{
				mNavManager.bUsePortalClip = widget->cast<GCheckBox>()->bChecked;
				updatePath();
				return true;
			});

			auto UpdateViewFunc = [this](float) { updateView(); };
			FWidgetProperty::Bind(frame->addSlider("Zoom"), mZoom, 0.1, 10, 1, UpdateViewFunc);

			onNavChanged.add([this]()
			{
				handleNavChanged();
			});
			restart();
			return true;
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			if (mPlaceEquipClass)
			{
				endPlace();
			}
			
			cleanup();
			bNavDirty = false;
#if 0
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 10)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 12)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 14)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 16)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(14.5, 16.5)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(8, 16), Math::DegToRad(180)));
#else
			load("StoreSim/Debug.bin");
#endif
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			if (bNavDirty)
			{
				bNavDirty = false;
				buildNavMesh();
			}
		}

		void cleanup()
		{
			Coroutines::Stop(mHandleNavBuild);

			for (auto equip : mEquipments)
			{
				delete equip;
			}
			mEquipments.clear();
			mNavManager.cleanup();
		}

		bool bShowAABB = true;
		bool bShowBSP = true;
		bool bShowBSPNode = false;
		bool bShowNavArea = true;
		bool bShowPortal = true;

		int mIndexNodeDraw = 0;

		bool save(char const* path)
		{
			auto dirView = FFileUtility::GetDirectory(path);
			if (!FFileSystem::CreateDirectorySequence(dirView.toCString()))
			{
				return false;
			}

			OutputFileSerializer serializer;
			if (!serializer.open(path))
				return false;

			IArchive ar{ serializer, false };

			int numEuqip = mEquipments.size();
			ar & numEuqip;
			for (auto equip : mEquipments)
			{
				ar & equip->mClass->name;
				equip->serialize(ar);
			}
		}

		bool load(char const* path)
		{
			cleanup();
			InputFileSerializer serializer;
			if (!serializer.open(path))
				return false;

			IArchive ar{ serializer, true };

			int numEuqip = 0;
			ar & numEuqip;
			for( int i = 0 ;i < numEuqip; ++i)
			{
				std::string className;
				ar & className;

				Equipment* equip = EquipmentFactory::Instance().create(className.c_str());
				if (equip == nullptr)
					return false;

				mEquipments.push_back(equip);
				equip->serialize(ar);
				equip->notifyTrasformChanged();
			}

			bNavDirty = true;
		}

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.pushXForm();
			g.transformXForm(mWorldToScreen, true);

			auto DrawAreaList = [&](EquipmentClass& equipClass)
			{
				auto areaList = equipClass.getAreaList();
				for (auto const& area : areaList)
				{
					switch (area.flags)
					{
					case EAreaFlags::Aisle:
						RenderUtility::SetBrush(g, EColor::Blue);
						break;
					case EAreaFlags::Block:
						RenderUtility::SetBrush(g, EColor::Red);
						break;
					}
					g.drawRect(area.bound.min, area.bound.getSize());
				}
			};

			for (auto equip : mEquipments)
			{
				g.pushXForm();
				g.transformXForm(ToRender(equip->transform), false);
				DrawAreaList(*equip->mClass);
				g.popXForm();

				if (bShowAABB)
				{
					RenderUtility::SetBrush(g, EColor::Null);
					RenderUtility::SetPen(g, EColor::Red);
					g.drawRect(equip->bound.min, equip->bound.getSize());
				}
			}

			if (mPlaceEquipClass)
			{
				g.pushXForm();
				g.transformXForm(ToRender(mPlaceXForm), false);
				DrawAreaList(*mPlaceEquipClass);
				g.popXForm();

				if (bShowAABB)
				{
					AABB bound = mPlaceEquipClass->calcAreaBound(mPlaceXForm);
					RenderUtility::SetBrush(g, EColor::Null);
					RenderUtility::SetPen(g, EColor::Red);
					g.drawRect(bound.min, bound.getSize());
				}
			}

			if (bShowBSP)
			{
				auto& bspTree = mNavManager.mTree;
				RenderUtility::SetFont(g, FONT_S16);

				if (bShowBSPNode)
				{

					struct TreeDrawer
					{
						void draw(TArray < Bsp2D::Tree::Node*>& nodes, Color3f const& color)
						{
							g.setTextColor(color);
							g.setPen(color);
							for (auto node : nodes)
							{
								draw(node);
							}
						}

						void draw(Bsp2D::Tree::Node* node)
						{
							if (node->isLeaf())
							{
								auto& leaf = bspTree.mLeaves[~node->tag];
								for (int i = 0; i < leaf.edges.size(); ++i)
								{
									Bsp2D::Tree::Edge& edge = bspTree.mEdges[leaf.edges[i]];
									g.drawLine(edge.v[0], edge.v[1]);
									Vector2 v = 0.5*(edge.v[0] + edge.v[1]);
									g.drawLine(v, v + edge.plane.getNormal());
									g.drawTextF(v, "%d", leaf.edges[i]);
								}

							}
							else if (0)
							{
								Bsp2D::Tree::Edge& edge = bspTree.mEdges[node->idxEdge];
								g.drawLine(edge.v[0], edge.v[1]);
								Vector2 v = (edge.v[0] + edge.v[1]) / 2;
								g.drawTextF(v, "%u", node->tag);
							}
						}
						void draw2(Bsp2D::Tree::Node* node)
						{
							Bsp2D::Tree::Edge& edge = bspTree.mEdges[node->idxEdge];
							g.drawLine(edge.v[0], edge.v[1]);
							Vector2 v = (edge.v[0] + edge.v[1]) / 2;
							g.drawTextF(v, "%u", node->tag);
						}

						void drawNodes()
						{
							draw(frontNods, RenderUtility::GetColor(EColor::Brown));
							draw(backNods, RenderUtility::GetColor(EColor::Magenta));
						}
						void gen(Bsp2D::Tree::Node* node)
						{
							if (node->isLeaf())
								return;
							gen(node->front, frontNods);
							gen(node->back, backNods);
						}
						void gen(Bsp2D::Tree::Node* node, TArray < Bsp2D::Tree::Node*>& nodes)
						{
							nodes.push_back(node);
							if (node->isLeaf())
								return;

							gen(node->front, nodes);
							gen(node->back, nodes);
						}

						void drawNode(Bsp2D::Tree::Node* node)
						{

							Bsp2D::Tree::Edge& edge = bspTree.mEdges[node->idxEdge];
							Vector2 v = (edge.v[0] + edge.v[1]) / 2;

							auto parent = node->parent;
							if (parent)
							{
								Bsp2D::Tree::Edge& edgeP = bspTree.mEdges[parent->idxEdge];
								Vector2 vP = (edgeP.v[0] + edgeP.v[1]) / 2;
								RenderUtility::SetPen(g, EColor::Orange);
								g.drawLine(v, vP);
							}

							RenderUtility::SetPen(g, EColor::Purple);
							//g.drawLine(edge.v[0], edge.v[1]);
						}
						RHIGraphics2D& g;
						Bsp2D::Tree&   bspTree;

						TArray< Bsp2D::Tree::Node* > frontNods;
						TArray< Bsp2D::Tree::Node* > backNods;

					};

					TreeDrawer drawer{ g , bspTree };
					if (bspTree.mNodes.isValidIndex(mIndexNodeDraw))
					{
						drawer.gen(bspTree.mNodes[mIndexNodeDraw]);
						drawer.drawNodes();

						auto color = RenderUtility::GetColor(EColor::Orange);
						g.setTextColor(color);
						g.setPen(color);
						drawer.draw2(bspTree.mNodes[mIndexNodeDraw]);

			
					}
	

#if 0
					for (Bsp2D::Tree::Node* node : bspTree.mNodes)
					{
						drawer.drawNode(node);
					}
#endif
				}
				else
				{
					g.setTextColor(Color3f(0, 1, 0));
					int indexLeaf = 0;
					for (Bsp2D::Tree::Leaf& leaf : bspTree.mLeaves)
					{
						for (int i = 0; i < leaf.edges.size(); ++i)
						{
							Bsp2D::Tree::Edge& edge = bspTree.mEdges[leaf.edges[i]];
							RenderUtility::SetPen(g, EColor::Yellow);
							g.drawLine(edge.v[0], edge.v[1]);

							Vector2 const rectSize = Vector2(0.2, 0.2);
							Vector2 const offset = rectSize / 2;

							//g.drawRect(edge.v[0] - offset, rectSize);
							//g.drawRect(edge.v[1] - offset, rectSize);

							Vector2 v1 = (edge.v[0] + edge.v[1]) / 2;
							Vector2 v2 = v1 + 0.8 * edge.plane.getNormal();
							g.drawTextF(0.5*(v1 + v2), "%d", indexLeaf);
							//g.drawTextF(0.5*(v1 + v2), "%d %d", leaf.edges[i], indexLeaf);
							Vector2 offsetH = 0.2 * Math::GetNormal(edge.v[1] - edge.v[0]);
							g.drawTextF(edge.v[0] + 0.2 * edge.plane.getNormal() + offsetH, "%d", i);
							g.drawTextF(edge.v[1] + 0.2 * edge.plane.getNormal() - offsetH, "%d", i);
							RenderUtility::SetPen(g, EColor::Green);
							g.drawLine(v1, v2);
						}

						++indexLeaf;
					}
				}
			}

			if (bShowPortal)
			{
				auto& bspTree = mNavManager.mTree;
				auto& buidler = mNavManager.mBuilder;
#if 0
				for (auto splane : buidler.mSPlanes)
				{
#if 1
					RenderUtility::SetPen(g, EColor::Cyan);
					RenderUtility::SetBrush(g, EColor::Green);
					g.drawLine(splane->debugPos[0], splane->debugPos[1]);

					Vector2 const rectSize = Vector2(0.2, 0.2);
					Vector2 const offset = rectSize / 2;
					g.drawRect(splane->debugPos[0] - offset, rectSize);
					g.drawRect(splane->debugPos[1] - offset, rectSize);
#endif
				}
#endif

				g.setTextColor(Color3f(1, 1, 0));
				for (auto splane : buidler.mSPlanes)
				{
					for (auto const& portal : splane->portals)
					{
						RenderUtility::SetBrush(g, EColor::Blue);
						RenderUtility::SetPen(g, EColor::Orange);
						g.drawLine(portal.v[0], portal.v[1]);

						Vector2 const rectSize = Vector2(0.2, 0.2);
						Vector2 const offset = rectSize / 2;

						g.drawRect(portal.v[0] - offset, rectSize);
						g.drawRect(portal.v[1] - offset, rectSize);
						Vector2 mid = 0.5 * (portal.v[0] + portal.v[1]);


						RenderUtility::SetPen(g, EColor::Brown);
						if (Bsp2D::Tree::IsLeafTag(portal.frontTag))
						{
							g.drawLine(mid, bspTree.getLeafByTag(portal.frontTag).debugPos);
						}

						if (Bsp2D::Tree::IsLeafTag(portal.backTag))
						{
							g.drawLine(mid, bspTree.getLeafByTag(portal.backTag).debugPos);
						}

						g.drawTextF(mid, "%u %u", ~portal.frontTag, ~portal.backTag);
					}
				}
			}
			if (bShowNavArea)
			{
				auto& navMesh = mNavManager.mNavMesh;
				g.beginBlend(0.2);
				for (int indexArea = 0; indexArea < navMesh.areas.size(); ++indexArea)
				{
					auto& area = navMesh.areas[indexArea];

					if (indexArea == mFromIndex)
						RenderUtility::SetBrush(g, EColor::Blue, COLOR_LIGHT);
					else if (indexArea == mToIndex)
						RenderUtility::SetBrush(g, EColor::Green);
					else
						RenderUtility::SetBrush(g, EColor::Yellow);

					RenderUtility::SetPen(g, EColor::White);
					g.drawPolygon(area.vertices.data(), area.vertices.size());
				}
				g.endBlend();

				for (auto const& link : navMesh.links)
				{
					RenderUtility::SetPen(g, EColor::Brown);
					g.drawLine(link.pos, link.area[0]->center);
					g.drawLine(link.pos, link.area[1]->center);
				}

				auto const& path = mPath;
				if (path.size() > 1)
				{
					RenderUtility::SetPen(g, EColor::Magenta);
					g.drawLineStrip(path.data(), path.size());
				}
			}

			DrawDebugCommit(::Global::GetIGraphics2D());

			g.popXForm();

			if (mPlaceEquipClass)
			{
				g.drawTextF(Vec2i(10,10), "Can Place = %d", mCanPlace);
			}
			g.endRender();
		}

		float mZoom = 1.0f;
		Vector2 mViewPos = 0.5 * Vector2(20, 20);
		void updateView()
		{
			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, mViewPos, Vector2(0, 1), mZoom * screenSize.y / float(20 + 5), true);
			mScreenToWorld = mWorldToScreen.inverse();
		}

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		EquipmentClass* mPlaceEquipClass = nullptr;
		XForm2D mPlaceXForm;
		bool    mCanPlace;
		int     mPlaceStep = INDEX_NONE;
		int     mIndexEquipSelected = INDEX_NONE;

		void startPlace()
		{
			mPlaceEquipClass = TestBlockClass::StaticClass();
			mPlaceXForm.setIdentity();
			mPlaceStep = 0;
		}

		void endPlace()
		{
			mPlaceEquipClass = nullptr;
			mPlaceStep = INDEX_NONE;
		}


		NavManager mNavManager;
		bool bNavDirty = false;
		TMultiDelegate<void()> onNavChanged;

		Coroutines::ExecutionHandle mHandleNavBuild;
		void buildNavMesh()
		{
			mHandleNavBuild = Coroutines::Start([&]
			{
				TArray< Bsp2D::PolyArea* > areaList;

				for (auto equip : mEquipments)
				{
					for (auto& area : equip->navAreas)
					{
						areaList.push_back(&area);
					}
				}

				mNavManager.buildMesh(areaList);
				onNavChanged.broadcast();
			});
		}

		Equipment* spawn(EquipmentClass& equipClass, XForm2D const& xForm)
		{
			auto equip = equipClass.create();
			equip->transform = xForm;
			equip->notifyTrasformChanged();
			mEquipments.push_back(equip);

			bNavDirty = true;
			return equip;
		}


		Vector2 mFromPos;
		Vector2 mToPos;
		int mFromIndex = INDEX_NONE;
		int mToIndex = INDEX_NONE;
		TArray< Vector2 > mPath;

		float mSnapAngle  = 5;
		float mSnapLength = 0.5;

		static float SnapValue(float value, float snapFactor)
		{
			return Math::Floor(value / snapFactor) * snapFactor;
		}
		static Vector2 SnapValue(Vector2 value, float snapFactor)
		{
			return Vector2(SnapValue(value.x, snapFactor), SnapValue(value.y, snapFactor));
		}


		bool mbDraging = false;
		Vector2 mLastDragPos;
		MsgReply onMouse(MouseMsg const& msg) override
		{
			Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());
			if (msg.isControlDown())
			{
				if (msg.onLeftDown())
				{
					mbDraging = true;
					mLastDragPos = worldPos;
				}
				else if (msg.onMoving() && mbDraging)
				{
					Vector2 offset = mLastDragPos - worldPos;
					mViewPos += offset;
					updateView();
				}
				else if (msg.onLeftUp())
				{
					mbDraging = false;
				}
			}
			else if ( mPlaceEquipClass )
			{
				if (msg.onMoving())
				{
					if (msg.isShiftDown())
					{
						XForm2D xform = mPlaceXForm;
						xform.setTranslation(worldPos);
						auto equipBound = mPlaceEquipClass->calcAreaBound(xform);

						equipBound.expand(Vector2(1, 1));

						OverlapResult overlapResult;
						if (haveOverlap(equipBound, overlapResult))
						{
							Vector2 dir = mPlaceXForm.getRotation().getXDir();
							Vector2 dirX = overlapResult.equip->transform.getRotation().getXDir();
							Vector2 dirY = overlapResult.equip->transform.getRotation().getXDir();

							float dotX = Math::Dot(dir, dirX);
							float dotY = Math::Dot(dir, dirY);

							float dot;
							if (Math::Abs(dotX) > Math::Abs(dotY))
							{
								dir = dirX;
								if (dotX < 0)
									dir = -dir;
							}
							else
							{
								dir = dirY;
								if (dotY < 0)
									dir = -dir;
							}

							xform.setBaseXDir(dir);
							mPlaceXForm = xform;
						}
					}
					else
					{
						if (msg.onLeftDown() || msg.onMoving())
						{

							switch (mPlaceStep)
							{
							case 0:
								{
									mPlaceXForm.setTranslation(SnapValue(worldPos, mSnapLength));
									mCanPlace = canPlace(*mPlaceEquipClass, mPlaceXForm);
								}
								break;
							case 1:
								{
									Vector2 dir = worldPos - mPlaceXForm.getPos();
									float len = dir.normalize();
									if (len == 0)
									{
										dir = Vector2(1, 0);
									}

									float angle = Math::ATan2(dir.y, dir.x);
									mPlaceXForm.setRoatation(SnapValue(angle, Math::DegToRad(mSnapAngle)));
									mCanPlace = canPlace(*mPlaceEquipClass, mPlaceXForm);
								}
								break;
							}
						}
						else if (msg.onRightDown())
						{
							switch (mPlaceStep)
							{
							case 0:
								{
									endPlace();
								}
								break;
							case 1:
								{
									mPlaceStep = 0;
								}
								break;
							}
						}
					}

				}
				else if (msg.onLeftDown())
				{
	
					switch (mPlaceStep)
					{
					case 0:
						{					
							mPlaceStep += 1;
						}
						break;
					case 1:
						{
							if (canPlace(*mPlaceEquipClass, mPlaceXForm))
							{
								auto equip = spawn(*mPlaceEquipClass, mPlaceXForm);
								endPlace();
							}
							else
							{


							}
						}
						break;
					}
				}
				else if (msg.onRightDown())
				{
					switch (mPlaceStep)
					{
					case 0:
						{
							endPlace();
						}
						break;
					case 1:
						{
							mPlaceStep = 0;
						}
						break;
					}
				}
			}
			else
			{
				Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());

				if (msg.onLeftDown())
				{
					mFromIndex = mNavManager.getAreaIndex(worldPos);
					mFromPos = worldPos;
					updatePath();
				}
				else if (msg.onRightDown())
				{
					mToIndex = mNavManager.getAreaIndex(worldPos);
					mToPos = worldPos;
					updatePath();
				}
			}

			return BaseClass::onMouse(msg);
		}

		void handleNavChanged()
		{
			if (mFromIndex != INDEX_NONE)
			{
				mFromIndex = mNavManager.getAreaIndex(mFromPos);
			}
			if (mToIndex != INDEX_NONE)
			{
				mToIndex = mNavManager.getAreaIndex(mToPos);
			}
			updatePath();
		}

		void updatePath()
		{
			if (mFromIndex != INDEX_NONE && mToIndex != INDEX_NONE)
			{
				auto fromPos = mNavManager.mTree.mLeaves[mFromIndex].debugPos;
				auto toPos = mNavManager.mTree.mLeaves[mToIndex].debugPos;
				DrawDebugClear();
				mNavManager.findPath(mFromIndex, mFromPos, mToIndex, mToPos, mPath);
			}
		};


		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::X:
					{
						startPlace();
					}
					break;
				case EKeyCode::Z:
					{
						DrawDebugClear();
						Coroutines::Resume(mHandleNavBuild);
					}
					break;
				case EKeyCode::M:
					++mIndexNodeDraw;
					break;
				case EKeyCode::N:
					--mIndexNodeDraw;
					break;
				case EKeyCode::O:
					{
						save("StoreSim/test.bin");
					}
					break;
				case EKeyCode::P:
					{
						load("StoreSim/test.bin");
					}
					break;
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
		struct OverlapResult
		{
			Equipment* equip;
			int        indexArea;
		};
		bool haveOverlap(AABB const& bound, OverlapResult& overlapResult)
		{
			bool result = false;
			float minDist = Math::MaxFloat;
			for (auto equipCheck : mEquipments)
			{
				if (!bound.isIntersect(equipCheck->bound))
					continue;

				auto checkAreaList = equipCheck->mClass->getAreaList();
				XForm2D xFormRel = equipCheck->transform;
				for (Area const& areaCheck : checkAreaList)
				{
					SATSolver solver;
					if (!solver.testBoxIntersect(bound.min, bound.getSize(), areaCheck.bound.min, areaCheck.bound.getSize(), xFormRel))
					{
						if (solver.fResult > Bsp2D::WallThickness)
						{
							result = true;
							if (solver.fResult < minDist)
							{
								overlapResult.equip = equipCheck;
								overlapResult.indexArea = &areaCheck - checkAreaList.data();
							}
						}
					}
				}
			}
			return result;
		}

		bool canPlace(EquipmentClass& equipClass, XForm2D const& xform)
		{
			auto placeAreaList = equipClass.getAreaList();
			auto equipBound = equipClass.calcAreaBound(xform);

			for (auto equipCheck : mEquipments)
			{
				if (!equipBound.isIntersect(equipCheck->bound) )
					continue;
				
				auto checkAreaList = equipCheck->mClass->getAreaList();
				XForm2D xFormRel = XForm2D::MakeRelative(equipCheck->transform, xform);
				for (Area const& areaPlace : placeAreaList)
				{
					for (Area const& areaCheck : checkAreaList)
					{
						if ( !HaveBlock(areaPlace.flags, areaCheck.flags) )
							continue;

						SATSolver solver;
						if (!solver.testBoxIntersect(areaPlace.bound.min, areaPlace.bound.getSize(), areaCheck.bound.min, areaCheck.bound.getSize(), xFormRel))
						{
							if (solver.fResult > Bsp2D::WallThickness)
								return false;
						}
					}
				}
			}

			return true;
		}
	protected:

		TArray<Equipment*> mEquipments;
	};

	REGISTER_STAGE_ENTRY("Store Sim", TestStage, EExecGroup::Dev);


}//namespace StoreSim