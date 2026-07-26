#include "Stage/TestStageHeader.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>
#include "GameRenderSetup.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHICommand.h"

namespace CircleFit
{
	using namespace Render;

	struct FittedCircle
	{
		Vector2 centerInit = Vector2::Zero();
		Vector2 center = Vector2::Zero();
		float radius = 0.0f;
		float error = 0.0f;
		bool bValid = false;
	};

	static float CalcRadiusAndError(TArray<Vector2> const& points, Vector2 const& center, float& outRadius)
	{
		if (points.empty())
		{
			outRadius = 0.0f;
			return 0.0f;
		}

		float sumDist = 0.0f;
		for (Vector2 const& point : points)
		{
			sumDist += std::sqrt((point - center).length2());
		}

		outRadius = sumDist / float(points.size());

		float error = 0.0f;
		for (Vector2 const& point : points)
		{
			float diff = std::sqrt((point - center).length2()) - outRadius;
			error += diff * diff;
		}

		return error;
	}

	static bool Solve3x3(double a[3][4], double result[3])
	{
		for (int col = 0; col < 3; ++col)
		{
			int pivot = col;
			for (int row = col + 1; row < 3; ++row)
			{
				if (std::abs(a[row][col]) > std::abs(a[pivot][col]))
					pivot = row;
			}

			if (std::abs(a[pivot][col]) < 1e-12)
				return false;

			if (pivot != col)
			{
				for (int k = col; k < 4; ++k)
					std::swap(a[pivot][k], a[col][k]);
			}

			double inv = 1.0 / a[col][col];
			for (int k = col; k < 4; ++k)
				a[col][k] *= inv;

			for (int row = 0; row < 3; ++row)
			{
				if (row == col)
					continue;

				double factor = a[row][col];
				for (int k = col; k < 4; ++k)
					a[row][k] -= factor * a[col][k];
			}
		}

		for (int i = 0; i < 3; ++i)
			result[i] = a[i][3];

		return true;
	}
	static Vector2 CalcInitialCenter(TArray<Vector2> const& points)
	{
		Vector2 mean = Vector2::Zero();
		for (Vector2 const& p : points)
			mean += p;
		mean /= float(points.size());

		if (points.size() < 3)
			return mean;

		float scale = 0.0f;
		for (Vector2 const& p : points)
		{
			scale = Math::Max(scale, Math::Distance(p, mean));
		}

		if (scale < 1e-4f)
			return mean;

		double normal[3][4] = {};

		//ax + by + c = -(x^2 + y^2) 
		for (Vector2 const& p : points)
		{
			double x = (p.x - mean.x) / scale;
			double y = (p.y - mean.y) / scale;
			double b = -(x * x + y * y);
			double row[3] = { x, y, 1.0 };

			for (int r = 0; r < 3; ++r)
			{
				for (int c = 0; c < 3; ++c)
					normal[r][c] += row[r] * row[c];

				normal[r][3] += row[r] * b;
			}
		}

		double result[3];
		if (Solve3x3(normal, result))
		{
			Vector2 normalizedCenter;
			normalizedCenter.x = float(-0.5 * result[0]);
			normalizedCenter.y = float(-0.5 * result[1]);

			return mean + scale * normalizedCenter;
		}

		return mean;
	}

	static Vector2 CalcCircleFitGradient(TArray<Vector2> const& points, Vector2 const& center, float radius)
	{
		Vector2 grad = Vector2::Zero();
		if (points.empty())
			return grad;

		for (Vector2 const& point : points)
		{
			Vector2 offset = center - point;
			float dist = std::sqrt(offset.length2());
			if (dist < 1e-4f)
				continue;

			float residual = dist - radius;
			grad += (2.0f * residual / dist) * offset;
		}

		grad /= float(points.size());
		return grad;
	}

	struct AdamOptimizer2D
	{
		Vector2 m = Vector2::Zero();
		Vector2 v = Vector2::Zero();
		float lr = 1.0f;
		float beta1 = 0.9f;
		float beta2 = 0.999f;
		float eps = 1e-8f;
		int stepCount = 0;

		Vector2 step(Vector2 const& grad)
		{
			++stepCount;

			m = beta1 * m + (1.0f - beta1) * grad;
			v.x = beta2 * v.x + (1.0f - beta2) * grad.x * grad.x;
			v.y = beta2 * v.y + (1.0f - beta2) * grad.y * grad.y;

			float bias1 = 1.0f - std::pow(beta1, float(stepCount));
			float bias2 = 1.0f - std::pow(beta2, float(stepCount));
			Vector2 mHat = m / bias1;
			Vector2 vHat(v.x / bias2, v.y / bias2);

			return Vector2(
				lr * mHat.x / (std::sqrt(vHat.x) + eps),
				lr * mHat.y / (std::sqrt(vHat.y) + eps));
		}
	};

