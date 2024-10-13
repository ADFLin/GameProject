#include "Stage/TestStageHeader.h"

#include "Math/Vector2.h"
#include "Math/Math2D.h"
#include "MarcoCommon.h"
#include "Template/ArrayView.h"
#include "Math/GeometryPrimitive.h"
#include "GameRenderSetup.h"
#include "Collision2D/SATSolver.h"
#include "Collision2D/Bsp2D.h"

#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"
#include "Async/Coroutines.h"
#include "DebugDraw.h"

#include "Algo/AStar.h"

namespace StoreSim
{
	using Math::Vector2;
	using Math::XForm2D;
	using namespace Render;


	enum class EAreaFlags
	{
		Empty = 0,
		Block = 0x10,

		Aisle = BIT(0),


		Shared = 0xf,
	};

	EAreaFlags UniqueMask(EAreaFlags flags)
	{
		return EAreaFlags(uint32(flags) & ~uint32(EAreaFlags::Shared));
	}
	EAreaFlags SharedMask(EAreaFlags flags)
	{
		return EAreaFlags(uint32(flags) & uint32(EAreaFlags::Shared));
	}

	bool CanWalkable(EAreaFlags a)
	{
		return a != EAreaFlags::Block;
	}

	bool HaveBlock(EAreaFlags a, EAreaFlags b)
	{
		EAreaFlags ua = UniqueMask(a);
		EAreaFlags ub = UniqueMask(b);
		if (ua != EAreaFlags::Empty || ub != EAreaFlags::Empty)
			return true;

		EAreaFlags sa = SharedMask(a);
		EAreaFlags sb = SharedMask(b);

		if (sa == EAreaFlags::Empty || sb == EAreaFlags::Empty)
			return false;

		return sa != sb;
	}

	SUPPORT_ENUM_FLAGS_OPERATION(EAreaFlags);

	using AABB = Math::TAABBox<Vector2>;
	void GetVertices(AABB const& bound, XForm2D const& xForm, Vector2 outVertices[4])
	{
		outVertices[0] = xForm.transformPosition(bound.min);
		outVertices[1] = xForm.transformPosition(Vector2(bound.max.x, bound.min.y));
		outVertices[2] = xForm.transformPosition(bound.max);
		outVertices[3] = xForm.transformPosition(Vector2(bound.min.x, bound.max.y));
	}

	struct Area
	{
		AABB bound;
		EAreaFlags flags;
	};



	struct NavArea
	{
		TArray< int > links;
		TArray< Vector2 > polygon;

		static bool Compare(Vector2 const& v1, Vector2 const& v2)
		{
			Vector2 diff = (v1 - v2).abs();
			return diff.x < 1e-3 && diff.y < 1e-3;
		}

		void sortPolygon()
		{
			if (polygon.empty())
				return;

			CHECK(polygon.size() % 2 == 0);
			TArray< Vector2 > sorted;
			sorted.push_back(polygon[0]);
			Vector2 curPos = polygon[1];

			int num = polygon.size();
			if (num > 2)
			{
				polygon[1] = polygon[num - 1];
				polygon[0] = polygon[num - 2];
			}
			num -= 2;
			while (num > 0)
			{
				bool bFound = false;
				for (int i = 0; i < num; i += 2)
				{
					if (Compare(curPos, polygon[i]))
					{
						sorted.push_back(curPos);
						curPos = polygon[i + 1];

						if (num > i + 2)
						{
							polygon[i + 1] = polygon[num - 1];
							polygon[i + 0] = polygon[num - 2];
						}
						num -= 2;
						bFound = true;
						break;
					}
				}

				if (!bFound)
				{
					polygon.clear();
					return;
				}
			}

			polygon = std::move(sorted);
		}
	};


	struct NavLink
	{
		Vector2  pos;
		NavArea* area[2];
		int      polygon[2];
	};
	struct NavMesh
	{
		TArray< NavArea > areas;
		TArray< NavLink > links;

