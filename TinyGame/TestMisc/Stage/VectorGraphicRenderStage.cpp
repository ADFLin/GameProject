
#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"
#include "RHI/RHICommand.h"
#include "RHI/RHIGraphics2D.h"
#include "RHI/Font.h"
#include "Math/GeometryPrimitive.h"
#include "Renderer/RenderTransform2D.h"
#include "FileSystem.h"

#include <cstring>
#include "ProfileSystem.h"

namespace VGR
{
	using namespace Render;

	struct CurveSegment
	{
		Vector2 start;
		Vector2 end;
		Vector2 control;

		int findIntersection(float y, float outT[2]) const
		{
			float a = start.y + end.y - 2 * control.y;
			float b = control.y - start.y;
			float c = start.y - y;
			if (Math::Abs(a) < 1e-6f)
			{
				if (Math::Abs(b) < 1e-6f)
					return 0;

				outT[0] = outT[1] = -c / (2 * b);
				return 1;
			}

			float det = b * b - a * c;
			if (det < 0)
			{
				return 0;
			}

			det = Math::Sqrt(det);

			outT[0] = (-b + det) / a;
			outT[1] = (-b - det) / a;
			return 2;
		}
		bool trySplitYMonotonic(CurveSegment& other)
		{
			float denom = start.y - 2 * control.y + end.y;
			if ( Math::Abs(denom) < 1e-5 )
				return false;

			float t = (start.y - control.y) / denom;

			if (t <= 0 || t >= 1 )
				return false;

			split(t, other);
			return true;
		}

		Vector2 get(float t) const
		{
			return Math::BezierLerp(start, control, end, t);
		}
		void drawDebug(RHIGraphics2D& g) const
		{
			RenderUtility::SetBrush(g, EColor::Null);

			Vector2 size = Vector2(4,4);
			RenderUtility::SetPen(g, EColor::Red);
			g.drawRect(start - 0.5 * size, size);
			g.drawRect(end - 0.5 * size, size);
			RenderUtility::SetPen(g, EColor::Blue);
			g.drawRect(control - 0.5 * size, size);
		}

		void draw(RHIGraphics2D& g, int num) const
		{
			float delta = 1.0f / num;
			Vector2 p0 = get(0);

			float t = 0;
			for (int i = 0; i < num; ++i)
			{
				t += delta;
				Vector2 p1 = get(t);
				g.drawLine(p0, p1);
				p0 = p1;
			}
		}

		void split(float s, CurveSegment& other)
		{
			Vector2 a = Math::LinearLerp(start, control, s);
			Vector2 b = Math::LinearLerp(control, end, s);
			Vector2 c = Math::LinearLerp(a, b, s);

			other.start = c;
			other.control = b;
			other.end = end;

			control = a;
			end = c;
		}

		static void Split(CurveSegment const& src, float s, CurveSegment& left, CurveSegment& right)
		{
			Vector2 a = Math::LinearLerp(src.start, src.control, s);
			Vector2 b = Math::LinearLerp(src.control, src.end, s);
			Vector2 c = Math::LinearLerp(a, b, s);

			left.start = src.start;
			left.control = a;
			left.end = c;

			right.start = c;
			right.control = b;
			right.end = src.end;
		}
	};


	class TrueTypeFileLoader
	{
	public:
		struct FontMetrics
		{
			uint16 unitsPerEm = 0;
			int16 ascender = 0;
			int16 descender = 0;
			int16 lineGap = 0;
		};

		struct GlyphData
		{
			struct Contour
			{
				int segmentOffset = 0;
				int segmentCount = 0;
			};

			uint32 codepoint = 0;
			uint16 glyphIndex = 0;
			TArray< CurveSegment > segments;
			TArray< Contour > contours;
			Math::TAABBox< Vector2 > bound;
			bool bHasOutline = false;
			uint16 advanceWidth = 0;
			int16 leftSideBearing = 0;
			int16 rightSideBearing = 0;
		};

		struct KerningPair
		{
			uint32 firstCodepoint = 0;
			uint32 secondCodepoint = 0;
			int16 value = 0;
		};

		FontMetrics const& getFontMetrics() const { return mFontMetrics; }
		TArray< KerningPair > const& getKerningPairs() const { return mKerningPairs; }

		bool load(char const* path)
		{
			mBuffer.clear();
			mTables.clear();
			mGlyphOffsets.clear();
			mGlyphCodepoints.clear();
			mKerningPairs.clear();
			mCMap = CMapInfo();
			mGlyphTableOffset = 0;
			mNumGlyphs = 0;
			mNumHMetrics = 0;
			mFontMetrics = FontMetrics();
			mIndexToLocFormat = 0;

			if (!FFileUtility::LoadToBuffer(path, mBuffer))
				return false;

			if (mBuffer.size() < OffsetTableSize)
				return false;

			size_t offset = OffsetTableNumTablesOffset;
			uint16 numTables = readU16(offset);
			if (!checkRange(OffsetTableSize, TableRecordSize * numTables))
				return false;

			mTables.reserve(numTables);
			for (uint16 i = 0; i < numTables; ++i)
			{
				offset = OffsetTableSize + TableRecordSize * i;
				Table table;
				for (int tagIndex = 0; tagIndex < TableTagSize; ++tagIndex)
				{
					table.tag[tagIndex] = char(readU8(offset));
				}

				readU32(offset); // checksum
				table.offset = readU32(offset);
				table.length = readU32(offset);
				if (!checkRange(table.offset, table.length))
					return false;

				mTables.push_back(table);
			}

			Table const* head = findTable("head");
			Table const* maxp = findTable("maxp");
			Table const* hhea = findTable("hhea");
			Table const* loca = findTable("loca");
			if (!head || !maxp || !hhea || !loca || !findTable("glyf") || !findTable("cmap") || !findTable("hmtx"))
				return false;

			if (head->length < HeadMinSize || maxp->length < MaxpMinSize || hhea->length < HheaMinSize)
				return false;

			offset = head->offset + HeadUnitsPerEmOffset;
			mFontMetrics.unitsPerEm = readU16(offset);
			offset = head->offset + HeadIndexToLocFormatOffset;
			mIndexToLocFormat = readS16(offset);
			offset = maxp->offset + MaxpNumGlyphsOffset;
			mNumGlyphs = readU16(offset);
			offset = hhea->offset + HheaNumHMetricsOffset;
			mNumHMetrics = readU16(offset);
			offset = hhea->offset + HheaAscenderOffset;
			mFontMetrics.ascender = readS16(offset);
			mFontMetrics.descender = readS16(offset);
			mFontMetrics.lineGap = readS16(offset);
			if (!loadGlyphOffsets(*loca) || !chooseCMap())
				return false;

			buildGlyphCodepointMap();
			return loadKerningPairs();
		}