	static float CalcSearchScale(TArray<Vector2> const& points, Vector2 const& center)
	{
		float result = 1.0f;
		for (Vector2 const& point : points)
		{
			result = Math::Max(result, std::sqrt((point - center).length2()));
		}
		return result;
	}

	static void RefineByDirectionSearch(TArray<Vector2> const& points, FittedCircle& result, float step)
	{
		Vector2 const dirs[] =
		{
			Vector2( 1,  0), Vector2(-1,  0), Vector2( 0,  1), Vector2( 0, -1),
			Vector2( 1,  1), Vector2( 1, -1), Vector2(-1,  1), Vector2(-1, -1),
		};

		while (step > 1e-4f)
		{
			bool bImproved = false;
			for (Vector2 dir : dirs)
			{
				float dirLen = std::sqrt(dir.length2());
				dir /= dirLen;
				Vector2 testCenter = result.center + step * dir;

				float testRadius;
				float testError = CalcRadiusAndError(points, testCenter, testRadius);
				if (testError < result.error)
				{
					result.center = testCenter;
					result.radius = testRadius;
					result.error = testError;
					bImproved = true;
				}
			}

			if (!bImproved)
				step *= 0.5f;
		}

		result.error = CalcRadiusAndError(points, result.center, result.radius);
	}

	static void RefineByAdam(TArray<Vector2> const& points, FittedCircle& result, float scale)
	{
		AdamOptimizer2D optimizer;
		optimizer.lr = Math::Max(0.05f, 0.02f * scale);

		for (int iter = 0; iter < 256; ++iter)
		{
			float radius;
			float error = CalcRadiusAndError(points, result.center, radius);
			Vector2 grad = CalcCircleFitGradient(points, result.center, radius);
			if (grad.length2() < 1e-10f)
				break;

			Vector2 update = optimizer.step(grad);
			float updateLen = std::sqrt(update.length2());
			float maxUpdateLen = Math::Max(1.0f, 0.25f * scale);
			if (updateLen > maxUpdateLen)
				update *= maxUpdateLen / updateLen;

			Vector2 testCenter = result.center - update;

			float testRadius;
			float testError = CalcRadiusAndError(points, testCenter, testRadius);
			if (testError < result.error)
			{
				result.center = testCenter;
				result.radius = testRadius;
				result.error = testError;

				if (Math::Abs(error - testError) < 1e-5f)
					break;
			}
			else
			{
				optimizer.lr *= 0.5f;
				if (optimizer.lr < 1e-4f)
					break;
			}
		}
	}

	static FittedCircle FitGeometricL2(TArray<Vector2> const& points)
	{
		FittedCircle result;
		if (points.empty())
			return result;

		result.center = CalcInitialCenter(points);
		result.centerInit = result.center;
		result.error = CalcRadiusAndError(points, result.center, result.radius);
		result.bValid = true;

		if (points.size() < 3)
			return result;

		float scale = CalcSearchScale(points, result.center);
		RefineByAdam(points, result, scale);
		RefineByDirectionSearch(points, result, Math::Max(1.0f, 0.1f * scale));
		return result;
	}

	static float CalcCircleFitSimilarity(TArray<Vector2> const& points, FittedCircle const& circle)
	{
		if (!circle.bValid || points.size() < 4 || circle.radius < 1e-4f)
			return 0.0f;

		TArray<float> residuals;
		residuals.reserve(points.size());

		float sumSq = 0.0f;
		for (Vector2 const& point : points)
		{
			Vector2 offset = point - circle.center;
			float dist = std::sqrt(offset.length2());
			float residual = Math::Abs(dist - circle.radius);
			residuals.push_back(residual);
			sumSq += residual * residual;
		}

		float rmse = std::sqrt(sumSq / float(points.size()));

		std::sort(residuals.begin(), residuals.end());
		float medianError = residuals[residuals.size() / 2];
		float radialError = 0.35f * rmse + 0.65f * medianError;
		float baseTolerance = Math::Max(6.0f, 0.05f * circle.radius);
		float radiusDifficultyScale = Math::Clamp(std::sqrt(circle.radius / 120.0f), 1.0f, 2.0f);
		float tolerance = baseTolerance * radiusDifficultyScale;
		float radialScore = std::exp(-Math::Square(radialError / tolerance));

		return 100.0f * radialScore;
	}

