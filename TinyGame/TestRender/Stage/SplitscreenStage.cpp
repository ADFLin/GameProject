#include "Stage/TestStageHeader.h"

#include "GameRenderSetup.h"
#include "RHI/ShaderProgram.h"
#include "FileSystem.h"
#include "RHI/ShaderManager.h"
#include "RHI/RHICommand.h"
#include "RHI/DrawUtility.h"
#include "RHI/GpuProfiler.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIUtility.h"
#include "RenderDebug.h"
#include "Rect.h"
#include "TinyCore/DebugDraw.h"

#define USE_SPLITSCREEN_LIB 0

#if USE_SPLITSCREEN_LIB
#include "SplitScreen/Voronoi/VoronoiBuilder.h"
#include "SplitScreen/Voronoi/VoronoiDiagram.h"
#include "SplitScreen/Voronoi/VoronoiBounds.h"
#include "SplitScreen/Voronoi/VoronoiNormalizationScope.h"
#include "SplitScreen/Voronoi/Balancing/FloydRelaxation.h"
#include "SplitScreen/Voronoi/Balancing/UniformTransformRelaxation.h"
#endif
using namespace Render;

#if USE_SPLITSCREEN_LIB
namespace SplitScreen
{
	struct ScreenRegion
	{
		struct RangeInt
		{
			int start;
			int length;
		};
		RangeInt Range;
		Vector2 Center;
		Vector2 Uv;
		Vector3 Position;
	};

	struct ScreenSplit
	{
		Line Line;
		float Blend;
	};

	/// <summary>
	/// Regions in screen space.
	/// </summary>
	struct ScreenRegions
	{
		TArray<ScreenRegion> Regions;
		TArray<Vector2> Points;
		TArray<ScreenSplit> Splits;

		int Length() { return Regions.size(); }

		ScreenRegions()
		{
		}

		void AddRegion(VoronoiDiagram& voronoiDiagram, VoronoiRegion const& region, Vector3 position, Vector2 uv)
		{
			auto points = voronoiDiagram.GetRegionPoints(region);
			Vector2 center = voronoiDiagram.GetCentroid(region);

			Rect bounds = voronoiDiagram.Bounds;
			Vector2 offset = -Vector2(bounds.getCenter());
			Vector2 scale = Vector2(bounds.getSize()) / 2.0;
			scale.x = 1 / scale.x;
			scale.y = 1 / scale.y;

			ScreenRegion screenRegion;
			screenRegion.Range.start = Points.size(); 
			screenRegion.Range.length = points.size();
			screenRegion.Center = (center + offset) * scale;
			screenRegion.Position = position;
			screenRegion.Uv = uv;
			Regions.push_back(screenRegion);

			for (int i = 0; i < points.size(); ++i)
			{
				Vector2 point = points[i];

				// To screen space
				point = (point + offset) * scale;
				Points.push_back(point);
			}
		}

		void AddSplit(VoronoiDiagram& voronoiDiagram, VoronoiRegion const& regionA, VoronoiRegion const& regionB, float blend)
		{
			if (blend == 1)
				return;

			Line edge;
			if (voronoiDiagram.TryFindEdgeBetweenRegions(regionA, regionB, edge))
			{
				Rect bounds = voronoiDiagram.Bounds;
				Vector2 offset = -Vector2(bounds.getCenter());
				Vector2 scale = Vector2(bounds.getSize()) / 2.0;
				scale.x = 1 / scale.x;
				scale.y = 1 / scale.y;

				ScreenSplit split;
				split.Line = Line((edge.From + offset) * scale, (edge.To + offset) * scale);
				split.Blend = 1 - blend;
				Splits.push_back(split);
			}
		}

		void Clear()
		{
			Regions.clear();
			Points.clear();
			Splits.clear();
		}

		ScreenRegion const& GetRegion(int index)
		{
			return Regions[index];
		}

		TArrayView<Vector2 const> GetRegionPoints(int index)
		{
			auto& region = Regions[index];
			return TArrayView<Vector2 const>(Points.data() + region.Range.start, region.Range.length);
		}

		Rect GetRegionRect(int index)
		{
			auto const& uv = GetRegion(index).Uv;
			auto points = GetRegionPoints(index);
			Vector2 min = Vector2(Math::MaxFloat, Math::MaxFloat);
			Vector2 max = Vector2(-Math::MaxFloat, -Math::MaxFloat);
			for (int i = 0; i < points.size(); ++i)
			{
				Vector2 point = (points[i] - uv) * 0.5f + Vector2(0.5f, 0.5f);
				min = Math::Min(min, point);
				max = Math::Max(max, point);
			}
			Rect rect;
			rect.min = min;
			rect.max = max;
			return rect;
		}
	};

	struct Balancing
	{
		bool Enabled;
		float StepSize;
		//[Range(0, 5)]
		int RelaxationIterations;

		bool Active() { return Enabled; }

		static Balancing Default()
		{
			Balancing result;
			result.RelaxationIterations = 1;
			result.StepSize = 0.009f;
			return result;
		}
	};

	enum class ECameraCentering
	{
		None,
		ScreenCentered,
		PlayerCentered,
	};

	enum class EBlendShape
	{
		Region,
		Circle,
	};

	struct Translating
	{
		//[MaxValue(0)]
		float Blend;
		EBlendShape BlendShape;
		ECameraCentering Centering;
		//[HideInInspector]
		//[Range(0, 0.5f)]
		float Border;

		static Translating Default()
		{
			Translating result;
			result.BlendShape = EBlendShape::Region;
			result.Blend = 0.2f;
			result.Centering = ECameraCentering::ScreenCentered;
			return result;
		}
	};


	class Blend
	{
	public:
		static constexpr float Half = 0.5f;

		static void BlendLine(Vector3 a, Vector3 b, float ab, Vector3& p0, Vector3& p1)
		{
			p0 = Math::Lerp(a, b, ab * Half);
			p1 = Math::Lerp(b, a, ab * Half);
		}

		static void BlendLine(Vector2 a, Vector2 b, float ab, Vector2& p0, Vector2& p1)
		{
			p0 = Math::LinearLerp(a, b, ab * Half);
			p1 = Math::LinearLerp(b, a, ab * Half);
		}

		static void BlendTriangle(Vector3 a, Vector3 b, Vector3 c, 
			float ab, float bc, float ac, 
			Vector3& p0, Vector3& p1, Vector3& p2)
		{
			float d0 = 1.0f / (ab + ac + 1.0f);
			p0 = (a * 1.0f + b * ab + c * ac) * d0;

			float d1 = 1.0f / (ab + bc + 1.0f);
			p1 = (b * 1.0f + a * ab + c * bc) * d1;

			float d2 = 1.0f / (ac + bc + 1.0f);
			p2 = (c * 1.0f + a * ac + b * bc) * d2;
		}

