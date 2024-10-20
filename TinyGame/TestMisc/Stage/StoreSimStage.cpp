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
#include "BehaviorTree/TreeBuilder.h"


#if 1
#include "Async/Coroutines.h"
#include "Misc/DebugDraw.h"
#define DEBUG_BREAK() CO_YEILD(nullptr)
#define DEBUG_POINT(V, C) DrawDebugPoint(V, C, 5);
#define DEBUG_LINE(V1, V2, C) DrawDebugLine(V1, V2, C, 2);
#define DEBUG_TEXT(V, TEXT, C) DrawDebugText(V, TEXT, C);
#else
#define DEBUG_BREAK()
#define DEBUG_POINT(V, C)
#define DEBUG_LINE(V1, V2)
#define DEBUG_TEXT(V, TEXT, C)
#endif

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
		Vector2 center;

		bool sortPolygon()
		{
			CHECK(!polygon.empty());
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
					if (Bsp2D::IsEqual(curPos, polygon[i]))
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
					return false;
				}
			}

			polygon = std::move(sorted);
			return true;
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
		int posIndex;
		Vector2  pos;
	};
	struct AStarNode : AStar::NodeBaseT< AStarNode, FindState, float >
	{
		AStarNode* child = nullptr;
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
			return a.area == b.area && a.entrance == b.entrance && a.posIndex == b.posIndex;
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
				CHECK(link.area[0] == area || link.area[1] == area);
				FindState state;
				int index = (link.area[0] == area) ? 1 : 0;
				int indexPolygon = link.polygon[index];
				state.area = link.area[index];
				state.entrance = indexLink;
				state.pos = link.pos;
				state.posIndex = 0;
				addSreachNode(state, aNode);
				state.pos = state.area->polygon[indexPolygon];
				state.posIndex = 1;
				addSreachNode(state, aNode);
				state.pos = state.area->polygon[(indexPolygon + 1) % state.area->polygon.size()];
				state.posIndex = 2;
				addSreachNode(state, aNode);
			}
		}

		struct Portal
		{
			Vector2 from;
			Vector2 to;
			int clipCount = 0;

			Vector2 getPos(float t)
			{
				return Math::LinearLerp(from, to, t);
			}
		};

		struct PortalView
		{
			static int CheckSide(Portal const& portal, Vector2 const& pos, Vector2 const& posTarget, float& outT)
			{
				if (Math::LineSegmentTest(pos, posTarget, portal.from, portal.to, outT))
				{
					return 0;
				}
				else
				{
					if (outT > 1)
						return 1;
					else
						return -1;
				}
			}

			static int Clip(Portal& portal, Vector2 const& pos, Portal const& clipPortal, float outT[2])
			{
				float tFrom;
				int sideFrom = CheckSide(portal, pos, clipPortal.from, tFrom);

				float tTo;
				int sideTo = CheckSide(portal, pos, clipPortal.to, tTo);

				outT[0] = tFrom;
				outT[1] = tTo;
#if 0
				DEBUG_LINE(pos, clipPortal.from, Color3f(1, 1, 0));
				DEBUG_POINT(portal.getPos(tFrom), Color3f(0, 1, 1));
				DEBUG_LINE(pos, clipPortal.to, Color3f(1, 1, 0));
				DEBUG_POINT(portal.getPos(tTo), Color3f(0, 1, 1));
#endif

				if (sideFrom + sideTo) //LL 0X X0 RR
				{
					if (sideFrom == sideTo) //LL RR
					{
						return sideFrom;
					}
					else if (sideFrom == 0) // 0X
					{
						if (sideTo > 1)
							portal.to = portal.getPos(tFrom);
						else
							portal.from = portal.getPos(tFrom);

						portal.clipCount += 1;
						DEBUG_LINE(portal.from, portal.to, RenderUtility::GetColor(portal.clipCount));

					}
					else //X0
					{
						CHECK(sideTo == 0);
						if (sideFrom > 1)
							portal.from = portal.getPos(tTo);
						else
							portal.to = portal.getPos(tTo);

						portal.clipCount += 1;
						DEBUG_LINE(portal.from, portal.to, RenderUtility::GetColor(portal.clipCount));
					}

				}
				else //LR 00
				{
					if (sideFrom == 0) // 00
					{
						CHECK(sideTo == 0);
						Vector2 temp = portal.getPos(tFrom);
						portal.to = portal.getPos(tTo);
						portal.from = temp;

						portal.clipCount += 1;
						DEBUG_LINE(portal.from, portal.to, RenderUtility::GetColor(portal.clipCount));
					}
					else // LR
					{
						
					}
				}
				return 0;
			}
		};
		bool findPath(int indexFrom, Vector2 const& fromPos , int indexTo, Vector2 const& toPos, bool bUsePortalClip = true)
		{
			FindState state;
			state.area = &mMesh->areas[indexFrom];
			state.entrance = INDEX_NONE;
			state.posIndex = INDEX_NONE;
			state.pos = fromPos;

			mGoalPos = toPos;
			mGoalArea = &mMesh->areas[indexTo];

			mPath.clear();
			SreachResult sreachResult;
			if (!sreach(state, sreachResult))
			{
				return false;
			}

			sreachResult.globalNode->child = nullptr;
			this->constructPath(sreachResult.globalNode, [](NodeType* node)
			{
				if (node->parent)
				{
					node->parent->child = node;
				}
			});


			mPath.push_back(fromPos);
			DEBUG_POINT(fromPos, Color3f(1, 0, 0));

			if ( bUsePortalClip )
			{
				auto GetPortal = [&](NodeType* node) -> Portal
				{
					auto const& link = mMesh->links[node->state.entrance];
					DEBUG_POINT(link.pos, Color3f(0, 0, 1));
					auto area = node->state.area;
					int index = (area == link.area[0]) ? link.polygon[0] : link.polygon[1];
					Portal portal;
					portal.from = area->polygon[index];
					portal.to = area->polygon[(index + 1) % area->polygon.size()];
					return portal;
				};

				auto curNode = sreachResult.startNode;

				curNode = curNode->child;
				Vector2 viewPos = fromPos;
				TArray< Portal > portalQueue;

				while (curNode)
				{
					if (!(curNode->state.entrance != INDEX_NONE))
						break;

					Portal portal = GetPortal(curNode);
					DEBUG_LINE(portal.from, portal.to, Color3f(1, 1, 0));

					portalQueue.push_back(portal);

					curNode = curNode->child;
				}

				Portal portal;
				int indexQueue = 0;
				int indexEnd = 0;
				while (indexQueue < portalQueue.size())
				{
					portal = portalQueue[indexQueue];

					int indexClip = indexQueue + 1;
					for (; indexClip < portalQueue.size(); ++indexClip)
					{
						Portal const& portalClip = portalQueue[indexClip];
						float t[2];
						int side = PortalView::Clip(portal, viewPos, portalClip, t);

						if (side != 0)
						{
							viewPos = (side > 0) ? portal.to : portal.from;
							mPath.push_back(viewPos);
							break;
						}
					}

					if (indexClip == portalQueue.size())
						break;

					++indexQueue;
				}

				while (indexQueue < portalQueue.size())
				{
					portal = portalQueue[indexQueue];
					int indexClip = indexQueue + 1;
					for (; indexClip < portalQueue.size(); ++indexClip)
					{
						Portal const& portalClip = portalQueue[indexClip];
						float t[2];
						int side = PortalView::Clip(portal, viewPos, portalClip, t);
						if (side != 0)
						{
							viewPos = (side > 0) ? portal.to : portal.from;
							mPath.push_back(viewPos);
							break;
						}
					}
					if (indexClip != portalQueue.size())
					{
						++indexQueue;
						continue;
					}

					float t;
					int side = PortalView::CheckSide(portal, viewPos, toPos, t);
					if (side == 0)
						break;

					if (indexQueue + 1 != portalQueue.size())
					{
						portal = portalQueue[indexQueue];
						float t[2];
						PortalView::Clip(portal, viewPos, portalQueue[portalQueue.size() - 1], t);
					}

					viewPos = (side > 0) ? portal.to : portal.from;
					mPath.push_back(viewPos);
					++indexQueue;
				}

				mPath.push_back(viewPos);
				++indexQueue;
			}
			else
			{
				buildPath(sreachResult.globalNode);
			}
			
			mPath.push_back(toPos);
			DEBUG_POINT(toPos, Color3f(0, 1, 0));
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
					if (indexEdge == 8)
					{
						int i = 1;
					}
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
				if (indexArea == 8)
				{
					int i = 1;
				}
				if ( area.polygon.size() > 0 && !area.sortPolygon())
				{
					LogWarning(0, "Area %d polygon error", indexArea);
				}
				auto& leaf = mTree.mLeaves[indexArea];
				leaf.debugPos = Math::CalcPolygonCentroid(area.polygon);
				area.center = leaf.debugPos;
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
						return Bsp2D::IsEqual(pos, portal.v[0]);
					});
					link.polygon[1] = areaBack.polygon.findIndexPred([&portal](Vector2 const& pos)
					{
						return Bsp2D::IsEqual(pos, portal.v[1]);
					});

					if (link.polygon[0] == INDEX_NONE || link.polygon[1] == INDEX_NONE)
					{
						continue;
					}
					int indexLink = mNavMesh.links.size();
					mNavMesh.links.push_back(link);
					areaFront.links.push_back(indexLink);
					areaBack.links.push_back(indexLink);
				}
			}
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

		bool bOptimizePath = false;
		bool bUsePortalClip = false;

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

			if (!mPathFinder.findPath(indexFrom, fromPos, indexTo, toPos, bUsePortalClip))
				return false;

			outPath = std::move(mPathFinder.mPath);

			if (bOptimizePath)
			{
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
			}
			return true;
		}

		void cleanup()
		{
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
#if 0
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
#else
			AABB result; result.invalidate();
			for (int i = 0; i < areaList.size(); ++i)
			{
				Vector2 v[4];
				GetVertices(areaList[i].bound, xForm, v);
				for (int i = 0; i < ARRAY_SIZE(v); ++i)
				{
					result.addPoint(v[i]);
				}
			}
			return result;
#endif
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

	template< typename TFunc >
	class TMultiDelegate
	{
	public:
		template< typename F >
		void add(F&& func)
		{
			mDelegates.push_back(Func(func));
		}

		template< typename ...TArgs >
		void boardcast(TArgs&& ...args)
		{
			for (auto const& func : mDelegates)
			{
				func(args...);
			}
		}

		using Func = std::function< TFunc >;
		std::vector< Func > mDelegates;
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

#if 1

			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 10)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 12)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 14)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(10, 16)));
			spawn(*TestBlockClass::StaticClass(), XForm2D(Vector2(8, 16), Math::DegToRad(180)));
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
						RenderUtility::SetBrush(g, EColor::Blue);
					else if (indexArea == mToIndex)
						RenderUtility::SetBrush(g, EColor::Green);
					else
						RenderUtility::SetBrush(g, EColor::Yellow);

					RenderUtility::SetPen(g, EColor::White);
					g.drawPolygon(area.polygon.data(), area.polygon.size());
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
				onNavChanged.boardcast();
			});
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
				if (msg.onMoving() || msg.onLeftDown())
				{
					
					switch (mPlaceStep)
					{
					case 0:
						{					
							mPlaceXForm.setTranslation(SnapValue(worldPos, mSnapLength));

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

							float angle = Math::ATan2(dir.y, dir.x);
							mPlaceXForm.setRoatation(SnapValue(angle, Math::DegToRad(mSnapAngle)));
							mCanPlace = canPlace(*mPlaceEquipClass, mPlaceXForm);

							if (msg.onLeftDown())
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
						if (!solver.testBoxIntersect(areaPlace.bound.min, areaPlace.bound.getSize(), areaCheck.bound.min, areaCheck.bound.getSize(), xFormRel))
						{
							if (solver.fResult > 1e-3)
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