	class Stage : public StageBase , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();
			mRenderer.mScale = 1.0f;
			mRenderer.mOffset = Vector2::Zero();

			DevFrame* frame = WidgetUtility::CreateDevFrame();
			frame->addCheckBox("Show Circle", bShowCircle);
			frame->addCheckBox("Show LineStrip", bShowLineStrip);
			frame->addCheckBox("Show Points", bShowPoints);
			frame->addButton("Restart", [this]()
			{
				restart();
			});

			restart();
			return true;
		}

		void restart()
		{
			mPoints.clear();
			mFit = FittedCircle();
			mLastPoint = Vector2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();

			auto& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			g.beginRender();


			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::Black);
			//g.drawRect(Vec2i(0, 0), Global::GetScreenSize());


			if (bShowCircle && mFit.bValid)
			{
				g.setPen(RenderUtility::GetColor(EColor::Yellow), 2);
				RenderUtility::SetBrush(g, EColor::Null);
				mRenderer.drawCircle(g, mFit.center, mFit.radius);

				g.setPen(RenderUtility::GetColor(EColor::Red), 1);
				RenderUtility::SetBrush(g, EColor::Red);
				mRenderer.drawCircle(g, mFit.center, 4.0f);

				g.setPen(RenderUtility::GetColor(EColor::Green), 1);
				RenderUtility::SetBrush(g, EColor::Green);
				mRenderer.drawCircle(g, mFit.centerInit, 4.0f);
			}


			if (bShowLineStrip && mPoints.size() >= 2)
			{
				int const lineStripWidth = 6;
				float const endCapRadius = 0.5f * lineStripWidth;
				g.setPen(RenderUtility::GetColor(EColor::Green), lineStripWidth);
				g.drawLineStrip(mPoints.data(), int(mPoints.size()));

				g.setPen(RenderUtility::GetColor(EColor::Green), 1);
				RenderUtility::SetBrush(g, EColor::Green);
				g.drawCircle(mPoints.front(), endCapRadius);
				g.drawCircle(mPoints.back(), endCapRadius);
			}

			if (bShowPoints)
			{
				g.setPen(RenderUtility::GetColor(EColor::White), 1);
				RenderUtility::SetBrush(g, EColor::White);
				for (Vector2 const& point : mPoints)
				{
					mRenderer.drawCircle(g, point, 3.0f);
				}
			}

			g.setTextColor(Color3ub(255, 255, 0));
			InlineString<256> str;
			str.format("Points = %d", int(mPoints.size()));
			g.drawText(Vector2(10, 10), str);

			if (mFit.bValid)
			{
				float fitPct = CalcCircleFitSimilarity(mPoints, mFit);
				str.format("Center = (%.2f, %.2f)  Radius = %.2f  Fit Pct = %.2f%%", mFit.center.x, mFit.center.y, mFit.radius, fitPct);
				g.drawText(Vector2(10, 30), str);
			}

			g.setTextColor(Color3ub(180, 180, 180));
			g.drawText(Vector2(10, 50), "LMB drag: add points   R: clear   Backspace: undo");

			g.endRender();
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown() || (msg.onMoving() && msg.isLeftDown()))
			{
				addPoint(Vector2(msg.getPos()));
			}

			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::R:
					restart();
					break;
				case EKeyCode::Back:
					if (!mPoints.empty())
					{
						mPoints.pop_back();
						mLastPoint = mPoints.empty() ? Vector2(std::numeric_limits<float>::max(), std::numeric_limits<float>::max()) : mPoints.back();
						updateFit();
					}
					break;
				default:
					break;
				}
			}

			return BaseClass::onKey(msg);
		}

	private:
		void addPoint(Vector2 const& point)
		{
			if (!mPoints.empty() && (point - mLastPoint).length2() < 9.0f)
				return;

			mPoints.push_back(point);
			mLastPoint = point;
			updateFit();
		}

		void updateFit()
		{
			mFit = FitGeometricL2(mPoints);
		}

		SimpleRenderer mRenderer;
		TArray<Vector2> mPoints;
		Vector2 mLastPoint;
		FittedCircle mFit;
		bool bShowCircle = false;
		bool bShowLineStrip = true;
		bool bShowPoints = false;
	};
}

REGISTER_STAGE_ENTRY("Circle Fit", CircleFit::Stage, EExecGroup::Test, "Algorithm|Math");