		bool loadGlyph(uint32 codepoint, GlyphData& outGlyph) const
		{
			uint16 glyphIndex = getGlyphIndex(codepoint);
			if (glyphIndex == 0)
				return false;

			outGlyph = GlyphData();
			outGlyph.codepoint = codepoint;
			outGlyph.glyphIndex = glyphIndex;
			getGlyphMetric(glyphIndex, outGlyph.advanceWidth, outGlyph.leftSideBearing);

			Math::TAABBox< Vector2 > bound;
			bound.invalidate();
			if (!appendGlyphSegments(glyphIndex, RenderTransform2D::Identity(), outGlyph.segments, &outGlyph.contours, 0, &bound))
				return false;

			outGlyph.bound = bound;
			outGlyph.bHasOutline = !outGlyph.segments.empty();
			if (outGlyph.bHasOutline)
			{
				outGlyph.rightSideBearing = int16(outGlyph.advanceWidth - outGlyph.leftSideBearing - int16(bound.max.x - bound.min.x));
			}
			else
			{
				outGlyph.bound.setZero();
				outGlyph.rightSideBearing = int16(outGlyph.advanceWidth - outGlyph.leftSideBearing);
			}
			return true;
		}

	private:
		static constexpr uint32 OffsetTableSize = 12;
		static constexpr uint32 OffsetTableNumTablesOffset = 4;
		static constexpr uint32 TableRecordSize = 16;
		static constexpr uint32 TableTagSize = 4;

		static constexpr uint32 HeadUnitsPerEmOffset = 18;
		static constexpr uint32 HeadIndexToLocFormatOffset = 50;
		static constexpr uint32 HeadMinSize = 54;
		static constexpr uint32 MaxpNumGlyphsOffset = 4;
		static constexpr uint32 MaxpMinSize = 6;
		static constexpr uint32 HheaAscenderOffset = 4;
		static constexpr uint32 HheaNumHMetricsOffset = 34;
		static constexpr uint32 HheaMinSize = 36;

		static constexpr uint32 GlyphHeaderSize = 10;
		static constexpr int GlyphBoundValueCount = 4;
		static constexpr uint32 GlyphInstructionLengthSize = 2;
		static constexpr uint32 ShortLocaEntrySize = 2;
		static constexpr uint32 LongLocaEntrySize = 4;
		static constexpr uint32 ShortLocaScale = 2;

		static constexpr uint32 CMapHeaderSize = 4;
		static constexpr uint32 CMapRecordSize = 8;
		static constexpr uint16 CMapPlatformWindows = 3;
		static constexpr uint16 CMapEncodingSymbol = 0;
		static constexpr uint16 CMapEncodingUnicodeBmp = 1;
		static constexpr uint16 CMapEncodingUnicodeFull = 10;
		static constexpr uint16 CMapFormat4 = 4;
		static constexpr uint16 CMapFormat12 = 12;
		static constexpr uint32 CMapFormat4EndCodeOffset = 14;
		static constexpr uint32 CMapFormat4ReservedPadSize = 2;
		static constexpr uint32 CMapFormat12GroupOffset = 16;
		static constexpr uint32 CMapFormat12GroupSize = 12;
		static constexpr uint32 UnicodeBmpMax = 0xffff;

		static constexpr uint8 GlyphFlagOnCurve = 0x01;
		static constexpr uint8 GlyphFlagXShortVector = 0x02;
		static constexpr uint8 GlyphFlagYShortVector = 0x04;
		static constexpr uint8 GlyphFlagRepeat = 0x08;
		static constexpr uint8 GlyphFlagXSameOrPositive = 0x10;
		static constexpr uint8 GlyphFlagYSameOrPositive = 0x20;

		static constexpr uint32 HMetricSize = 4;
		static constexpr uint32 LeftSideBearingSize = 2;
		static constexpr float F2Dot14Scale = 16384.0f;
		static constexpr int MaxCompositeDepth = 8;

		static constexpr uint16 KernVersion0 = 0;
		static constexpr uint32 KernHeaderSize = 4;
		static constexpr uint32 KernSubtableHeaderSize = 6;
		static constexpr uint32 KernFormat0HeaderSize = 8;
		static constexpr uint32 KernFormat0PairSize = 6;
		static constexpr uint16 KernFormat0 = 0;
		static constexpr uint16 KernCoverageHorizontal = 0x0001;
		static constexpr uint16 KernCoverageMinimum = 0x0002;
		static constexpr uint16 KernCoverageCrossStream = 0x0004;
		static constexpr uint16 KernCoverageOverride = 0x0008;
		static constexpr uint16 KernCoverageFormatShift = 8;

		struct Table
		{
			char tag[TableTagSize];
			uint32 offset;
			uint32 length;
		};

		struct CMapInfo
		{
			uint32 offset = 0;
			uint16 format = 0;
		};

		struct GlyphPoint
		{
			Vector2 pos;
			bool bOnCurve;
		};

		static Vector2 MidPoint(Vector2 const& a, Vector2 const& b)
		{
			return 0.5f * (a + b);
		}

		static void AddBound(Math::TAABBox< Vector2 >& bound, Math::TAABBox< Vector2 > const& srcBound, RenderTransform2D const& transform)
		{
			Vector2 p0 = transform.transformPosition(srcBound.min);
			Vector2 p1 = transform.transformPosition(Vector2(srcBound.max.x, srcBound.min.y));
			Vector2 p2 = transform.transformPosition(srcBound.max);
			Vector2 p3 = transform.transformPosition(Vector2(srcBound.min.x, srcBound.max.y));
			bound.addPoint(p0);
			bound.addPoint(p1);
			bound.addPoint(p2);
			bound.addPoint(p3);
		}

		static void AddSegment(TArray< CurveSegment >& outSegments, CurveSegment const& segment)
		{
			outSegments.push_back(segment);
		}

		static void AddLine(Vector2 const& start, Vector2 const& end, TArray< CurveSegment >& outSegments)
		{
			if ((end - start).length2() < 1e-6f)
				return;

			CurveSegment segment;
			segment.start = start;
			segment.control = MidPoint(start, end);
			segment.end = end;
			AddSegment(outSegments, segment);
		}

		static void AddQuad(Vector2 const& start, Vector2 const& control, Vector2 const& end, TArray< CurveSegment >& outSegments)
		{
			if ((end - start).length2() < 1e-6f)
				return;

			CurveSegment segment;
			segment.start = start;
			segment.control = control;
			segment.end = end;
			AddSegment(outSegments, segment);
		}

		bool appendGlyphSegments(uint16 glyphIndex, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours, int depth, Math::TAABBox< Vector2 >* bound) const
		{
			if (depth > MaxCompositeDepth || glyphIndex + 1 >= mGlyphOffsets.size())
				return false;

			uint32 glyphStart = mGlyphTableOffset + mGlyphOffsets[glyphIndex];
			uint32 glyphEnd = mGlyphTableOffset + mGlyphOffsets[glyphIndex + 1];
			if (glyphEnd <= glyphStart)
				return true;
			if (!checkRange(glyphStart, glyphEnd - glyphStart) || glyphEnd - glyphStart < GlyphHeaderSize)
				return false;

			size_t pos = glyphStart;
			int16 numContours = readS16(pos);
			Math::TAABBox< Vector2 > glyphBound;
			glyphBound.min.x = float(readS16(pos));
			glyphBound.min.y = float(readS16(pos));
			glyphBound.max.x = float(readS16(pos));
			glyphBound.max.y = float(readS16(pos));
			if (bound && numContours != 0)
			{
				AddBound(*bound, glyphBound, transform);
			}

			if (numContours >= 0)
				return appendSimpleGlyph(glyphStart, glyphEnd, numContours, transform, outSegments, outContours);

			return appendCompositeGlyph(glyphStart, glyphEnd, transform, outSegments, outContours, depth, bound);
		}