		void clear()
		{
			areas.clear();
			links.clear();
		}
	};


	struct FindState
	{
		NavArea* area;
		int entrance;
		Vector2  pos;
	};
	struct AStarNode : AStar::NodeBaseT< AStarNode, FindState, float >
	{

	};
	class PathFinder : public AStar::AStarT< PathFinder, AStarNode >
	{
	public:
		typedef FindState StateType;
		typedef AStarNode NodeType;
		
		ScoreType calcHeuristic(StateType& state)
		{
			return Math::Distance(state.pos, mGoalPos);
		}
		ScoreType calcDistance(StateType& a, StateType& b)
		{
			return Math::Distance(a.pos, b.pos);
		}
		
		bool      isEqual(StateType& a, StateType& b)
		{
			return a.area == b.area;
		}
		bool      isGoal(StateType& state)
		{
			return state.area == mGoalArea;
		}

		//  call addSreachNode for all possible next state
		void      processNeighborNode(NodeType& aNode)
		{
			auto area = aNode.state.area;
			for (int indexLink : area->links)
			{
				if ( indexLink == aNode.state.entrance )
					continue;

				auto const& link = mMesh->links[indexLink];

				FindState state;
				int index = (link.area[0] == area) ? 1 : 0;
				int indexPolygon = link.polygon[index];
				state.area = link.area[index];
				state.entrance = indexLink;
				state.pos = link.pos;

				addSreachNode(state, aNode);
			}
		}

		template< typename T >
		struct TRange
		{
			T min;
			T max;
		};
		struct PortalView
		{
			Vector2 pos;
			Vector2 portal[2];


			int checkSide(Vector2 const& posTarget)
			{
				float t;
				if (Math::LineLineTest(portal[0], portal[1] - portal[0], pos, posTarget - pos, t))
				{



				}
				else
				{


				}
			}
		};
		bool findPath(int indexFrom, Vector2 const& fromPos , int indexTo, Vector2 const& toPos)
		{
			FindState state;
			state.area = &mMesh->areas[indexFrom];
			state.entrance = INDEX_NONE;
			state.pos = fromPos;

			mGoalPos = toPos;
			mGoalArea = &mMesh->areas[indexTo];

			mPath.clear();
			SreachResult result;
			if (!sreach(state, result))
			{
				return false;
			}

			mPath.push_back(fromPos);
			buildPath(result.globalNode);
			mPath.push_back(toPos);
			return true;
		}

		void buildPath(NodeType* node)
		{
			if (node->parent)
			{
				buildPath(node->parent);
			}
			mPath.push_back(node->state.pos);
		};

		TArray< Vector2 > mPath;

		NavMesh* mMesh;
		Vector2  mGoalPos;
		NavArea* mGoalArea;
	};

	class NavManager
	{
	public:

		Vector2 mBoundMin = Vector2(0, 0);
		Vector2 mBoundMax = Vector2(20, 20);

		Coroutines::ExecutionHandle mHandleBuild;

