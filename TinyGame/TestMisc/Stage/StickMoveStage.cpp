#include "StickMoveStage.h"

#include "Stage/TestStageHeader.h"


#include "GameSettingPanel.h"

#define USE_NEW_METHOD 1

namespace StickMove
{

	static bool RayCircleTest(Vector2 pos, Vector2 offset, Vector2 center, float radius, float t[2])
	{
		Vector2 d = center - pos;
		float a = offset.length2();
		float b = offset.dot(d) / a;
		float c = ( d.length2() - radius * radius ) / a;

		float delta = b * b - c;
		if( delta < 0 )
			return false;

		delta = Math::Sqrt(delta);
		t[0] = ( b - delta);
		t[1] = ( b + delta);
		return true;
	}

	static Vector2 RotateVector(Vector2 const& v, float theta)
	{
		Rotation2D rotation(theta);
		return rotation.rotate(v);
	}

	void Stick::init(MoveBound& moveBound , Vector2 const& pivotPos , Vector2 const& endpointPos )
	{
		mMoveBound = &moveBound;
		moveSpeed = DefaultMoveSpeed();

		mIndexPivot = 0;
		auto& pivot = getPivot();
		auto& endpoint = getEndpoint();

		pivot.pos = pivotPos;
		endpoint.pos = endpointPos;
		mDir = endpoint.pos - pivot.pos;
		mLength = mDir.normalize();

		pivot.indexEdge = mMoveBound->findTouchEdge(pivot.pos);
		endpoint.indexEdge = mMoveBound->findTouchEdge(endpoint.pos);
		evalMovePoint();
	}

	void Stick::tick(float dt)
	{
		mMoveTime += dt;
		if( mMoveTime >= mMoveDuration )
		{
			changePivot();
			evalMovePoint();
		}
		else
		{
			auto& pivot = getPivot();
			auto& endpoint = getEndpoint();

			Vector2 dir = RotateVector(mDir, mMoveAngle * mMoveTime / mMoveDuration);
			endpoint.pos = pivot.pos + dir * mLength;
		}
	}

	void Stick::changeMoveSpeed(float value)
	{
		if( mMoveDuration != 0 )
		{
			float ratio = mMoveTime / mMoveDuration;
			mDir = RotateVector(mDir, mMoveAngle * ratio);
			mMoveAngle = mMoveAngle * (1 - ratio);
			mMoveDuration = Math::Abs(mMoveAngle) / value;
			mMoveTime = 0;
		}
		moveSpeed = value;
	}

	void Stick::evalMovePoint()
	{
		int count = 0;
		for( ;; )
		{
			auto& pivot = getPivot();
			auto& endpoint = getEndpoint();

			MoveResult moveResult;
			int moveState = mMoveBound->calcMovePoint(pivot.pos, mDir, mLength, endpoint.indexEdge, moveResult);
			if( moveState == Move_OK )
			{
				mMoveAngle = moveResult.angle;
				mMovePoint = moveResult.point;
				break;
			}


			if( count == 1 )
			{
				mMovePoint = endpoint;
				if( moveState == Move_Block )
				{
					mMoveAngle = 0;
				}
				else
				{
					assert(moveState == Move_NoPos);
					mMoveAngle = 2 * Math::PI;
				}
				break;
			}

			mIndexPivot = 1 - mIndexPivot;
			mDir *= -1;
			++count;
		}
		mMoveDuration = Math::Abs(mMoveAngle) / moveSpeed;
		mMoveTime = 0;
	}

	void Stick::changePivot()
	{
		mIndexPivot = 1 - mIndexPivot;
		mEndpoints[mIndexPivot] = mMovePoint;

		mDir = getEndpoint().pos - getPivot().pos;
		mDir.normalize();
	}