		static void BlendTriangle(Vector2 a, Vector2 b, Vector2 c,
			float ab, float bc, float ac,
			Vector2& p0, Vector2& p1, Vector2& p2)
		{
			float d0 = 1.0f / (ab + ac + 1.0f);
			p0 = (a * 1.0f + b * ab + c * ac) * d0;

			float d1 = 1.0f / (ab + bc + 1.0f);
			p1 = (b * 1.0f + a * ab + c * bc) * d1;

			float d2 = 1.0f / (ac + bc + 1.0f);
			p2 = (c * 1.0f + a * ac + b * bc) * d2;
		}

		static void BlendQuad(Vector3 a, Vector3 b, Vector3 c, Vector3 d,
			float ab, float bc, float ac, float ad, float bd, float cd,
			Vector3& p0, Vector3& p1, Vector3& p2, Vector3& p3)
		{
			float d0 = 1.0f / (ab + ac + ad + 1.0f);
			p0 = (a * 1.0f + b * ab + c * ac + d * ad) * d0;

			float d1 = 1.0f / (ab + bc + bd + 1.0f);
			p1 = (b * 1.0f + a * ab + c * bc + d * bd) * d1;

			float d2 = 1.0f / (ac + bc + cd + 1.0f);
			p2 = (c * 1.0f + a * ac + b * bc + d * cd) * d2;

			float d3 = 1.0f / (ad + bd + cd + 1.0f);
			p3 = (d * 1.0f + a * ad + b * bd + c * cd) * d3;
		}

		static void BlendQuad(Vector2 a, Vector2 b, Vector2 c, Vector2 d,
			float ab, float bc, float ac, float ad, float bd, float cd,
			Vector2& p0, Vector2& p1, Vector2& p2, Vector2& p3)
		{
			float divider0 = 1.0f / (ab + ac + ad + 1.0f);
			p0 = (a * 1.0f + b * ab + c * ac + d * ad) * divider0;

			float divider1 = 1.0f / (ab + bc + bd + 1.0f);
			p1 = (b * 1.0f + a * ab + c * bc + d * bd) * divider1;

			float divider2 = 1.0f / (ac + bc + cd + 1.0f);
			p2 = (c * 1.0f + a * ac + b * bc + d * cd) * divider2;

			float divider3 = 1.0f / (ad + bd + cd + 1.0f);
			p3 = (d * 1.0f + a * ad + b * bd + c * cd) * divider3;
		}

		static void ForceToQuad(float& ab, float& bc, float& ac, float& ad, float& bd, float& cd)
		{
			ForceToTriangle(ab, bd, ad);
			ForceToTriangle(ab, bc, ac);
			ForceToTriangle(bc, cd, bd);
			ForceToTriangle(ac, cd, ad);

			ForceToTriangle(ab, bd, ad);
			ForceToTriangle(ac, cd, ad);
			ForceToTriangle(ab, bc, ac);
			ForceToTriangle(bc, cd, bd);
		}

		static void ForceToTriangle(float& a, float& b, float& c)
		{
			float _a = 1 - a;
			float _b = 1 - b;
			float _c = 1 - c;

			float ta = Math::Min(_a, _b + _c);
			float tb = Math::Min(_b, _c + _a);
			float tc = Math::Min(_c, _a + _b);

			a = 1 - ta;
			b = 1 - tb;
			c = 1 - tc;
		}

		static float Linear(float distance, float blendStart, float blendEnd)
		{
			return Math::Clamp((distance - blendStart) / (blendEnd - blendStart), 0.0f, 1.0f);
		}

		static float LinearSafe(float distance, float blendStart, float blendEnd)
		{
			if ((blendStart - blendEnd) <= 1e-6)
			{
				return 0;
				//return (distance > blendStart) ? 0 : 1;
			}

			return Linear(distance, blendStart, blendEnd);
		}
	};

	struct SplitScreen4
	{
	public:
		VoronoiBuilder mVoronoiBuilder;
		VoronoiDiagram mVoronoiDiagram;
		TArray<Vector2> Sites;

		Vector3 m_Position0;
		Vector3 m_Position1;
		Vector3 m_Position2;
		Vector3 m_Position3;
		float m_Scale;

		SplitScreen4()
		{
			m_Position0 = Vector3::Zero();
			m_Position1 = Vector3::Zero();
			m_Position2 = Vector3::Zero();
			m_Position3 = Vector3::Zero();
			m_Scale = 0;
			Sites.reserve(4);
		}

		/// <summary>
		/// Recreate split screen with set of player positions.
		/// </summary>
		void Reset(Vector3 w0, Vector3 w1, Vector3 w2, Vector3 w3, float scale = 2, float aspectRatio = 1, float BoundsRadius = 0.3f)
		{
			m_Position0 = w0;
			m_Position1 = w1;
			m_Position2 = w2;
			m_Position3 = w3;

			Sites[0] = w0.xy();
			Sites[1] = w1.xy();
			Sites[2] = w2.xy();
			Sites[3] = w3.xy();

			auto localBounds = VoronoiBounds::CreateFromSites(aspectRatio, Sites, BoundsRadius);
			m_Scale = scale / localBounds.getSize().x;

			mVoronoiBuilder.SetSites(w0.xy(), w1.xy(), w2.xy(), w3.xy());
			mVoronoiBuilder.Construct(mVoronoiDiagram, localBounds);
		}

		/// <summary>
		/// Balance screen regions to similar area.
		/// </summary>
		void Balance(Balancing const& balanceSettings)
		{
			VoronoiNormalizationScope normalization(mVoronoiBuilder, mVoronoiDiagram, Sites);
			Vector4 position = Vector4::Zero();
			if (balanceSettings.Enabled)
			{
				if (balanceSettings.RelaxationIterations != 0)
					FloydRelaxation::Execute(mVoronoiBuilder, mVoronoiDiagram, Sites, balanceSettings.RelaxationIterations);

				UniformTransformRelaxation::Execute(mVoronoiBuilder, mVoronoiDiagram, Sites, position, 200, 0.009f);
			}
		}

