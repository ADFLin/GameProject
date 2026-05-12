#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "GameGlobal.h"
#include "RenderUtility.h"

#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/RHIUtility.h"

#include "DataStructure/Array.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace ImageUpscaale
{
	using namespace Render;

	struct RGBA8
	{
		uint8 r = 0;
		uint8 g = 0;
		uint8 b = 0;
		uint8 a = 0;

		bool isTransparent() const { return a < 16; }
		bool operator == (RGBA8 const& rhs) const
		{
			return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
		}
		bool operator != (RGBA8 const& rhs) const
		{
			return !(*this == rhs);
		}
	};

	class HybridImageUpscaler
	{
	public:
		enum class PixelType : uint8
		{
			None,
			Line,
			Fill,
		};

		struct Anchor
		{
			int gridX = 0;
			int gridY = 0;
			int screenX = 0;
			int screenY = 0;
			uint8 connects = 0;
			bool bEndpoint = false;
		};

		struct DebugSegment
		{
			int x0 = 0;
			int y0 = 0;
			int x1 = 0;
			int y1 = 0;
		};

		struct ContourPoint
		{
			int x = 0;
			int y = 0;
		};

		struct ContourEdge
		{
			ContourPoint from;
			ContourPoint to;
			RGBA8 color;
			bool bUsed = false;
		};

		struct ContourSegment
		{
			ContourPoint from;
			ContourPoint to;
			RGBA8 color;
		};

		void setScale(int sx, int sy)
		{
			mScaleX = std::max(1, sx);
			mScaleY = std::max(1, sy);
		}

		bool buildFromPixels(int width, int height, RGBA8 const* pixels, TArray<RGBA8>& outPixels)
		{
			if (width <= 0 || height <= 0 || pixels == nullptr)
				return false;

			mWidth = width;
			mHeight = height;
			mOutputWidth = width * mScaleX;
			mOutputHeight = height * mScaleY;
			mSource.assign(pixels, pixels + width * height);
			mType.clear();
			mType.resize(width * height, PixelType::None);
			mAnchors.clear();
			mAnchors.resize(width * height, Anchor{});
			mDebugSegments.clear();
			mBoundarySegmentMask.clear();
			mBoundarySegmentMask.resize(width * height, 0);

			classifyPixels();
			buildAnchors();
			projectBoundaryAnchorsToRasterSegments();
			detectLineEndings();
			connectLineAnchors();
			connectFillAnchors();
			collectRasterBoundarySegments();
			expandBoundaryAnchorsWithoutSegments();

			outPixels.clear();
			outPixels.resize(mOutputWidth * mOutputHeight, RGBA8{ 0, 0, 0, 0 });
			renderGraph(outPixels);
			for (int i = 0; i < std::max(mScaleX, mScaleY); ++i)
			{
				enhance(outPixels, 0);
				clipHorizontalOuterBoundary(outPixels);
			}
			return true;
		}

		int getOutputWidth() const { return mOutputWidth; }
		int getOutputHeight() const { return mOutputHeight; }

		bool buildAnchorDebugImage(int debugScale, TArray<RGBA8>& outPixels, int& outWidth, int& outHeight) const
		{
			if (mWidth <= 0 || mHeight <= 0 || mAnchors.size() != mSource.size())
				return false;

			debugScale = std::max(1, debugScale);
			outWidth = mWidth * debugScale;
			outHeight = mHeight * debugScale;
			outPixels.clear();
			outPixels.resize(outWidth * outHeight, RGBA8{ 0, 0, 0, 255 });

			auto debugIndex = [outWidth](int x, int y)
			{
				return y * outWidth + x;
			};
			auto isDebugInside = [outWidth, outHeight](int x, int y)
			{
				return unsigned(x) < unsigned(outWidth) && unsigned(y) < unsigned(outHeight);
			};
			auto mapX = [this, debugScale, outWidth](int x)
			{
				return std::max(0, std::min(outWidth - 1, (x * debugScale + mScaleX / 2) / mScaleX));
			};
			auto mapY = [this, debugScale, outHeight](int y)
			{
				return std::max(0, std::min(outHeight - 1, (y * debugScale + mScaleY / 2) / mScaleY));
			};
			auto setPixel = [&](int x, int y, RGBA8 color)
			{
				if (isDebugInside(x, y))
				{
					outPixels[debugIndex(x, y)] = color;
				}
			};
			auto drawDebugLine = [&](int x0, int y0, int x1, int y1, RGBA8 color)
			{
				int dx = std::abs(x1 - x0);
				int dy = std::abs(y1 - y0);
				int sx = (x0 < x1) ? 1 : -1;
				int sy = (y0 < y1) ? 1 : -1;
				int err = dx - dy;
				for (;;)
				{
					setPixel(x0, y0, color);
					if (x0 == x1 && y0 == y1)
						break;

					int e2 = 2 * err;
					if (e2 > -dy)
					{
						err -= dy;
						x0 += sx;
					}
					if (e2 < dx)
					{
						err += dx;
						y0 += sy;
					}
				}
			};
			auto drawDebugSquare = [&](int cx, int cy, int radius, RGBA8 color)
			{
				for (int y = cy - radius; y <= cy + radius; ++y)
				{
					for (int x = cx - radius; x <= cx + radius; ++x)
					{
						setPixel(x, y, color);
					}
				}
			};

			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					RGBA8 color = mSource[index(x, y)];
					if (color.isTransparent())
						continue;

					RGBA8 dimColor{ uint8(color.r / 3), uint8(color.g / 3), uint8(color.b / 3), 255 };
					for (int oy = 0; oy < debugScale; ++oy)
					{
						for (int ox = 0; ox < debugScale; ++ox)
						{
							setPixel(x * debugScale + ox, y * debugScale + oy, dimColor);
						}
					}
				}
			}

			RGBA8 connectionColor{ 0, 220, 255, 255 };
			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (mSource[idx].isTransparent())
						continue;

					int dirs[] = { 2, 3, 4, 5 };
					for (int dir : dirs)
					{
						int nx = x + DirDX(dir);
						int ny = y + DirDY(dir);
						if (!isInside(nx, ny))
							continue;

						int nidx = index(nx, ny);
						int opp = (dir + 4) % 8;
						if (mSource[nidx].isTransparent())
							continue;
						if (!getConnect(mAnchors[idx], dir) && !getConnect(mAnchors[nidx], opp))
							continue;

						drawDebugLine(mapX(mAnchors[idx].screenX), mapY(mAnchors[idx].screenY),
						              mapX(mAnchors[nidx].screenX), mapY(mAnchors[nidx].screenY),
						              connectionColor);
					}
				}
			}

			RGBA8 anchorColor{ 255, 64, 64, 255 };
			for (int idx = 0; idx < int(mAnchors.size()); ++idx)
			{
				if (mSource[idx].isTransparent())
					continue;

				int x = mapX(mAnchors[idx].screenX);
				int y = mapY(mAnchors[idx].screenY);
				setPixel(x, y, anchorColor);
			}

			RGBA8 edgeSegmentColor{ 255, 64, 255, 255 };
			RGBA8 endpointColor{ 255, 255, 255, 255 };
			for (DebugSegment const& segment : mDebugSegments)
			{
				int x0 = mapX(segment.x0);
				int y0 = mapY(segment.y0);
				int x1 = mapX(segment.x1);
				int y1 = mapY(segment.y1);
				drawDebugLine(x0, y0, x1, y1, edgeSegmentColor);
				drawDebugSquare(x0, y0, 3, endpointColor);
				drawDebugSquare(x1, y1, 3, endpointColor);
			}

			return true;
		}

	private:
		static int DirDX(int dir)
		{
			static int const Table[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
			return Table[dir];
		}

		static int DirDY(int dir)
		{
			static int const Table[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };
			return Table[dir];
		}

		int index(int x, int y) const { return y * mWidth + x; }
		int outputIndex(int x, int y) const { return y * mOutputWidth + x; }

		bool isInside(int x, int y) const
		{
			return unsigned(x) < unsigned(mWidth) && unsigned(y) < unsigned(mHeight);
		}

		bool isOutputInside(int x, int y) const
		{
			return unsigned(x) < unsigned(mOutputWidth) && unsigned(y) < unsigned(mOutputHeight);
		}

		bool isLinePixel(int idx) const
		{
			return mType[idx] == PixelType::Line;
		}

		void setConnect(Anchor& anchor, int dir)
		{
			anchor.connects |= uint8(1 << dir);
		}

		bool getConnect(Anchor const& anchor, int dir) const
		{
			return !!(anchor.connects & uint8(1 << dir));
		}

		PixelType getResultType(int mode, int lineCount, int fillCount) const
		{
			if (mode == 1)
				return PixelType::Line;
			if (mode == 2)
				return PixelType::Fill;
			return (lineCount >= fillCount) ? PixelType::Line : PixelType::Fill;
		}

		void classifyPixels()
		{
			for (int i = 0; i < int(mSource.size()); ++i)
			{
				if (!mSource[i].isTransparent())
				{
					mType[i] = PixelType::Fill;
				}
			}
		}

		void buildAnchors()
		{
			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					Anchor& anchor = mAnchors[index(x, y)];
					anchor.gridX = x;
					anchor.gridY = y;
					anchor.screenX = x * mScaleX + mScaleX / 2;
					anchor.screenY = y * mScaleY + mScaleY / 2;
				}
			}
		}

		bool isBoundaryPixel(int x, int y) const
		{
			if (!isInside(x, y))
				return false;

			int idx = index(x, y);
			if (mSource[idx].isTransparent())
				return false;

			for (int dir = 0; dir < 8; ++dir)
			{
				int nx = x + DirDX(dir);
				int ny = y + DirDY(dir);
				if (!isInside(nx, ny) || mSource[index(nx, ny)].isTransparent())
					return true;
			}
			return false;
		}

		int mapGridXToScreen(float x) const
		{
			return std::max(0, std::min(mOutputWidth - 1, int(std::floor(x * float(mScaleX) - 0.5f + 0.0001f))));
		}

		int mapGridYToScreen(float y) const
		{
			return std::max(0, std::min(mOutputHeight - 1, int(std::floor(y * float(mScaleY) - 0.5f + 0.0001f))));
		}

		static bool isSamePoint(ContourPoint const& a, ContourPoint const& b)
		{
			return a.x == b.x && a.y == b.y;
		}

		static float pointDistanceSq(ContourPoint const& a, ContourPoint const& b)
		{
			float dx = float(a.x - b.x);
			float dy = float(a.y - b.y);
			return dx * dx + dy * dy;
		}

		static int64 contourCross(ContourPoint const& origin, ContourPoint const& a, ContourPoint const& b)
		{
			return int64(a.x - origin.x) * int64(b.y - origin.y) - int64(a.y - origin.y) * int64(b.x - origin.x);
		}

		static float polygonArea(std::vector<ContourPoint> const& points)
		{
			if (points.size() < 3)
				return 0.0f;

			int64 area2 = 0;
			int const count = int(points.size());
			for (int i = 0; i < count; ++i)
			{
				ContourPoint const& a = points[i];
				ContourPoint const& b = points[(i + 1) % count];
				area2 += int64(a.x) * int64(b.y) - int64(a.y) * int64(b.x);
			}
			return 0.5f * float(std::abs(area2));
		}

		static void buildConvexHull(std::vector<ContourPoint> points, std::vector<ContourPoint>& outHull)
		{
			outHull.clear();
			if (points.size() >= 2 && isSamePoint(points.front(), points.back()))
				points.pop_back();

			std::sort(points.begin(), points.end(),
				[](ContourPoint const& a, ContourPoint const& b)
				{
					return (a.x != b.x) ? (a.x < b.x) : (a.y < b.y);
				});
			points.erase(std::unique(points.begin(), points.end(),
				[](ContourPoint const& a, ContourPoint const& b)
				{
					return isSamePoint(a, b);
				}), points.end());

			if (points.size() <= 2)
			{
				outHull = points;
				return;
			}

			std::vector<ContourPoint> hull;
			hull.reserve(points.size() * 2);
			for (ContourPoint const& point : points)
			{
				while (hull.size() >= 2 && contourCross(hull[hull.size() - 2], hull[hull.size() - 1], point) <= 0)
					hull.pop_back();
				hull.push_back(point);
			}

			int const lowerSize = int(hull.size());
			for (int i = int(points.size()) - 2; i >= 0; --i)
			{
				ContourPoint const& point = points[i];
				while (int(hull.size()) > lowerSize && contourCross(hull[hull.size() - 2], hull[hull.size() - 1], point) <= 0)
					hull.pop_back();
				hull.push_back(point);
			}

			if (!hull.empty())
				hull.pop_back();
			outHull = hull;
		}

		static float pointSegmentDistanceSq(float px, float py, ContourPoint const& a, ContourPoint const& b, float* outT = nullptr)
		{
			float ax = float(a.x);
			float ay = float(a.y);
			float bx = float(b.x);
			float by = float(b.y);
			float vx = bx - ax;
			float vy = by - ay;
			float lenSq = vx * vx + vy * vy;
			float t = 0.0f;
			if (lenSq > 0.0001f)
			{
				t = ((px - ax) * vx + (py - ay) * vy) / lenSq;
				t = std::max(0.0f, std::min(1.0f, t));
			}

			if (outT)
				*outT = t;

			float dx = px - (ax + vx * t);
			float dy = py - (ay + vy * t);
			return dx * dx + dy * dy;
		}

		static void markRdpRange(std::vector<ContourPoint> const& points, int first, int last, float epsilonSq, std::vector<uint8>& keep)
		{
			if (last <= first + 1)
				return;

			float bestDistSq = -1.0f;
			int bestIndex = INDEX_NONE;
			for (int i = first + 1; i < last; ++i)
			{
				float distSq = pointSegmentDistanceSq(float(points[i].x), float(points[i].y), points[first], points[last]);
				if (distSq > bestDistSq)
				{
					bestDistSq = distSq;
					bestIndex = i;
				}
			}

			if (bestIndex != INDEX_NONE && bestDistSq > epsilonSq)
			{
				keep[bestIndex] = 1;
				markRdpRange(points, first, bestIndex, epsilonSq, keep);
				markRdpRange(points, bestIndex, last, epsilonSq, keep);
			}
		}

		static void simplifyOpenContour(std::vector<ContourPoint> const& inPoints, float epsilon, std::vector<ContourPoint>& outPoints)
		{
			outPoints.clear();
			if (inPoints.empty())
				return;
			if (inPoints.size() <= 2)
			{
				outPoints = inPoints;
				return;
			}

			std::vector<uint8> keep(inPoints.size(), 0);
			keep.front() = 1;
			keep.back() = 1;
			markRdpRange(inPoints, 0, int(inPoints.size()) - 1, epsilon * epsilon, keep);
			for (int i = 0; i < int(inPoints.size()); ++i)
			{
				if (keep[i])
					outPoints.push_back(inPoints[i]);
			}
		}

		static void simplifyClosedContour(std::vector<ContourPoint> points, float epsilon, std::vector<ContourPoint>& outPoints)
		{
			outPoints.clear();
			if (points.size() >= 2 && isSamePoint(points.front(), points.back()))
				points.pop_back();
			if (points.size() <= 3)
			{
				outPoints = points;
				return;
			}

			int indexA = 0;
			int indexB = 1;
			float bestDistSq = -1.0f;
			for (int i = 0; i < int(points.size()); ++i)
			{
				for (int j = i + 1; j < int(points.size()); ++j)
				{
					float distSq = pointDistanceSq(points[i], points[j]);
					if (distSq > bestDistSq)
					{
						bestDistSq = distSq;
						indexA = i;
						indexB = j;
					}
				}
			}
			if (indexA > indexB)
				std::swap(indexA, indexB);

			std::vector<ContourPoint> chainA;
			std::vector<ContourPoint> chainB;
			for (int i = indexA; i <= indexB; ++i)
				chainA.push_back(points[i]);
			for (int i = indexB; i < int(points.size()); ++i)
				chainB.push_back(points[i]);
			for (int i = 0; i <= indexA; ++i)
				chainB.push_back(points[i]);

			std::vector<ContourPoint> simplifiedA;
			std::vector<ContourPoint> simplifiedB;
			simplifyOpenContour(chainA, epsilon, simplifiedA);
			simplifyOpenContour(chainB, epsilon, simplifiedB);

			outPoints = simplifiedA;
			for (int i = 1; i + 1 < int(simplifiedB.size()); ++i)
				outPoints.push_back(simplifiedB[i]);
		}

		static void mergeShallowContourCorners(std::vector<ContourPoint>& points, bool bClosed)
		{
			static float constexpr MaxShallowCornerDistance = 1.35f;
			static float constexpr MinStraightDot = 0.74f;
			if (points.size() < (bClosed ? 4u : 3u))
				return;

			bool bChanged = true;
			while (bChanged && points.size() >= (bClosed ? 4u : 3u))
			{
				bChanged = false;
				int const count = int(points.size());
				int const first = bClosed ? 0 : 1;
				int const last = bClosed ? count : count - 1;
				for (int i = first; i < last; ++i)
				{
					int const prev = (i + count - 1) % count;
					int const next = (i + 1) % count;
					ContourPoint const& p0 = points[prev];
					ContourPoint const& p1 = points[i];
					ContourPoint const& p2 = points[next];

					float vx0 = float(p1.x - p0.x);
					float vy0 = float(p1.y - p0.y);
					float vx1 = float(p2.x - p1.x);
					float vy1 = float(p2.y - p1.y);
					float len0 = std::sqrt(vx0 * vx0 + vy0 * vy0);
					float len1 = std::sqrt(vx1 * vx1 + vy1 * vy1);
					if (len0 <= 0.001f || len1 <= 0.001f)
						continue;

					float dot = (vx0 * vx1 + vy0 * vy1) / (len0 * len1);
					if (dot < MinStraightDot)
						continue;

					float distSq = pointSegmentDistanceSq(float(p1.x), float(p1.y), p0, p2);
					if (distSq > MaxShallowCornerDistance * MaxShallowCornerDistance)
						continue;

					points.erase(points.begin() + i);
					bChanged = true;
					break;
				}
			}
		}

		void addExposedContourEdges(std::vector<ContourEdge>& edges) const
		{
			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (mSource[idx].isTransparent())
						continue;

					RGBA8 color = mSource[idx];
					auto isTransparentAt = [&](int tx, int ty)
					{
						return !isInside(tx, ty) || mSource[index(tx, ty)].isTransparent();
					};
					auto addEdge = [&](int x0, int y0, int x1, int y1)
					{
						edges.push_back(ContourEdge{ ContourPoint{ x0, y0 }, ContourPoint{ x1, y1 }, color, false });
					};

					if (isTransparentAt(x, y - 1))
						addEdge(x, y, x + 1, y);
					if (isTransparentAt(x + 1, y))
						addEdge(x + 1, y, x + 1, y + 1);
					if (isTransparentAt(x, y + 1))
						addEdge(x + 1, y + 1, x, y + 1);
					if (isTransparentAt(x - 1, y))
						addEdge(x, y + 1, x, y);
				}
			}
		}

		void buildContourSegments(std::vector<ContourSegment>& outSegments) const
		{
			static float constexpr SimplifyEpsilon = 0.85f;

			std::vector<ContourEdge> edges;
			addExposedContourEdges(edges);
			for (int edgeIndex = 0; edgeIndex < int(edges.size()); ++edgeIndex)
			{
				if (edges[edgeIndex].bUsed)
					continue;

				std::vector<ContourPoint> contour;
				ContourEdge const& firstEdge = edges[edgeIndex];
				RGBA8 color = firstEdge.color;
				ContourPoint start = firstEdge.from;
				ContourPoint current = firstEdge.to;
				edges[edgeIndex].bUsed = true;
				contour.push_back(start);
				contour.push_back(current);

				for (;;)
				{
					if (isSamePoint(current, start))
						break;

					int nextIndex = INDEX_NONE;
					for (int candidateIndex = 0; candidateIndex < int(edges.size()); ++candidateIndex)
					{
						if (edges[candidateIndex].bUsed)
							continue;
						if (edges[candidateIndex].color != color)
							continue;
						if (!isSamePoint(edges[candidateIndex].from, current))
							continue;

						nextIndex = candidateIndex;
						break;
					}

					if (nextIndex == INDEX_NONE)
						break;

					edges[nextIndex].bUsed = true;
					current = edges[nextIndex].to;
					contour.push_back(current);
				}

				if (contour.size() < 3)
					continue;

				std::vector<ContourPoint> simplified;
				bool const bClosed = isSamePoint(contour.front(), contour.back());
				std::vector<ContourPoint> simplifySource = contour;
				if (bClosed)
				{
					std::vector<ContourPoint> hull;
					buildConvexHull(contour, hull);
					float const contourArea = polygonArea(contour);
					float const hullArea = polygonArea(hull);
					if (hull.size() >= 3 && hullArea > 0.001f && contourArea / hullArea > 0.86f)
					{
						simplifySource = hull;
					}
				}

				if (bClosed)
					simplifyClosedContour(simplifySource, SimplifyEpsilon, simplified);
				else
					simplifyOpenContour(simplifySource, SimplifyEpsilon, simplified);
				mergeShallowContourCorners(simplified, bClosed);

				if (simplified.size() < 2)
					continue;

				int const numSegmentPoints = int(simplified.size());
				int const numSegments = bClosed ? numSegmentPoints : numSegmentPoints - 1;
				for (int i = 0; i < numSegments; ++i)
				{
					int next = (i + 1) % numSegmentPoints;
					if (!isSamePoint(simplified[i], simplified[next]))
						outSegments.push_back(ContourSegment{ simplified[i], simplified[next], color });
				}
			}
		}

		void projectAnchorsToContourSegments(std::vector<ContourSegment> const& segments)
		{
			static float constexpr MaxDistance = 2.25f;
			float const maxDistanceSq = MaxDistance * MaxDistance;

			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (!isBoundaryPixel(x, y))
						continue;

					float px = float(x) + 0.5f;
					float py = float(y) + 0.5f;
					float bestDistSq = maxDistanceSq;
					float bestT = 0.0f;
					ContourSegment const* bestSegment = nullptr;
					for (ContourSegment const& segment : segments)
					{
						if (segment.color != mSource[idx])
							continue;

						float t = 0.0f;
						float distSq = pointSegmentDistanceSq(px, py, segment.from, segment.to, &t);
						if (distSq < bestDistSq)
						{
							bestDistSq = distSq;
							bestT = t;
							bestSegment = &segment;
						}
					}

					if (!bestSegment)
						continue;

					float sx = float(bestSegment->from.x) + float(bestSegment->to.x - bestSegment->from.x) * bestT;
					float sy = float(bestSegment->from.y) + float(bestSegment->to.y - bestSegment->from.y) * bestT;

					Anchor& anchor = mAnchors[idx];
					float screenX = sx * float(mScaleX) - 0.5f;
					float screenY = sy * float(mScaleY) - 0.5f;
					anchor.screenX = std::max(0, std::min(mOutputWidth - 1, int(screenX + 0.5f)));
					anchor.screenY = std::max(0, std::min(mOutputHeight - 1, int(screenY + 0.5f)));
					mBoundarySegmentMask[idx] = 1;
				}
			}
		}

		void snapEndpointAnchorsToContourSegments(std::vector<ContourSegment> const& segments)
		{
			static float constexpr MaxEndpointDistance = 2.0f;
			float const maxEndpointDistanceSq = MaxEndpointDistance * MaxEndpointDistance;

			struct SegmentEndpoint
			{
				ContourPoint point;
				RGBA8 color;
			};

			std::vector<SegmentEndpoint> endpoints;
			for (ContourSegment const& segment : segments)
			{
				auto addEndpoint = [&](ContourPoint const& point)
				{
					for (SegmentEndpoint const& endpoint : endpoints)
					{
						if (endpoint.color == segment.color && isSamePoint(endpoint.point, point))
							return;
					}
					endpoints.push_back(SegmentEndpoint{ point, segment.color });
				};

				addEndpoint(segment.from);
				addEndpoint(segment.to);
			}

			TArray<uint8> snapped;
			snapped.resize(mWidth * mHeight, 0);
			for (SegmentEndpoint const& endpoint : endpoints)
			{
				int bestIdx = INDEX_NONE;
				float bestDistSq = maxEndpointDistanceSq;
				for (int y = 0; y < mHeight; ++y)
				{
					for (int x = 0; x < mWidth; ++x)
					{
						int idx = index(x, y);
						if (snapped[idx] || !isBoundaryPixel(x, y))
							continue;
						if (mSource[idx] != endpoint.color)
							continue;

						float dx = (float(x) + 0.5f) - float(endpoint.point.x);
						float dy = (float(y) + 0.5f) - float(endpoint.point.y);
						float distSq = dx * dx + dy * dy;
						if (distSq < bestDistSq)
						{
							bestDistSq = distSq;
							bestIdx = idx;
						}
					}
				}

				if (bestIdx == INDEX_NONE)
					continue;

				float screenX = float(endpoint.point.x) * float(mScaleX) - 0.5f;
				float screenY = float(endpoint.point.y) * float(mScaleY) - 0.5f;
				Anchor& anchor = mAnchors[bestIdx];
				anchor.screenX = std::max(0, std::min(mOutputWidth - 1, int(screenX + 0.5f)));
				anchor.screenY = std::max(0, std::min(mOutputHeight - 1, int(screenY + 0.5f)));
				mBoundarySegmentMask[bestIdx] = 1;
				snapped[bestIdx] = 1;
			}
		}

		void projectBoundaryAnchorsToRasterSegments()
		{
			static int constexpr Radius = 4;
			static int constexpr MinPoints = 4;
			static float constexpr MaxPerpRatio = 0.18f;

			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (!isBoundaryPixel(x, y))
						continue;

					RGBA8 color = mSource[idx];
					int count = 0;
					float sumX = 0.0f;
					float sumY = 0.0f;
					for (int yy = y - Radius; yy <= y + Radius; ++yy)
					{
						for (int xx = x - Radius; xx <= x + Radius; ++xx)
						{
							if (!isInside(xx, yy) || !isBoundaryPixel(xx, yy))
								continue;
							if (mSource[index(xx, yy)] != color)
								continue;

							sumX += xx * float(mScaleX) + mScaleX * 0.5f;
							sumY += yy * float(mScaleY) + mScaleY * 0.5f;
							++count;
						}
					}

					if (count < MinPoints)
						continue;

					float meanX = sumX / count;
					float meanY = sumY / count;
					float covXX = 0.0f;
					float covXY = 0.0f;
					float covYY = 0.0f;
					for (int yy = y - Radius; yy <= y + Radius; ++yy)
					{
						for (int xx = x - Radius; xx <= x + Radius; ++xx)
						{
							if (!isInside(xx, yy) || !isBoundaryPixel(xx, yy))
								continue;
							if (mSource[index(xx, yy)] != color)
								continue;

							float px = xx * float(mScaleX) + mScaleX * 0.5f;
							float py = yy * float(mScaleY) + mScaleY * 0.5f;
							float dx = px - meanX;
							float dy = py - meanY;
							covXX += dx * dx;
							covXY += dx * dy;
							covYY += dy * dy;
						}
					}

					float trace = covXX + covYY;
					float diff = covXX - covYY;
					float root = std::sqrt(std::max(0.0f, diff * diff + 4.0f * covXY * covXY));
					float alongVar = 0.5f * (trace + root);
					float perpVar = 0.5f * (trace - root);
					if (alongVar <= 1.0f || perpVar > alongVar * MaxPerpRatio)
						continue;

					float angle = 0.5f * std::atan2(2.0f * covXY, covXX - covYY);
					float dirX = std::cos(angle);
					float dirY = std::sin(angle);
					float centerX = x * float(mScaleX) + mScaleX * 0.5f;
					float centerY = y * float(mScaleY) + mScaleY * 0.5f;
					float t = (centerX - meanX) * dirX + (centerY - meanY) * dirY;
					float projX = meanX + t * dirX;
					float projY = meanY + t * dirY;

					float minX = x * float(mScaleX);
					float minY = y * float(mScaleY);
					float maxX = minX + float(mScaleX - 1);
					float maxY = minY + float(mScaleY - 1);
					Anchor& anchor = mAnchors[idx];
					anchor.screenX = int(std::max(minX, std::min(maxX, projX)) + 0.5f);
					anchor.screenY = int(std::max(minY, std::min(maxY, projY)) + 0.5f);
				}
			}
		}

		bool calcBoundaryLineDirection(int x, int y, float& outDirX, float& outDirY) const
		{
			static int constexpr Radius = 4;
			static int constexpr MinPoints = 4;
			static float constexpr MaxPerpRatio = 0.24f;

			if (!isBoundaryPixel(x, y))
				return false;

			RGBA8 color = mSource[index(x, y)];
			int count = 0;
			float sumX = 0.0f;
			float sumY = 0.0f;
			for (int yy = y - Radius; yy <= y + Radius; ++yy)
			{
				for (int xx = x - Radius; xx <= x + Radius; ++xx)
				{
					if (!isInside(xx, yy) || !isBoundaryPixel(xx, yy))
						continue;
					if (mSource[index(xx, yy)] != color)
						continue;

					sumX += float(xx);
					sumY += float(yy);
					++count;
				}
			}

			if (count < MinPoints)
				return false;

			float meanX = sumX / count;
			float meanY = sumY / count;
			float covXX = 0.0f;
			float covXY = 0.0f;
			float covYY = 0.0f;
			for (int yy = y - Radius; yy <= y + Radius; ++yy)
			{
				for (int xx = x - Radius; xx <= x + Radius; ++xx)
				{
					if (!isInside(xx, yy) || !isBoundaryPixel(xx, yy))
						continue;
					if (mSource[index(xx, yy)] != color)
						continue;

					float dx = float(xx) - meanX;
					float dy = float(yy) - meanY;
					covXX += dx * dx;
					covXY += dx * dy;
					covYY += dy * dy;
				}
			}

			float trace = covXX + covYY;
			float diff = covXX - covYY;
			float root = std::sqrt(std::max(0.0f, diff * diff + 4.0f * covXY * covXY));
			float alongVar = 0.5f * (trace + root);
			float perpVar = 0.5f * (trace - root);
			if (alongVar <= 1.0f || perpVar > alongVar * MaxPerpRatio)
				return false;

			float angle = 0.5f * std::atan2(2.0f * covXY, covXX - covYY);
			outDirX = std::cos(angle);
			outDirY = std::sin(angle);
			if (outDirX < 0.0f)
			{
				outDirX = -outDirX;
				outDirY = -outDirY;
			}
			return true;
		}

		void collectRasterBoundarySegments()
		{
			std::vector<ContourSegment> segments;
			buildContourSegments(segments);
			projectAnchorsToContourSegments(segments);
			snapEndpointAnchorsToContourSegments(segments);

			for (ContourSegment const& segment : segments)
			{
				mDebugSegments.push_back(DebugSegment{
					mapGridXToScreen(float(segment.from.x)),
					mapGridYToScreen(float(segment.from.y)),
					mapGridXToScreen(float(segment.to.x)),
					mapGridYToScreen(float(segment.to.y)),
				});
			}
		}

		void expandBoundaryAnchorsWithoutSegments()
		{
			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (!isBoundaryPixel(x, y) || mBoundarySegmentMask[idx])
						continue;

					float normalX = 0.0f;
					float normalY = 0.0f;
					for (int dir = 0; dir < 8; ++dir)
					{
						int nx = x + DirDX(dir);
						int ny = y + DirDY(dir);
						if (!isInside(nx, ny) || mSource[index(nx, ny)].isTransparent())
						{
							normalX += float(DirDX(dir));
							normalY += float(DirDY(dir));
						}
					}

					float normalLen = std::sqrt(normalX * normalX + normalY * normalY);
					if (normalLen <= 0.001f)
						continue;

					normalX /= normalLen;
					normalY /= normalLen;
					float contourOffset = 0.5f * float(std::min(mScaleX, mScaleY) - 1);
					float minX = x * float(mScaleX);
					float minY = y * float(mScaleY);
					float maxX = minX + float(mScaleX - 1);
					float maxY = minY + float(mScaleY - 1);

					Anchor& anchor = mAnchors[idx];
					float projX = float(anchor.screenX) + normalX * contourOffset;
					float projY = float(anchor.screenY) + normalY * contourOffset;
					anchor.screenX = int(std::max(minX, std::min(maxX, projX)) + 0.5f);
					anchor.screenY = int(std::max(minY, std::min(maxY, projY)) + 0.5f);
				}
			}
		}

		void clipHorizontalOuterBoundary(TArray<RGBA8>& outPixels)
		{
			for (int y = 0; y < mHeight; ++y)
			{
				int x = 0;
				while (x < mWidth)
				{
					int startX = x;
					while (startX < mWidth)
					{
						if (!mSource[index(startX, y)].isTransparent() &&
						    (y + 1 >= mHeight || mSource[index(startX, y + 1)].isTransparent()))
						{
							break;
						}
						++startX;
					}

					if (startX >= mWidth)
						break;

					int endX = startX;
					while (endX + 1 < mWidth &&
					       !mSource[index(endX + 1, y)].isTransparent() &&
					       (y + 1 >= mHeight || mSource[index(endX + 1, y + 1)].isTransparent()))
					{
						++endX;
					}

					if (endX - startX + 1 >= 3)
					{
						int left = mAnchors[index(startX, y)].screenX;
						int right = mAnchors[index(endX, y)].screenX;
						if (left > right)
							std::swap(left, right);

						int clipY = mAnchors[index(startX, y)].screenY;
						for (int sx = startX + 1; sx <= endX; ++sx)
						{
							clipY = std::max(clipY, mAnchors[index(sx, y)].screenY);
						}

						left = std::max(0, left - mScaleX / 2);
						right = std::min(mOutputWidth - 1, right + mScaleX / 2);
						for (int oy = clipY + 1; oy < mOutputHeight; ++oy)
						{
							for (int ox = left; ox <= right; ++ox)
							{
								int outIdx = outputIndex(ox, oy);
								outPixels[outIdx] = RGBA8{ 0, 0, 0, 0 };
								if (mOutputType.size() == outPixels.size())
								{
									mOutputType[outIdx] = PixelType::None;
								}
							}
						}
					}

					x = endX + 1;
				}
			}
		}

		void detectLineEndings()
		{
			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (!isLinePixel(idx))
						continue;

					int lineNeighbours = 0;
					for (int dir = 0; dir < 8; ++dir)
					{
						int nx = x + DirDX(dir);
						int ny = y + DirDY(dir);
						if (isInside(nx, ny) && isLinePixel(index(nx, ny)))
						{
							++lineNeighbours;
						}
					}

					mAnchors[idx].bEndpoint = lineNeighbours <= 1;
				}
			}
		}

		void connectLineAnchors()
		{
			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (!isLinePixel(idx))
						continue;

					for (int dir = 0; dir < 8; ++dir)
					{
						int nx = x + DirDX(dir);
						int ny = y + DirDY(dir);
						if (isInside(nx, ny) && isLinePixel(index(nx, ny)))
						{
							setConnect(mAnchors[idx], dir);
						}
					}
				}
			}
		}

		void connectFillAnchors()
		{
			static int const DiagDirs[4] = { 1, 3, 5, 7 };
			static int const CardA[4] = { 0, 4, 4, 0 };
			static int const CardB[4] = { 2, 2, 6, 6 };

			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (isLinePixel(idx) || mSource[idx].isTransparent())
						continue;

					Anchor& anchor = mAnchors[idx];
					for (int dir = 0; dir < 8; dir += 2)
					{
						int nx = x + DirDX(dir);
						int ny = y + DirDY(dir);
						if (!isInside(nx, ny))
						{
							setConnect(anchor, dir);
							continue;
						}
						int nidx = index(nx, ny);
						if (isLinePixel(nidx) || !(mSource[nidx] == mSource[idx]))
							continue;
						setConnect(anchor, dir);
					}

					for (int i = 0; i < 4; ++i)
					{
						int dir = DiagDirs[i];
						int nx = x + DirDX(dir);
						int ny = y + DirDY(dir);
						if (!isInside(nx, ny))
							continue;
						int nidx = index(nx, ny);
						if (isLinePixel(nidx) || !(mSource[nidx] == mSource[idx]))
							continue;

						int ax = x + DirDX(CardA[i]);
						int ay = y + DirDY(CardA[i]);
						int bx = x + DirDX(CardB[i]);
						int by = y + DirDY(CardB[i]);
						if (isInside(ax, ay) && isInside(bx, by))
						{
							if (isLinePixel(index(ax, ay)) && isLinePixel(index(bx, by)))
								continue;
						}

						setConnect(anchor, dir);
					}
				}
			}
		}

		void drawLine(TArray<RGBA8>& outPixels, int x0, int y0, int x1, int y1, RGBA8 colorA, RGBA8 colorB, PixelType type)
		{
			int dx = std::abs(x1 - x0);
			int dy = std::abs(y1 - y0);
			int sx = (x0 < x1) ? 1 : -1;
			int sy = (y0 < y1) ? 1 : -1;
			int total = std::max(dx, dy) + 1;
			int half = total / 2;
			int err = dx - dy;
			int step = 0;

			for (;;)
			{
				if (isOutputInside(x0, y0))
				{
					int idx = outputIndex(x0, y0);
					outPixels[idx] = (step < half) ? colorA : colorB;
					if (mOutputType.size() == outPixels.size())
						mOutputType[idx] = type;
				}

				if (x0 == x1 && y0 == y1)
					break;

				int e2 = 2 * err;
				if (e2 > -dy)
				{
					err -= dy;
					x0 += sx;
				}
				if (e2 < dx)
				{
					err += dx;
					y0 += sy;
				}
				++step;
			}
		}

		void renderGraph(TArray<RGBA8>& outPixels)
		{
			mOutputType.clear();
			mOutputType.resize(outPixels.size(), PixelType::None);

			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (mSource[idx].isTransparent())
						continue;

					tryDrawConnection(outPixels, x, y, 2);
					tryDrawConnection(outPixels, x, y, 3);
					tryDrawConnection(outPixels, x, y, 4);
					tryDrawConnection(outPixels, x, y, 5);
				}
			}

			for (int y = 0; y < mHeight; ++y)
			{
				for (int x = 0; x < mWidth; ++x)
				{
					int idx = index(x, y);
					if (mSource[idx].isTransparent())
						continue;

					Anchor const& anchor = mAnchors[idx];
					for (int dir = 0; dir < 8; ++dir)
					{
						int nx = x + DirDX(dir);
						int ny = y + DirDY(dir);
						if (isInside(nx, ny) || !getConnect(anchor, dir))
							continue;

						int steps = mOutputWidth + mOutputHeight;
						if (DirDX(dir) > 0)
							steps = std::min(steps, mOutputWidth - 1 - anchor.screenX);
						if (DirDX(dir) < 0)
							steps = std::min(steps, anchor.screenX);
						if (DirDY(dir) > 0)
							steps = std::min(steps, mOutputHeight - 1 - anchor.screenY);
						if (DirDY(dir) < 0)
							steps = std::min(steps, anchor.screenY);
						int ex = anchor.screenX + DirDX(dir) * steps;
						int ey = anchor.screenY + DirDY(dir) * steps;

						drawLine(outPixels, anchor.screenX, anchor.screenY, ex, ey, mSource[idx], mSource[idx], mType[idx]);
					}
				}
			}

			for (int idx = 0; idx < int(mAnchors.size()); ++idx)
			{
				if (mSource[idx].isTransparent())
					continue;

				Anchor const& anchor = mAnchors[idx];
				if (isOutputInside(anchor.screenX, anchor.screenY))
				{
					int outIdx = outputIndex(anchor.screenX, anchor.screenY);
					outPixels[outIdx] = mSource[idx];
					mOutputType[outIdx] = mType[idx];
				}
			}
		}

		void tryDrawConnection(TArray<RGBA8>& outPixels, int x, int y, int dir)
		{
			int nx = x + DirDX(dir);
			int ny = y + DirDY(dir);
			if (!isInside(nx, ny))
				return;

			int idx = index(x, y);
			int nidx = index(nx, ny);
			if (mSource[idx].isTransparent() || mSource[nidx].isTransparent())
				return;

			int opp = (dir + 4) % 8;
			if (!getConnect(mAnchors[idx], dir) && !getConnect(mAnchors[nidx], opp))
				return;

			PixelType type = (mType[idx] == PixelType::Line || mType[nidx] == PixelType::Line) ? PixelType::Line : PixelType::Fill;
			drawLine(outPixels,
			         mAnchors[idx].screenX, mAnchors[idx].screenY,
			         mAnchors[nidx].screenX, mAnchors[nidx].screenY,
			         mSource[idx], mSource[nidx], type);
		}

		bool isEligibleForEnhance(int mode, PixelType type) const
		{
			if (mode == 1)
				return type == PixelType::Line;
			if (mode == 2)
				return type != PixelType::Line;
			return true;
		}

		void enhance(TArray<RGBA8>& outPixels, int mode)
		{
			TArray<RGBA8> src = outPixels;
			TArray<PixelType> srcType = mOutputType;

			for (int y = 0; y < mOutputHeight; ++y)
			{
				for (int x = 0; x < mOutputWidth; ++x)
				{
					int idx = outputIndex(x, y);
					bool bBackground = src[idx].isTransparent();
					bool bOverrideFill = (mode == 1 && srcType[idx] == PixelType::Fill);
					if (!bBackground && !bOverrideFill)
						continue;

					int lineNeighbours = 0;
					for (int dy = -1; dy <= 1; ++dy)
					{
						for (int dx = -1; dx <= 1; ++dx)
						{
							if (dx == 0 && dy == 0)
								continue;
							int nx = x + dx;
							int ny = y + dy;
							if (isOutputInside(nx, ny) && srcType[outputIndex(nx, ny)] == PixelType::Line)
								++lineNeighbours;
						}
					}

					struct Vote
					{
						RGBA8 color;
						int count = 0;
					};

					Vote votes[8];
					int voteCount = 0;
					int lineCount = 0;
					int fillCount = 0;
					bool bSuppressFill = lineNeighbours >= 3;

					for (int dy = -1; dy <= 1; ++dy)
					{
						for (int dx = -1; dx <= 1; ++dx)
						{
							if (dx == 0 && dy == 0)
								continue;
							int nx = x + dx;
							int ny = y + dy;
							if (!isOutputInside(nx, ny))
								continue;

							int nidx = outputIndex(nx, ny);
							if (src[nidx].isTransparent())
								continue;
							if (!isEligibleForEnhance(mode, srcType[nidx]))
								continue;
							if (bSuppressFill && srcType[nidx] != PixelType::Line)
								continue;

							int voteIndex = -1;
							for (int i = 0; i < voteCount; ++i)
							{
								if (votes[i].color == src[nidx])
								{
									voteIndex = i;
									break;
								}
							}
							if (voteIndex == -1 && voteCount < 8)
							{
								voteIndex = voteCount++;
								votes[voteIndex].color = src[nidx];
								votes[voteIndex].count = 0;
							}
							if (voteIndex != -1)
								++votes[voteIndex].count;

							if (srcType[nidx] == PixelType::Line)
								++lineCount;
							else
								++fillCount;
						}
					}

					int minVotes = (mode == 1) ? 1 : 2;
					int bestCount = minVotes;
					RGBA8 tieColors[8];
					int tieCount = 0;
					for (int i = 0; i < voteCount; ++i)
					{
						if (votes[i].count > bestCount)
						{
							bestCount = votes[i].count;
							tieCount = 1;
							tieColors[0] = votes[i].color;
						}
						else if (votes[i].count == bestCount && bestCount > minVotes && tieCount < 8)
						{
							tieColors[tieCount++] = votes[i].color;
						}
					}

					if (tieCount > 0)
					{
						if (tieCount == 1)
						{
							outPixels[idx] = tieColors[0];
						}
						else
						{
							int r = 0;
							int g = 0;
							int b = 0;
							int a = 0;
							for (int i = 0; i < tieCount; ++i)
							{
								r += tieColors[i].r;
								g += tieColors[i].g;
								b += tieColors[i].b;
								a += tieColors[i].a;
							}
							outPixels[idx] = RGBA8{ uint8(r / tieCount), uint8(g / tieCount), uint8(b / tieCount), uint8(a / tieCount) };
						}
						mOutputType[idx] = getResultType(mode, lineCount, fillCount);
					}
				}
			}

			for (int y = 0; y < mOutputHeight; ++y)
			{
				for (int x = 0; x < mOutputWidth; ++x)
				{
					int idx = outputIndex(x, y);
					RGBA8 color = src[idx];
					if (color.isTransparent() || !isEligibleForEnhance(mode, srcType[idx]))
						continue;

					bool bHasSameNeighbour = false;
					for (int dy = -1; dy <= 1 && !bHasSameNeighbour; ++dy)
					{
						for (int dx = -1; dx <= 1 && !bHasSameNeighbour; ++dx)
						{
							if (dx == 0 && dy == 0)
								continue;

							int nx = x + dx;
							int ny = y + dy;
							if (!isOutputInside(nx, ny))
								continue;

							int nidx = outputIndex(nx, ny);
							if (src[nidx] == color && isEligibleForEnhance(mode, srcType[nidx]))
							{
								bHasSameNeighbour = true;
							}
						}
					}

					if (bHasSameNeighbour)
						continue;

					for (int dy = -1; dy <= 1; ++dy)
					{
						for (int dx = -1; dx <= 1; ++dx)
						{
							if (dx == 0 && dy == 0)
								continue;

							int nx = x + dx;
							int ny = y + dy;
							if (!isOutputInside(nx, ny))
								continue;

							int nidx = outputIndex(nx, ny);
							if (src[nidx].isTransparent())
							{
								outPixels[nidx] = color;
								mOutputType[nidx] = srcType[idx];
							}
						}
					}
				}
			}
		}

		int mWidth = 0;
		int mHeight = 0;
		int mScaleX = 6;
		int mScaleY = 6;
		int mOutputWidth = 0;
		int mOutputHeight = 0;
		TArray<RGBA8> mSource;
		TArray<PixelType> mType;
		TArray<PixelType> mOutputType;
		TArray<Anchor> mAnchors;
		TArray<DebugSegment> mDebugSegments;
		TArray<uint8> mBoundarySegmentMask;
	};

	class Stage : public StageBase
		        , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

			::Global::GUI().cleanupWidget();
			WidgetUtility::CreateDevFrame();
			return true;
		}

		bool setupRenderResource(ERenderSystem systemName) override
		{
			using namespace Render;

			mSourceTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, SourceW, SourceH).AddFlags(TCF_RenderTarget | TCF_CreateSRV));
			VERIFY_RETURN_FALSE(mSourceTexture.isValid());

			mSourceFrameBuffer = RHICreateFrameBuffer();
			VERIFY_RETURN_FALSE(mSourceFrameBuffer.isValid());
			mSourceFrameBuffer->addTexture(*mSourceTexture);

			mUpscaler.setScale(Scale, Scale);

			drawSourceTexture();
			mbNeedReadback = true;
			return true;
		}

		void preShutdownRenderSystem(bool bReInit) override
		{
			mAnchorDebugTexture.release();
			mUpscaledTexture.release();
			mSourceFrameBuffer.release();
			mSourceTexture.release();
		}

		void onRender(float dFrame) override
		{
			using namespace Render;
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			Vec2i screenSize = ::Global::GetScreenSize();

			if (mbNeedReadback)
			{
				readSourceAndUpscale();
				mbNeedReadback = false;
			}

			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.08f, 0.09f, 0.11f, 1.0f), 1);
			RHISetViewport(commandList, 0, 0, screenSize.x, screenSize.y);

			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();
			g.setViewportSize(screenSize.x, screenSize.y);
			g.beginRender();

			Vector2 pos(20, 40);
			RenderUtility::SetBrush(g, EColor::White);
			g.setSampler(TStaticSamplerState<ESampler::Point, ESampler::Clamp, ESampler::Clamp>::GetRHI());
			if (mSourceTexture)
			{
				g.drawTexture(*mSourceTexture, pos, Vector2(SourceW * float(Scale), SourceH * float(Scale)));
			}

			if (mUpscaledTexture)
			{
				g.drawTexture(*mUpscaledTexture, Vector2(300, 40), Vector2(float(mUpscaledTexture->getSizeX()), float(mUpscaledTexture->getSizeY())));
			}
			if (mAnchorDebugTexture)
			{
				g.drawTexture(*mAnchorDebugTexture, Vector2(20, 270), Vector2(float(mAnchorDebugTexture->getSizeX()), float(mAnchorDebugTexture->getSizeY())));
			}

			RenderUtility::SetPen(g, EColor::White);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(pos, Vector2(SourceW * float(Scale), SourceH * float(Scale)));
			if (mUpscaledTexture)
			{
				g.drawRect(Vector2(300, 40), Vector2(float(mUpscaledTexture->getSizeX()), float(mUpscaledTexture->getSizeY())));
			}
			if (mAnchorDebugTexture)
			{
				g.drawRect(Vector2(20, 270), Vector2(float(mAnchorDebugTexture->getSizeX()), float(mAnchorDebugTexture->getSizeY())));
			}

			g.drawText(Vector2(20, 12), "Source texture rendered by RHIGraphics2D");
			g.drawText(Vector2(300, 12), "Anchor upscale after RHIReadTexture");
			g.drawText(Vector2(20, 246), "Anchor debug display x2");
			g.endRender();
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown() && msg.getCode() == EKeyCode::R)
			{
				drawSourceTexture();
				mbNeedReadback = true;
				return MsgReply::Handled();
			}
			return BaseClass::onKey(msg);
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1100;
			systemConfigs.screenHeight = 720;
		}

	private:
		void drawSourceTexture()
		{
			using namespace Render;
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHIGraphics2D& g = ::Global::GetRHIGraphics2D();

			RHISetFrameBuffer(commandList, mSourceFrameBuffer);
			RHIClearRenderTargets(commandList, EClearBits::Color, &LinearColor(0, 0, 0, 0), 1);
			RHISetViewport(commandList, 0, 0, SourceW, SourceH);
			g.setViewportSize(SourceW, SourceH);
			g.beginRender();

			RenderUtility::SetPen(g, EColor::Null);
			g.setBrush(Color3ub(246, 220, 94));
			Vector2 triangle[] =
			{
				Vector2(20, 4),
				Vector2(5, 23),
				Vector2(35, 23),
			};
			g.drawPolygon(triangle, ARRAY_SIZE(triangle));

			g.endRender();
			g.flush();

			RHISetFrameBuffer(commandList, nullptr);
			RHIResourceTransition(commandList, { mSourceTexture }, EResourceTransition::Present);
			RHIFlushCommand(commandList);

			Vec2i screenSize = ::Global::GetScreenSize();
			g.setViewportSize(screenSize.x, screenSize.y);
		}

		void readSourceAndUpscale()
		{
			using namespace Render;

			TArray<uint8> readData;
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			RHIResourceTransition(commandList, { mSourceTexture }, EResourceTransition::Present);
			RHIFlushCommand(commandList);
			RHIReadTexture(*mSourceTexture, ETexture::RGBA8, 0, readData);
			if (readData.size() < SourceW * SourceH * 4)
				return;

			TArray<RGBA8> sourcePixels;
			sourcePixels.resize(SourceW * SourceH);
			for (int i = 0; i < int(sourcePixels.size()); ++i)
			{
				uint8 const* p = readData.data() + i * 4;
				uint8 alpha = p[3];
				if (alpha < 16 && (p[0] || p[1] || p[2]))
				{
					alpha = 255;
				}
				sourcePixels[i] = RGBA8{ p[0], p[1], p[2], alpha };
			}

			TArray<RGBA8> upscaledPixels;
			if (!mUpscaler.buildFromPixels(SourceW, SourceH, sourcePixels.data(), upscaledPixels))
				return;

			mUpscaledTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, mUpscaler.getOutputWidth(), mUpscaler.getOutputHeight()).AddFlags(TCF_CreateSRV), upscaledPixels.data());

			TArray<RGBA8> anchorDebugPixels;
			int debugWidth = 0;
			int debugHeight = 0;
			if (mUpscaler.buildAnchorDebugImage(Scale * 2, anchorDebugPixels, debugWidth, debugHeight))
			{
				mAnchorDebugTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, debugWidth, debugHeight).AddFlags(TCF_CreateSRV), anchorDebugPixels.data());
			}
		}

		static int constexpr SourceW = 40;
		static int constexpr SourceH = 28;
		static int constexpr Scale = 6;

		HybridImageUpscaler mUpscaler;
		RHITexture2DRef mSourceTexture;
		RHITexture2DRef mUpscaledTexture;
		RHITexture2DRef mAnchorDebugTexture;
		RHIFrameBufferRef mSourceFrameBuffer;
		bool mbNeedReadback = false;
	};

	REGISTER_STAGE_ENTRY("Image Upscaale", Stage, EExecGroup::FeatureDev, "Render|RHI");
}