	void MoveBound::initRect(Vector2 const& size)
	{
		mEdges.resize(4);
		mEdges[0].pos = Vector2(0, 0);
		mEdges[0].offset = Vector2(size.x, 0);
		mEdges[0].normal = Vector2(0, 1);

		mEdges[1].pos = Vector2(0, 0);
		mEdges[1].offset = Vector2(0, size.y);
		mEdges[1].normal = Vector2(1, 0);

		mEdges[2].pos = Vector2(0, size.y);
		mEdges[2].offset = Vector2(size.x, 0);
		mEdges[2].normal = Vector2(0, -1);

		mEdges[3].pos = Vector2(size.x, 0);
		mEdges[3].offset = Vector2(0, size.y);
		mEdges[3].normal = Vector2(-1, 0);
	}

	void MoveBound::initPolygon(Vector2 vertices[], int numVertices)
	{
		mEdges.resize(numVertices);
		int prevIndex = numVertices - 1;
		for( int i = 0; i < numVertices; ++i )
		{
			auto& edge = mEdges[prevIndex];
			edge.pos = vertices[prevIndex];
			edge.offset = vertices[i] - vertices[prevIndex];
			edge.normal = Vector2(edge.offset.y, -edge.offset.x);
			edge.normal.normalize();
		}
	}

	int MoveBound::findTouchEdge(Vector2 const& pos)
	{
		for( int idx = 0; idx < mEdges.size(); ++idx )
		{
			auto const& edge = mEdges[idx];
			Vector2 d = pos - edge.pos;

			float t = d.dot(edge.offset) / edge.offset.length2();
			if( 0 <= t && t <= 1 )
			{
				Vector2 dp = d - t * edge.offset;
				if( dp.length2() < 1e-5 )
					return idx;
			}

		}
		return -1;
	}

	int MoveBound::calcMovePoint(Vector2 const& pivot, Vector2 const& dir, float length, int idxEdgeContact, MoveResult& outResult)
	{
		std::vector< Vector2 >  colPosVec;

		float const NoValue = -100;
		float candidateC = NoValue;
		float candidateAngleFactor;
		bool  bCandidateNeg = false;
		ContactPoint candidatePoint;

		int count = 0;
		for( int idx = 0; idx < mEdges.size(); ++idx )
		{
			auto const& edge = mEdges[idx];
			float t[2];
			if( !RayCircleTest(edge.pos, edge.offset, pivot, length, t) )
				continue;

			for( int i = 0; i < 2; ++i )
			{
				if( 0 > t[i] || t[i] > 1 )
					continue;

				++count;
				Vector2 pos = edge.pos + t[i] * edge.offset;
				Vector2 d = pos - pivot;
				d.normalize();

				float c = dir.dot(d);
				float s = dir.cross(d);

				float const error = 1e-5;
				if( 1 - c < error )
					continue;

				float angleFactor = 1.0;
				if( 1 + c < error )
				{
					if( idxEdgeContact != -1 && dir.cross(mEdges[idxEdgeContact].normal) < 0 )
						angleFactor = -1;
				}
				else
				{
					angleFactor = (s >= 0) ? 1 : -1;
					if( idxEdgeContact != -1 && dir.cross(mEdges[idxEdgeContact].normal) * s < 0 )
						continue;
				}

				if( candidateC < c )
				{
					candidateC = c;
					candidatePoint.pos = pos;
					candidatePoint.indexEdge = idx;
					candidateAngleFactor = angleFactor;
				}
			}
		}

		if( count == 0 )
			return Move_NoPos;

		if( candidateC == NoValue )
			return Move_Block;

		outResult.point = candidatePoint;
		outResult.angle = candidateAngleFactor * Math::ACos(candidateC);
		return Move_OK;
	}

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage()
		{
		}


		enum
		{

			UI_BoundSizeX = BaseClass::NEXT_UI_ID,
			UI_BoundSizeY,
			UI_Radius,
			UI_MoveSpeedFactor,

			NEXT_UI_ID,
		};