		/// <summary>
		/// Returns the value of how much the screens are balanced. Where 0 means fully balanced.
		/// </summary>
		float GetBalanceError()
		{
			float area = mVoronoiDiagram.Bounds.getArea();
			Vector4 values = Vector4(
				mVoronoiDiagram.GetRegionArea(mVoronoiDiagram.Regions[0]),
				mVoronoiDiagram.GetRegionArea(mVoronoiDiagram.Regions[1]),
				mVoronoiDiagram.GetRegionArea(mVoronoiDiagram.Regions[2]),
				mVoronoiDiagram.GetRegionArea(mVoronoiDiagram.Regions[3]));
			Vector4 diff = (values / area) - Vector4(1.0f / 4.0f, 1.0f / 4.0f, 1.0f / 4.0f, 1.0f / 4.0f);
			return Math::Sqrt(Math::Dot(diff, diff));
		}

		void UpdatePositions(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3)
		{
			m_Position0 = p0;
			m_Position1 = p1;
			m_Position2 = p2;
			m_Position3 = p3;
		}

		/// <summary>
		/// Creates players screen regions.
		/// </summary>
		void CreateScreens(Translating translating, ScreenRegions& screenRegions)
		{
			auto const& bounds = mVoronoiDiagram.Bounds;

			VoronoiRegion const& S0 = mVoronoiDiagram.Regions[0];
			VoronoiRegion const& S1 = mVoronoiDiagram.Regions[1];
			VoronoiRegion const& S2 = mVoronoiDiagram.Regions[2];
			VoronoiRegion const& S3 = mVoronoiDiagram.Regions[3];

			Vector2 center0 = mVoronoiDiagram.GetCentroid(S0);
			Vector2 center1 = mVoronoiDiagram.GetCentroid(S1);
			Vector2 center2 = mVoronoiDiagram.GetCentroid(S2);
			Vector2 center3 = mVoronoiDiagram.GetCentroid(S3);

			Vector2 player0 = m_Position0.xy();
			Vector2 player1 = m_Position1.xy();
			Vector2 player2 = m_Position2.xy();
			Vector2 player3 = m_Position3.xy();

			// Calculate distances between regions
			float distance01;
			float distance02;
			float distance12;
			float distance03;
			float distance32;
			float distance31;
			if (translating.BlendShape == EBlendShape::Region)
			{
				distance01 = mVoronoiDiagram.GetDistanceBetweenRegions(S0, m_Scale, -center0 * m_Scale + player0, S1, m_Scale, -center1 * m_Scale + player1);
				distance02 = mVoronoiDiagram.GetDistanceBetweenRegions(S0, m_Scale, -center0 * m_Scale + player0, S2, m_Scale, -center2 * m_Scale + player2);
				distance12 = mVoronoiDiagram.GetDistanceBetweenRegions(S1, m_Scale, -center1 * m_Scale + player1, S2, m_Scale, -center2 * m_Scale + player2);
				distance03 = mVoronoiDiagram.GetDistanceBetweenRegions(S0, m_Scale, -center0 * m_Scale + player0, S3, m_Scale, -center3 * m_Scale + player3);
				distance32 = mVoronoiDiagram.GetDistanceBetweenRegions(S3, m_Scale, -center3 * m_Scale + player3, S2, m_Scale, -center2 * m_Scale + player2);
				distance31 = mVoronoiDiagram.GetDistanceBetweenRegions(S3, m_Scale, -center3 * m_Scale + player3, S1, m_Scale, -center1 * m_Scale + player1);
			}
			else
			{
				auto circle0 = Circle(player0, m_Scale * mVoronoiDiagram.GetRegionInscribedCircleRadius(S0));
				auto circle1 = Circle(player1, m_Scale * mVoronoiDiagram.GetRegionInscribedCircleRadius(S1));
				auto circle2 = Circle(player2, m_Scale * mVoronoiDiagram.GetRegionInscribedCircleRadius(S2));
				auto circle3 = Circle(player3, m_Scale * mVoronoiDiagram.GetRegionInscribedCircleRadius(S3));

				distance01 = Circle::GetDistance(circle0, circle1);
				distance02 = Circle::GetDistance(circle0, circle2);
				distance12 = Circle::GetDistance(circle1, circle2);
				distance03 = Circle::GetDistance(circle0, circle3);
				distance32 = Circle::GetDistance(circle3, circle2);
				distance31 = Circle::GetDistance(circle3, circle1);
			}

			Vector3 cc0;
			Vector3 cc1;
			Vector3 cc2;
			Vector3 cc3;
			switch (translating.Centering)
			{
			case ECameraCentering::PlayerCentered:
				cc0 = m_Position0;
				cc1 = m_Position1;
				cc2 = m_Position2;
				cc3 = m_Position3;
				break;
			case ECameraCentering::ScreenCentered:
				{
					Vector2 centerOffset0 = center0 - Vector2(bounds.getCenter());
					centerOffset0 *= m_Scale;

					Vector2 centerOffset1 = center1 - Vector2(bounds.getCenter());
					centerOffset1 *= m_Scale;

					Vector2 centerOffset2 = center2 - Vector2(bounds.getCenter());
					centerOffset2 *= m_Scale;

					Vector2 centerOffset3 = center3 - Vector2(bounds.getCenter());
					centerOffset3 *= m_Scale;

					cc0 = Vector3(player0 - centerOffset0, m_Position0.z);
					cc1 = Vector3(player1 - centerOffset1, m_Position1.z);
					cc2 = Vector3(player2 - centerOffset2, m_Position2.z);
					cc3 = Vector3(player3 - centerOffset3, m_Position3.z);
					break;
				}
			case ECameraCentering::None:
				{
					Vector2 centerOffset0 = player0 - Vector2(bounds.getCenter());
					centerOffset0 *= m_Scale;

					Vector2 centerOffset1 = player1 - Vector2(bounds.getCenter());
					centerOffset1 *= m_Scale;

					Vector2 centerOffset2 = player2 - Vector2(bounds.getCenter());
					centerOffset2 *= m_Scale;

					Vector2 centerOffset3 = player3 - Vector2(bounds.getCenter());
					centerOffset3 *= m_Scale;

					cc0 = Vector3(player0 - centerOffset0, m_Position0.z);
					cc1 = Vector3(player1 - centerOffset1, m_Position1.z);
					cc2 = Vector3(player2 - centerOffset2, m_Position2.z);
					cc3 = Vector3(player3 - centerOffset3, m_Position3.z);
					break;
				}

			default:
				;
				//throw new NotImplementedException();
			}

			// Calculate blend values
			auto blend = translating.Blend;
			float start = blend;
			float end = 0;
			float blend01 = Blend::LinearSafe(distance01, start, end);
			float blend12 = Blend::LinearSafe(distance12, start, end);
			float blend02 = Blend::LinearSafe(distance02, start, end);
			float blend03 = Blend::LinearSafe(distance03, start, end);
			float blend31 = Blend::LinearSafe(distance31, start, end);
			float blend32 = Blend::LinearSafe(distance32, start, end);
			Blend::ForceToQuad(blend01, blend12, blend02, blend03, blend31, blend32);

			// Blend world positions

			Vector3 p0;
			Vector3 p1; 
			Vector3 p2;
			Vector3 p3;
			Blend::BlendQuad(
				cc0, cc1, cc2, cc3,
				blend01, blend12, blend02, blend03, blend31, blend32,
				p0, p1, p2, p3);

			Vector2 boundsOffset = -Vector2(bounds.getCenter());
			Vector2 boundsScale = Vector2(bounds.getSize()) / 2;
			boundsScale.x = 1.0 / boundsScale.x;
			boundsScale.y = 1.0 / boundsScale.y;

			// Blend uv positions
			Vector2 u0;
			Vector2 u1;
			Vector2 u2;
			Vector2 u3;
			switch (translating.Centering)
			{
			case ECameraCentering::PlayerCentered:
				Vector2 au = (center0 + boundsOffset) * boundsScale;
				Vector2 bu = (center1 + boundsOffset) * boundsScale;
				Vector2 cu = (center2 + boundsOffset) * boundsScale;
				Vector2 du = (center3 + boundsOffset) * boundsScale;

				Blend::BlendQuad(
					au, bu, cu, du,
					blend01, blend12, blend02, blend03, blend31, blend32,
					u0, u1, u2, u3);
					break;
			case ECameraCentering::ScreenCentered:
			case ECameraCentering::None:
				u0 = Vector2::Zero();
				u1 = Vector2::Zero();
				u2 = Vector2::Zero();
				u3 = Vector2::Zero();
				break;

			default:
				;
				//throw new NotImplementedException();
			}

			// Update screen regions
			screenRegions.Clear();
			screenRegions.AddRegion(mVoronoiDiagram, S0, p0, u0);
			screenRegions.AddRegion(mVoronoiDiagram, S1, p1, u1);
			screenRegions.AddRegion(mVoronoiDiagram, S2, p2, u2);
			screenRegions.AddRegion(mVoronoiDiagram, S3, p3, u3);
			screenRegions.AddSplit(mVoronoiDiagram, S0, S1, blend01);
			screenRegions.AddSplit(mVoronoiDiagram, S1, S2, blend12);
			screenRegions.AddSplit(mVoronoiDiagram, S0, S2, blend02);
			screenRegions.AddSplit(mVoronoiDiagram, S0, S3, blend03);
			screenRegions.AddSplit(mVoronoiDiagram, S3, S1, blend31);
			screenRegions.AddSplit(mVoronoiDiagram, S3, S2, blend32);
		}
#if 0
		void DrawDelaunayDual()
		{
			var triangles = new NativeList<Triangle>(Allocator.Temp);
			DelaunayDual.GetTriangles(Sites[0], Sites[1], Sites[2], Sites[3], triangles);
			foreach(var triangle in triangles)
				triangle.Draw(Color.black);
			triangles.Dispose();
		}