		void buildMesh(TArrayView< Bsp2D::PolyArea* > areaList)
		{

			mTree.build(areaList.data(), areaList.size(), mBoundMin, mBoundMax);

			for (auto& leaf : mTree.mLeaves)
			{
				AABB bound; bound.invalidate();

				for (auto indexEdge : leaf.edges)
				{
					auto const& edge = mTree.mEdges[indexEdge];
					bound.addPoint(edge.v[0]);
					bound.addPoint(edge.v[1]);
				}

				leaf.debugPos = bound.getCenter();
			}

			mBuilder.mTree = &mTree;
			mPathFinder.mMesh = &mNavMesh;

			mHandleBuild = Coroutines::Start([&]
			{
				mBuilder.build(mBoundMin, mBoundMax);
				mNavMesh.clear();
				mNavMesh.areas.resize(mTree.mLeaves.size());

				for (int indexArea = 0; indexArea < mNavMesh.areas.size(); ++indexArea)
				{
					auto& area = mNavMesh.areas[indexArea];
					auto& leaf = mTree.mLeaves[indexArea];
					uint32 tag = ~uint32(indexArea);

					for (auto indexEdge : leaf.edges)
					{
						auto const& edge = mTree.mEdges[indexEdge];
						area.polygon.push_back(edge.v[0]);
						area.polygon.push_back(edge.v[1]);
					}
				}

				for (auto* splane : mBuilder.mSPlanes)
				{
					for (auto const& portal : splane->portals)
					{
						if (Bsp2D::Tree::IsLeafTag(portal.frontTag))
						{
							auto& areaFront = mNavMesh.areas[~portal.frontTag];
							areaFront.polygon.push_back(portal.v[0]);
							areaFront.polygon.push_back(portal.v[1]);
						}
						if (Bsp2D::Tree::IsLeafTag(portal.backTag))
						{
							auto& areaBack = mNavMesh.areas[~portal.backTag];
							areaBack.polygon.push_back(portal.v[1]);
							areaBack.polygon.push_back(portal.v[0]);
						}
					}
				}
				for (int indexArea = 0; indexArea < mNavMesh.areas.size(); ++indexArea)
				{
					auto& area = mNavMesh.areas[indexArea];
					area.sortPolygon();
					auto& leaf = mTree.mLeaves[indexArea];
					leaf.debugPos = Math::CalcPolygonCentroid(area.polygon);
				}

				for (auto* splane : mBuilder.mSPlanes)
				{
					for (auto const& portal : splane->portals)
					{
						if (Bsp2D::Tree::IsLeafTag(portal.frontTag) == false || Bsp2D::Tree::IsLeafTag(portal.backTag) == false)
							continue;

						auto& areaFront = mNavMesh.areas[~portal.frontTag];
						auto& areaBack = mNavMesh.areas[~portal.backTag];
						NavLink link;
						link.pos = 0.5 * (portal.v[0] + portal.v[1]);
						link.area[0] = &areaFront;
						link.area[1] = &areaBack;
						link.polygon[0] = areaFront.polygon.findIndexPred([&portal](Vector2 const& pos)
						{
							return NavArea::Compare(pos, portal.v[0]);
						});
						link.polygon[1] = areaBack.polygon.findIndexPred([&portal](Vector2 const& pos)
						{
							return NavArea::Compare(pos, portal.v[1]);
						});

						int indexLink = mNavMesh.links.size();
						mNavMesh.links.push_back(link);
						areaFront.links.push_back(indexLink);
						areaBack.links.push_back(indexLink);
					}
				}
			});
		}
		bool findPath(Vector2 const& fromPos, Vector2 const& toPos, TArray<Vector2>& outPath)
		{
			int indexFrom = getAreaIndex(fromPos);
			if (indexFrom == INDEX_NONE)
				return false;
			int indexTo = getAreaIndex(toPos);
			if (indexTo == INDEX_NONE)
				return false;
			return findPath(indexFrom, fromPos, indexTo, toPos, outPath);
		}

		bool findPath(int indexFrom, Vector2 const& fromPos, int indexTo, Vector2 const& toPos, TArray<Vector2>& outPath)
		{
			Bsp2D::Tree::ColInfo colInfo;
			if (!mTree.segmentTest(fromPos, toPos, colInfo))
			{
				outPath.clear();
				outPath.push_back(fromPos);
				outPath.push_back(toPos);
				return true;
			}

			if (!mPathFinder.findPath(indexFrom, fromPos, indexTo, toPos))
				return false;

			outPath = std::move(mPathFinder.mPath);

			for (int indexStart = 0; indexStart < outPath.size() - 2; ++indexStart)
			{
				int indexTest = indexStart + 2;
				for (; indexTest < outPath.size(); ++indexTest)
				{
					if (mTree.segmentTest(outPath[indexStart], outPath[indexTest], colInfo))
						break;
				}
				if (indexTest != indexStart + 2)
				{
					outPath.erase(outPath.begin() + (indexStart + 1), outPath.begin() + (indexTest - 1));
				}
			}
			
			return true;
		}

