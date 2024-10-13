
#include "Stage/TestStageHeader.h"
#include "Collision2D/Bsp2D.h"
#include "Holder.h"
#include "InputManager.h"

namespace Bsp2D
{
	class MyRenderer : public SimpleRenderer
	{
	public:
		void draw(Graphics2D& g, PolyArea const& poly)
		{
			drawPoly(g, &poly.mVertices[0], poly.getVertexNum());
		}
		void drawPolyInternal(Graphics2D& g, PolyArea const& poly, Vec2i buf[])
		{
			drawPoly(g, &poly.mVertices[0], poly.getVertexNum(), buf);
		}
	};

	class TreeDrawVisitor
	{
	public:
		TreeDrawVisitor(Tree& t, Graphics2D& g, MyRenderer& r)
			:tree(t), g(g), renderer(r)
		{
		}

		void visit(Tree::Leaf& leaf);
		void visit(Tree::Node& node);

		Tree&       tree;
		Graphics2D& g;
		MyRenderer&   renderer;
	};


	class TestStage : public StageBase
		            , public MyRenderer
	{
		using BaseClass = StageBase;
	public:

		enum
		{
			UI_ADD_POLYAREA = BaseClass::NEXT_UI_ID,
			UI_BUILD_TREE,
			UI_TEST_INTERATION,
			UI_ACTOR_MOVE,

		};

		enum ControlMode
		{
			CMOD_NONE,
			CMOD_CREATE_POLYAREA,
			CMOD_TEST_INTERACTION,
			CMOD_ACTOR_MOVE,
		};

		ControlMode mCtrlMode;
		TPtrHolder< PolyArea >  mPolyEdit;
		using PolyAreaVec = TArray< PolyArea* >;
		PolyAreaVec mPolyAreaMap;
		Tree     mTree;
		bool     mDrawTree;
		Vector2    mSegment[2];
		bool     mHaveCol;
		Vector2    mPosCol;


		struct Actor
		{
			Vector2 pos;
			Vector2 size;
		};
		Actor       mActor;

		TestStage();


		void moveActor(Actor& actor, Vector2 const& offset);

		void testPlane()
		{

			Plane plane;
			plane.init(Vector2(0, 1), Vector2(1, 1));
			float dist = plane.calcDistance(Vector2(0, 0));

			int i = 1;
		}

		void testTree()
		{
			Tree tree;
			PolyArea poly(Vector2(0, 0));
			poly.pushVertex(Vector2(1, 0));
			poly.pushVertex(Vector2(0, 1));

			PolyArea* list[] = { &poly };
			tree.build(list, 1, Vector2(-1000, -1000), Vector2(1000, 1000));
		}

		bool onInit() override;
		void onUpdate(GameTimeSpan deltaTime) override;



		void onRender(float dFrame) override;
		void restart();


		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;
		bool onWidgetEvent(int event, int id, GWidget* ui) override;

	protected:

	};
}

REGISTER_STAGE_ENTRY("Bsp Test", Bsp2D::TestStage, EExecGroup::Test, "Algorithm");

namespace Bsp2D
{

	TestStage::TestStage()
	{
		mCtrlMode = CMOD_NONE;
		mDrawTree = false;
		mActor.pos = Vector2(10, 10);
		mActor.size = Vector2(5, 5);
	}

	bool TestStage::onInit()
	{
		//testTree();

		::Global::GUI().cleanupWidget();

		DevFrame* frame = WidgetUtility::CreateDevFrame();
		frame->addButton(UI_BUILD_TREE, "Build Tree");
		frame->addButton(UI_ADD_POLYAREA, "Add PolyArea");
		frame->addButton(UI_TEST_INTERATION, "Test Collision");
#if 0
		frame->addButton(UI_ACTOR_MOVE, "Actor Move");
#endif
		restart();
		return true;
	}

	void TestStage::onUpdate(GameTimeSpan deltaTime)
	{
		BaseClass::onUpdate(deltaTime);

		InputManager& im = InputManager::Get();

		if (mCtrlMode == CMOD_ACTOR_MOVE)
		{

			float speed = 0.4f;
			Vector2 offset = Vector2(0, 0);

			if (im.isKeyDown(EKeyCode::W))
				offset += Vector2(0, -1);
			else if (im.isKeyDown(EKeyCode::S))
				offset += Vector2(0, 1);

			if (im.isKeyDown(EKeyCode::A))
				offset += Vector2(-1, 0);
			else if (im.isKeyDown(EKeyCode::D))
				offset += Vector2(1, 0);

			float len = offset.length2();
			if (len > 0)
			{
				offset *= (speed / sqrt(len));
				moveActor(mActor, offset);
			}
		}
	}

	void TestStage::restart()
	{
		mCtrlMode = CMOD_NONE;
		mPolyEdit.clear();
		mHaveCol = false;

		mSegment[0] = Vector2(0, 0);
		mSegment[1] = Vector2(10, 10);

		for (auto poly : mPolyAreaMap)
		{
			delete poly;
		}
		mPolyAreaMap.clear();
		mTree.clear();
	}