		void DrawRegions(BlendShape blendShape)
		{


			CheckIsCreated();

			var regions = VoronoiDiagram.Regions;

			VoronoiRegion S0 = regions[0];
			VoronoiRegion S1 = regions[1];
			VoronoiRegion S2 = regions[2];
			VoronoiRegion S3 = regions[3];

			Vector2 center0 = VoronoiDiagram.GetCentroid(S0);
			Vector2 center1 = VoronoiDiagram.GetCentroid(S1);
			Vector2 center2 = VoronoiDiagram.GetCentroid(S2);
			Vector2 center3 = VoronoiDiagram.GetCentroid(S3);

			Vector2 player0 = m_Position0.xy;
			Vector2 player1 = m_Position1.xy;
			Vector2 player2 = m_Position2.xy;
			Vector2 player3 = m_Position3.xy;

			VoronoiDiagram.DrawRegion(S0, PlayerColor.PlayerA);
			VoronoiDiagram.DrawRegion(S1, PlayerColor.PlayerB);
			VoronoiDiagram.DrawRegion(S2, PlayerColor.PlayerC);
			VoronoiDiagram.DrawRegion(S3, PlayerColor.PlayerD);

			// Draw centers
			Gizmos.DrawPoint(center0, 0.003f * VoronoiDiagram.Bounds.width, Color.white);
			Gizmos.DrawPoint(center1, 0.003f * VoronoiDiagram.Bounds.width, Color.white);
			Gizmos.DrawPoint(center2, 0.003f * VoronoiDiagram.Bounds.width, Color.white);
			Gizmos.DrawPoint(center3, 0.003f * VoronoiDiagram.Bounds.width, Color.white);

			// Draw screen regions
			if (blendShape == BlendShape.Region)
			{
				VoronoiDiagram.DrawRegion(S0, m_Scale, -center0 * m_Scale + player0, new Color32(255, 255, 255, 128));
				VoronoiDiagram.DrawRegion(S1, m_Scale, -center1 * m_Scale + player1, new Color32(255, 255, 255, 128));
				VoronoiDiagram.DrawRegion(S2, m_Scale, -center2 * m_Scale + player2, new Color32(255, 255, 255, 128));
				VoronoiDiagram.DrawRegion(S3, m_Scale, -center3 * m_Scale + player3, new Color32(255, 255, 255, 128));
			}
			else
			{
				Gizmos.DrawPoint(player0, m_Scale * VoronoiDiagram.GetRegionInscribedCircleRadius(S0), new Color32(255, 255, 255, 128));
				Gizmos.DrawPoint(player1, m_Scale * VoronoiDiagram.GetRegionInscribedCircleRadius(S1), new Color32(255, 255, 255, 128));
				Gizmos.DrawPoint(player2, m_Scale * VoronoiDiagram.GetRegionInscribedCircleRadius(S2), new Color32(255, 255, 255, 128));
				Gizmos.DrawPoint(player3, m_Scale * VoronoiDiagram.GetRegionInscribedCircleRadius(S3), new Color32(255, 255, 255, 128));
			}

			// Draw inscribed circles
			Gizmos.DrawPointWire(center0, VoronoiDiagram.GetRegionInscribedCircleRadius(S0), Color.white);
			Gizmos.DrawPointWire(center1, VoronoiDiagram.GetRegionInscribedCircleRadius(S1), Color.white);
			Gizmos.DrawPointWire(center2, VoronoiDiagram.GetRegionInscribedCircleRadius(S2), Color.white);
			Gizmos.DrawPointWire(center3, VoronoiDiagram.GetRegionInscribedCircleRadius(S3), Color.white);

		}
#endif
	};