		void cleanup()
		{
			Coroutines::Stop(mHandleBuild);
			mTree.clear();
			mBuilder.cleanup();
		}


		int getAreaIndex(Vector2 const& pos)
		{
			auto node = mTree.getLeaf(pos);
			if (node)
			{
				return ~node->tag;
			}
			return INDEX_NONE;
		}

		NavMesh mNavMesh;
		Bsp2D::Tree mTree;
		Bsp2D::PortalBuilder mBuilder;
		PathFinder mPathFinder;

	};

	class Equipment;
	class EquipmentClass
	{
	public:
		virtual Equipment* create() = 0;
		virtual TArrayView<Area const> getAreaList() = 0;

		AABB calcAreaBound(XForm2D const& xForm)
		{
			auto areaList = getAreaList();
			CHECK(areaList.size() > 0);
			AABB localBound = areaList[0].bound;
			for (int i = 1; i < areaList.size(); ++i)
			{
				localBound += areaList[i].bound;
			}

			Vector2 v[4];
			GetVertices(localBound, xForm, v);
			AABB result; result.invalidate();
			for (int i = 0; i < ARRAY_SIZE(v); ++i)
			{
				result.addPoint(v[i]);
			}
			return result;
		}
	};

	class Equipment
	{
	public:
		XForm2D transform;
		EquipmentClass* mClass;
		AABB    bound;


		TArray< Bsp2D::PolyArea > navAreas;

		void notifyTrasformChanged()
		{
			updateBound();
			updateNavArea();
		}

		void updateBound()
		{
			bound = mClass->calcAreaBound(transform);
		}

		static void BuildArea(Bsp2D::PolyArea& area, AABB const& bound, XForm2D const& xForm)
		{
			area.mVertices.resize(4);
			GetVertices(bound, xForm, area.mVertices.data());
		}
		void updateNavArea()
		{
			navAreas.clear();

			auto areaList = mClass->getAreaList();
			for (auto const& area : areaList)
			{
				if (CanWalkable(area.flags))
					continue;

				Bsp2D::PolyArea polyArea;
				BuildArea(polyArea, area.bound, transform);
				navAreas.push_back(std::move(polyArea));
			}
		}
	};


	class Actor
	{
	public:



	};



	template< typename MyClass, typename TEquipment >
	class TEquipmentClass : public EquipmentClass
	{
	public:
		virtual Equipment* create()
		{
			return new TEquipment;
		}

		static MyClass* StaticClass()
		{
			static MyClass StaticResult;
			return &StaticResult;
		}
	};

	class TestBlockClass : public TEquipmentClass<TestBlockClass, Equipment>
	{
	public:
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


	RenderTransform2D ToRender(XForm2D const& xform)
	{
		RenderTransform2D result;
		result.M = xform.getRotation().toMatrix();
		result.P = xform.getPos();
		return result;
	}


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

			Vec2i screenSize = ::Global::GetScreenSize();
			mWorldToScreen = RenderTransform2D::LookAt(screenSize, 0.5 * Vector2(20, 20), Vector2(0, 1), screenSize.y / float(20 + 5), true);
			mScreenToWorld = mWorldToScreen.inverse();