		BaseSettingPanel* mPanel;
		virtual bool onInit()
		{
			if( !BaseClass::onInit() )
				return false;
			::Global::GUI().cleanupWidget();


			mPanel = new BaseSettingPanel(UI_ANY, Vec2i(500, 0), Vec2i(300, 400), nullptr);
			mPanel->setEventCallback(WidgetEventCallBack(this, &TestStage::notifySettingEvent));
			{
				auto slider = mPanel->addSlider(UI_BoundSizeX, "BoundSizeX");
				slider->setRange(10, 400);
			}
			{
				auto slider = mPanel->addSlider(UI_BoundSizeY, "BoundSizeY");
				slider->setRange(10, 400);
			}
			{
				auto slider = mPanel->addSlider(UI_Radius, "Radius");
				slider->setRange(10, 200);
			}
			{
				auto slider = mPanel->addSlider(UI_MoveSpeedFactor, "MoveSpeedFactor");
				slider->setRange(10, 400);
			}
			::Global::GUI().addWidget(mPanel);

			restart();
			return true;
		}

		void updateGUI()
		{
			mPanel->findChildT<GSlider>(UI_BoundSizeX)->setValue(mBoundRectSize.x);
			mPanel->findChildT<GSlider>(UI_BoundSizeY)->setValue(mBoundRectSize.y);
			mPanel->findChildT<GSlider>(UI_Radius)->setValue(mSticks[0].getLength());
			mPanel->findChildT<GSlider>(UI_MoveSpeedFactor)->setValue( 100.0 * mSticks[0].moveSpeed / Stick::DefaultMoveSpeed() );
		}

		bool notifySettingEvent(int event, int id, GWidget* ui)
		{
			switch( id )
			{
			case UI_BoundSizeX:
				mBoundRectSize.x = ui->cast< GSlider >()->getValue();
				mMoveBound.initRect(mBoundRectSize);
				break;
			case UI_BoundSizeY:
				mBoundRectSize.y = ui->cast< GSlider >()->getValue();
				mMoveBound.initRect(mBoundRectSize);
				break;
			case UI_Radius:
				mSticks[0].setLength( ui->cast< GSlider >()->getValue() );
				break;
			case UI_MoveSpeedFactor:
				mSticks[0].changeMoveSpeed( Stick::DefaultMoveSpeed() * ui->cast< GSlider >()->getValue() / 100.0 );
				break;
			}
			return false;
		}

		virtual void onEnd()
		{
			BaseClass::onEnd();
		}

		virtual void onUpdate(GameTimeSpan deltaTime)
		{
			BaseClass::onUpdate(deltaTime);

			for (auto& stick : mSticks)
			{
				stick.tick(deltaTime);
			}
		}

		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();

			Vector2 screenOffset = Vector2(50, 50);
			float renderScale = 2;

			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::Gray);
			g.drawRect(screenOffset, renderScale * mBoundRectSize);

			g.setPen(Color3ub(255, 0, 0), 2);
			for( auto& stick : mSticks )
			{
				Vector2 p0 = screenOffset + renderScale * stick.getPivot().pos;
				Vector2 p1 = screenOffset + renderScale * stick.getEndpoint().pos;
				g.drawLine(p0, p1);
			}
		}

		void restart()
		{
			mSticks.clear();
			mBoundRectSize = Vector2(200, 100);
			mMoveBound.initRect(mBoundRectSize);
			Stick stick;
			stick.init(mMoveBound, Vector2(150, 100), Vector2(100, 0));
			mSticks.push_back(stick);
			updateGUI();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if(msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R: restart(); break;
				case EKeyCode::T:
					{
						Stick stick;
						stick.init(mMoveBound, Vector2(0, 0), Vector2(Global::Random() % 100, 0));
						mSticks.push_back(stick);
					}
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		std::vector< Stick > mSticks;
		MoveBound mMoveBound;
		Vector2     mBoundRectSize;

	};

}//namespace StickMove




REGISTER_STAGE_ENTRY("Stick Move", StickMove::TestStage, EExecGroup::Test );