		bool appendSimpleGlyph(uint32 glyphStart, uint32 glyphEnd, int numContours, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours) const
		{
			if (numContours == 0)
				return true;

			size_t pos = glyphStart + sizeof(int16) + sizeof(int16) * GlyphBoundValueCount;
			if (pos + sizeof(uint16) * numContours + GlyphInstructionLengthSize > glyphEnd)
				return false;

			TArray< uint16 > endPts;
			endPts.resize(numContours);
			for (int i = 0; i < numContours; ++i)
			{
				endPts[i] = readU16(pos);
			}

			uint16 numPoints = endPts.back() + 1;
			uint16 instructionLength = readU16(pos);
			pos += instructionLength;
			if (pos > glyphEnd)
				return false;

			TArray< uint8 > flags;
			flags.reserve(numPoints);
			while (flags.size() < numPoints)
			{
				if (pos >= glyphEnd)
					return false;

				uint8 flag = readU8(pos);
				flags.push_back(flag);
				if (flag & GlyphFlagRepeat)
				{
					if (pos >= glyphEnd)
						return false;

					uint8 repeatCount = readU8(pos);
					for (uint8 i = 0; i < repeatCount; ++i)
					{
						if (flags.size() >= numPoints)
							return false;

						flags.push_back(flag);
					}
				}
			}

			TArray< GlyphPoint > points;
			points.resize(numPoints);
			int16 curX = 0;
			for (uint16 i = 0; i < numPoints; ++i)
			{
				uint8 flag = flags[i];
				int16 dx = 0;
				if (flag & GlyphFlagXShortVector)
				{
					if (pos >= glyphEnd)
						return false;

					dx = int16(readU8(pos));
					if (!(flag & GlyphFlagXSameOrPositive))
						dx = -dx;
				}
				else if (!(flag & GlyphFlagXSameOrPositive))
				{
					if (pos + sizeof(int16) > glyphEnd)
						return false;

					dx = readS16(pos);
				}

				curX += dx;
				points[i].pos.x = float(curX);
				points[i].bOnCurve = !!(flag & GlyphFlagOnCurve);
			}

			int16 curY = 0;
			for (uint16 i = 0; i < numPoints; ++i)
			{
				uint8 flag = flags[i];
				int16 dy = 0;
				if (flag & GlyphFlagYShortVector)
				{
					if (pos >= glyphEnd)
						return false;

					dy = int16(readU8(pos));
					if (!(flag & GlyphFlagYSameOrPositive))
						dy = -dy;
				}
				else if (!(flag & GlyphFlagYSameOrPositive))
				{
					if (pos + sizeof(int16) > glyphEnd)
						return false;

					dy = readS16(pos);
				}

				curY += dy;
				points[i].pos.y = float(curY);
			}

			uint16 contourStart = 0;
			for (int contourIndex = 0; contourIndex < numContours; ++contourIndex)
			{
				uint16 contourEnd = endPts[contourIndex];
				if (!appendContour(points, contourStart, contourEnd, transform, outSegments, outContours))
					return false;

				contourStart = contourEnd + 1;
			}

			return true;
		}

		bool appendContour(TArray< GlyphPoint > const& points, uint16 contourStart, uint16 contourEnd, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours) const
		{
			int num = int(contourEnd - contourStart + 1);
			if (num <= 0)
				return true;

			int segmentOffset = int(outSegments.size());

			auto GetPoint = [&](int index) -> GlyphPoint const&
			{
				return points[contourStart + (index % num)];
			};

			int startIndex = 0;
			int consumed = 0;
			Vector2 start;
			if (GetPoint(0).bOnCurve)
			{
				start = GetPoint(0).pos;
				startIndex = 1;
				consumed = 1;
			}
			else if (GetPoint(num - 1).bOnCurve)
			{
				start = GetPoint(num - 1).pos;
			}
			else
			{
				start = MidPoint(GetPoint(num - 1).pos, GetPoint(0).pos);
			}

			Vector2 transformedStart = transform.transformPosition(start);
			Vector2 cur = transformedStart;
			int index = startIndex;
			while (consumed < num)
			{
				GlyphPoint const& p0 = GetPoint(index);
				if (p0.bOnCurve)
				{
					Vector2 end = transform.transformPosition(p0.pos);
					AddLine(cur, end, outSegments);
					cur = end;
					++index;
					++consumed;
				}
				else
				{
					GlyphPoint const& p1 = GetPoint(index + 1);
					Vector2 control = transform.transformPosition(p0.pos);
					Vector2 end = p1.bOnCurve ? transform.transformPosition(p1.pos) : transform.transformPosition(MidPoint(p0.pos, p1.pos));
					AddQuad(cur, control, end, outSegments);
					cur = end;
					if (p1.bOnCurve)
					{
						index += 2;
						consumed += 2;
					}
					else
					{
						++index;
						++consumed;
					}
				}
			}

			AddLine(cur, transformedStart, outSegments);
			int segmentCount = int(outSegments.size()) - segmentOffset;
			if (outContours && segmentCount > 0)
			{
				outContours->push_back({ segmentOffset, segmentCount });
			}
			return true;
		}

		bool appendCompositeGlyph(uint32 glyphStart, uint32 glyphEnd, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours, int depth, Math::TAABBox< Vector2 >* bound) const
		{
			enum
			{
				Arg1And2AreWords = 0x0001,
				ArgsAreXYValues = 0x0002,
				HaveUniformScale = 0x0008,
				MoreComponents = 0x0020,
				HaveXAndYScale = 0x0040,
				HaveTwoByTwo = 0x0080,
			};

			size_t pos = glyphStart + GlyphHeaderSize;
			bool bMore = true;
			while (bMore)
			{
				if (pos + sizeof(uint16) * 2 > glyphEnd)
					return false;

				uint16 flags = readU16(pos);
				uint16 componentGlyphIndex = readU16(pos);
				int arg1 = 0;
				int arg2 = 0;
				if (flags & Arg1And2AreWords)
				{
					if (pos + sizeof(int16) * 2 > glyphEnd)
						return false;

					arg1 = readS16(pos);
					arg2 = readS16(pos);
				}
				else
				{
					if (pos + sizeof(int8) * 2 > glyphEnd)
						return false;

					arg1 = int8(readU8(pos));
					arg2 = int8(readU8(pos));
				}

				RenderTransform2D component = RenderTransform2D::Identity();
				if (flags & ArgsAreXYValues)
				{
					component.P = Vector2(float(arg1), float(arg2));
				}
				else
				{
					return false;
				}

				if (flags & HaveUniformScale)
				{
					if (pos + sizeof(int16) > glyphEnd)
						return false;

					float scale = readF2Dot14(pos);
					component.M = Matrix2::Scale(Vector2(scale, scale));
				}
				else if (flags & HaveXAndYScale)
				{
					if (pos + sizeof(int16) * 2 > glyphEnd)
						return false;

					float scaleX = readF2Dot14(pos);
					float scaleY = readF2Dot14(pos);
					component.M = Matrix2::Scale(Vector2(scaleX, scaleY));
				}
				else if (flags & HaveTwoByTwo)
				{
					if (pos + sizeof(int16) * 4 > glyphEnd)
						return false;

					float m00 = readF2Dot14(pos);
					float m01 = readF2Dot14(pos);
					float m10 = readF2Dot14(pos);
					float m11 = readF2Dot14(pos);
					component.M = Matrix2(m00, m10, m01, m11);
				}

				if (!appendGlyphSegments(componentGlyphIndex, component * transform, outSegments, outContours, depth + 1, bound))
					return false;

				bMore = !!(flags & MoreComponents);
			}

			return true;
		}