	struct ScreenData
	{
		Vector3 worldPos;

		//Camera Camera;
		//Transform Target;
		
		//RenderTexture RenderTarget;
		//Mesh Mesh;
	};

	enum class DrawFlags
	{
		None,
		Regions = 1 << 1,
		DelaunayDual = 1 << 4,
	};

	enum class BalancingMode
	{
		None,
		UniformTransformRelaxation,
	};




	/// <summary>
	/// Split screen implementation with the <see cref="MonoBehaviour"/> component.
	/// This component adaptively constructs splits and combines them for camera to present.
	/// It only works with up to 4 split screens.
	/// </summary>
	class SplitScreenEffect
	{
	public:
		TArray<ScreenData> Screens;
		//[MaxValue(0)]
		float Distance = 10;
		//[Range(0, 2)]
		float BoundsRadius = 0.3f;
		Translating mTranslating = Translating::Default();
		Balancing mBalancing = Balancing::Default();
		bool UseAspectRatio = true;
		DrawFlags mDrawFlags = DrawFlags::Regions;

		SplitScreen4 m_SplitScreen4;
		ScreenRegions m_ScreenRegions;

		Matrix4 m_WorldToPlane;
		Matrix4 m_PlaneToWorld;
		float m_AspectRation;
		float m_Scale;


		/// <summary>
		/// Returns command buffer that contains commands for rendering current frame split screen.
		/// </summary>

		/// <summary>
		/// Add new screen.
		/// </summary>
#if 0
		void AddScreen(Camera camera, Transform target)
		{
			CheckIsCreated();
			Screens.Add(new ScreenData{ Camera = camera, Target = target });
		}
#endif

		/// <summary>
		/// Clears all screen data.
		/// </summary>
		void Clear()
		{
			Screens.clear();
		}


		void Update()
		{
			{
				UpdateBounds();
				UpdatePlaneMatrices();
			}

			auto& screenRegions = m_ScreenRegions;
			switch (Screens.size())
			{
			case 4:

				m_SplitScreen4.Reset(
					GetScreenTargetPositionPS(0),
					GetScreenTargetPositionPS(1),
					GetScreenTargetPositionPS(2),
					GetScreenTargetPositionPS(3),
					m_Scale,
					m_AspectRation,
					BoundsRadius);

				if (mBalancing.Active())
				{
					m_SplitScreen4.Balance(mBalancing);
				}


				{
					m_SplitScreen4.UpdatePositions(
						GetScreenTranslatingPositionPS(0),
						GetScreenTranslatingPositionPS(1),
						GetScreenTranslatingPositionPS(2),
						GetScreenTranslatingPositionPS(3)
					);

					m_SplitScreen4.CreateScreens(mTranslating, screenRegions);
				}
				break;
			}

			for (int i = 0; i < screenRegions.Length(); ++i)
			{
				UpdateScreen(i, screenRegions.GetRegion(i), screenRegions.GetRegionPoints(i), screenRegions.GetRegionRect(i));
			}


		}


		bool UpdateScreen(int screenIndex, ScreenRegion region, TArrayView<Vector2 const> points, Rect rect)
		{
#if 0
			if (points.Length < 3)
				throw new ArgumentException($"Something failed and screen region at index {screenIndex} was incorrectly generated.");

			var camera = GetComponent<Camera>();
			var screen = Screens[screenIndex];
			var screenCamera = screen.Camera;
			bool dirty = false;

			float width = Screen.width;
			float height = Screen.height;

			// Updates render texture
			if (screen.RenderTarget == null || (screen.RenderTarget.width != width || screen.RenderTarget.height != height))
			{
				screen.RenderTarget ? .Release();
				screen.RenderTarget = new RenderTexture((int)width, (int)height, 24);
				//screen.RenderTarget.filterMode = FilterMode.Point;
				screen.RenderTarget.name = $"ScreenTexture{screenIndex}";
				dirty = true;
			}

			// Updates camera
			if (screenCamera != null)
			{
				using (Markers.UpdateCamera.Auto())
				{
					// Update position
					var position = region.Position;
					Matrix4x4 viewMatrix = m_PlaneToWorld;
					var cameraPosition = viewMatrix.MultiplyPoint(new Vector3(position.x, position.y, position.z)) + transform.rotation * new Vector3(0, 0, -Distance);

					screenCamera.transform.position = cameraPosition;

					screenCamera.transform.rotation = camera.transform.rotation;

					// Keep in sync other camera options
					screenCamera.orthographicSize = camera.orthographicSize;
					screenCamera.orthographic = camera.orthographic;
					screenCamera.fieldOfView = camera.fieldOfView;
					screenCamera.farClipPlane = camera.farClipPlane;
					screenCamera.nearClipPlane = camera.nearClipPlane;
					screenCamera.fieldOfView = camera.fieldOfView;

					screenCamera.targetTexture = screen.RenderTarget;

					Vector2 rectMin = math.max(rect.min, 0);
					Vector2 rectMax = math.min(rect.max, 1);
					rect = new Rect(rectMin, rectMax - rectMin);

					// Simulate scissor rect with viewport and projection matric offset
					if (MinimizeCameraRect)
					{
						float inverseWidth = 1 / rect.width;
						float inverseHeight = 1 / rect.height;
						Matrix4x4 matrix1 = Matrix4x4.TRS(
							new Vector3(-rect.x * 2 * inverseWidth, -rect.y * 2 * inverseHeight, 0),
							Quaternion.identity, Vector3.one);
						Matrix4x4 matrix2 = Matrix4x4.TRS(
							new Vector3(inverseWidth - 1, inverseHeight - 1, 0),
							Quaternion.identity,
							new Vector3(inverseWidth, inverseHeight, 1));
						screenCamera.projectionMatrix = matrix1 * matrix2 * camera.projectionMatrix;

						screenCamera.rect = rect;
					}
					else
					{
						screenCamera.projectionMatrix = camera.projectionMatrix;
						screenCamera.rect = new Rect(0, 0, 1, 1);
					}
				}
			}

			// Updates mesh
			using (Markers.UpdateMesh.Auto())
			{
				if (screen.Mesh == null)
				{
					screen.Mesh = new Mesh();
					screen.Mesh.name = $"ScreenMesh{screenIndex}";
					dirty = true;
				}
				else
				{
					screen.Mesh.Clear();
				}

				var mesh = screen.Mesh;

				var vertices = new NativeList<Vector3>(region.Range.length * 3, Allocator.Temp);
				var uvs = new NativeList<Vector2>(region.Range.length * 3, Allocator.Temp);
				var indices = new NativeList<int>(region.Range.length * 3, Allocator.Temp);

				ConvexPolygon.Triangulate(points, region.Center, region.Uv, vertices, uvs, indices);

				mesh.SetVertices(vertices.AsArray());
				mesh.SetUVs(0, uvs.AsArray());
				mesh.SetIndices(indices.AsArray(), MeshTopology.Triangles, 0);

				vertices.Dispose();
				uvs.Dispose();
				indices.Dispose();
			}

			return dirty;
#else
			return true;
#endif
		}

