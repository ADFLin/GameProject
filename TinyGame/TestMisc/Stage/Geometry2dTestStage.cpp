#include "Stage/TestStageHeader.h"

#include "TVector2.h"
#include "Geometry2d.h"
#include "Collision2D/SATSolver.h"

#include "Math/Math2D.h"
#include "InputManager.h"


#include <limits>
namespace G2D
{
	typedef std::vector< Vector2 > Vertices;
	inline Vector2 normalize(Vector2 const& v)
	{
		float len = sqrt(v.length2());
		if( len < 1e-5 )
			return Vector2::Zero();
		return (1 / len) * v;

	}
}

namespace Geom2D
{
	template<>
	struct PolyProperty< ::G2D::Vertices >
	{
		typedef ::G2D::Vertices PolyType;
		static void  Setup(PolyType& p, int size) { p.resize(size); }
		static int   Size(PolyType const& p) { return p.size(); }
		static Vector2 const& Vertex(PolyType const& p, int idx) { return p[idx]; }
		static void  UpdateVertex(PolyType& p, int idx, Vector2 const& value) { p[idx] = value; }
	};
}
namespace G2D
{

	class QHullTestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		QHullTestStage()
			:mTestPos(0, 0)
			, mbInside(false)
		{
		}

		SimpleRenderer mRenderer;
		Vertices mVertices;
		Vertices mHullPoly;
		std::vector< int >   mIndexHull;

		Vector2 mTestPos;
		bool   mbInside;
		bool   mbConvex;
		Vector2 mBoundCenter;
		Vector2 mBoundSize;
		Vector2 mBoundAxisX;

		void updateHull()
		{
			mHullPoly.clear();
			mIndexHull.clear();
			if( mVertices.size() >= 3 )
			{
				mIndexHull.resize(mVertices.size());

				int num = Geom2D::QuickHull(mVertices, &mIndexHull[0]);
				mIndexHull.resize(num);
				for( int i = 0; i < num; ++i )
				{
					mHullPoly.push_back(mVertices[mIndexHull[i]]);
				}
			}

			Geom2D::CalcMinimumBoundingRectangle(mVertices, mIndexHull.data(), mIndexHull.size(), mBoundCenter, mBoundAxisX, mBoundSize);
			mbConvex = Geom2D::IsConvex(mVertices);
		}

		virtual bool onInit()
		{

			{
				Vertices vertices{ Vector2(-1,0) , Vector2(0,-1)  , Vector2(1,0) };
				Geom2D::CalcMinimumBoundingRectangle(vertices, mBoundCenter, mBoundAxisX, mBoundSize);
			}
			::Global::GUI().cleanupWidget();
			restart();
			return true;
		}

		virtual void onEnd()
		{

		}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();

			RenderUtility::SetPen(g, EColor::Gray);
			RenderUtility::SetBrush(g, EColor::Gray);
			g.drawRect(Vec2i(0, 0), Global::GetDrawEngine().getScreenSize());


			RenderUtility::SetPen(g, EColor::Yellow);
			RenderUtility::SetBrush(g, EColor::Null);
			if( !mHullPoly.empty() )
			{
				mRenderer.drawPoly(g, &mHullPoly[0], mHullPoly.size());
			}

