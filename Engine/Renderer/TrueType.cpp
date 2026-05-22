#include "TrueType.h"

#include "PlatformConfig.h"
#include "FileSystem.h"
#include "ProfileSystem.h"

#if SYS_PLATFORM_WIN
#include "WindowsHeader.h"
#endif

namespace Render
{
	bool TrueTypeFileLoader::loadFromBuffer(TArrayView<uint8 const> buffer)
	{
		mTables.clear();
		mGlyphOffsets.clear();
		mGlyphCodepoints.clear();
		mKerningPairs.clear();
		mCMap = CMapInfo();
		mCMaps.clear();
		mGlyphTableOffset = 0;
		mNumGlyphs = 0;
		mNumHMetrics = 0;
		mFontMetrics = FontMetrics();
		mIndexToLocFormat = 0;

		mBuffer.assign(buffer.begin(), buffer.end());
		return loadInternal();
	}

	bool TrueTypeFileLoader::load(char const* path)
	{
		mTables.clear();
		mGlyphOffsets.clear();
		mGlyphCodepoints.clear();
		mKerningPairs.clear();
		mCMap = CMapInfo();
		mCMaps.clear();
		mGlyphTableOffset = 0;
		mNumGlyphs = 0;
		mNumHMetrics = 0;
		mFontMetrics = FontMetrics();
		mIndexToLocFormat = 0;

		mBuffer.clear();
		if (!FFileUtility::LoadToBuffer(path, mBuffer))
			return false;

		return loadInternal();
	}

	bool TrueTypeFileLoader::loadInternal()
	{
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

	bool TrueTypeFileLoader::loadGlyph(uint32 codepoint, GlyphData& outGlyph) const
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

	bool TrueTypeFileLoader::appendGlyphSegments(uint16 glyphIndex, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours, int depth, Math::TAABBox< Vector2 >* bound) const
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

	bool TrueTypeFileLoader::appendSimpleGlyph(uint32 glyphStart, uint32 glyphEnd, int numContours, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours) const
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

	bool TrueTypeFileLoader::appendContour(TArray< GlyphPoint > const& points, uint16 contourStart, uint16 contourEnd, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours) const
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

	bool TrueTypeFileLoader::appendCompositeGlyph(uint32 glyphStart, uint32 glyphEnd, RenderTransform2D const& transform, TArray< CurveSegment >& outSegments, TArray< GlyphData::Contour >* outContours, int depth, Math::TAABBox< Vector2 >* bound) const
	{
		enum
		{
			Arg1And2AreWords = 0x0001,
			ArgsAreXYValues = 0x0002,
			RoundXYToGrid = 0x0004,
			HaveUniformScale = 0x0008,
			MoreComponents = 0x0020,
			HaveXAndYScale = 0x0040,
			HaveTwoByTwo = 0x0080,
			ScaledComponentOffset = 0x0800,
			UnscaledComponentOffset = 0x1000,
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

			Vector2 componentOffset = Vector2::Zero();
			if (flags & ArgsAreXYValues)
			{
				componentOffset = Vector2(float(arg1), float(arg2));
			}
			else
			{
				return false;
			}

			RenderTransform2D component = RenderTransform2D::Identity();
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
				component.M = Matrix2(m00, m01, m10, m11);
			}

			if ((flags & ScaledComponentOffset) && !(flags & UnscaledComponentOffset))
			{
				component.P = component.M.leftMul(componentOffset);
			}
			else
			{
				component.P = componentOffset;
			}
			if (flags & RoundXYToGrid)
			{
				component.P.x = Math::Round(component.P.x);
				component.P.y = Math::Round(component.P.y);
			}

			if (!appendGlyphSegments(componentGlyphIndex, component * transform, outSegments, outContours, depth + 1, bound))
				return false;

			bMore = !!(flags & MoreComponents);
		}

		return true;
	}

	bool TrueTypeFileLoader::loadGlyphOffsets(Table const& loca)
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