		void UpdatePlaneMatrices()
		{
#if 0
			// TODO: Cleanup this method

			var camera = GetComponent<Camera>();

			Quaternion rotation = !camera.orthographic ? (Quaternion)quaternion.RotateX(math.radians(90)) : transform.rotation; // transform.rotation
			m_PlaneToWorld = Matrix4.TRS(rotation * new Vector3(0, 0, -Distance), rotation, 1);

			m_PlaneToWorld.c2 = -m_PlaneToWorld.c2;
			m_WorldToPlane = math.inverse(m_PlaneToWorld);

			if (!UseAspectRatio)
			{
				if (!camera.orthographic)
				{
					Vector4 ddd = math.mul(camera.projectionMatrix, new Vector4(0, 0, -Distance, 1));
					float d = ddd.z / ddd.w;

					Vector4 ss0 = new Vector4(-1, -1, d, 1);
					Vector4 ss1 = new Vector4(1, 1, d, 1);

					Vector4 ww0 = math.mul(math.inverse(camera.projectionMatrix), ss0);
					Vector4 ww1 = math.mul(math.inverse(camera.projectionMatrix), ss1);

					ww0 /= ww0.w;
					ww1 /= ww1.w;

					Matrix4 orth = Matrix4.OrthoOffCenter(ww0.x, ww1.x, ww0.y, ww1.y, camera.nearClipPlane, camera.farClipPlane);
					m_WorldToPlane = math.mul(orth, m_WorldToPlane);
				}
				else
				{
					m_WorldToPlane = math.mul(camera.projectionMatrix, m_WorldToPlane);
				}
			}

			m_PlaneToWorld = math.inverse(m_WorldToPlane);
#endif
		}

		void UpdateBounds()
		{
#if 0
			// TODO: Cleanup this method

			if (!UseAspectRatio)
			{
				m_AspectRation = 1;
				m_Scale = 2;
			}
			else
			{
				var camera = GetComponent<Camera>();

				Vector4 lookAtSS = math.mul(camera.projectionMatrix, new Vector4(0, 0, -Distance, 1));
				float z = lookAtSS.z / lookAtSS.w;

				Vector4 minSS = new Vector4(-1, -1, z, 1);
				Vector4 maxSS = new Vector4(1, 1, z, 1);

				Matrix4 invProjectionMatrix = math.inverse(camera.projectionMatrix);
				Vector4 minPS = math.mul(invProjectionMatrix, minSS);
				Vector4 maxPS = math.mul(invProjectionMatrix, maxSS);

				minPS /= minPS.w;
				maxPS /= maxPS.w;

				Vector2 size = maxPS.xy - minPS.xy;
				m_AspectRation = size.x / size.y;
				m_Scale = size.x;
			}
#else

			m_AspectRation = 1;
			m_Scale = 2;
#endif
		}

		Vector3 GetScreenTargetPositionPS(int screenIndex)
		{
			auto positionWS = Screens[screenIndex].worldPos;

#if 0

			Matrix4 viewMatrix = m_WorldToPlane;
			Vector3 positionPS = viewMatrix.MultiplyPoint(positionWS);

			return Vector3(positionPS.x, positionPS.y, positionPS.z);
#else
			return positionWS;
#endif
		}