	bool gShowEdgeNormal = false;
	int gIdxNode = 0;
	struct MoveDBG
	{
		Vector2 outOffset;
		float frac;
	} gMoveDBG;
	void TreeDrawVisitor::visit(Tree::Node& node)
	{
#if 0
		if (gIdxNode != node.tag)
			return;
#endif
		Tree::Edge& edge = tree.mEdges[node.idxEdge];
		Vector2 mid = renderer.convertToScreen((edge.v[0] + edge.v[1]) / 2);
		InlineString< 32 > str;
		str.format("%u", node.tag);
		g.setTextColor(Color3ub(0, 255, 125));
		g.drawText(mid, str);
	}

	void TreeDrawVisitor::visit(Tree::Leaf& leaf)
	{
		for (int i = 0; i < leaf.edges.size(); ++i)
		{
			Tree::Edge& edge = tree.mEdges[leaf.edges[i]];

			Vec2i pos[2];
			RenderUtility::SetPen(g, EColor::Blue);
			renderer.drawLine(g, edge.v[0], edge.v[1], pos);

			Vec2i const rectSize = Vec2i(6, 6);
			Vec2i const offset = rectSize / 2;

			g.drawRect(pos[0] - offset, rectSize);
			g.drawRect(pos[1] - offset, rectSize);

			Vector2 v1 = (edge.v[0] + edge.v[1]) / 2;
			Vector2 v2 = v1 + 0.8 * edge.plane.getNormal();
			RenderUtility::SetPen(g, EColor::Green);
			renderer.drawLine(g, v1, v2, pos);
		}
	}

	void TestStage::onRender(float dFrame)
	{
		Graphics2D& g = Global::GetGraphics2D();

		RenderUtility::SetBrush(g, EColor::Gray);
		RenderUtility::SetPen(g, EColor::Gray);
		g.drawRect(Vec2i(0, 0), ::Global::GetScreenSize());

		RenderUtility::SetPen(g, EColor::Black);
		RenderUtility::SetBrush(g, EColor::Yellow);

		Vec2i sizeRect = Vec2i(6, 6);
		Vec2i offsetRect = sizeRect / 2;

		for (PolyAreaVec::iterator iter(mPolyAreaMap.begin()), itEnd(mPolyAreaMap.end());
			iter != itEnd; ++iter)
		{
			PolyArea* poly = *iter;
			draw(g, *poly);
		}

		if (mTree.getRoot())
		{
			TreeDrawVisitor visitor(mTree, g, *this);
			mTree.visit(visitor);

			if (gShowEdgeNormal)
			{
				RenderUtility::SetPen(g, EColor::Blue);

				for (int i = 0; i < mTree.mEdges.size(); ++i)
				{
					Tree::Edge& edge = mTree.mEdges[i];
					Vector2 mid = (edge.v[0] + edge.v[1]) / 2;
					drawLine(g, mid, mid + 0.5 * edge.plane.getNormal());
				}
			}
		}

		switch (mCtrlMode)
		{
		case CMOD_CREATE_POLYAREA:
			if (mPolyEdit)
			{
				Vec2i buf[32];
				drawPolyInternal(g, *mPolyEdit, buf);
				int const lenRect = 6;
				RenderUtility::SetBrush(g, EColor::White);
				for (int i = 0; i < mPolyEdit->getVertexNum(); ++i)
				{
					g.drawRect(buf[i] - offsetRect, sizeRect);
				}
			}
			break;
		case CMOD_TEST_INTERACTION:
			{
				RenderUtility::SetPen(g, EColor::Red);

				Vec2i pos[2];
				drawLine(g, mSegment[0], mSegment[1], pos);
				RenderUtility::SetBrush(g, EColor::Green);
				g.drawRect(pos[0] - offsetRect, sizeRect);
				RenderUtility::SetBrush(g, EColor::Blue);
				g.drawRect(pos[1] - offsetRect, sizeRect);
				if (mHaveCol)
				{
					RenderUtility::SetBrush(g, EColor::Red);
					g.drawRect(convertToScreen(mPosCol) - offsetRect, sizeRect);
				}
			}
			break;
		case CMOD_ACTOR_MOVE:
			{
				drawRect(g, mActor.pos - mActor.size / 2, mActor.size);

				InlineString< 256 > str;
				str.format("frac = %f offset = ( %f %f )",
					gMoveDBG.frac, gMoveDBG.outOffset.x, gMoveDBG.outOffset.y);


				g.drawText(Vec2i(20, 20), str);

				LogMsg(str);
			}
			break;
		}

	}