		bool loadGlyphOffsets(Table const& loca)
		{
			Table const* glyf = findTable("glyf");
			mGlyphTableOffset = glyf->offset;
			mGlyphOffsets.resize(size_t(mNumGlyphs) + 1);
			if (mIndexToLocFormat == 0)
			{
				if (loca.length < ShortLocaEntrySize * (uint32(mNumGlyphs) + 1))
					return false;

				size_t offset = loca.offset;
				for (uint16 i = 0; i <= mNumGlyphs; ++i)
				{
					mGlyphOffsets[i] = uint32(readU16(offset)) * ShortLocaScale;
				}
			}
			else
			{
				if (loca.length < LongLocaEntrySize * (uint32(mNumGlyphs) + 1))
					return false;

				size_t offset = loca.offset;
				for (uint16 i = 0; i <= mNumGlyphs; ++i)
				{
					mGlyphOffsets[i] = readU32(offset);
				}
			}

			return true;
		}

		bool chooseCMap()
		{
			Table const* cmap = findTable("cmap");
			if (!cmap || cmap->length < CMapHeaderSize)
				return false;

			size_t offset = cmap->offset;
			readU16(offset); // version
			uint16 numTables = readU16(offset);
			if (cmap->length < CMapHeaderSize + CMapRecordSize * numTables)
				return false;

			CMapInfo fallback;
			for (uint16 i = 0; i < numTables; ++i)
			{
				size_t record = cmap->offset + CMapHeaderSize + CMapRecordSize * i;
				uint16 platformID = readU16(record);
				uint16 encodingID = readU16(record);
				uint32 subtableOffset = cmap->offset + readU32(record);
				if (!checkRange(subtableOffset, sizeof(uint16)))
					continue;

				uint16 format = readU16At(subtableOffset);
				if (format != CMapFormat4 && format != CMapFormat12)
					continue;

				if (format == CMapFormat12 && platformID == CMapPlatformWindows && encodingID == CMapEncodingUnicodeFull)
				{
					mCMap = { subtableOffset, format };
					return true;
				}

				if (format == CMapFormat4 && platformID == CMapPlatformWindows && (encodingID == CMapEncodingUnicodeBmp || encodingID == CMapEncodingSymbol))
				{
					mCMap = { subtableOffset, format };
					return true;
				}

				if (fallback.offset == 0)
				{
					fallback = { subtableOffset, format };
				}
			}

			mCMap = fallback;
			return mCMap.offset != 0;
		}

		void buildGlyphCodepointMap()
		{
			mGlyphCodepoints.clear();
			mGlyphCodepoints.resize(mNumGlyphs, 0);

			if (mCMap.format == CMapFormat4)
			{
				buildGlyphCodepointMapFormat4();
			}
			else if (mCMap.format == CMapFormat12)
			{
				buildGlyphCodepointMapFormat12();
			}
		}

		void buildGlyphCodepointMapFormat4()
		{
			uint32 offset = mCMap.offset;
			size_t header = offset;
			readU16(header); // format
			uint16 length = readU16(header);
			if (!checkRange(offset, length) || length < CMapFormat4EndCodeOffset + sizeof(uint16))
				return;

			readU16(header); // language
			uint16 segCount = readU16(header) / sizeof(uint16);
			size_t endCodeOffset = offset + CMapFormat4EndCodeOffset;
			size_t startCodeOffset = endCodeOffset + sizeof(uint16) * segCount + CMapFormat4ReservedPadSize;
			if (!checkRange(startCodeOffset, sizeof(uint16) * segCount * 3))
				return;

			for (uint16 i = 0; i < segCount; ++i)
			{
				size_t segOffset = sizeof(uint16) * i;
				uint16 startCode = readU16At(startCodeOffset + segOffset);
				uint16 endCode = readU16At(endCodeOffset + segOffset);
				if (startCode == UnicodeBmpMax && endCode == UnicodeBmpMax)
					continue;

				for (uint32 codepoint = startCode; codepoint <= endCode; ++codepoint)
				{
					uint16 glyphIndex = getGlyphIndexFormat4(codepoint);
					if (glyphIndex != 0 && glyphIndex < mGlyphCodepoints.size() && mGlyphCodepoints[glyphIndex] == 0)
					{
						mGlyphCodepoints[glyphIndex] = codepoint;
					}
				}
			}
		}

		void buildGlyphCodepointMapFormat12()
		{
			uint32 offset = mCMap.offset;
			if (!checkRange(offset, CMapFormat12GroupOffset))
				return;

			size_t header = offset;
			readU16(header); // format
			readU16(header); // reserved
			readU32(header); // length
			readU32(header); // language
			uint32 numGroups = readU32(header);
			uint32 groupOffset = offset + CMapFormat12GroupOffset;
			if (!checkRange(groupOffset, CMapFormat12GroupSize * numGroups))
				return;

			for (uint32 i = 0; i < numGroups; ++i)
			{
				size_t record = groupOffset + CMapFormat12GroupSize * i;
				uint32 startCharCode = readU32(record);
				uint32 endCharCode = readU32(record);
				uint32 startGlyphID = readU32(record);
				for (uint32 codepoint = startCharCode; codepoint <= endCharCode; ++codepoint)
				{
					uint32 glyphIndex = startGlyphID + codepoint - startCharCode;
					if (glyphIndex != 0 && glyphIndex < mGlyphCodepoints.size() && mGlyphCodepoints[glyphIndex] == 0)
					{
						mGlyphCodepoints[glyphIndex] = codepoint;
					}
				}
			}
		}

		bool loadKerningPairs()
		{
			Table const* kern = findTable("kern");
			if (!kern)
				return true;
			if (kern->length < KernHeaderSize)
				return false;

			size_t pos = kern->offset;
			uint16 version = readU16(pos);
			if (version != KernVersion0)
				return true;

			uint16 numTables = readU16(pos);
			size_t subtable = pos;
			for (uint16 tableIndex = 0; tableIndex < numTables; ++tableIndex)
			{
				if (!checkRange(subtable, KernSubtableHeaderSize))
					return false;

				size_t subtableHeader = subtable;
				readU16(subtableHeader); // version
				uint16 length = readU16(subtableHeader);
				uint16 coverage = readU16(subtableHeader);
				if (length < KernSubtableHeaderSize || !checkRange(subtable, length))
					return false;

				uint16 format = coverage >> KernCoverageFormatShift;
				uint16 flags = coverage & 0xff;
				if (format == KernFormat0 &&
				    (flags & KernCoverageHorizontal) &&
				    !(flags & (KernCoverageMinimum | KernCoverageCrossStream | KernCoverageOverride)))
				{
					loadKerningFormat0(subtable + KernSubtableHeaderSize, length - KernSubtableHeaderSize);
				}

				subtable += length;
				if (subtable > kern->offset + kern->length)
					return false;
			}

			return true;
		}

