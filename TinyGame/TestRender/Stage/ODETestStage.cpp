#include "Stage/TestStageHeader.h"
#include <cstring>
#include <cmath>
#include "RHI/RHIGraphics2D.h"
#include "GameRenderSetup.h"
#include "ConsoleSystem.h"

namespace ODE
{
	using Scalar = double;

	class Trajectory
	{
	public:

		Trajectory()
		{
			mPoints.resize(2 * mMaxPointCount);
		}

		void addPoint(Vector2 const& pos)
		{
			if (mPointCount < mMaxPointCount)
			{
				CHECK(mIndexStart == 0);
				mPoints[mPointCount] = pos;
				++mPointCount;
			}
			else
			{
				int indexNext = mIndexStart + mPointCount;
				if (indexNext == 2 * mMaxPointCount)
				{
					std::copy(mPoints.data() + mIndexStart + 1, mPoints.data() + indexNext, mPoints.data());

					indexNext = mMaxPointCount - 1;
					mIndexStart = 0;
				}
				else
				{
					mIndexStart += 1;
				}

				mPoints[indexNext] = pos;
			}
		}

		TArrayView< Vector2 > getPoints() { return TArrayView<Vector2>(mPoints.data() + mIndexStart, mPointCount); }

		int mIndexStart = 0;
		int mPointCount = 0;
		int mMaxPointCount = 3000;
		std::vector< Vector2 > mPoints;
	};

	template< int Dim, typename Type = Scalar >
	struct TValues
	{
		using ScalarType = Type;

		TValues() = default;

		TValues( std::initializer_list<Type > l)
		{
			for (int i = 0; i < Dim; ++i)
				mValues[i] = l.begin()[i];
		}

		template< class TExpr >
		TValues(TExpr&& rhs)
		{
			for (int i = 0; i < Dim; ++i)
				mValues[i] = rhs[i];
		}

		TValues& operator = (TValues const& rhs)
		{
			for (int i = 0; i < Dim; ++i)
				mValues[i] = rhs.mValues[i];

			return *this;
		}


		template< class TExpr >
		TValues& operator = (TExpr&& rhs)
		{
			for (int i = 0; i < Dim; ++i)
				mValues[i] = rhs[i];

			return *this;
		}
		ScalarType  operator[](int index) const { return mValues[index]; }
		ScalarType& operator[](int index) { return mValues[index]; }

		operator ScalarType*() { return mValues; }

		ScalarType mValues[Dim];
	};

	template< typename T >
	struct TExprAccess
	{


	};

	namespace Expr
	{
		template< typename Type >
		struct TExprScalar
		{
			TExprScalar(Type v) :value(v) {}

			auto operator[](int index) const { return value; }
			Type value;
		};


		template< class T >
		struct ExprTraits
		{

		};

		template< int Dim, typename Type >
		struct ExprTraits < TValues<Dim, Type > >
		{
			using RefType = TValues<Dim, Type >;
		};

		template< typename Type >
		struct ExprTraits < TExprScalar< Type > >
		{
			using RefType = TExprScalar< Type >;
		};


#define DFINE_EXPR_OP( NAME , OP )\
		template < typename LV , typename RV >\
		struct NAME\
		{\
			NAME(LV const& lhs, RV const& rhs)\
				:lhs(lhs),rhs(rhs){}\
			auto operator[](int index) const { return lhs[index] OP rhs[index]; }\
			typename ExprTraits< LV >::RefType lhs;\
			typename ExprTraits< RV >::RefType rhs;\
		};\
		template< typename LV , typename RV >\
		NAME< LV, RV > operator OP (LV const& lhs, RV const& rhs)\
		{\
			return NAME<LV, RV>(lhs, rhs);\
		}\
		template< typename RV >\
		NAME< TExprScalar<Scalar>, RV > operator OP (Scalar lhs, RV const& rhs)\
		{\
			return NAME<TExprScalar<Scalar>, RV>(lhs, rhs);\
		}\
		template< typename LV >\
		NAME< LV , TExprScalar<Scalar> > operator OP (LV const& lhs, Scalar rhs)\
		{\
			return NAME<LV , TExprScalar<Scalar>>(lhs, rhs); \
		}\
		template< typename LV, typename RV  >\
		struct ExprTraits< NAME< LV , RV > >\
		{\
			using RefType = NAME< LV , RV >;\
		};

		DFINE_EXPR_OP(TExprAdd, +);
		DFINE_EXPR_OP(TExprSub, -);
		DFINE_EXPR_OP(TExprMul, *);
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

			ConsoleSystem::Get().registerCommand("ODE.UpdateCurve", &TestStage::updateCurve, this);
			updateCurve();
			restart();
			return true;
		}