	void TestStage::moveActor(Actor& actor, Vector2 const& offset)
	{
		Vector2 half = actor.size / 2;

		Vector2 corner[4];
		corner[0] = actor.pos + Vector2(half.x, half.y);
		corner[1] = actor.pos + Vector2(-half.x, half.y);
		corner[2] = actor.pos + Vector2(-half.x, -half.y);
		corner[3] = actor.pos + Vector2(half.x, -half.y);

		Vector2 outOffset = offset;

		int idxCol = INDEX_NONE;
		int idxHitEdge = INDEX_NONE;
		float frac = 1.0f;
		for (int i = 0; i < 4; ++i)
		{
			Tree::ColInfo info;
			if (!mTree.segmentTest(corner[i], corner[i] + outOffset, info))
				continue;

			if (idxHitEdge == info.indexEdge && frac <= info.frac)
				continue;

			Tree::Edge& edge = mTree.getEdge(info.indexEdge);

			float dotVal = outOffset.dot(edge.plane.getNormal());
			if (dotVal > 0)
				continue;


			frac = info.frac;
			assert(frac < 1.0f);
			idxCol = i;
			outOffset -= ((1 - info.frac) * (dotVal)) * edge.plane.getNormal();
		}


		frac = 1.0f;
		for (int i = 0, prev = 3; i < 4; prev = i++)
		{
			Vector2& p1 = corner[prev];
			Vector2& p2 = corner[i];

			Plane plane;
			plane.init(p1, p2);
			float dotValue = plane.getNormal().dot(outOffset);

			if (dotValue <= 0)
				continue;

			Tree::ColInfo info;
			if (!mTree.segmentTest(p1 + outOffset, p2 + outOffset, info))
				continue;

			//ignore corner collision
			if ((idxCol == prev && info.frac < FLOAT_EPSILON) ||
				(idxCol == i && info.frac > 1 - FLOAT_EPSILON))
				continue;

			Tree::Edge& edge = mTree.getEdge(info.indexEdge);
			int idx = (outOffset.dot(edge.v[1] - edge.v[0]) >= 0) ? 0 : 1;

			if ((p1 - edge.v[idx]).length2() < FLOAT_EPSILON ||
				(p2 - edge.v[idx]).length2() < FLOAT_EPSILON)
				continue;

			float dist = plane.calcDistance(edge.v[idx]);

			if (dist < 0)
				continue;

			frac = dist / dotValue;

			if (frac > 1)
				continue;

			break;
		}

		gMoveDBG.outOffset = outOffset;
		gMoveDBG.frac = frac;

		actor.pos += frac * outOffset;

	}

	bool TestStage::onWidgetEvent(int event, int id, GWidget* ui)
	{
		switch (id)
		{
		case UI_ADD_POLYAREA:
			mCtrlMode = CMOD_CREATE_POLYAREA;
			return false;
		case UI_BUILD_TREE:
			if (!mPolyAreaMap.empty())
			{
				Vec2i size = ::Global::GetScreenSize();
				mTree.build(&mPolyAreaMap[0], (int)mPolyAreaMap.size(),
					Vector2(1, 1), Vector2(size.x / 10 - 1, size.y / 10 - 1));
			}
			return false;
		case UI_TEST_INTERATION:
			mCtrlMode = CMOD_TEST_INTERACTION;
			return false;
		case UI_ACTOR_MOVE:
			mCtrlMode = CMOD_ACTOR_MOVE;
			return false;
		}
		return BaseClass::onWidgetEvent(event, id, ui);
	}

	MsgReply TestStage::onMouse(MouseMsg const& msg)
	{
		switch (mCtrlMode)
		{
		case CMOD_CREATE_POLYAREA:
			if (msg.onLeftDown())
			{
				Vector2 pos = convertToWorld(msg.getPos());
				if (mPolyEdit)
				{
					mPolyEdit->pushVertex(pos);
				}
				else
				{
					mPolyEdit.reset(new PolyArea(pos));
				}
			}
			else if (msg.onRightDown())
			{
				if (mPolyEdit)
				{
					if (mPolyEdit->getVertexNum() < 3)
					{
						mPolyEdit.clear();
					}
					else
					{
						mPolyAreaMap.push_back(mPolyEdit.release());
					}
				}
			}
			break;
		case CMOD_TEST_INTERACTION:
			if (msg.onLeftDown() || msg.onRightDown())
			{
				if (msg.onLeftDown())
				{
					mSegment[0] = convertToWorld(msg.getPos());
				}
				else if (msg.onRightDown())
				{
					mSegment[1] = convertToWorld(msg.getPos());
				}
				Tree::ColInfo info;
				mHaveCol = mTree.segmentTest(mSegment[0], mSegment[1], info);
				if (mHaveCol)
				{
					mPosCol = mSegment[0] + info.frac * (mSegment[1] - mSegment[0]);
				}
			}
			break;
		case CMOD_ACTOR_MOVE:
			if (msg.onRightDown())
			{
				mActor.pos = convertToWorld(msg.getPos());
			}
		}
		return BaseClass::onMouse(msg);
	}

	MsgReply TestStage::onKey(KeyMsg const& msg)
	{
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::Q: --gIdxNode; break;
			case EKeyCode::W: ++gIdxNode; break;
			case EKeyCode::F2: gShowEdgeNormal = !gShowEdgeNormal; break;
			}
		}

		return BaseClass::onKey(msg);
	}

}