		void loadKerningFormat0(size_t offset, uint32 length)
		{
			if (length < KernFormat0HeaderSize || !checkRange(offset, length))
				return;

			size_t pos = offset;
			uint16 numPairs = readU16(pos);
			readU16(pos); // searchRange
			readU16(pos); // entrySelector
			readU16(pos); // rangeShift
			if (length < KernFormat0HeaderSize + KernFormat0PairSize * uint32(numPairs))
				return;

			for (uint16 i = 0; i < numPairs; ++i)
			{
				uint16 leftGlyph = readU16(pos);
				uint16 rightGlyph = readU16(pos);
				int16 value = readS16(pos);
				if (value == 0)
					continue;

				if (leftGlyph >= mGlyphCodepoints.size() || rightGlyph >= mGlyphCodepoints.size())
					continue;

				uint32 firstCodepoint = mGlyphCodepoints[leftGlyph];
				uint32 secondCodepoint = mGlyphCodepoints[rightGlyph];
				if (firstCodepoint == 0 || secondCodepoint == 0)
					continue;

				mKerningPairs.push_back({ firstCodepoint, secondCodepoint, value });
			}
		}

		uint16 getGlyphIndex(uint32 codepoint) const
		{
			if (mCMap.format == CMapFormat4)
				return getGlyphIndexFormat4(codepoint);
			if (mCMap.format == CMapFormat12)
				return getGlyphIndexFormat12(codepoint);
			return 0;
		}

		uint16 getGlyphIndexFormat4(uint32 codepoint) const
		{
			if (codepoint > UnicodeBmpMax)
				return 0;

			uint32 offset = mCMap.offset;
			size_t header = offset;
			readU16(header); // format
			uint16 length = readU16(header);
			if (!checkRange(offset, length) || length < CMapFormat4EndCodeOffset + sizeof(uint16))
				return 0;

			readU16(header); // language
			uint16 segCount = readU16(header) / sizeof(uint16);
			size_t endCodeOffset = offset + CMapFormat4EndCodeOffset;
			size_t startCodeOffset = endCodeOffset + sizeof(uint16) * segCount + CMapFormat4ReservedPadSize;
			size_t idDeltaOffset = startCodeOffset + sizeof(uint16) * segCount;
			size_t idRangeOffsetOffset = idDeltaOffset + sizeof(uint16) * segCount;
			for (uint16 i = 0; i < segCount; ++i)
			{
				size_t segOffset = sizeof(uint16) * i;

				uint16 endCode = readU16At(endCodeOffset + segOffset);
				uint16 startCode = readU16At(startCodeOffset + segOffset);
				if (codepoint < startCode || codepoint > endCode)
					continue;

				int16 idDelta = readS16At(idDeltaOffset + segOffset);
				uint16 idRangeOffset = readU16At(idRangeOffsetOffset + segOffset);
				if (idRangeOffset == 0)
					return uint16((codepoint + idDelta) & UnicodeBmpMax);

				size_t glyphOffset = idRangeOffsetOffset + segOffset + idRangeOffset + sizeof(uint16) * (codepoint - startCode);
				if (!checkRange(glyphOffset, sizeof(uint16)))
					return 0;

				uint16 glyphIndex = readU16At(glyphOffset);
				if (glyphIndex == 0)
					return 0;

				return uint16((glyphIndex + idDelta) & UnicodeBmpMax);
			}

			return 0;
		}

		uint16 getGlyphIndexFormat12(uint32 codepoint) const
		{
			uint32 offset = mCMap.offset;
			if (!checkRange(offset, CMapFormat12GroupOffset))
				return 0;

			size_t header = offset;
			readU16(header); // format
			readU16(header); // reserved
			readU32(header); // length
			readU32(header); // language
			uint32 numGroups = readU32(header);
			uint32 groupOffset = offset + CMapFormat12GroupOffset;
			if (!checkRange(groupOffset, CMapFormat12GroupSize * numGroups))
				return 0;

			for (uint32 i = 0; i < numGroups; ++i)
			{
				size_t record = groupOffset + CMapFormat12GroupSize * i;
				uint32 startCharCode = readU32(record);
				uint32 endCharCode = readU32(record);
				if (codepoint < startCharCode || codepoint > endCharCode)
					continue;

				return uint16(readU32(record) + codepoint - startCharCode);
			}

			return 0;
		}

		void getGlyphMetric(uint16 glyphIndex, uint16& outAdvanceWidth, int16& outLeftSideBearing) const
		{
			Table const* hmtx = findTable("hmtx");
			if (!hmtx || mNumHMetrics == 0)
			{
				outAdvanceWidth = 0;
				outLeftSideBearing = 0;
				return;
			}

			uint16 metricIndex = Math::Min(glyphIndex, uint16(mNumHMetrics - 1));
			uint32 metricOffset = hmtx->offset + HMetricSize * metricIndex;
			if (!checkRange(metricOffset, HMetricSize))
			{
				outAdvanceWidth = 0;
				outLeftSideBearing = 0;
				return;
			}

			size_t metric = metricOffset;
			outAdvanceWidth = readU16(metric);
			outLeftSideBearing = readS16(metric);
			if (glyphIndex >= mNumHMetrics)
			{
				uint32 lsbOffset = hmtx->offset + HMetricSize * mNumHMetrics + LeftSideBearingSize * (glyphIndex - mNumHMetrics);
				if (checkRange(lsbOffset, LeftSideBearingSize))
				{
					size_t lsb = lsbOffset;
					outLeftSideBearing = readS16(lsb);
				}
			}
		}

		Table const* findTable(char const* tag) const
		{
			for (Table const& table : mTables)
			{
				if (std::memcmp(table.tag, tag, TableTagSize) == 0)
					return &table;
			}

			return nullptr;
		}

		bool checkRange(size_t offset, size_t size) const
		{
			return offset <= mBuffer.size() && size <= mBuffer.size() - offset;
		}

		uint8 readU8At(size_t offset) const
		{
			return mBuffer[offset];
		}

		uint8 readU8(size_t& offset) const
		{
			uint8 result = readU8At(size_t(offset));
			offset += sizeof(uint8);
			return result;
		}

		uint16 readU16At(size_t offset) const
		{
			return uint16((uint16(mBuffer[offset]) << 8) | uint16(mBuffer[offset + 1]));
		}

		uint16 readU16(size_t& offset) const
		{
			uint16 result = readU16At(size_t(offset));
			offset += sizeof(uint16);
			return result;
		}

		int16 readS16At(size_t offset) const
		{
			return int16(readU16At(size_t(offset)));
		}

		int16 readS16(size_t& offset) const
		{
			return int16(readU16(offset));
		}

		uint32 readU32At(size_t offset) const
		{
			return (uint32(mBuffer[offset]) << 24) | (uint32(mBuffer[offset + 1]) << 16) | (uint32(mBuffer[offset + 2]) << 8) | uint32(mBuffer[offset + 3]);
		}

