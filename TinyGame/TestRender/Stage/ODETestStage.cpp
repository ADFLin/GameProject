#include "Stage/TestStageHeader.h"
#include <cstring>
#include <cmath>
#include "RHI/RHIGraphics2D.h"
#include "GameRenderSetup.h"
#include "ConsoleSystem.h"
#include "Renderer/RenderThread.h"

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
			int index = (mIndexStart + mPointCount) % mMaxPointCount;
			if (mPointCount < mMaxPointCount)
			{
				++mPointCount;
			}
			else
			{
				mIndexStart = (mIndexStart + 1) % mMaxPointCount;
				index = (mIndexStart + mPointCount - 1) % mMaxPointCount;
			}

			mPoints[index] = pos;
			mPoints[index + mMaxPointCount] = pos;
		}

		TArrayView< Vector2 > getPoints()
		{
			return TArrayView<Vector2>(mPoints.data() + mIndexStart, mPointCount);
		}

		int mIndexStart = 0;
		int mPointCount = 0;
		int mMaxPointCount = 3000;
		TArray< Vector2 > mPoints;
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

		template< class TExpr >
		TValues& operator += (TExpr const& rhs)
		{
			for (int i = 0; i < Dim; ++i)
				mValues[i] += rhs[i];

			return *this;
		}

		template< class TExpr >
		TValues& operator -= (TExpr const& rhs)
		{
			for (int i = 0; i < Dim; ++i)
				mValues[i] -= rhs[i];

			return *this;
		}

		TValues& operator *= (Scalar s)
		{
			for (int i = 0; i < Dim; ++i)
				mValues[i] *= s;

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


#define DEFINE_EXPR_OP( NAME , OP )\
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

		DEFINE_EXPR_OP(TExprAdd, +);
		DEFINE_EXPR_OP(TExprSub, -);
		DEFINE_EXPR_OP(TExprMul, *);
	}

#define ODE_STATE_ADD_FIELD(field) result.field = lhs.field; result.field += rhs.field;
#define ODE_STATE_SUB_FIELD(field) result.field = lhs.field; result.field -= rhs.field;
#define ODE_STATE_SCALE_FIELD(field) result.field = value.field; result.field *= s;
#define ODE_STATE_ADD_ASSIGN_FIELD(field) lhs.field += rhs.field;
#define ODE_STATE_SUB_ASSIGN_FIELD(field) lhs.field -= rhs.field;
#define ODE_STATE_SCALE_ASSIGN_FIELD(field) value.field *= s;

#define ODE_APPLY(macro, args) ODE_APPLY_IMPL(macro, args)
#define ODE_APPLY_IMPL(macro, args) macro args
#define ODE_SELECT_1_8(_1, _2, _3, _4, _5, _6, _7, _8, NAME, ...) NAME

#define ODE_FOR_EACH_1(M, F1) M(F1)
#define ODE_FOR_EACH_2(M, F1, F2) M(F1) M(F2)
#define ODE_FOR_EACH_3(M, F1, F2, F3) M(F1) M(F2) M(F3)
#define ODE_FOR_EACH_4(M, F1, F2, F3, F4) M(F1) M(F2) M(F3) M(F4)
#define ODE_FOR_EACH_5(M, F1, F2, F3, F4, F5) M(F1) M(F2) M(F3) M(F4) M(F5)
#define ODE_FOR_EACH_6(M, F1, F2, F3, F4, F5, F6) M(F1) M(F2) M(F3) M(F4) M(F5) M(F6)
#define ODE_FOR_EACH_7(M, F1, F2, F3, F4, F5, F6, F7) M(F1) M(F2) M(F3) M(F4) M(F5) M(F6) M(F7)
#define ODE_FOR_EACH_8(M, F1, F2, F3, F4, F5, F6, F7, F8) M(F1) M(F2) M(F3) M(F4) M(F5) M(F6) M(F7) M(F8)

#define ODE_FOR_EACH(M, ...) \
	ODE_APPLY(ODE_SELECT_1_8(__VA_ARGS__, ODE_FOR_EACH_8, ODE_FOR_EACH_7, ODE_FOR_EACH_6, ODE_FOR_EACH_5, ODE_FOR_EACH_4, ODE_FOR_EACH_3, ODE_FOR_EACH_2, ODE_FOR_EACH_1), (M, __VA_ARGS__))