		Vector3 GetScreenTranslatingPositionPS(int screenIndex)
		{
			auto positionWS = Screens[screenIndex].worldPos;
#if 0

			Matrix4 viewMatrix = m_WorldToPlane;
			Vector3 positionPS = viewMatrix.MultiplyPoint(positionWS);

			return Vector3(positionPS.x, positionPS.y, positionPS.z);
#else
			return positionWS;
#endif
		}
	};
}
#endif
class SplitScreenTestStage : public StageBase
				           , public IGameRenderSetup
{
	using BaseClass = StageBase;
public:
	SplitScreenTestStage() {}



	struct Player 
	{
		Vector2 pos;
		Vector2 vel = Vector2::Zero();
		Vector2 moveDir = Vector2::Zero();
		int viewIndex;

		void update(float dt)
		{
			float const moveAcc = 5;

			Vector2 acc = Math::GetNormalSafe(moveDir) * moveAcc;

			float const maxSpeed = 10;
			float const damping = 0.5;
			if (acc.length2() > 0)
			{
				vel += moveDir;
				float speed = vel.length();
				if (speed > maxSpeed)
				{
					vel *= maxSpeed / speed;
				}
			}
			else
			{
				vel *= damping;
			}
			pos += vel * dt;
		}
	};

	TArray< Player > mPlayers;

	bool onInit() override
	{
		if (!BaseClass::onInit())
			return false;
		::Global::GUI().cleanupWidget();

		auto frame = WidgetUtility::CreateDevFrame();
		
		
		test();
		restart();
		return true;
	}
#if USE_SPLITSCREEN_LIB
	SplitScreen::SplitScreenEffect mEffect;
#endif
	void test()
	{
#if USE_SPLITSCREEN_LIB
		mEffect.Screens.resize(4);

		mEffect.Screens[0].worldPos = 2 * Vector3(1, 1, 0);
		mEffect.Screens[1].worldPos = 2 * Vector3(-1, 1, 0);
		mEffect.Screens[2].worldPos = 2 * Vector3(-1, -1, 0);
		mEffect.Screens[3].worldPos = 2 * Vector3(1, -1, 0);

		mEffect.Update();
#endif
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() 
	{
		mPlayers.resize(2);
		{
			auto& player = mPlayers[0];
			player.pos = Vector2(0, 0);
		}

		{
			auto& player = mPlayers[1];
			player.pos = Vector2(5, 0);
		}

	}


	static Vector2 GetProjection(Vector2 const& v, Vector2 const& dir)
	{
		return v.dot(dir) * dir;
	}


	void updateViews(float deltaTime)
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		float defaultZoom = float(screenSize.x) / 100;

		auto SeupSingleView = [&](Vector2 const& lookPos)
		{
			mNumView = 1;
			auto& view = mViews[0];
			view.index = 0;
			view.screenOffset = Vector2(0, 0);
			view.pos = lookPos;
			view.zoom = defaultZoom;
			view.area = { Vector2(0,0) , Vector2(screenSize.x, 0) , screenSize , Vector2(0, screenSize.y) };

			for (auto& player : mPlayers)
			{
				player.viewIndex = view.index;
			}
		};

		RenderTransform2D worldToScreen = RenderTransform2D::LookAt(screenSize, Vector2(0, 0), Vector2(0, 1), defaultZoom, true);


		if (mPlayers.size() == 1)
		{
			SeupSingleView(mPlayers[0].pos);
			return;
		}
		else if (mPlayers.size() == 2)
		{
			Player& playerA = mPlayers[0];
			Player& playerB = mPlayers[1];
			Vector2 centerWorld = 0.5 * (playerA.pos + playerB.pos);

			Vector2 posA = worldToScreen.transformPosition(playerA.pos);
			Vector2 posB = worldToScreen.transformPosition(playerB.pos);
			Vector2 offset = posB - posA;

			Vector2 dir = offset;
			float distScreen = dir.normalize();
			if (distScreen == 0)
			{
				SeupSingleView(centerWorld);
				return;
			}

			float distances[2];
			Vector2 center = 0.5 * Vector2(screenSize);
			Vector2 normal = Vector2(dir.y, -dir.x);
			Math::LineAABBTest(center, normal, Vector2(0, 0), screenSize, distances);

			Vector2 p0 = center + normal * distances[0];
			Vector2 p1 = center + normal * distances[1];

			float error = 1e-4;
			if (Math::Abs(p0.y) < error || Math::Abs(p1.y) < error)
			{
				if (Math::Abs(p1.y) < error)
				{
					using namespace std;
					swap(p0, p1);
				}

				p0.y = 0;
				p1.y = screenSize.y;

				int indexLeft = dir.x > 0 ? 0 : 1;
				mViews[indexLeft].area = 
				{
					Vector2(0,0) , p0 , p1 , Vector2(0, screenSize.y)
				};
				mViews[1 - indexLeft].area =
				{
					p0, Vector2(screenSize.x, 0) , screenSize,  p1 ,
				};
			}
			else
			{
				if (Math::Abs(p1.x) < error)
				{
					using namespace std;
					swap(p0, p1);
				}

				p0.x = 0;
				p1.x = screenSize.x;
				int indexTop = dir.y > 0 ? 0 : 1;
				mViews[indexTop].area =
				{
					Vector2(0,0) , Vector2(screenSize.x, 0) , p1 , p0
				};
				mViews[1 - indexTop].area =
				{
					p0, p1 , screenSize,  Vector2(0, screenSize.y)
				};
			}

			for (int i = 0; i < 2; ++i)
			{
				View& view = mViews[i];
				Vector2 pos = CalcPolygonCentroid(view.area);
				view.screenOffset = pos - center;

				DrawDebugPoint(pos, Color3f(1, 0, 0), 5);
				Vector2 pos2 = center + GetProjection(view.screenOffset, dir);
				DrawDebugPoint(pos2, Color3f(0, 1, 0), 5);
				DrawDebugLine(pos, pos2, Color3f(1, 1, 0), 2);
			}

			Vector2 p0S = center + GetProjection(mViews[0].screenOffset, dir);
			Vector2 p1S = center + GetProjection(mViews[1].screenOffset, dir);
			float dist2 = Math::Distance(p0S, p1S);
	
			float alpha = (distScreen - dist2) / 250;
			if (alpha <= 0)
			{
				SeupSingleView(centerWorld);
				return;
			}

			if (alpha > 1)
				alpha = 1;

			alpha = alpha * alpha * (3.0f - 2.0f*alpha);

			mNumView = 2;
			for (int i = 0; i < 2; ++i)
			{
				View& view = mViews[i];
				view.screenOffset = Math::LinearLerp(GetProjection(view.screenOffset, dir), view.screenOffset, alpha);
				view.index = i;
				view.pos = mPlayers[i].pos;
				view.zoom = defaultZoom;
				mPlayers[i].viewIndex = i;
			}
			return;
		}
		else if (mPlayers.size() == 3)
		{
			Player& playerA = mPlayers[0];
			Player& playerB = mPlayers[1];
			Player& playerC = mPlayers[2];
			Vector2 posA = worldToScreen.transformPosition(playerA.pos);
			Vector2 posB = worldToScreen.transformPosition(playerB.pos);
			Vector2 posC = worldToScreen.transformPosition(playerC.pos);

			Vector2 dBA = posB - posA;
			Vector2 dCA = posC - posA;

			if (dBA.cross(dCA) < 1e-4)
			{





			}


			Vector2 baseScreen = Math::GetCircumcirclePoint(posA, posB, posC);


			float coords[3];
			if (!Math::Barycentric(baseScreen, posA, posB, posC, coords))
			{




			}
			else
			{




			}
		}
	}

	void onUpdate(GameTimeSpan deltaTime) override
	{
		BaseClass::onUpdate(deltaTime);

		DrawDebugClear();

		for (int playerIndex = 0; playerIndex < mPlayers.size(); ++playerIndex)
		{
			auto& player = mPlayers[playerIndex];
			player.update(deltaTime);
		}

		updateViews(deltaTime);
	}

	struct View
	{
		int     index;
		Vector2 pos;
		Vector2 screenOffset;
		float   zoom;
		TRect<float>    areaRect;
		TArray<Vector2> area;
	};

	View mViews[4];
	int  mNumView;
	static constexpr int ColorMap[] = { EColor::Red, EColor::Green, EColor::Blue };


	void render(RHICommandList& commandList, View const& view)
	{

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.enablePen(false);
		g.setTextureState(nullptr);
		g.resetRenderState();
#if 1
		g.setCustomRenderState([&view](RHICommandList& commandList, RenderBatchedElement& element)
		{
			RHISetBlendState(commandList, TStaticBlendState< CWM_None >::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<
				false, ECompareFunc::Always, true, ECompareFunc::Always, 
				EStencil::Replace, EStencil::Replace, EStencil::Replace 
			>::GetRHI(), BIT(view.index));

		});
		g.drawPolygon(view.area.data(), view.area.size());
#endif
#if 1

		auto SetupDrawState = [&](RHICommandList& commandList)
		{
			RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
			RHISetDepthStencilState(commandList, TStaticDepthStencilState<
				true, ECompareFunc::Always, true, ECompareFunc::Equal,
				EStencil::Keep, EStencil::Keep, EStencil::Keep
			>::GetRHI(), BIT(view.index));
		};

		Vec2i screenSize = ::Global::GetScreenSize();
		RenderTransform2D worldToScreen = RenderTransform2D::LookAt(screenSize, view.pos, Vector2(0, 1), view.zoom, true) * RenderTransform2D::Translate(view.screenOffset);

		g.pushXForm();
		g.transformXForm(worldToScreen, false);

		//RenderUtility::SetBrush(g, view.index == 0 ? EColor::Red : EColor::Green);
		//g.drawRect(Vector2::Zero(), screenSize);

#if 1
		g.setTextureState(mTex);
		//g.setSampler(TStaticSamplerState<ESampler::Trilinear>::GetRHI());
		g.setCustomRenderState([&](RHICommandList& commandList, RenderBatchedElement& element, GraphicsDefinition::RenderState const& state)
		{
			SetupDrawState(commandList);
			BatchedRender::SetupShaderState(commandList, g.getBaseTransform(), state);
		});
		RenderUtility::SetBrush(g, EColor::White);
		float len = 80;
		g.drawTexture(Vector2(-len, -len), Vector2(len, len));
		g.drawTexture(Vector2(   0, -len), Vector2(len, len));
		g.drawTexture(Vector2(   0,    0), Vector2(len, len));
		g.drawTexture(Vector2(-len,    0), Vector2(len, len));

#endif


		g.setTextureState(nullptr);
		g.resetRenderState();
		g.setCustomRenderState([&](RHICommandList& commandList, RenderBatchedElement& element, GraphicsDefinition::RenderState const& state)
		{
			SetupDrawState(commandList);
		});

		RenderUtility::SetPen(g, EColor::Red);
		g.drawLine(Vector2(0, 0), Vector2(1000, 0));
		RenderUtility::SetPen(g, EColor::Green);
		g.drawLine(Vector2(0, 0), Vector2(0, 1000));


		RenderUtility::SetPen(g, EColor::Black);
		for (int playerIndex = 0; playerIndex < mPlayers.size(); ++playerIndex)
		{
			auto& player = mPlayers[playerIndex];
			RenderUtility::SetBrush(g, ColorMap[playerIndex]);
			g.drawCircle(player.pos, 1);
		}

		g.popXForm();
#endif
		g.flush();
		g.resetRenderState();
	}

	void onRender(float dFrame) override
	{
		Vec2i screenSize = ::Global::GetScreenSize();

		RHICommandList& commandList = RHICommandList::GetImmediateList();
		RHISetFrameBuffer(commandList, nullptr);
		RHIClearRenderTargets(commandList, EClearBits::Color | EClearBits::Stencil , &LinearColor(0, 0, 0, 1), 1);

		RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
		g.beginRender();

		for (int i = 0; i < mNumView; ++i)
		{
			render(commandList, mViews[i]);
		}

		g.setPenWidth(4);
		RenderUtility::SetBrush(g, EColor::Null);

		int controlViewIndex = INDEX_NONE;
		if (mPlayers.size() > 1)
		{
			controlViewIndex = mPlayers[mPlayerControlIndex].viewIndex;
		}

		for (int i = 0; i < mNumView; ++i)
		{
			auto& view = mViews[i];
			if (view.index == controlViewIndex )
				continue;

			RenderUtility::SetPen(g, ColorMap[view.index]);
			g.drawPolygon(view.area.data(), view.area.size());
		}

		if (controlViewIndex != INDEX_NONE)
		{
			auto& view = mViews[controlViewIndex];
			RenderUtility::SetPen(g, EColor::Yellow);
			g.drawPolygon(view.area.data(), view.area.size());
		}

		DrawDebugCommit(::Global::GetIGraphics2D());
		g.endRender();
	}

	MsgReply onMouse(MouseMsg const& msg) override
	{
		return BaseClass::onMouse(msg);
	}


	int mPlayerControlIndex = 0;

	MsgReply onKey(KeyMsg const& msg) override
	{
		auto& playerControlled = mPlayers[mPlayerControlIndex];
		if (msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			case EKeyCode::A: playerControlled.moveDir.x = -1; break;
			case EKeyCode::D: playerControlled.moveDir.x =  1; break;
			case EKeyCode::S: playerControlled.moveDir.y = -1; break;
			case EKeyCode::W: playerControlled.moveDir.y =  1; break;
			case EKeyCode::Z: 
				--mPlayerControlIndex; 
				if (mPlayerControlIndex < 0) 
					mPlayerControlIndex += mPlayers.size(); 
				break;
			case EKeyCode::X:
				++mPlayerControlIndex;
				if (mPlayerControlIndex >= mPlayers.size())
					mPlayerControlIndex = 0;
				break;
			case EKeyCode::N:
				{
					mPlayers.resize(mPlayers.size() + 1);
					mPlayers.back().pos = playerControlled.pos;
				}
				break;
			case EKeyCode::M:
				if ( mPlayers.size() > 1)
				{
					mPlayers.pop_back();
				}
				break;
			}
		}
		else
		{
			switch (msg.getCode())
			{
			case EKeyCode::A: if (playerControlled.moveDir.x == -1) playerControlled.moveDir.x = 0; break;
			case EKeyCode::D: if (playerControlled.moveDir.x == 1)  playerControlled.moveDir.x = 0; break;
			case EKeyCode::S: if (playerControlled.moveDir.y == -1) playerControlled.moveDir.y = 0; break;
			case EKeyCode::W: if (playerControlled.moveDir.y == 1)  playerControlled.moveDir.y = 0; break;
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

	RHITexture2DRef mTex;
	bool setupRenderResource(ERenderSystem systemName) override
	{
		mTex = RHIUtility::LoadTexture2DFromFile("Texture/UVChecker.png", TextureLoadOption().FlipV());

		GTextureShowManager.registerTexture("Tex", mTex);
		return true;
	}


	void preShutdownRenderSystem(bool bReInit = false) override
	{

	}


	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1280;
		systemConfigs.screenHeight = 720;
	}

protected:
};

REGISTER_STAGE_ENTRY("Split Screen", SplitScreenTestStage, EExecGroup::GraphicsTest);