		uint32 readU32(size_t& offset) const
		{
			uint32 result = readU32At(size_t(offset));
			offset += sizeof(uint32);
			return result;
		}

		float readF2Dot14At(size_t offset) const
		{
			return float(readS16At(size_t(offset))) / F2Dot14Scale;
		}

		float readF2Dot14(size_t& offset) const
		{
			return float(readS16(offset)) / F2Dot14Scale;
		}

		TArray< uint8 > mBuffer;
		TArray< Table > mTables;
		TArray< uint32 > mGlyphOffsets;
		TArray< uint32 > mGlyphCodepoints;
		TArray< KerningPair > mKerningPairs;
		CMapInfo mCMap;
		uint32 mGlyphTableOffset = 0;
		uint16 mNumGlyphs = 0;
		uint16 mNumHMetrics = 0;
		FontMetrics mFontMetrics;
		int16 mIndexToLocFormat = 0;
	};


	struct RasterizeSettings
	{
		float size = 1;
		Math::TAABBox< Vector2 > bound;
	};

	template< typename TFunc >
	static void Rasterize(RasterizeSettings const& settings, TArray< CurveSegment > const& segments, TFunc func)
	{
		struct Intersection
		{
			CurveSegment const* segment;
			float minY;
			float maxY;
			float posX;
			float t;
		};

		struct RasterSegment
		{
			CurveSegment const* segment;
			float minY;
			float maxY;
		};

		float halfSize = 0.5 * settings.size;
		Vector2 curPos;
		Vec2i pxIndex;
		curPos.y = settings.bound.min.y + halfSize;
		pxIndex.y = 0;

		TArray<Intersection> intersectionList;
		TArray<RasterSegment> rasterSegments;
		rasterSegments.reserve(segments.size());
		for (CurveSegment const& segment : segments)
		{
			RasterSegment rasterSegment;
			rasterSegment.segment = &segment;
			rasterSegment.minY = Math::Min(segment.start.y, Math::Min(segment.control.y, segment.end.y));
			rasterSegment.maxY = Math::Max(segment.start.y, Math::Max(segment.control.y, segment.end.y));
			rasterSegments.push_back(rasterSegment);
		}


		std::sort(rasterSegments.begin(), rasterSegments.end(),
			[](auto const& lhs, auto const& rhs)
		{
			return lhs.minY < rhs.minY;
		}
		);

		int indexRasterCheck = 0;
		TArray< RasterSegment* > activeSegments;
		activeSegments.reserve(rasterSegments.size());

		auto TestCount = [](Intersection& intersection, float yValue)
		{
			if (yValue < intersection.minY || yValue > intersection.maxY)
				return 0;

			float outT[2];
			if (!intersection.segment->findIntersection(yValue, outT))
				return 0;

			int index = (Math::Abs(outT[0] - intersection.t) <= Math::Abs(outT[1] - intersection.t)) ? 0 : 1;
			int count = 0;
			if (0 <= outT[index] && outT[index] <= 1.0f)
			{
				++count;
			}
			return count;
		};

		while (curPos.y < settings.bound.max.y + halfSize)
		{
			for (int i = 0; i < activeSegments.size(); )
			{
				if (activeSegments[i]->maxY < curPos.y)
				{
					activeSegments.removeIndexSwap(i);
				}
				else
				{
					++i;
				}
			}
			while (indexRasterCheck < rasterSegments.size())
			{
				if (rasterSegments[indexRasterCheck].minY > curPos.y)
					break;

				activeSegments.push_back(&rasterSegments[indexRasterCheck]);
				++indexRasterCheck;
			}


			intersectionList.clear();

			for (RasterSegment const* rasterSegment : activeSegments)
			{
				CurveSegment const& segment = *rasterSegment->segment;
				auto AddPosConditional = [&](float t)
				{
					if (0 <= t && t <= 1.0f)
					{
						intersectionList.push_back({ &segment, rasterSegment->minY, rasterSegment->maxY, segment.get(t).x, t });
					}
				};

				float outT[2];
				int num = segment.findIntersection(curPos.y, outT);
				if (num)
				{
					AddPosConditional(outT[0]);
					if (Math::Abs(outT[0] - outT[1]) > 1e-5)
					{
						AddPosConditional(outT[1]);
					}
				}
			}

			std::sort(intersectionList.begin(), intersectionList.end(),
				[](auto const& lhs, auto const& rhs)
			{
				return lhs.posX < rhs.posX;
			}
			);

			{
				int index = 0;
				while (index < intersectionList.size())
				{
					int nextIndex = index + 1;
					if (nextIndex >= intersectionList.size())
						break;

					if (Math::Abs(intersectionList[index].posX - intersectionList[nextIndex].posX) < 1e-4)
					{
						auto RunTest = [&](float yValue)
						{
							int countA = TestCount(intersectionList[index], yValue);
							int countB = TestCount(intersectionList[nextIndex], yValue);
							return countA == 0 && countB == 0;
						};
						float offset = 0.1 * settings.size;
						if (RunTest(curPos.y + offset) || RunTest(curPos.y - offset))
						{
							++index;
						}
						else
						{
							intersectionList.removeIndex(nextIndex);
						}
					}
					else
					{
						++index;
					}
				}
			}

			if (!intersectionList.empty())
			{
				int curIndex = 0;

				curPos.x = settings.bound.min.x + halfSize;
				pxIndex.x = 0;

				bool bInside = false;
				while (curPos.x < settings.bound.max.x + halfSize)
				{
					while (curIndex < intersectionList.size())
					{
						if (bInside)
						{
							if (curPos.x <= intersectionList[curIndex].posX)
								break;
						}
						else
						{
							if (curPos.x < intersectionList[curIndex].posX)
								break;
						}

						++curIndex;
						bInside = !bInside;
					}

					if (bInside)
					{
						func(pxIndex, curPos);
					}


					curPos.x += settings.size;
					pxIndex.x += 1;
				}
			}

			curPos.y += settings.size;
			pxIndex.y += 1;
		}
	}

	class TrueTypeCharDataProvider : public ICustomCharDataProvider
	{
	public:
		TrueTypeCharDataProvider() = default;

		TrueTypeCharDataProvider(HashString name, TrueTypeFileLoader& loader, int pixelSize, int sampleFactor = 4)
		{
			initialize(name, loader, pixelSize, sampleFactor);
		}

		bool initialize(HashString name, TrueTypeFileLoader& loader, int pixelSize, int sampleFactor = 4)
		{
			if (pixelSize <= 0 || sampleFactor <= 0)
				return false;

			mLoader = &loader;
			mPixelSize = pixelSize;
			mSampleFactor = sampleFactor;
			TrueTypeFileLoader::FontMetrics const& metrics = mLoader->getFontMetrics();
			if (metrics.unitsPerEm == 0)
			{
				mLoader = nullptr;
				return false;
			}

			mScale = float(pixelSize) / metrics.unitsPerEm;
			mFontHeight = Math::CeilToInt(float(metrics.ascender - metrics.descender + metrics.lineGap) * mScale);
			mBaseline = float(metrics.ascender) * mScale;
			return mFontHeight > 0;
		}