#define STATE_OP(Self, ...) \
	friend Self operator+(Self const& lhs, Self const& rhs) \
	{ \
		Self result{}; \
		ODE_FOR_EACH(ODE_STATE_ADD_FIELD, __VA_ARGS__) \
		return result; \
	} \
	friend Self operator-(Self const& lhs, Self const& rhs) \
	{ \
		Self result{}; \
		ODE_FOR_EACH(ODE_STATE_SUB_FIELD, __VA_ARGS__) \
		return result; \
	} \
	friend Self operator*(Scalar s, Self const& value) \
	{ \
		Self result{}; \
		ODE_FOR_EACH(ODE_STATE_SCALE_FIELD, __VA_ARGS__) \
		return result; \
	} \
	friend Self operator*(Self const& value, Scalar s) \
	{ \
		return s * value; \
	} \
	friend Self& operator+=(Self& lhs, Self const& rhs) \
	{ \
		ODE_FOR_EACH(ODE_STATE_ADD_ASSIGN_FIELD, __VA_ARGS__) \
		return lhs; \
	} \
	friend Self& operator-=(Self& lhs, Self const& rhs) \
	{ \
		ODE_FOR_EACH(ODE_STATE_SUB_ASSIGN_FIELD, __VA_ARGS__) \
		return lhs; \
	} \
	friend Self& operator*=(Self& value, Scalar s) \
	{ \
		ODE_FOR_EACH(ODE_STATE_SCALE_ASSIGN_FIELD, __VA_ARGS__) \
		return value; \
	}


	struct RK4Method
	{
		template< typename TModel>
		static void Step(TModel const& model, typename TModel::State& state, Scalar t, Scalar dt)
		{
			using namespace Expr;
			auto k1 = model.evalDerivative(t, state);
			auto k2 = model.evalDerivative(t + 0.5 * dt, state + 0.5 * dt * k1);
			auto k3 = model.evalDerivative(t + 0.5 * dt, state + 0.5 * dt * k2);
			auto k4 = model.evalDerivative(t + dt, state + dt * k3);
			state = state + (dt / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
		}
	};

	struct ButcherRKMethod
	{
		template< typename TModel >
		static void Step(TModel const& model, typename TModel::State& state, Scalar t, Scalar dt)
		{
			using namespace Expr;
			auto k1 = model.evalDerivative(t, state);
			auto k2 = model.evalDerivative(t + (dt / 4.0), state + (dt / 4.0) * k1);
			auto k3 = model.evalDerivative(t + (dt / 4.0), state + (dt / 8.0) * k1 + (dt / 8.0) * k2);
			auto k4 = model.evalDerivative(t + (dt / 2.0), state - (dt / 2.0) * k2 + dt * k3);
			auto k5 = model.evalDerivative(t + (3.0 * dt / 4.0), state + (3.0 * dt / 16.0) * k1 + (9.0 * dt / 16.0) * k4);
			auto k6 = model.evalDerivative(t + dt, state - (3.0 * dt / 7.0) * k1 + (2.0 * dt / 7.0) * k2 + (12.0 * dt / 7.0) * k3 - (12.0 * dt / 7.0) * k4 + (8.0 * dt / 7.0) * k5);
			state = state + (dt / 90.0) * (7.0 * k1 + 32.0 * k3 + 12.0 * k4 + 32.0 * k5 + 7.0 * k6);
		}
	};

	struct EulerMethod
	{
		template< typename TModel >
		static void Step(TModel const& model, typename TModel::State& state, Scalar t, Scalar dt)
		{
			using namespace Expr;
			state = state + dt * model.evalDerivative(t, state);
		}
	};

	using V2 = TValues<2>;
	using V22 = TValues<2, V2>;

	struct Pendulum
	{
		Scalar L0 = 1.0;
		Scalar m = 1.0;
		Scalar g = 9.8;

		Pendulum()
		{
		}



		struct State
		{
			State() {}

			State(EForceInit)
			{
				x = Math::PI / 2;
				dx = 0;
			}

			Scalar x;
			Scalar dx;

			STATE_OP(State, x, dx);
		};

		State evalDerivative(Scalar t, State const& s) const
		{
			State state;
			state.x = s.dx;
			state.dx = -g / L0 * sin(s.x);
			return state;
		}

		Vector2 getTracePos(State const& s) const
		{
			Vector2 p1 = L0 * Vector2(sin(s.x), cos(s.x));
			return p1;
		}
	};

	struct ElasticPendulum
	{
		Scalar L0 = 1.0;
		Scalar m = 1.0;
		Scalar k = 20.0;
		Scalar g = 9.8;

		ElasticPendulum()
		{
		}


		struct State
		{
			State() {}

			State(EForceInit)
			{
				x = { 0.5 , Math::PI / 2 };
				dx = { 0, 0 };
			}
			V2 x;
			V2 dx;

			STATE_OP(State, x, dx);
		};

		State evalDerivative(Scalar t, State const& s) const
		{
			Scalar r = s.x[0];
			Scalar dr = s.dx[0];
			Scalar theta = s.x[1];
			Scalar w = s.dx[1];

			State state;
			state.x = s.dx;
			state.dx[0] = (L0 + r) * w * w - k / m * r + g * cos(theta);
			state.dx[1] = -g / (L0 + r) * sin(theta) - 2 * dr / (L0 + r) * w;
			return state;
		}

		Vector2 getTracePos(State const& s) const
		{
			Vector2 p1 = (L0 + s.x[0]) * Vector2(sin(s.x[1]), cos(s.x[1]));
			return p1;
		}
	};


	struct DoublePendulum
	{
		Scalar l1;
		Scalar l2;
		Scalar m1;
		Scalar m2;

		Scalar g = 9.8;
		DoublePendulum()
		{
			l1 = 1;
			l2 = 1;
			m1 = 1;
			m2 = 1;
		}

		struct State
		{
			State() {}

			State(EForceInit)
			{
				theta = { Math::PI / 2 , 2 * Math::PI / 4 };
				w = { 0 , 0 };
			}


			V2 theta;
			V2 w;

			STATE_OP(State, theta, w);
		};

		State evalDerivative(Scalar t, State const& s) const
		{
			V2 theta = s.theta;
			V2 w = s.w;
			Scalar l21 = l2 / l1;
			Scalar l12 = l1 / l2;
			Scalar m = m1 + m2;
			Scalar delta = theta[0] - theta[1];
			Scalar cd = cos(delta);
			Scalar sd = sin(delta);

			Scalar f1 = -l21 * (m1 / m) * (w[1] * w[1]) * sd - (g / l1) * sin(theta[0]);
			Scalar f2 = l12 * (w[0] * w[0]) * sd - (g / l2) * sin(theta[1]);
			Scalar a1 = l21 * (m2 / m) * cd;
			Scalar a2 = l12 * cd;

			State state;
			state.theta = s.w;
			state.w[0] = (f1 - a1 * f2) / (1 - a1 * a2);
			state.w[1] = (f2 - a2 * f1) / (1 - a1 * a2);
			return state;
		}


		Scalar calcEnergy(State const& s) const
		{
			Scalar result = 0;
			Scalar v1 = l1 * s.w[0];
			Scalar v2 = l2 * s.w[1];
			result += 0.5 * (m1 + m2)* v1 * v1;
			result += 0.5 * m2 * v2 * v2;
			result += m2 * l1 * l2 * s.w[0] * s.w[1] * cos(s.theta[0] - s.theta[1]);
			Scalar h1 = l1 * (1 - cos(s.theta[0]));
			result += m1 * g * h1;
			result += m2 * g * (h1 + l2 * (1 - cos(s.theta[1])));

			return result;
		}


		Vector2 getTracePos(State const& s) const
		{
			Vector2 p1 = l1 * Vector2(sin(s.theta[0]), cos(s.theta[0]));
			Vector2 p2 = p1 + l2 * Vector2(sin(s.theta[1]), cos(s.theta[1]));
			return p2;
		}
	};

	template <typename TModel>
	struct TSimulation
	{
		TModel model;
		typename TModel::State state{ ForceInit };
		Scalar time = 0.0;


		template <typename TMethod>
		void step(Scalar dt)
		{
			TMethod::Step(model, state, time, dt);
			time += dt;
		}
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

			IConsoleSystem::Get().registerCommand("ODE.UpdateCurve", &TestStage::updateCurve, this);

			auto frame = ::WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Show Curve", bShowCurve);
			frame->addCheckBox("Show Trace", bShowTrace);

			initSimModel();
			updateCurve();
			restart();
			return true;
		}

		void onEnd() override
		{
			IConsoleSystem::Get().unregisterAllCommandsByObject(this);
			BaseClass::onEnd();
		}

		void restart()
		{

		}

		template <typename TMethod>
		void updateCurve(TArray<Vector2>& curve, Scalar h, int numStep = 500)
		{
			struct Model
			{
				using State = V2;
				State evalDerivative(Scalar t, State const& s) const
				{
					return { s[1] , -s[0] };
				}
			};

			curve.resize(numStep + 1);

			Scalar t = 0;

			Model model;
			Model::State state = { 0 , 1 };
			curve[0] = Vector2(t, state[0]);
			for (int i = 1; i <= numStep; ++i)
			{
				TMethod::Step(model, state, t, h);
				t += h;
				curve[i] = Vector2(t, state[0]);
			}
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

			updateCurve<EulerMethod>(mCurveEuler, h, numStep);
			updateCurve<ButcherRKMethod>(mCurveRK4, h, numStep);
		}


		Scalar E0;
		Scalar accTime = 0;

		class ITrajectoryProducer
		{
		public:
			virtual void    update(int num, Scalar dt) = 0;
			virtual Vector2 getPos() = 0;
		};

		struct SimModelTrace
		{
			std::unique_ptr<ITrajectoryProducer> modelProducer;
			Trajectory trace;
			Color3f colorTrace;
			Color3f colorModel;
			std::function< void(RHIGraphics2D&, SimModelTrace&) > drawFunc;
			void update(int num, Scalar dt)
			{
				modelProducer->update(num, dt);
				trace.addPoint(modelProducer->getPos());
			}

			void drawModel(RHIGraphics2D& g)
			{
				if (drawFunc)
				{
					drawFunc(g, *this);
				}
			}

			template< typename TModel >
			class TModelProducer : public ITrajectoryProducer
			                     , public TSimulation<TModel>
			{
			public:
				Vector2 getPos()
				{
					return model.getTracePos(state);
				}

			};

			template< typename TModel, typename TMethod >
			TSimulation<TModel>& buildSimlution()
			{
				class ModelProducer : public TModelProducer< TModel >
				{
				public:
					void  update(int num, Scalar dt)
					{
						for (int i = num; i; --i)
						{
							TSimulation<TModel>::step<TMethod>(dt);
						}
					}
				};
				ModelProducer* producer = new ModelProducer;
				modelProducer.reset(producer);
				drawFunc = [](RHIGraphics2D& g, SimModelTrace& modelTrace)
				{
					Draw(g, modelTrace.getModel<TModel>(), modelTrace.getState<TModel>(), modelTrace.colorModel);
				};
				return *producer;
			}

			template< typename TModel >
			TModel& getModel()
			{
				return static_cast<TModelProducer<TModel>*>(modelProducer.get())->model;
			}

			template< typename TModel >
			auto const& getState()
			{
				return static_cast<TModelProducer<TModel>*>(modelProducer.get())->state;
			}
		};

		static void Draw(RHIGraphics2D& g, Pendulum& model, Pendulum::State const& state, Color3f const& color)
		{
			Vector2 p1 = model.L0 * Vector2(sin(state.x), cos(state.x));
			g.setPen(color, 3);
			g.drawLine(Vector2(0, 0), p1);
		}
		static void Draw(RHIGraphics2D& g, DoublePendulum& model, DoublePendulum::State const& state, Color3f const& color)
		{
			Vector2 p1 = model.l1 * Vector2(sin(state.theta[0]), cos(state.theta[0]));
			Vector2 p2 = p1 + model.l2 * Vector2(sin(state.theta[1]), cos(state.theta[1]));

			g.setPen(color, 3);
			g.drawLine(Vector2(0, 0), p1);
			g.drawLine(p1, p2);
		}
		static void Draw(RHIGraphics2D& g, ElasticPendulum& model, ElasticPendulum::State const& state, Color3f const& color)
		{
			Vector2 p1 = (model.L0 + state.x[0]) * Vector2(sin(state.x[1]), cos(state.x[1]));
			g.setPen(color, 3);
			g.drawLine(Vector2(0, 0), p1);
		}

		TArray<SimModelTrace> mModelTraces;

		template< typename TModel, typename TMethod >
		TSimulation<TModel>& addSimlution(Color3f const& colorTrace, Color3f const& colorModel)
		{
			SimModelTrace modelTrace;
			modelTrace.colorTrace = colorTrace;
			modelTrace.colorModel = colorModel;
			auto& model = modelTrace.buildSimlution<TModel, TMethod>();
			mModelTraces.push_back(std::move(modelTrace));
			return model;
		}

		void initSimModel()
		{
			using Method = ButcherRKMethod;
			auto& simlution = addSimlution<DoublePendulum, Method>(Color3f(1, 0, 0), Color3f(0.5, 0, 0));
			E0 = simlution.model.calcEnergy(simlution.state);
			addSimlution<Pendulum, Method>(Color3f(0, 0, 1), Color3f(0, 0, 0.5));
			addSimlution<ElasticPendulum, Method>(Color3f(0, 1, 0), Color3f(0, 0.5, 0));
		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);

			Scalar dt = 0.001;
			accTime += deltaTime;
			if (accTime > dt)
			{
				int num = Math::FloorToInt(accTime / dt);
				for (auto& model : mModelTraces)
				{
					model.update(num, dt);
				}
				accTime -= num * dt;
			}
		}

		bool bShowCurve = false;
		bool bShowTrace = true;


		TArray< Vector2 > mCurve;
		TArray< Vector2 > mCurveEuler;
		TArray< Vector2 > mCurveRK4;

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			using namespace Render;

			Vec2i screenSize = ::Global::GetScreenSize();
			RenderThread::AddCommand("InitState", [screenSize]
			{
				RHICommandList& commandList = RHICommandList::GetImmediateList();
				RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);
				RHISetFrameBuffer(commandList, nullptr);
				RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0.8, 0.8, 0.8, 0), 1);
			});

			g.beginRender();


			g.pushXForm();

			g.translateXForm(screenSize.x / 2, 200);
			g.scaleXForm(160, 160);

			if (bShowTrace)
			{
				for (auto& modelTrace : mModelTraces)
				{
					g.setPen(modelTrace.colorTrace, 1);
					auto points = modelTrace.trace.getPoints();
					if (points.size() >= 2)
					{
						g.drawLineStrip(points.data(), points.size());
					}
				}
			}
			for (auto& modelTrace : mModelTraces)
			{
				modelTrace.drawModel(g);
			}
			g.popXForm();
	
			if (bShowCurve)
			{
				g.pushXForm();
				g.translateXForm(10, screenSize.y * 0.5);
				g.scaleXForm(2.5, 20);

				g.setPen(Color3f(1, 0, 0), 5);
				g.drawLineStrip(mCurve.data(), mCurve.size());

				g.setPen(Color3f(0, 0, 1), 1);
				g.drawLineStrip(mCurveEuler.data(), mCurveEuler.size());

				g.setPen(Color3f(0, 1, 1), 1);
				g.drawLineStrip(mCurveRK4.data(), mCurveRK4.size());

				g.popXForm();
			}

			{
				auto& DP = mModelTraces[0].getModel<DoublePendulum>();
				auto const& state = mModelTraces[0].getState<DoublePendulum>();
				g.drawTextF(Vector2(100, 100), "E / E0 = %lf", DP.calcEnergy(state) / E0);
			}

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

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.bUseRenderThread = true;
		}

	protected:
	};


	BITWISE_RELLOCATABLE_FAIL(TestStage::SimModelTrace);
	REGISTER_STAGE_ENTRY("ODE Test", TestStage, EExecGroup::PhyDev, "Physics|Math");

} // end namespace Solver