	bool TrueTypeFileLoader::chooseCMap()
	{
		Table const* cmap = findTable("cmap");
		if (!cmap || cmap->length < CMapHeaderSize)
			return false;

		size_t offset = cmap->offset;
		readU16(offset); // version
		uint16 numTables = readU16(offset);
		if (cmap->length < CMapHeaderSize + CMapRecordSize * numTables)
			return false;

		auto GetCMapScore = [](uint16 platformID, uint16 encodingID, uint16 format) -> int
		{
			if (platformID == CMapPlatformWindows)
			{
				if (format == CMapFormat12 && encodingID == CMapEncodingUnicodeFull)
					return 100;

				if (format == CMapFormat4 && encodingID == CMapEncodingUnicodeBmp)
					return 95;

				if (format == CMapFormat4 &&
					(encodingID == CMapEncodingShiftJIS ||
					 encodingID == CMapEncodingPRC ||
					 encodingID == CMapEncodingBig5 ||
					 encodingID == CMapEncodingWansung))
					return 20;

				if (format == CMapFormat4 && encodingID == CMapEncodingSymbol)
					return 10;
			}

			if (platformID == CMapPlatformUnicode)
			{
				if (format == CMapFormat12)
					return 80;

				if (format == CMapFormat4)
					return 70;
			}

			return 1;
		};

		struct CMapCandidate
		{
			CMapInfo cmap;
			int score = 0;
		};

		TArray< CMapCandidate > candidates;
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

			int score = GetCMapScore(platformID, encodingID, format);
			candidates.push_back({ { subtableOffset, format, platformID, encodingID }, score });
		}

		if (candidates.empty())
			return false;

		std::sort(candidates.begin(), candidates.end(), [](CMapCandidate const& a, CMapCandidate const& b)
		{
			return a.score > b.score;
		});

		mCMaps.clear();
		for (CMapCandidate const& candidate : candidates)
		{
			mCMaps.push_back(candidate.cmap);
		}