		bool getCharData(uint32 charWord, CharImageData& outData) override
		{
			if (!mLoader)
				return false;

			TrueTypeFileLoader::GlyphData glyph;
			if (!mLoader->loadGlyph(charWord, glyph))
				return false;

			int segmentCount = glyph.segments.size();
			for (int i = 0; i < segmentCount; ++i)
			{
				CurveSegment other;
				if (glyph.segments[i].trySplitYMonotonic(other))
				{
					glyph.segments.push_back(other);
				}
			}

			setupDesc(glyph, outData);
			outData.imageWidth = Math::Max(1, outData.width);
			outData.imageHeight = Math::Max(1, outData.height);
			outData.pixelSize = 4;
			outData.imageData.resize(outData.imageWidth * outData.imageHeight * outData.pixelSize, 0);

			if (!glyph.bHasOutline || outData.width <= 0 || outData.height <= 0)
				return true;

			float sampleSize = 1.0f / (mSampleFactor * mScale);
			float sampleWeight = 255.0f / float(mSampleFactor * mSampleFactor);
			RasterizeSettings settings;
			settings.size = sampleSize;
			settings.bound = glyph.bound;

			Rasterize(settings, glyph.segments, [&](Vec2i const& sampleIndex, Vector2 const& pos)
			{
				Vec2i pixelIndex;
				pixelIndex.x = sampleIndex.x / mSampleFactor;
				pixelIndex.y = outData.imageHeight - 1 - sampleIndex.y / mSampleFactor;
				if (pixelIndex.x < 0 || pixelIndex.y < 0 || pixelIndex.x >= outData.imageWidth || pixelIndex.y >= outData.imageHeight)
					return;

				int imageIndex = (pixelIndex.x + pixelIndex.y * outData.imageWidth) * outData.pixelSize;
				uint8 alpha = uint8(Math::Min(255.0f, float(outData.imageData[imageIndex + 3]) + sampleWeight));
				outData.imageData[imageIndex + 0] = 255;
				outData.imageData[imageIndex + 1] = 255;
				outData.imageData[imageIndex + 2] = 255;
				outData.imageData[imageIndex + 3] = alpha;
			});

			return true;
		}

		bool getCharDesc(uint32 charWord, CharImageDesc& outDesc) override
		{
			if (!mLoader)
				return false;

			TrueTypeFileLoader::GlyphData glyph;
			if (!mLoader->loadGlyph(charWord, glyph))
				return false;

			setupDesc(glyph, outDesc);
			return true;
		}

		int getFontHeight() const override
		{
			return mFontHeight;
		}

		void getKerningPairs(std::unordered_map< uint32, float >& outKerningMap) const override
		{
			outKerningMap.clear();
			if (!mLoader)
				return;

			for (TrueTypeFileLoader::KerningPair const& pair : mLoader->getKerningPairs())
			{
				uint32 key = Math::PairingFunction(pair.firstCodepoint, pair.secondCodepoint);
				outKerningMap[key] = float(pair.value) * mScale;
			}
		}

	private:

		void setupDesc(TrueTypeFileLoader::GlyphData const& glyph, CharImageDesc& outDesc) const
		{
			if (glyph.bHasOutline)
			{
				outDesc.width = Math::Max(1, Math::CeilToInt((glyph.bound.max.x - glyph.bound.min.x) * mScale));
				outDesc.height = Math::Max(1, Math::CeilToInt((glyph.bound.max.y - glyph.bound.min.y) * mScale));
				outDesc.kerning = glyph.bound.min.x * mScale;
				outDesc.offsetX = 0;
				outDesc.offsetY = mBaseline - glyph.bound.max.y * mScale;
				outDesc.advance = (glyph.advanceWidth - glyph.bound.min.x) * mScale;
			}
			else
			{
				outDesc.width = 1;
				outDesc.height = 1;
				outDesc.kerning = 0;
				outDesc.offsetX = 0;
				outDesc.offsetY = 0;
				outDesc.advance = glyph.advanceWidth * mScale;
			}
		}

		HashString getName(){ return mName; }

		TrueTypeFileLoader* mLoader = nullptr;
		HashString mName;
		float mScale = 1;
		float mBaseline = 0;
		int mPixelSize = 0;
		int mFontHeight = 0;
		int mSampleFactor = 4;
	};




	class TestStage : public StageBase
	                , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		TArray< CurveSegment > mSegments;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();

			if (!mTTFLoader.load("C:/Windows/Fonts/arial.ttf"))
				return false;

			TrueTypeFileLoader::FontMetrics const& metrics = mTTFLoader.getFontMetrics();
			if (metrics.unitsPerEm == 0)
				return false;


			provider.initialize("arial-ttf", mTTFLoader, 48, 4);
			drawer.initialize(provider);

			auto frame = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(frame->addSlider("Size") , mSettings.size , 1 , 20, [&](float value)
			{
				rasterize(mSettings);
			});