			auto frame = WidgetUtility::CreateDevFrame();
			frame->addButton("Restart", [this](int event, GWidget*) -> bool
			{
				restart();
				return false;
			});
			frame->addCheckBox("Show AABB", bShowAABB);
			frame->addCheckBox("Show BSP", bShowBSP);
			frame->addCheckBox("Show Portal", bShowPortal);
			frame->addCheckBox("Show NavArea", bShowNavArea);
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

#if 1

			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 10)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 12)));
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
			for (auto equip : mEquipments)
			{
				delete equip;
			}
			mEquipments.clear();

			mNavManager.cleanup();

		}

		bool bShowAABB = true;
		bool bShowBSP = true;
		bool bShowNavArea = true;
		bool bShowPortal = true;

		void onRender(float dFrame) override
		{
			Vec2i screenSize = ::Global::GetScreenSize();

			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.beginRender();
			g.pushXForm();
			g.transformXForm(mWorldToScreen, false);

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
				g.transformXForm(ToRender(equip->transform), true);
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
				g.transformXForm(ToRender(mPlaceXForm), true);
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
				auto& buidler = mNavManager.mBuilder;
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
						RenderUtility::SetPen(g, EColor::Green);
						g.drawLine(v1, v2);
					}
				}

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


				if (bShowPortal)
				{
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
						}
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
						RenderUtility::SetBrush(g, EColor::Blue);
					else if (indexArea == mToIndex)
						RenderUtility::SetBrush(g, EColor::Green);
					else
						RenderUtility::SetBrush(g, EColor::Yellow);

					RenderUtility::SetPen(g, EColor::White);
					g.drawPolygon(area.polygon.data(), area.polygon.size());
				}
				g.endBlend();

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

		RenderTransform2D mWorldToScreen;
		RenderTransform2D mScreenToWorld;

		EquipmentClass* mPlaceEquipClass = nullptr;
		XForm2D mPlaceXForm;
		bool    mCanPlace;
		int     mPlaceStep = INDEX_NONE;

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


		void buildNavMesh()
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
		}

		Equipment* spawn(EquipmentClass& equipClass, XForm2D const& xForm)
		{
			auto equip = equipClass.create();
			equip->mClass = &equipClass;
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

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if ( mPlaceEquipClass )
			{
				if (msg.onMoving() || msg.onLeftDown())
				{
					Vector2 worldPos = mScreenToWorld.transformPosition(msg.getPos());
					switch (mPlaceStep)
					{
					case 0:
						{
							mPlaceXForm.setTranslation(worldPos);

							mCanPlace = canPlace(*mPlaceEquipClass, mPlaceXForm);
							if (msg.onLeftDown())
							{
								mPlaceStep += 1;
							}
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
							mPlaceXForm.setBaseXDir(dir);
							mCanPlace = canPlace(*mPlaceEquipClass, mPlaceXForm);

							if (msg.onLeftDown())
							{
								auto equip = spawn(*mPlaceEquipClass, mPlaceXForm);
								endPlace();
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

				auto UpdatePath = [&]()
				{
					if (mFromIndex != INDEX_NONE && mToIndex != INDEX_NONE)
					{
						auto fromPos = mNavManager.mTree.mLeaves[mFromIndex].debugPos;
						auto toPos = mNavManager.mTree.mLeaves[mToIndex].debugPos;
						mNavManager.findPath(mFromIndex, mFromPos, mToIndex, mToPos, mPath);
					}
				};

				if (msg.onLeftDown())
				{
					mFromIndex = mNavManager.getAreaIndex(worldPos);
					mFromPos = worldPos;
					UpdatePath();
				}
				else if (msg.onRightDown())
				{
					mToIndex = mNavManager.getAreaIndex(worldPos);
					mToPos = worldPos;
					UpdatePath();
				}
			}

			return BaseClass::onMouse(msg);
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
						startPlace();
					}
					break;
				case EKeyCode::Z:
					{
						DrawDebugClear();
						Coroutines::Resume(mNavManager.mHandleBuild);
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
						if (solver.testBoxIntersect(areaPlace.bound.min, areaPlace.bound.getSize(), areaCheck.bound.min, areaCheck.bound.getSize(), xFormRel))
							continue;


						return false;
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