		template< typename TFy, typename T >
		static void RK4Step(TFy&& Fy, Scalar& x0, T& y0, Scalar h)
		{
			using namespace Expr;

			T ky1 = Fy(x0, y0);
			Scalar x1 = x0 + 0.5 * h;
			T y1 = y0 + (0.5 * h) * ky1;

			T  ky2 = Fy(x1, y1);
			Scalar x2 = x0 + 0.5 * h;
			T  y2 = y0 + (0.5 * h) * ky2;

			T  ky3 = Fy(x2, y2);
			Scalar x3 = x0 + h;
			T  y3 = y0 + h * ky3;

			T  ky4 = Fy(x3, y3);
			Scalar x = x0 + h;
			T  y = y0 + (h / 6.0) * (ky1 + 2.0 * ky2 + 2.0 * ky3 + ky4);

			x0 = x;
			y0 = y;
		}

		void updateCurve(int numStep = 500)
		{
			Scalar xA = 0;
			Scalar xB = 100 * Math::PI;
			mCurve.resize(numStep + 1);
			Scalar h = (xB - xA) / numStep;
			for (int i = 0; i <= numStep; ++i)
			{
				Scalar x = xA + i * h;
				mCurve[i] = Vector2(x, Math::Sin(x));
			}


			auto Fy = [](Scalar x, Scalar y, Scalar dy)
			{
				return dy;
			};

			auto Fdy = [](Scalar x, Scalar y, Scalar dy)
			{
				return -y;
			};


			{
				mCurveEular.resize(numStep + 1);

				Scalar x0 = 0;
				Scalar y0 = 0;
				Scalar dy0 = 1;

				mCurveEular[0] = Vector2(x0, y0);
				for (int i = 1; i <= numStep; ++i)
				{
					Scalar x = xA + i * h;
					Scalar dy = dy0 + h * Fdy(x0, y0, dy0);
					Scalar y = y0  + h * Fy(x0, y0, dy0);
					mCurveEular[i] = Vector2(x, y);

					x0 = x;
					y0 = y;
					dy0 = dy;
				}
			}

#if 1
			{
				auto Fy = [](Scalar x, V2 y)
				{
					return V2{ y[1] , -y[0] };
				};

				mCurveRK4.resize(numStep + 1);

				Scalar x0 = 0;
				V2 y0 = { 0 , 1 };

				mCurveRK4[0] = Vector2(x0, y0[0]);
				for (int i = 1; i <= numStep; ++i)
				{
					RK4Step(Fy, x0, y0, h);
					mCurveRK4[i] = Vector2(x0, y0[0]);
				}
			}
#endif
		}

		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			E0 = mDP.calcEnergy();
		}



		using V2 = TValues<2>;
		using V22 = TValues<2,V2>;

		struct ElasticPendulum
		{
			Scalar L0 = 1.0;
			Scalar m = 1.0;
			Scalar k = 20.0;
			V2 x;
			V2 dx;
			Scalar t;
			Scalar g = 9.8;

			ElasticPendulum()
			{
				t = 0;
				x = { 0.5 , Math::PI / 2 };
				dx = { 0, 0 };
			}
			void simulate(Scalar dt)
			{
				auto Fy = [&](Scalar x, V22 y)
				{
					Scalar r = y[0][0];
					Scalar dr = y[1][0];
					Scalar theta = y[0][1];
					Scalar w = y[1][1];

					V22 out;
					out[0] = y[1];
					out[1][0] = (L0 + r) * w * w - k / m * r + g * cos(theta);
					out[1][1] = -g / (L0 + r) * sin(theta) - 2 * dr / (L0 + r) * w;
					return out;
				};

				V22 y;
				y[0] = x;
				y[1] = dx;
				RK4Step(Fy, t, y, dt);
				x = y[0];
				dx = y[1];
			}

		};


		struct DoublePendulum
		{
			Scalar l1;
			Scalar l2;
			Scalar m1;
			Scalar m2;

			V2     theta;
			V2     w;
			Scalar t;

			Scalar g = 9.8;
			DoublePendulum()
			{
				l1 = 1;
				l2 = 1;
				m1 = 1;
				m2 = 1;

				theta = { Math::PI / 2 , 2 * Math::PI / 4 };
				w = { 0 , 0 };
				t = 0;
			}

			Scalar calcEnergy() const
			{
				Scalar result = 0;
				Scalar v1 = l1 * w[0];
				Scalar v2 = l2 * w[1];
				result += 0.5 * ( m1 + m2 )* v1 * v1;
				result += 0.5 * m2 * v2 * v2;
				result += m2 * l1 * l2 * w[0] * w[1] * cos(theta[0] - theta[1]);
				Scalar h1 = l1 * ( 1 - cos(theta[0]));
				result += m1 * g * h1;
				result += m2 * g * ( h1 + l2 * (1 - cos(theta[1])) );

				return result;
			}


			void simulate(Scalar dt)
			{
				auto Fy = [&](Scalar x, V22 y)
				{
					V2 theta = y[0];
					V2 w = y[1];
					Scalar l21 = l2 / l1;
					Scalar l12 = l1 / l2;
					Scalar m = m1 + m2;
					Scalar delta = theta[0] - theta[1];
					Scalar c = cos(delta);
					Scalar s = sin(delta);

					Scalar f1 = -l21 * (m1 / m) * (w[1] * w[1]) * s - (g / l1) * sin(theta[0]);
					Scalar f2 = l12 * (w[0] * w[0]) * s - (g / l2) * sin(theta[1]);
					Scalar a1 = l21 * (m2 / m) * c;
					Scalar a2 = l12 * c;
					V22 out;
					out[0] = w;
					out[1][0] = (f1 - a1 * f2) / (1 - a1 * a2);
					out[1][1] = (f2 - a2 * f1) / (1 - a1 * a2);
					return out;
				};

				V22 y;
				y[0] = theta;
				y[1] = w;
				RK4Step(Fy, t, y, dt);
				theta = y[0];
				w = y[1];
			}

		};


		DoublePendulum mDP;
		ElasticPendulum mEP;
		Scalar E0;
		Trajectory mTrace;
		Trajectory mTrace2;
		Scalar accTime = 0;

		void tick() 
		{
			Scalar dt = 0.001;

			accTime += gDefaultTickTime / 1000.0;
			if ( accTime > dt )
			{
				do
				{
					mDP.simulate(dt);
					mEP.simulate(dt);
					accTime -= dt;
				}
				while (accTime > dt);
				{
					Vector2 p1 = mDP.l1 * Vector2(sin(mDP.theta[0]), cos(mDP.theta[0]));
					Vector2 p2 = p1 + mDP.l2 * Vector2(sin(mDP.theta[1]), cos(mDP.theta[1]));
					mTrace.addPoint(p2);
				}
				{
					Vector2 p1 = (mEP.L0 + mEP.x[0]) * Vector2(sin(mEP.x[1]), cos(mEP.x[1]));
					mTrace2.addPoint(p1);
				}
			}
		}
		void updateFrame(int frame) {}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			int frame = time / gDefaultTickTime;
			for (int i = 0; i < frame; ++i)
				tick();

			updateFrame(frame);
		}


		std::vector< Vector2 > mCurve;
		std::vector< Vector2 > mCurveEular;
		std::vector< Vector2 > mCurveRK4;

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			using namespace Render;

			Vec2i screenSize = ::Global::GetScreenSize();
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);

			g.beginRender();


			g.pushXForm();

			g.translateXForm(screenSize.x / 2, 200);
			g.scaleXForm(160, 160);
			{
				g.setPen(Color3f(1, 0, 0), 1);
				auto points = mTrace.getPoints();
				if (points.size() >= 2)
				{
					g.drawLineStrip(points.data(), points.size());
				}
			}
			{
				g.setPen(Color3f(0, 1, 0), 1);
				auto points = mTrace2.getPoints();
				if (points.size() >= 2)
				{
					g.drawLineStrip(points.data(), points.size());
				}
			}
			{
				Vector2 p1 = mDP.l1 * Vector2(sin(mDP.theta[0]), cos(mDP.theta[0]));
				Vector2 p2 = p1 + mDP.l2 * Vector2(sin(mDP.theta[1]), cos(mDP.theta[1]));

				g.setPen(Color3f(1, 0, 1), 3);
				g.drawLine(Vector2(0, 0), p1);
				g.drawLine(p1, p2);
			}
			{
				Vector2 p1 = ( mEP.L0 + mEP.x[0] ) * Vector2(sin(mEP.x[1]), cos(mEP.x[1]));
				g.setPen(Color3f(1, 1, 0), 3);
				g.drawLine(Vector2(0, 0), p1);
			}

			g.popXForm();
	
#if 0
			g.pushXForm();
			g.translateXForm(10 , screenSize.y * 0.5);
			g.scaleXForm(2.5, 20);

			g.setPen(Color3f(1, 0, 0), 5);
			g.drawLineStrip(mCurve.data(), mCurve.size());

			g.setPen(Color3f(0, 0, 1), 1);
			g.drawLineStrip(mCurveEular.data(), mCurveEular.size());

			g.setPen(Color3f(0, 1, 1), 1);
			g.drawLineStrip(mCurveRK4.data(), mCurveRK4.size());

			g.popXForm();
#endif
			//Vector2 pos[] = { Vector2(0,0) , Vector2(100,100), Vector2(200,0) };
			//g.drawLineStrip(pos, ARRAY_SIZE(pos));

			g.drawText(Vector2(100, 100), InlineString<>::Make("E / E0 = %lf" , mDP.calcEnergy() / E0 ));

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

	REGISTER_STAGE_ENTRY("ODE Test", TestStage, EExecGroup::PhyDev, "Physics|Math");

} // end namespace Solver