			restart();
			return true;
		}


		TrueTypeCharDataProvider provider;

		FontDrawer drawer;


		void onEnd() override
		{
			BaseClass::onEnd();
		}

		void restart() 
		{
			mSegments.clear();

			if (loadFontGlyph())
			{

			}
			else
			{
				mSegments.resize(8);
				{
					CurveSegment& segment = mSegments[0];
					segment.start = Vector2(100, 200);
					segment.control = Vector2(100, 100);
					segment.end = Vector2(200, 100);
				}
				{
					CurveSegment& segment = mSegments[1];
					segment.start = Vector2(200, 100);
					segment.control = Vector2(300, 100);
					segment.end = Vector2(300, 200);
				}
				{
					CurveSegment& segment = mSegments[2];
					segment.start = Vector2(300, 200);
					segment.control = Vector2(300, 300);
					segment.end = Vector2(200, 300);
				}
				{
					CurveSegment& segment = mSegments[3];
					segment.start = Vector2(200, 300);
					segment.control = Vector2(100, 300);
					segment.end = Vector2(100, 200);
				}

				Vector2 offset = Vector2(50, 50);
				{
					CurveSegment& segment = mSegments[4];
					segment.start = Vector2(100, 150) + offset;
					segment.control = Vector2(100, 100) + offset;
					segment.end = Vector2(150, 100) + offset;
				}
				{
					CurveSegment& segment = mSegments[5];
					segment.start = Vector2(150, 100) + offset;
					segment.control = Vector2(150, 250);
					segment.end = Vector2(200, 150) + offset;
				}
				{
					CurveSegment& segment = mSegments[6];
					segment.start = Vector2(200, 150) + offset;
					segment.control = Vector2(300, 300);
					segment.end = Vector2(150, 200) + offset;
				}
				{
					CurveSegment& segment = mSegments[7];
					segment.start = Vector2(150, 200) + offset;
					segment.control = Vector2(100, 300);
					segment.end = Vector2(100, 150) + offset;
				}

				mSettings.bound.min = Vector2(50, 50);
				mSettings.bound.max = Vector2(350, 350);
			}



			rasterize(mSettings);
		}

		void appendGlyphSegments(TrueTypeFileLoader::GlyphData const& glyph, Vector2 const& offset, float scale, TArray< CurveSegment >& outSegments)
		{
			for (CurveSegment const& srcSegment : glyph.segments)
			{
				auto TransformPos = [&](Vector2 const& pos)
				{
					return Vector2(offset.x + scale * pos.x, offset.y - scale * pos.y);
				};

				CurveSegment segment;
				segment.start = TransformPos(srcSegment.start);
				segment.control = TransformPos(srcSegment.control);
				segment.end = TransformPos(srcSegment.end);
				outSegments.push_back(segment);
			}
		}

		bool loadFontGlyph()
		{
			auto glyph = getGlyhData('R');
			if ( glyph == nullptr)
				return false;

			TrueTypeFileLoader::FontMetrics const& metrics = mTTFLoader.getFontMetrics();
			float scale = 220.0f / metrics.unitsPerEm;
			Vector2 offset = Vector2(100, 320);

			mSegments.reserve(glyph->segments.size());
			appendGlyphSegments(*glyph, offset, scale, mSegments);

			if (mSegments.empty())
				return false;

			Math::TAABBox< Vector2 > bound;
			bound.min = offset + scale * Vector2(glyph->bound.min.x, -glyph->bound.max.y);
			bound.max = offset + scale * Vector2(glyph->bound.max.x, -glyph->bound.min.y);

			mSettings.bound = bound;
			//Vector2 padding(20, 20);
			//mSettings.bound.min -= padding;
			//mSettings.bound.max += padding;
			return true;
		}

		TrueTypeFileLoader::GlyphData* getGlyhData(uint32 c)
		{
			auto iter = mGlyphMap.find(c);
			if (iter != mGlyphMap.end())
				return &iter->second;

			TrueTypeFileLoader::GlyphData glyph;
			if (!mTTFLoader.loadGlyph(c, glyph))
				return nullptr;


			int segmentCount = glyph.segments.size();
			for (int i = 0; i < segmentCount; ++i)
			{
				CurveSegment other;
				if (glyph.segments[i].trySplitYMonotonic(other))
				{
					glyph.segments.push_back(other);
				}
			}
			auto& pair = mGlyphMap.emplace(uint32(c), std::move(glyph));
			return &pair.first->second;
		}

		void drawText(RHIGraphics2D& g, float size, char const* text)
		{
			TrueTypeFileLoader::FontMetrics const& metrics = mTTFLoader.getFontMetrics();
			float scale = size / metrics.unitsPerEm;

			Vector2 textPos = Vector2::Zero();

			g.scaleXForm(scale, -scale);
			for (char const* ch = text; *ch; ++ch)
			{
				TrueTypeFileLoader::GlyphData* glyph = getGlyhData(*ch);

				if (glyph->bHasOutline)
				{
					g.pushXForm();
					g.translateXForm(textPos.x, textPos.y);
					for (auto& segment : glyph->segments)
					{
						segment.draw(g, 24);
					}
					g.popXForm();
				}

				textPos.x += glyph->advanceWidth;
			}
		}

		std::unordered_map< uint32, TrueTypeFileLoader::GlyphData > mGlyphMap;
		TrueTypeFileLoader mTTFLoader;
		RHITexture2DRef mFontTexture;




		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			RHIGraphics2D& g = Global::GetRHIGraphics2D();
			auto& commandList = RHICommandList::GetImmediateList();
			RHISetFrameBuffer(commandList, nullptr);
			RHIClearRenderTargets(commandList, EClearBits::All, &LinearColor(0.2, 0.2, 0.2, 1), 1);

			g.beginRender();

			RenderUtility::SetBrush(g, EColor::Purple);
			for (auto const& pos : mRasterDebugPosList)
			{
				Vector2 size = 0.5 * Vector2(mSettings.size, mSettings.size);
				RenderUtility::SetPen(g, EColor::Null);
				g.drawRect(pos - 0.5 * size, size);
			}

			for (auto& segment : mSegments)
			{
				RenderUtility::SetPen(g, EColor::Yellow);
				segment.draw(g, 32);
				segment.drawDebug(g);
			}



			RenderUtility::SetBrush(g, EColor::Null);
			for (auto const& pos : mDebugPosList)
			{
				Vector2 size = Vector2(4, 4);
				RenderUtility::SetPen(g, EColor::Green);
				g.drawRect(pos - 0.5 * size, size);
			}

			g.pushXForm();
			char const* text = "CurveSegment Text";
			g.translateXForm(80, 520);
			drawText(g, 72, text);
			g.popXForm();


			RenderUtility::SetBrush(g, EColor::White);
			g.setBlendState(ESimpleBlendMode::Translucent);
			g.setBlendAlpha(1.0f);
			g.drawTexture(*mFontTexture, Vector2(300,150), Vector2(200,200));



			g.setFont(drawer);
			g.drawText(Vector2(0, 400), L"ABCDEFG@");



			g.endRender();
		}


		TArray< Vector2 > mDebugPosList;
		TArray< Vector2 > mRasterDebugPosList;
		TArray< CurveSegment > mTextSegments;

		RasterizeSettings mSettings;

		void rasterize(RasterizeSettings const& settings)
		{
			mRasterDebugPosList.clear();
			Rasterize(settings, mSegments, [this](Vec2i const& pixelIndex, Vector2 const& pos)
			{
				mRasterDebugPosList.push_back(pos);
			});


			Vec2i texSize;
			Vector2 boundSize = settings.bound.getSize();
			texSize.x = Math::CeilToInt(boundSize.x / settings.size);
			texSize.y = Math::CeilToInt(boundSize.y / settings.size);


			TArray< Color4ub > fontData;
			fontData.resize(texSize.x * texSize.y, Color4ub(0,0,0,0));
			RasterizeSettings texSettings = settings;
			int const sampleFactor = 4;
			texSettings.size /= sampleFactor;

			{
				TIME_SCOPE("Rasterize");
				Rasterize(texSettings, mSegments, [&](Vec2i const& pixelIndex, Vector2 const& pos)
				{
					Vec2i texIndex = pixelIndex / sampleFactor;

					fontData[texIndex.x + texIndex.y * texSize.x].r += 1;
				});
			}

			float factor = 1.0f / (sampleFactor * sampleFactor);
			for (auto& c : fontData)
			{
				uint8 gray = uint8(255.0f * (float(c.r) * factor));
				c = Color4ub(gray, gray, gray, gray);
			}

			mFontTexture = RHICreateTexture2D(TextureDesc::Type2D(ETexture::RGBA8, texSize.x, texSize.y), fontData.data());
		}



		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				Vector2 pos = msg.getPos();
				mDebugPosList.clear();

				for (auto& segment : mSegments)
				{
					auto AddPosConditional = [&](float t)
					{
						if (0 <= t && t <= 1.0f)
						{
							mDebugPosList.push_back(segment.get(t));
						}
					};

					float outT[2];
					if (segment.findIntersection(pos.y, outT))
					{		
						AddPosConditional(outT[0]);
						if (Math::Abs(outT[0] - outT[1]) > 1e-5)
						{
							AddPosConditional(outT[1]);
						}
					}
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

	REGISTER_STAGE_ENTRY("VGR Test", TestStage, EExecGroup::MiscTest);
}