		mCMap = mCMaps[0];
		return mCMap.offset != 0;
	}

	void TrueTypeFileLoader::buildGlyphCodepointMap()
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

	void TrueTypeFileLoader::buildGlyphCodepointMapFormat4()
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
				uint16 glyphIndex = getGlyphIndexFormat4(mCMap, codepoint);
				if (glyphIndex != 0 && glyphIndex < mGlyphCodepoints.size() && mGlyphCodepoints[glyphIndex] == 0)
				{
					mGlyphCodepoints[glyphIndex] = codepoint;
				}
			}
		}
	}

	void TrueTypeFileLoader::buildGlyphCodepointMapFormat12()
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

	bool TrueTypeFileLoader::loadKerningPairs()
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

	void TrueTypeFileLoader::loadKerningFormat0(size_t offset, uint32 length)
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

	uint16 TrueTypeFileLoader::getGlyphIndex(uint32 codepoint) const
	{
		for (CMapInfo const& cmap : mCMaps)
		{
			uint16 glyphIndex = getGlyphIndex(cmap, codepoint);
			if (glyphIndex != 0)
				return glyphIndex;
		}

		return 0;
	}

	uint16 TrueTypeFileLoader::getGlyphIndex(CMapInfo const& cmap, uint32 codepoint) const
	{
		uint32 cmapCode = getEncodedCMapCode(cmap, codepoint);
		if (cmap.format == CMapFormat4)
			return getGlyphIndexFormat4(cmap, cmapCode);
		if (cmap.format == CMapFormat12)
			return getGlyphIndexFormat12(cmap, cmapCode);
		return 0;
	}

	uint32 TrueTypeFileLoader::getEncodedCMapCode(CMapInfo const& cmap, uint32 codepoint) const
	{
		if (cmap.platformID != CMapPlatformWindows)
			return codepoint;

		uint32 codePage = 0;
		switch (cmap.encodingID)
		{
		case CMapEncodingShiftJIS: codePage = 932; break;
		case CMapEncodingPRC:      codePage = 936; break;
		case CMapEncodingBig5:     codePage = 950; break;
		case CMapEncodingWansung:  codePage = 949; break;
		default:
			return codepoint;
		}

#if SYS_PLATFORM_WIN
		wchar_t text[2];
		int textLen = 1;
		if (codepoint <= UnicodeBmpMax)
		{
			text[0] = wchar_t(codepoint);
		}
		else
		{
			uint32 value = codepoint - 0x10000;
			text[0] = wchar_t(0xD800 + (value >> 10));
			text[1] = wchar_t(0xDC00 + (value & 0x3FF));
			textLen = 2;
		}

		char buffer[4];
		BOOL usedDefaultChar = FALSE;
		int numBytes = ::WideCharToMultiByte(codePage, WC_NO_BEST_FIT_CHARS, text, textLen, buffer, sizeof(buffer), nullptr, &usedDefaultChar);
		if (numBytes <= 0 || usedDefaultChar)
			return 0;

		uint32 result = 0;
		for (int i = 0; i < numBytes; ++i)
		{
			result = (result << 8) | uint8(buffer[i]);
		}
		return result;
#else
		return codepoint;
#endif
	}

	uint16 TrueTypeFileLoader::getGlyphIndexFormat4(CMapInfo const& cmap, uint32 codepoint) const
	{
		if (codepoint > UnicodeBmpMax)
			return 0;

		uint32 offset = cmap.offset;
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

	uint16 TrueTypeFileLoader::getGlyphIndexFormat12(CMapInfo const& cmap, uint32 codepoint) const
	{
		uint32 offset = cmap.offset;
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

	bool TrueTypeCharDataProvider::initialize(HashString name, TrueTypeFileLoader& loader, int pixelSize, int sampleFactor /*= 4*/)
	{
		if (pixelSize <= 0 || sampleFactor <= 0)
			return false;

		TrueTypeFileLoader::FontMetrics const& metrics = loader.getFontMetrics();
		if (metrics.unitsPerEm == 0)
			return false;

		float scale = float(pixelSize) / metrics.unitsPerEm;
		int fontHeight = Math::CeilToInt(float(metrics.ascender - metrics.descender + metrics.lineGap) * scale);
		int baseline = Math::CeilToInt(float(metrics.ascender) * scale);
		return initialize(name, loader, pixelSize, fontHeight, baseline, sampleFactor);
	}

	bool TrueTypeCharDataProvider::initialize(HashString name, TrueTypeFileLoader& loader, int pixelSize, int fontHeight, int baseline, int sampleFactor /*= 4*/)
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
		mFontHeight = fontHeight;
		mBaseline = float(baseline);
		return mFontHeight > 0;
	}

	bool TrueTypeCharDataProvider::getCharData(uint32 charWord, CharImageData& outData)
	{
		CHECK(mLoader);

		TIME_SCOPE("GetCharData");

		TrueTypeFileLoader::GlyphData glyph;
		if (!mLoader->loadGlyph(charWord, glyph))
			return false;

		int segmentCount = int(glyph.segments.size());
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
		RasterizeSettings settings;
		settings.size = sampleSize;
		settings.bound = glyph.bound;
		settings.bound.max.y = (mBaseline - outData.offsetY) / mScale;
		settings.bound.min.y = (mBaseline - (outData.offsetY + outData.height)) / mScale;

		Rasterize(settings, glyph.segments, [&](int sampleY, int sampleXStart, int sampleXEnd)
		{
			int pixelY = outData.imageHeight - 1 - sampleY / mSampleFactor;
			if (pixelY < 0 || pixelY >= outData.imageHeight)
				return;

			VisitSampleSpanCoverage(sampleXStart, sampleXEnd, mSampleFactor, outData.imageWidth, [&](int pixelX, int coverage)
			{
				int imageIndex = (pixelX + pixelY * outData.imageWidth) * outData.pixelSize;
				outData.imageData[imageIndex + 3] += uint8(coverage);
			});
		});

		float alphaScale = 255.0f / float(mSampleFactor * mSampleFactor);
		for (int y = 0; y < outData.imageHeight; ++y)
		{
			for (int x = 0; x < outData.imageWidth; ++x)
			{
				int imageIndex = (x + y * outData.imageWidth) * outData.pixelSize;
				uint8 coverage = outData.imageData[imageIndex + 3];
				if (coverage == 0)
					continue;

				uint8 alpha = uint8(Math::Min(255.0f, float(coverage) * alphaScale));
				outData.imageData[imageIndex + 0] = 255;
				outData.imageData[imageIndex + 1] = 255;
				outData.imageData[imageIndex + 2] = 255;
				outData.imageData[imageIndex + 3] = alpha;
			}
		}

		return true;
	}

}


