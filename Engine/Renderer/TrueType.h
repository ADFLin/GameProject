#ifndef TrueType_h__
#define TrueType_h__


#include "Template/ArrayView.h"
#include "Math/GeometryPrimitive.h"
#include "Math/Vector2.h"
#include "RenderTransform2D.h"
#include "HashString.h"
#include "RHI/Font.h"

#include <algorithm>

namespace Render
{
	using Math::Vector2;

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
			if (Math::Abs(denom) < 1e-5)
				return false;

			float t = (start.y - control.y) / denom;

			if (t <= 0 || t >= 1)
				return false;

			split(t, other);
			return true;
		}

		Vector2 get(float t) const
		{
			return Math::BezierLerp(start, control, end, t);
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

		bool loadFromBuffer(TArrayView<uint8 const> buffer);
		bool load(char const* path);

		bool loadInternal();

		bool loadGlyph(uint32 codepoint, GlyphData& outGlyph) const;

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
		static constexpr uint16 CMapPlatformUnicode = 0;
		static constexpr uint16 CMapPlatformWindows = 3;
		static constexpr uint16 CMapEncodingSymbol = 0;
		static constexpr uint16 CMapEncodingUnicodeBmp = 1;
		static constexpr uint16 CMapEncodingShiftJIS = 2;
		static constexpr uint16 CMapEncodingPRC = 3;
		static constexpr uint16 CMapEncodingBig5 = 4;
		static constexpr uint16 CMapEncodingWansung = 5;
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
			uint16 platformID = 0;
			uint16 encodingID = 0;
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

		bool appendGlyphSegments(uint16 glyphIndex, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours, int depth, Math::TAABBox< Vector2 >* bound) const;

		bool appendSimpleGlyph(uint32 glyphStart, uint32 glyphEnd, int numContours, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours) const;

		bool appendContour(TArray< GlyphPoint > const& points, uint16 contourStart, uint16 contourEnd, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours) const;

		bool appendCompositeGlyph(uint32 glyphStart, uint32 glyphEnd, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours, int depth, Math::TAABBox< Vector2 >* bound) const;

		bool loadGlyphOffsets(Table const& loca);

		bool chooseCMap();

		void buildGlyphCodepointMap();

		void buildGlyphCodepointMapFormat4();

		void buildGlyphCodepointMapFormat12();

		bool loadKerningPairs();

		void loadKerningFormat0(size_t offset, uint32 length);

		uint16 getGlyphIndex(uint32 codepoint) const;

		uint16 getGlyphIndex(CMapInfo const& cmap, uint32 codepoint) const;

		uint32 getEncodedCMapCode(CMapInfo const& cmap, uint32 codepoint) const;

		uint16 getGlyphIndexFormat4(CMapInfo const& cmap, uint32 codepoint) const;

		uint16 getGlyphIndexFormat12(CMapInfo const& cmap, uint32 codepoint) const;

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
		TArray< CMapInfo > mCMaps;
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

		struct RasterSegment
		{
			CurveSegment const* segment;
			float minY;
			float maxY;

			float posX;
			float t;
			int winding;
		};

		float halfSize = 0.5 * settings.size;
		int numY = Math::CeilToInt(settings.bound.getSize().y / settings.size);
		int maxXIndex = Math::CeilToInt(settings.bound.getSize().x / settings.size) - 1;
		if (numY <= 0 || maxXIndex < 0)
			return;

		TArray<RasterSegment> rasterSegments;
		rasterSegments.reserve(segments.size());
		for (CurveSegment const& segment : segments)
		{
			RasterSegment rasterSegment;
			rasterSegment.segment = &segment;
			rasterSegment.minY = Math::Min(segment.start.y, segment.end.y);
			rasterSegment.maxY = Math::Max(segment.start.y, segment.end.y);
			if (Math::Abs(rasterSegment.maxY - rasterSegment.minY) < 1e-6f)
				continue;

			rasterSegment.winding = (segment.end.y > segment.start.y) ? 1 : -1;
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

		for (int yIndex = 0; yIndex < numY; ++yIndex)
		{
			float curY = settings.bound.min.y + (float(yIndex) + 0.5f) * settings.size;
			for (int i = 0; i < activeSegments.size(); )
			{
				if (activeSegments[i]->maxY <= curY)
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
				if (rasterSegments[indexRasterCheck].minY > curY)
					break;

				if (rasterSegments[indexRasterCheck].maxY > curY)
				{
					activeSegments.push_back(&rasterSegments[indexRasterCheck]);
				}
				++indexRasterCheck;
			}

			for (int i = 0; i < activeSegments.size(); )
			{
				RasterSegment* rasterSegment = activeSegments[i];
				CurveSegment const& segment = *rasterSegment->segment;
				auto AddPosConditional = [&](float t) -> bool
				{
					if (0 <= t && t <= 1.0f)
					{
						rasterSegment->posX = segment.get(t).x;
						rasterSegment->t = t;
						return true;
					}
					return false;
				};

				float outT[2];
				int num = segment.findIntersection(curY, outT);
				bool bIntersect = false;
				if (num)
				{
					bIntersect = AddPosConditional(outT[0]);
					if (!bIntersect && num > 1)
					{
						bIntersect = AddPosConditional(outT[1]);
					}
				}

				if (!bIntersect)
				{
					activeSegments.removeIndexSwap(i);
				}
				else
				{
					++i;
				}
			}

			std::sort(activeSegments.begin(), activeSegments.end(),
				[](auto const& lhs, auto const& rhs)
			{
				return lhs->posX < rhs->posX;
			}
			);

			if (!activeSegments.empty())
			{
				int winding = 0;
				float spanStart = settings.bound.min.x;
				for (int index = 0; index < activeSegments.size(); ++index)
				{
					float edgeX = activeSegments[index]->posX;
					if (winding != 0)
					{
						int xStart = Math::FloorToInt((spanStart - settings.bound.min.x - halfSize) / settings.size) + 1;
						int xEnd = Math::FloorToInt((edgeX - settings.bound.min.x - halfSize) / settings.size);
						if (xStart < 0)
							xStart = 0;
						if (xEnd > maxXIndex)
							xEnd = maxXIndex;

						if (xEnd >= 0 && xStart <= xEnd)
						{
							func(yIndex, xStart, xEnd);
						}
					}

					spanStart = edgeX;
					winding += activeSegments[index]->winding;
				}
			}
		}
	}

	template< typename TFunc >
	static void VisitSampleSpanCoverage(int sampleXStart, int sampleXEnd, int sampleFactor, int pixelWidth, TFunc func)
	{
		if (sampleXEnd < sampleXStart)
			return;

		int pixelXStart = Math::Max(0, sampleXStart / sampleFactor);
		int pixelXEnd = Math::Min(pixelWidth - 1, sampleXEnd / sampleFactor);
		for (int pixelX = pixelXStart; pixelX <= pixelXEnd; ++pixelX)
		{
			int firstSample = Math::Max(sampleXStart, pixelX * sampleFactor);
			int lastSample = Math::Min(sampleXEnd, pixelX * sampleFactor + sampleFactor - 1);
			int coverage = lastSample - firstSample + 1;
			if (coverage > 0)
			{
				func(pixelX, coverage);
			}
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

		bool initialize(HashString name, TrueTypeFileLoader& loader, int pixelSize, int sampleFactor = 4);
		bool initialize(HashString name, TrueTypeFileLoader& loader, int pixelSize, int fontHeight, int baseline, int sampleFactor = 4);

		bool getCharData(uint32 charWord, CharImageData& outData) override;

		bool getCharDesc(uint32 charWord, CharImageDesc& outDesc) override
		{
			CHECK(mLoader);

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
			CHECK(mLoader);

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
				float top = mBaseline - glyph.bound.max.y * mScale;
				float bottom = mBaseline - glyph.bound.min.y * mScale;
				int topPixel = Math::FloorToInt(top);
				int bottomPixel = Math::CeilToInt(bottom);

				outDesc.width = Math::Max(1, Math::CeilToInt((glyph.bound.max.x - glyph.bound.min.x) * mScale));
				outDesc.height = Math::Max(1, bottomPixel - topPixel);
				outDesc.kerning = glyph.bound.min.x * mScale;
				outDesc.offsetX = 0;
				outDesc.offsetY = float(topPixel);
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

		HashString getName() { return mName; }

		TrueTypeFileLoader* mLoader = nullptr;
		HashString mName;
		float mScale = 1;
		float mBaseline = 0;
		int mPixelSize = 0;
		int mFontHeight = 0;
		int mSampleFactor = 4;
	};

}

#endif // TrueType_h__