			RenderUtility::SetPen(g, EColor::Green);
			RenderUtility::SetBrush(g, EColor::Null);
			if( !mVertices.empty() )
			{
				mRenderer.drawPoly(g, &mVertices[0], mVertices.size());
			}

			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);
			if( mBoundSize.length() > 0 )
			{
				Vector2 halfSize = 0.5 * mBoundSize;
				Vector2 boundAxisY = Math::Perp(mBoundAxisX);
				Vector2 vertices[4] =
				{
					mBoundCenter + halfSize.x * mBoundAxisX + halfSize.y * boundAxisY ,
					mBoundCenter - halfSize.x * mBoundAxisX + halfSize.y * boundAxisY ,
					mBoundCenter - halfSize.x * mBoundAxisX - halfSize.y * boundAxisY ,
					mBoundCenter + halfSize.x * mBoundAxisX - halfSize.y * boundAxisY ,
				};
				mRenderer.drawPoly(g, vertices, ARRAY_SIZE(vertices));
			}

			RenderUtility::SetBrush(g, EColor::Red);
			if( !mVertices.empty() )
			{
				for( int i = 0; i < mVertices.size(); ++i )
				{
					mRenderer.drawCircle(g, mVertices[i], 0.5);
				}
			}

			RenderUtility::SetBrush(g, mbInside ? EColor::Red : EColor::Yellow);
			mRenderer.drawCircle(g, mTestPos, 0.5);

			FixString<256> str;
			g.setTextColor( Color3ub(255, 255, 0) );
			g.drawText(Vector2(200, 50), str.format("Is Convex = %s", mbConvex ? "Yes" : "No"));

		}

		void restart()
		{
			updateTestPos(Vector2(0, 0));
		}


		void tick()
		{

		}

		void updateFrame(int frame)
		{

		}

		bool onMouse(MouseMsg const& msg)
		{
			if( !BaseClass::onMouse(msg) )
				return false;

			if( msg.onLeftDown() )
			{
				Vector2 wPos = mRenderer.convertToWorld(msg.getPos());
				if( InputManager::Get().isKeyDown(Keyboard::eCONTROL) )
				{
					updateTestPos(wPos);

				}
				else
				{
					mVertices.push_back(wPos);
					updateHull();
				}
			}
			else if( msg.onRightDown() )
			{
				if( !mVertices.empty() )
				{
					mVertices.pop_back();
					updateHull();
				}
			}
			return true;
		}

		void updateTestPos(Vector2 const& pos)
		{
			mTestPos = pos;
			if( mVertices.size() > 3 )
				mbInside = Geom2D::TestInSide(mVertices, mTestPos);
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			}
			return false;
		}

	protected:

	};


	struct PolyShape
	{
		int    numEdge;
		Vector2* normal;
		Vector2* vertex;
	};
	struct CircleShape
	{
		float radius;
	};


	static float const gRenderScale = 10.0f;

	class TestStage : public StageBase
	{
		typedef StageBase BaseClass;
	public:
		TestStage() {}


		enum Mode
		{
			MODE_POLY,
			MODE_CIRCLE,
		};

		Mode mMode;


		virtual bool onInit()
		{
			mMode = MODE_CIRCLE;
			mR = 10.0f;
			mPA = Vector2(10, 10);
			float const len1 = 10;
			mVA.push_back(Vector2(len1, 0));
			mVA.push_back(Vector2(0, len1));
			mVA.push_back(Vector2(-len1, 0));
			mVA.push_back(Vector2(0, -len1));

			mPB = Vector2(20, 20);
			float const len2 = 7;
			mVB.push_back(Vector2(len2, len2));
			mVB.push_back(Vector2(-len2, len2));
			mVB.push_back(Vector2(-len2, -len2));
			mVB.push_back(Vector2(len2, -len2));


			Geom2D::MinkowskiSum(mVA, mVB, mPoly);

			updateCollision();

			::Global::GUI().cleanupWidget();
			WidgetUtility::CreateDevFrame();
			restart();
			return true;
		}

		Vertices mPoly;

		void drawTest(Graphics2D& g)
		{
			FixString< 256 > str;
			Vec2i pos = Vector2(200, 200);
			for( int i = 0; i < mPoly.size(); ++i )
			{
				Vec2i p2 = pos + Vec2i(10 * mPoly[i]);
				g.drawLine(pos, p2);
				g.drawText(p2, str.format("%d", i));
			}
		}

		void updateCollision()
		{
			switch( mMode )
			{
			case MODE_POLY:
				mSAT.testIntersect(mPB, &mVB[0], mVB.size(), mPA, &mVA[0], mVA.size());
				break;
			case MODE_CIRCLE:
				mSAT.testIntersect(mPB, &mVB[0], mVB.size(), mPA, mR);
				break;
			}
		}

		void drawPolygon(Graphics2D& g, Vector2 const& pos, Vector2 v[], int num)
		{
			Vec2i buf[32];
			assert(num <= ARRAY_SIZE(buf));
			for( int i = 0; i < num; ++i )
			{
				buf[i] = Vec2i(gRenderScale * (pos + v[i]));
			}
			g.drawPolygon(buf, num);
		}

		virtual void onUpdate(long time)
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for( int i = 0; i < frame; ++i )
				tick();

			updateFrame(frame);
		}

		void onRender(float dFrame)
		{
			Graphics2D& g = Global::GetGraphics2D();

			RenderUtility::SetBrush(g, EColor::Gray);
			RenderUtility::SetPen(g, EColor::Gray);
			g.drawRect(Vec2i(0, 0), ::Global::GetDrawEngine().getScreenSize());

			if( mSAT.haveSA )
			{
				RenderUtility::SetBrush(g, EColor::White);
				RenderUtility::SetPen(g, EColor::Black);
			}
			else
			{
				RenderUtility::SetBrush(g, EColor::Red);
				RenderUtility::SetPen(g, EColor::Yellow);
			}

			switch( mMode )
			{
			case MODE_POLY:
				drawPolygon(g, mPA, &mVA[0], mVA.size());
				break;
			case MODE_CIRCLE:
				g.drawCircle(Vec2i(gRenderScale * mPA), int(mR * gRenderScale));
				break;
			}

			drawPolygon(g, mPB, &mVB[0], mVB.size());

			drawPolygon(g, Vector2(30, 30), &mPoly[0], mPoly.size());
			//drawTest( g );
			FixString< 64 > str;
			if( mSAT.haveSA )
			{
				str.format("No Collision ( dist = %f )", mSAT.fResult);
			}
			else
			{
				str.format("Collision ( depth = %f )", mSAT.fResult);
			}

			g.drawText(Vec2i(10, 10), str);
		}


		void restart()
		{

		}


		void tick()
		{

		}

		void updateFrame(int frame)
		{

		}

		bool onMouse(MouseMsg const& msg)
		{
			static Vec2i oldPos;
			if( msg.onLeftDown() )
			{
				oldPos = msg.getPos();
			}
			else if( msg.onMoving() )
			{
				if( msg.isLeftDown() )
				{
					Vec2i offset = msg.getPos() - oldPos;
					mPA += (1.0f / gRenderScale) * Vector2(offset);
					updateCollision();
					oldPos = msg.getPos();
				}
			}
			return false;
		}

		bool onKey(unsigned key, bool isDown)
		{
			if( !isDown )
				return false;

			switch( key )
			{
			case Keyboard::eR: restart(); break;
			case 'W': mMode = (mMode == MODE_CIRCLE) ? MODE_POLY : MODE_CIRCLE; break;
			case 'Q': updateCollision(); break;
			}
			return false;
		}
	protected:


		SATSolver mSAT;
		Vertices  mVA;
		float     mR;
		Vector2   mPA;
		Vector2   mPB;
		Vertices  mVB;

	};
}


REGISTER_STAGE("SAT Col Test", G2D::TestStage, EStageGroup::Test);
REGISTER_STAGE("QHull Test", G2D::QHullTestStage, EStageGroup::Test);
