#include "TrueType.h"

#include "PlatformConfig.h"
#include "FileSystem.h"
#include "ProfileSystem.h"

namespace Render
{
	namespace
	{
		static constexpr float TT_F26Dot6 = 64.0f;
		static constexpr float TT_F2Dot14 = 16384.0f;

		struct TTPoint
		{
			Vector2 orgPos = Vector2::Zero();
			Vector2 curPos = Vector2::Zero();
			bool bOnCurve = false;
			bool bTouchedX = false;
			bool bTouchedY = false;
		};

		struct TTZone
		{
			TArray< TTPoint >* points = nullptr;
			TArray< uint16 > const* endPts = nullptr;

			int size() const { return points ? int(points->size()) : 0; }
			TTPoint& operator[](int index) const { return (*points)[index]; }
			bool isValidIndex(int index) const { return points && 0 <= index && index < points->size(); }
		};

		struct TTFunctionDef
		{
			int start = -1;
			int end = -1;
		};

		struct TTGraphicsState
		{
			Vector2 freedom = Vector2(1, 0);
			Vector2 projection = Vector2(1, 0);
			Vector2 dualProjection = Vector2(1, 0);
			int zp0 = 1;
			int zp1 = 1;
			int zp2 = 1;
			int rp0 = 0;
			int rp1 = 0;
			int rp2 = 0;
			int loop = 1;
			float minimumDistance = 1.0f * TT_F26Dot6;
			float controlValueCutIn = 17.0f / 16.0f * TT_F26Dot6;
			float singleWidthCutIn = 0.0f;
			float singleWidthValue = 0.0f;
			bool bAutoFlip = true;
			int roundMode = 0;
			uint16 scanControl = 0;
			int scanType = 2;
		};

		struct TTExecutionContext
		{
			TrueTypeFileLoader const& loader;
			TTGraphicsState state;
			TArray< int > stack;
			TArray< int > storage;
			TArray< float > cvt;
			TArray< TTPoint > twilight;
			TTZone zones[3];
			TArray< TTFunctionDef > functions;
			float scale = 1.0f;
			float ppem = 0.0f;

			explicit TTExecutionContext(TrueTypeFileLoader const& inLoader)
				: loader(inLoader)
			{
			}

			float project(Vector2 const& value) const
			{
				return Math::Dot(value, state.projection);
			}

			float dualProject(Vector2 const& value) const
			{
				return Math::Dot(value, state.dualProjection);
			}

			float freedomProject(Vector2 const& value) const
			{
				return Math::Dot(value, state.freedom);
			}

			void movePoint(TTPoint& point, float distance)
			{
				point.curPos += distance * state.freedom;
				if (Math::Abs(state.freedom.x) >= Math::Abs(state.freedom.y))
					point.bTouchedX = true;
				if (Math::Abs(state.freedom.y) >= Math::Abs(state.freedom.x))
					point.bTouchedY = true;
			}

			float roundValue(float value) const
			{
				switch (state.roundMode)
				{
				case 0:
				default:
					return Math::Round(value / TT_F26Dot6) * TT_F26Dot6;
				case 1:
					return Math::Floor(value / TT_F26Dot6) * TT_F26Dot6;
				case 2:
					return Math::Ceil(value / TT_F26Dot6) * TT_F26Dot6;
				case 3:
					return (Math::Floor(value / TT_F26Dot6) + 0.5f) * TT_F26Dot6;
				case 4:
					return value;
				}
			}
		};

		static Vector2 TTUnitVector(int axis)
		{
			return axis == 0 ? Vector2(1, 0) : Vector2(0, 1);
		}

		static Vector2 TTNormalize(Vector2 value)
		{
			if (value.normalize() == 0.0f)
				return Vector2(1, 0);
			return value;
		}
	}

	bool TrueTypeFileLoader::loadFromBuffer(TArrayView<uint8 const> buffer)
	{
		mTables.clear();
		mGlyphOffsets.clear();
		mGlyphCodepoints.clear();
		mKerningPairs.clear();
		mCVTValues.clear();
		mFpgmRange = ProgramRange();
		mPrepRange = ProgramRange();
		mCMap = CMapInfo();
		mCMaps.clear();
		mGlyphTableOffset = 0;
		mNumGlyphs = 0;
		mNumHMetrics = 0;
		mFontMetrics = FontMetrics();
		mHintMetrics = HintMetrics();
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
		mCVTValues.clear();
		mFpgmRange = ProgramRange();
		mPrepRange = ProgramRange();
		mCMap = CMapInfo();
		mCMaps.clear();
		mGlyphTableOffset = 0;
		mNumGlyphs = 0;
		mNumHMetrics = 0;
		mFontMetrics = FontMetrics();
		mHintMetrics = HintMetrics();
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
		if (maxp->length >= MaxpMaxSizeOfInstructionsOffset + sizeof(uint16))
		{
			mHintMetrics.maxPoints = readU16At(maxp->offset + MaxpMaxPointsOffset);
			mHintMetrics.maxContours = readU16At(maxp->offset + MaxpMaxContoursOffset);
			mHintMetrics.maxCompositePoints = readU16At(maxp->offset + MaxpMaxCompositePointsOffset);
			mHintMetrics.maxCompositeContours = readU16At(maxp->offset + MaxpMaxCompositeContoursOffset);
			mHintMetrics.maxZones = readU16At(maxp->offset + MaxpMaxZonesOffset);
			mHintMetrics.maxTwilightPoints = readU16At(maxp->offset + MaxpMaxTwilightPointsOffset);
			mHintMetrics.maxStorage = readU16At(maxp->offset + MaxpMaxStorageOffset);
			mHintMetrics.maxFunctionDefs = readU16At(maxp->offset + MaxpMaxFunctionDefsOffset);
			mHintMetrics.maxInstructionDefs = readU16At(maxp->offset + MaxpMaxInstructionDefsOffset);
			mHintMetrics.maxStackElements = readU16At(maxp->offset + MaxpMaxStackElementsOffset);
			mHintMetrics.maxSizeOfInstructions = readU16At(maxp->offset + MaxpMaxSizeOfInstructionsOffset);
		}
		offset = hhea->offset + HheaNumHMetricsOffset;
		mNumHMetrics = readU16(offset);
		offset = hhea->offset + HheaAscenderOffset;
		mFontMetrics.ascender = readS16(offset);
		mFontMetrics.descender = readS16(offset);
		mFontMetrics.lineGap = readS16(offset);
		if (Table const* cvt = findTable("cvt "))
		{
			if ((cvt->length & 1) != 0)
				return false;

			mCVTValues.resize(cvt->length / sizeof(int16));
			size_t cvtOffset = cvt->offset;
			for (int i = 0; i < mCVTValues.size(); ++i)
			{
				mCVTValues[i] = readS16(cvtOffset);
			}
		}
		if (Table const* fpgm = findTable("fpgm"))
		{
			mFpgmRange = { fpgm->offset, fpgm->length };
		}
		if (Table const* prep = findTable("prep"))
		{
			mPrepRange = { prep->offset, prep->length };
		}
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
		if (!appendGlyphOutline(glyphIndex, RenderTransform2D::Identity(), outGlyph.points, outGlyph.endPts, 0))
			return false;
		if (!loadGlyphInstructions(glyphIndex, outGlyph.instructions))
			return false;

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

	bool TrueTypeFileLoader::loadGlyphInstructions(uint16 glyphIndex, TArray< uint8 >& outInstructions) const
	{
		outInstructions.clear();
		if (glyphIndex + 1 >= mGlyphOffsets.size())
			return false;

		uint32 glyphStart = mGlyphTableOffset + mGlyphOffsets[glyphIndex];
		uint32 glyphEnd = mGlyphTableOffset + mGlyphOffsets[glyphIndex + 1];
		if (glyphEnd <= glyphStart)
			return true;
		if (!checkRange(glyphStart, glyphEnd - glyphStart) || glyphEnd - glyphStart < GlyphHeaderSize)
			return false;

		size_t pos = glyphStart;
		int16 numContours = readS16(pos);
		pos = glyphStart + GlyphHeaderSize;
		if (numContours >= 0)
		{
			if (numContours == 0)
				return true;
			if (pos + sizeof(uint16) * numContours + GlyphInstructionLengthSize > glyphEnd)
				return false;

			for (int i = 0; i < numContours; ++i)
			{
				readU16(pos);
			}
			uint16 instructionLength = readU16(pos);
			if (pos + instructionLength > glyphEnd)
				return false;

			outInstructions.resize(instructionLength);
			for (int i = 0; i < instructionLength; ++i)
			{
				outInstructions[i] = readU8(pos);
			}
			return true;
		}

		enum
		{
			Arg1And2AreWords = 0x0001,
			HaveMoreComponents = 0x0020,
			HaveScale = 0x0008,
			HaveXYScale = 0x0040,
			HaveTwoByTwo = 0x0080,
			HaveInstructions = 0x0100,
		};

		uint16 flags = 0;
		do
		{
			if (pos + sizeof(uint16) * 2 > glyphEnd)
				return false;

			flags = readU16(pos);
			readU16(pos);
			size_t argSize = (flags & Arg1And2AreWords) ? sizeof(int16) * 2 : sizeof(int8) * 2;
			if (pos + argSize > glyphEnd)
				return false;
			pos += argSize;

			if (flags & HaveScale)
			{
				if (pos + sizeof(int16) > glyphEnd)
					return false;
				pos += sizeof(int16);
			}
			else if (flags & HaveXYScale)
			{
				if (pos + sizeof(int16) * 2 > glyphEnd)
					return false;
				pos += sizeof(int16) * 2;
			}
			else if (flags & HaveTwoByTwo)
			{
				if (pos + sizeof(int16) * 4 > glyphEnd)
					return false;
				pos += sizeof(int16) * 4;
			}
		}
		while (flags & HaveMoreComponents);

		if (!(flags & HaveInstructions))
			return true;
		if (pos + sizeof(uint16) > glyphEnd)
			return false;

		uint16 instructionLength = readU16(pos);
		if (pos + instructionLength > glyphEnd)
			return false;

		outInstructions.resize(instructionLength);
		for (int i = 0; i < instructionLength; ++i)
		{
			outInstructions[i] = readU8(pos);
		}
		return true;
	}

	bool TrueTypeFileLoader::hintGlyph(GlyphData& ioGlyph, float pixelSize) const
	{
		if (ioGlyph.points.empty() || pixelSize <= 0 || mFontMetrics.unitsPerEm == 0)
			return true;

		float scale = float(pixelSize) / float(mFontMetrics.unitsPerEm);
		if (scale <= 0.0f)
			return true;

		TTExecutionContext ctx(*this);
		ctx.scale = scale;
		ctx.ppem = pixelSize;
		ctx.storage.resize(Math::Max<int>(mHintMetrics.maxStorage, 64), 0);
		ctx.cvt.resize(mCVTValues.size());
		for (int i = 0; i < mCVTValues.size(); ++i)
		{
			ctx.cvt[i] = float(mCVTValues[i]) * scale * TT_F26Dot6;
		}
		ctx.twilight.resize(Math::Max<int>(mHintMetrics.maxTwilightPoints, 16));
		ctx.functions.resize(Math::Max<int>(mHintMetrics.maxFunctionDefs + 1, 512));

		TArray< TTPoint > glyphPoints;
		int const outlinePointCount = int(ioGlyph.points.size());
		glyphPoints.resize(outlinePointCount + 4);
		for (int i = 0; i < outlinePointCount; ++i)
		{
			TTPoint& dst = glyphPoints[i];
			dst.orgPos = ioGlyph.points[i].pos * (scale * TT_F26Dot6);
			dst.curPos = dst.orgPos;
			dst.bOnCurve = ioGlyph.points[i].bOnCurve;
		}

		{
			float leftSideX = (ioGlyph.bound.min.x - ioGlyph.leftSideBearing) * scale * TT_F26Dot6;
			float rightSideX = leftSideX + ioGlyph.advanceWidth * scale * TT_F26Dot6;
			float topY = ioGlyph.bound.max.y * scale * TT_F26Dot6;
			float bottomY = ioGlyph.bound.min.y * scale * TT_F26Dot6;
			glyphPoints[outlinePointCount + 0].orgPos = glyphPoints[outlinePointCount + 0].curPos = Vector2(leftSideX, 0.0f);
			glyphPoints[outlinePointCount + 1].orgPos = glyphPoints[outlinePointCount + 1].curPos = Vector2(rightSideX, 0.0f);
			glyphPoints[outlinePointCount + 2].orgPos = glyphPoints[outlinePointCount + 2].curPos = Vector2(0.0f, topY);
			glyphPoints[outlinePointCount + 3].orgPos = glyphPoints[outlinePointCount + 3].curPos = Vector2(0.0f, bottomY);
			for (int i = 0; i < 4; ++i)
			{
				glyphPoints[outlinePointCount + i].bOnCurve = true;
			}
		}

		ctx.zones[0] = { &ctx.twilight, nullptr };
		ctx.zones[1] = { &glyphPoints, &ioGlyph.endPts };
		ctx.zones[2] = { &glyphPoints, &ioGlyph.endPts };

		auto GetZone = [&](int index) -> TTZone&
		{
			return ctx.zones[Math::Clamp(index, 0, 2)];
		};

		auto Pop = [&](int& outValue) -> bool
		{
			if (ctx.stack.empty())
				return false;
			outValue = ctx.stack.back();
			ctx.stack.pop_back();
			return true;
		};

		auto Pop2 = [&](int& outA, int& outB) -> bool
		{
			return Pop(outB) && Pop(outA);
		};

		auto ParseFunctionDefs = [&](ProgramRange const& range)
		{
			if (range.length == 0)
				return;

			int ip = int(range.offset);
			int end = int(range.offset + range.length);
			while (ip < end)
			{
				uint8 op = readU8At(ip++);
				if (0xB0 <= op && op <= 0xB7)
				{
					int count = op - 0xB0 + 1;
					for (int i = 0; i < count && ip < end; ++i)
					{
						ctx.stack.push_back(readU8At(ip++));
					}
				}
				else if (0xB8 <= op && op <= 0xBF)
				{
					int count = op - 0xB8 + 1;
					for (int i = 0; i < count && ip + 1 < end; ++i)
					{
						ctx.stack.push_back(readS16At(ip));
						ip += 2;
					}
				}
				else if (op == 0x40)
				{
					int count = readU8At(ip++);
					for (int i = 0; i < count && ip < end; ++i)
					{
						ctx.stack.push_back(readU8At(ip++));
					}
				}
				else if (op == 0x41)
				{
					int count = readU8At(ip++);
					for (int i = 0; i < count && ip + 1 < end; ++i)
					{
						ctx.stack.push_back(readS16At(ip));
						ip += 2;
					}
				}
				else if (op == 0x2C)
				{
					if (ctx.stack.empty())
						return;
					int functionId = ctx.stack.back();
					ctx.stack.pop_back();
					int bodyStart = ip;
					int depth = 1;
					while (ip < end && depth > 0)
					{
						uint8 nestedOp = readU8At(ip++);
						if (0xB0 <= nestedOp && nestedOp <= 0xB7)
						{
							ip += nestedOp - 0xB0 + 1;
						}
						else if (0xB8 <= nestedOp && nestedOp <= 0xBF)
						{
							ip += 2 * (nestedOp - 0xB8 + 1);
						}
						else if (nestedOp == 0x40)
						{
							ip += 1 + readU8At(ip);
						}
						else if (nestedOp == 0x41)
						{
							ip += 1 + 2 * readU8At(ip);
						}
						else if (nestedOp == 0x2C)
						{
							++depth;
						}
						else if (nestedOp == 0x2D)
						{
							--depth;
						}
					}

					if (0 <= functionId && functionId < ctx.functions.size())
					{
						ctx.functions[functionId] = { bodyStart, ip - 1 };
					}
				}
			}

			ctx.stack.clear();
		};

		auto MovePointToCoord = [&](TTZone& zone, int pointIndex, float targetCoord) -> bool
		{
			if (!zone.isValidIndex(pointIndex))
				return false;
			TTPoint& point = zone[pointIndex];
			float currentCoord = ctx.project(point.curPos);
			ctx.movePoint(point, targetCoord - currentCoord);
			return true;
		};

		auto GetPointCoord = [&](TTZone& zone, int pointIndex, bool bUseCurrent) -> float
		{
			Vector2 const& pos = bUseCurrent ? zone[pointIndex].curPos : zone[pointIndex].orgPos;
			return bUseCurrent ? ctx.project(pos) : ctx.dualProject(pos);
		};

		auto ApplyMinimumDistance = [&](float value) -> float
		{
			if (value > 0.0f)
				return Math::Max(value, ctx.state.minimumDistance);
			if (value < 0.0f)
				return -Math::Max(-value, ctx.state.minimumDistance);
			return value;
		};

		auto SkipInstructionStream = [&](int& ioIp, int ioEnd, bool bStopAtElse) -> bool
		{
			int depth = 1;
			while (ioIp < ioEnd && depth > 0)
			{
				uint8 nestedOp = readU8At(ioIp++);
				if (0xB0 <= nestedOp && nestedOp <= 0xB7)
				{
					ioIp += nestedOp - 0xB0 + 1;
				}
				else if (0xB8 <= nestedOp && nestedOp <= 0xBF)
				{
					ioIp += 2 * (nestedOp - 0xB8 + 1);
				}
				else if (nestedOp == 0x40)
				{
					ioIp += 1 + readU8At(ioIp);
				}
				else if (nestedOp == 0x41)
				{
					ioIp += 1 + 2 * readU8At(ioIp);
				}
				else if (nestedOp == 0x58)
				{
					++depth;
				}
				else if (nestedOp == 0x59)
				{
					--depth;
				}
				else if (nestedOp == 0x1B && depth == 1 && bStopAtElse)
				{
					break;
				}
			}
			return depth == 0 || (bStopAtElse && ioIp <= ioEnd);
		};

		auto InterpolateUntouched = [&](TTZone& zone, bool bXAxis)
		{
			if (!zone.points || !zone.endPts)
				return;

			auto GetCur = [&](TTPoint const& p) -> float& { return bXAxis ? const_cast<float&>(p.curPos.x) : const_cast<float&>(p.curPos.y); };
			auto GetOrg = [&](TTPoint const& p) -> float { return bXAxis ? p.orgPos.x : p.orgPos.y; };
			auto IsTouched = [&](TTPoint const& p) -> bool { return bXAxis ? p.bTouchedX : p.bTouchedY; };
			auto SetTouched = [&](TTPoint& p) { if (bXAxis) p.bTouchedX = true; else p.bTouchedY = true; };

			uint16 contourStart = 0;
			for (uint16 contourEnd : *zone.endPts)
			{
				int firstTouched = -1;
				for (int i = contourStart; i <= contourEnd; ++i)
				{
					if (IsTouched((*zone.points)[i]))
					{
						firstTouched = i;
						break;
					}
				}

				if (firstTouched >= 0)
				{
					int prevTouched = firstTouched;
					int i = firstTouched + 1;
					while (i <= contourEnd)
					{
						if (IsTouched((*zone.points)[i]))
						{
							float orgA = GetOrg((*zone.points)[prevTouched]);
							float orgB = GetOrg((*zone.points)[i]);
							float curA = GetCur((*zone.points)[prevTouched]);
							float curB = GetCur((*zone.points)[i]);
							for (int j = prevTouched + 1; j < i; ++j)
							{
								TTPoint& p = (*zone.points)[j];
								float org = GetOrg(p);
								if (Math::Abs(orgB - orgA) < 1e-6f)
								{
									GetCur(p) += curA - orgA;
								}
								else
								{
									float alpha = (org - orgA) / (orgB - orgA);
									GetCur(p) = curA + alpha * (curB - curA);
								}
								SetTouched(p);
							}
							prevTouched = i;
						}
						++i;
					}

					float leadDelta = GetCur((*zone.points)[firstTouched]) - GetOrg((*zone.points)[firstTouched]);
					for (int j = contourStart; j < firstTouched; ++j)
					{
						TTPoint& p = (*zone.points)[j];
						GetCur(p) = GetOrg(p) + leadDelta;
						SetTouched(p);
					}

					float tailDelta = GetCur((*zone.points)[prevTouched]) - GetOrg((*zone.points)[prevTouched]);
					for (int j = prevTouched + 1; j <= contourEnd; ++j)
					{
						TTPoint& p = (*zone.points)[j];
						GetCur(p) = GetOrg(p) + tailDelta;
						SetTouched(p);
					}
				}

				contourStart = contourEnd + 1;
			}
		};

		std::function< bool(int&, int, int) > ExecProgram;
		ExecProgram = [&](int& ip, int end, int callDepth) -> bool
		{
			if (callDepth > 64)
				return false;

			while (ip < end)
			{
				uint8 op = readU8At(ip++);
				if (0xB0 <= op && op <= 0xB7)
				{
					int count = op - 0xB0 + 1;
					for (int i = 0; i < count && ip < end; ++i)
					{
						ctx.stack.push_back(readU8At(ip++));
					}
					continue;
				}
				if (0xB8 <= op && op <= 0xBF)
				{
					int count = op - 0xB8 + 1;
					for (int i = 0; i < count && ip + 1 < end; ++i)
					{
						ctx.stack.push_back(readS16At(ip));
						ip += 2;
					}
					continue;
				}
				if (op == 0x40)
				{
					int count = readU8At(ip++);
					for (int i = 0; i < count && ip < end; ++i)
					{
						ctx.stack.push_back(readU8At(ip++));
					}
					continue;
				}
				if (op == 0x41)
				{
					int count = readU8At(ip++);
					for (int i = 0; i < count && ip + 1 < end; ++i)
					{
						ctx.stack.push_back(readS16At(ip));
						ip += 2;
					}
					continue;
				}

				switch (op)
				{
				case 0x00: ctx.state.freedom = ctx.state.projection = ctx.state.dualProjection = Vector2(0, 1); break;
				case 0x01: ctx.state.freedom = ctx.state.projection = ctx.state.dualProjection = Vector2(1, 0); break;
				case 0x02: ctx.state.projection = ctx.state.dualProjection = Vector2(0, 1); break;
				case 0x03: ctx.state.projection = ctx.state.dualProjection = Vector2(1, 0); break;
				case 0x04: ctx.state.freedom = Vector2(0, 1); break;
				case 0x05: ctx.state.freedom = Vector2(1, 0); break;
				case 0x06:
				case 0x07:
				case 0x08:
				case 0x09:
					{
						int p1, p2;
						if (!Pop2(p1, p2))
							return false;
						TTZone& zone1 = GetZone(ctx.state.zp1);
						TTZone& zone2 = GetZone((op == 0x06 || op == 0x07) ? ctx.state.zp2 : ctx.state.zp1);
						if (!zone1.isValidIndex(p1) || !zone2.isValidIndex(p2))
							return false;
						Vector2 dir = zone2[p2].curPos - zone1[p1].curPos;
						if (op == 0x07 || op == 0x09)
							dir = Math::Perp(dir);
						dir = TTNormalize(dir);
						if (op == 0x06 || op == 0x07)
							ctx.state.projection = ctx.state.dualProjection = dir;
						else
							ctx.state.freedom = dir;
					}
					break;
				case 0x0A:
				case 0x0B:
					{
						int y, x;
						if (!Pop2(x, y))
							return false;
						Vector2 dir = TTNormalize(Vector2(float(x) / TT_F2Dot14, float(y) / TT_F2Dot14));
						if (op == 0x0A)
							ctx.state.projection = ctx.state.dualProjection = dir;
						else
							ctx.state.freedom = dir;
					}
					break;
				case 0x0C:
					ctx.stack.push_back(Math::RoundToInt(ctx.state.projection.x * TT_F2Dot14));
					ctx.stack.push_back(Math::RoundToInt(ctx.state.projection.y * TT_F2Dot14));
					break;
				case 0x0D:
					ctx.stack.push_back(Math::RoundToInt(ctx.state.freedom.x * TT_F2Dot14));
					ctx.stack.push_back(Math::RoundToInt(ctx.state.freedom.y * TT_F2Dot14));
					break;
				case 0x0E:
					ctx.state.freedom = ctx.state.projection;
					break;
				case 0x10: if (!Pop(ctx.state.rp0)) return false; break;
				case 0x11: if (!Pop(ctx.state.rp1)) return false; break;
				case 0x12: if (!Pop(ctx.state.rp2)) return false; break;
				case 0x13: if (!Pop(ctx.state.zp0)) return false; break;
				case 0x14: if (!Pop(ctx.state.zp1)) return false; break;
				case 0x15: if (!Pop(ctx.state.zp2)) return false; break;
				case 0x16:
					{
						int zone;
						if (!Pop(zone))
							return false;
						ctx.state.zp0 = ctx.state.zp1 = ctx.state.zp2 = zone;
					}
					break;
				case 0x17: if (!Pop(ctx.state.loop)) return false; break;
				case 0x18: ctx.state.roundMode = 0; break;
				case 0x19: ctx.state.roundMode = 3; break;
				case 0x1A:
					{
						int value;
						if (!Pop(value))
							return false;
						ctx.state.minimumDistance = float(value);
					}
					break;
				case 0x1B:
					if (!SkipInstructionStream(ip, end, false))
						return false;
					break;
				case 0x59:
				case 0x2D:
					return true;
				case 0x1C:
					{
						int offset;
						if (!Pop(offset))
							return false;
						ip += offset - 1;
					}
					break;
				case 0x1D: { int value; if (!Pop(value)) return false; ctx.state.controlValueCutIn = float(value); } break;
				case 0x1E: { int value; if (!Pop(value)) return false; ctx.state.singleWidthCutIn = float(value); } break;
				case 0x1F: { int value; if (!Pop(value)) return false; ctx.state.singleWidthValue = float(value); } break;
				case 0x20: if (ctx.stack.empty()) return false; ctx.stack.push_back(ctx.stack.back()); break;
				case 0x21: if (ctx.stack.empty()) return false; ctx.stack.pop_back(); break;
				case 0x22: ctx.stack.clear(); break;
				case 0x23: if (ctx.stack.size() < 2) return false; std::swap(ctx.stack[ctx.stack.size() - 1], ctx.stack[ctx.stack.size() - 2]); break;
				case 0x24: ctx.stack.push_back(int(ctx.stack.size())); break;
				case 0x25:
					{
						int index;
						if (!Pop(index) || index <= 0 || index > ctx.stack.size())
							return false;
						ctx.stack.push_back(ctx.stack[ctx.stack.size() - index]);
					}
					break;
				case 0x26:
					{
						int index;
						if (!Pop(index) || index <= 0 || index > ctx.stack.size())
							return false;
						int value = ctx.stack[ctx.stack.size() - index];
						ctx.stack.removeIndex(ctx.stack.size() - index);
						ctx.stack.push_back(value);
					}
					break;
				case 0x2A:
					{
						int count, functionId;
						if (!Pop2(count, functionId))
							return false;
						for (int i = 0; i < count; ++i)
						{
							if (!(0 <= functionId && functionId < ctx.functions.size()))
								return false;
							TTFunctionDef const& def = ctx.functions[functionId];
							int callIp = def.start;
							if (def.start < 0 || def.end < def.start || !ExecProgram(callIp, def.end, callDepth + 1))
								return false;
						}
					}
					break;
				case 0x2B:
					{
						int functionId;
						if (!Pop(functionId))
							return false;
						if (!(0 <= functionId && functionId < ctx.functions.size()))
							return false;
						TTFunctionDef const& def = ctx.functions[functionId];
						int callIp = def.start;
						if (def.start < 0 || def.end < def.start || !ExecProgram(callIp, def.end, callDepth + 1))
							return false;
					}
					break;
				case 0x2C:
					return false;
				case 0x2E:
				case 0x2F:
					{
						int point;
						if (!Pop(point))
							return false;
						TTZone& zone = GetZone(ctx.state.zp0);
						if (!zone.isValidIndex(point))
							return false;
						float target = GetPointCoord(zone, point, true);
						if (op == 0x2F)
							target = ctx.roundValue(target);
						if (!MovePointToCoord(zone, point, target))
							return false;
						ctx.state.rp0 = ctx.state.rp1 = point;
					}
					break;
				case 0x30:
					InterpolateUntouched(GetZone(ctx.state.zp2), false);
					break;
				case 0x31:
					InterpolateUntouched(GetZone(ctx.state.zp2), true);
					break;
				case 0x38:
					{
						int distance;
						if (!Pop(distance))
							return false;
						for (int i = 0; i < ctx.state.loop; ++i)
						{
							int pointIndex;
							if (!Pop(pointIndex))
								return false;
							TTZone& zone = GetZone(ctx.state.zp2);
							if (!zone.isValidIndex(pointIndex))
								return false;
							ctx.movePoint(zone[pointIndex], float(distance));
						}
						ctx.state.loop = 1;
					}
					break;
				case 0x39:
					{
						TTZone& zone0 = GetZone(ctx.state.zp0);
						TTZone& zone1 = GetZone(ctx.state.zp1);
						TTZone& zone2 = GetZone(ctx.state.zp2);
						if (!zone0.isValidIndex(ctx.state.rp1) || !zone1.isValidIndex(ctx.state.rp2))
							return false;
						float org1 = GetPointCoord(zone0, ctx.state.rp1, false);
						float org2 = GetPointCoord(zone1, ctx.state.rp2, false);
						float cur1 = GetPointCoord(zone0, ctx.state.rp1, true);
						float cur2 = GetPointCoord(zone1, ctx.state.rp2, true);
						for (int i = 0; i < ctx.state.loop; ++i)
						{
							int pointIndex;
							if (!Pop(pointIndex) || !zone2.isValidIndex(pointIndex))
								return false;

							float org = GetPointCoord(zone2, pointIndex, false);
							float target = org;
							if (Math::Abs(org2 - org1) < 1e-6f)
							{
								target = cur1;
							}
							else if (org <= Math::Min(org1, org2))
							{
								target = org + (cur1 - org1);
							}
							else if (org >= Math::Max(org1, org2))
							{
								target = org + (cur2 - org2);
							}
							else
							{
								float alpha = (org - org1) / (org2 - org1);
								target = cur1 + alpha * (cur2 - cur1);
							}
							if (!MovePointToCoord(zone2, pointIndex, target))
								return false;
						}
						ctx.state.loop = 1;
					}
					break;
				case 0x3C:
					{
						TTZone& zone = GetZone(ctx.state.zp1);
						TTZone& refZone = GetZone(ctx.state.zp0);
						if (!refZone.isValidIndex(ctx.state.rp0))
							return false;
						float refCoord = GetPointCoord(refZone, ctx.state.rp0, true);
						for (int i = 0; i < ctx.state.loop; ++i)
						{
							int pointIndex;
							if (!Pop(pointIndex) || !zone.isValidIndex(pointIndex))
								return false;
							if (!MovePointToCoord(zone, pointIndex, refCoord))
								return false;
						}
						ctx.state.loop = 1;
					}
					break;
				case 0x3A:
				case 0x3B:
					{
						int point, distance;
						if (!Pop2(distance, point))
							return false;
						TTZone& zone = GetZone(ctx.state.zp1);
						TTZone& refZone = GetZone(ctx.state.zp0);
						if (!zone.isValidIndex(point) || !refZone.isValidIndex(ctx.state.rp0))
							return false;
						float target = GetPointCoord(refZone, ctx.state.rp0, true) + float(distance);
						if (!MovePointToCoord(zone, point, target))
							return false;
						ctx.state.rp1 = ctx.state.rp0;
						ctx.state.rp2 = point;
						if (op == 0x3B)
							ctx.state.rp0 = point;
					}
					break;
				case 0x3E:
				case 0x3F:
					{
						int point, cvtIndex;
						if (!Pop2(cvtIndex, point))
							return false;
						TTZone& zone = GetZone(ctx.state.zp0);
						if (!zone.isValidIndex(point) || !(0 <= cvtIndex && cvtIndex < ctx.cvt.size()))
							return false;
						float target = ctx.cvt[cvtIndex];
						float current = GetPointCoord(zone, point, true);
						if (op == 0x3F && Math::Abs(target - current) > ctx.state.controlValueCutIn)
							target = current;
						if (op == 0x3F)
							target = ctx.roundValue(target);
						if (!MovePointToCoord(zone, point, target))
							return false;
						ctx.state.rp0 = ctx.state.rp1 = point;
					}
					break;
				case 0x42:
					{
						int location, value;
						if (!Pop2(location, value))
							return false;
						if (0 <= location && location < ctx.storage.size())
							ctx.storage[location] = value;
					}
					break;
				case 0x43:
					{
						int location;
						if (!Pop(location))
							return false;
						ctx.stack.push_back((0 <= location && location < ctx.storage.size()) ? ctx.storage[location] : 0);
					}
					break;
				case 0x44:
					{
						int cvtIndex, value;
						if (!Pop2(cvtIndex, value))
							return false;
						if (0 <= cvtIndex && cvtIndex < ctx.cvt.size())
							ctx.cvt[cvtIndex] = float(value);
					}
					break;
				case 0x45:
					{
						int cvtIndex;
						if (!Pop(cvtIndex))
							return false;
						ctx.stack.push_back((0 <= cvtIndex && cvtIndex < ctx.cvt.size()) ? Math::RoundToInt(ctx.cvt[cvtIndex]) : 0);
					}
					break;
				case 0x46:
				case 0x47:
					{
						int point;
						if (!Pop(point))
							return false;
						TTZone& zone = GetZone(ctx.state.zp2);
						if (!zone.isValidIndex(point))
							return false;
						Vector2 pos = (op == 0x46) ? zone[point].orgPos : zone[point].curPos;
						ctx.stack.push_back(Math::RoundToInt((op == 0x46) ? ctx.dualProject(pos) : ctx.project(pos)));
					}
					break;
				case 0x48:
					{
						int point, value;
						if (!Pop2(point, value))
							return false;
						TTZone& zone = GetZone(ctx.state.zp2);
						if (!zone.isValidIndex(point))
							return false;
						if (!MovePointToCoord(zone, point, float(value)))
							return false;
					}
					break;
				case 0x49:
				case 0x4A:
					{
						int p1, p2;
						if (!Pop2(p1, p2))
							return false;
						TTZone& zone1 = GetZone(ctx.state.zp0);
						TTZone& zone2 = GetZone(ctx.state.zp1);
						if (!zone1.isValidIndex(p1) || !zone2.isValidIndex(p2))
							return false;
						Vector2 a = (op == 0x49) ? zone1[p1].orgPos : zone1[p1].curPos;
						Vector2 b = (op == 0x49) ? zone2[p2].orgPos : zone2[p2].curPos;
						ctx.stack.push_back(Math::RoundToInt((op == 0x49) ? ctx.dualProject(b - a) : ctx.project(b - a)));
					}
					break;
				case 0x4B: ctx.stack.push_back(Math::RoundToInt(ctx.ppem)); break;
				case 0x4C: ctx.stack.push_back(Math::RoundToInt(ctx.ppem)); break;
				case 0x4D: ctx.state.bAutoFlip = true; break;
				case 0x4E: ctx.state.bAutoFlip = false; break;
				case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55:
				case 0x5A: case 0x5B:
					{
						int a, b;
						if (!Pop2(a, b))
							return false;
						bool result = false;
						switch (op)
						{
						case 0x50: result = a < b; break;
						case 0x51: result = a <= b; break;
						case 0x52: result = a > b; break;
						case 0x53: result = a >= b; break;
						case 0x54: result = a == b; break;
						case 0x55: result = a != b; break;
						case 0x5A: result = (a && b); break;
						case 0x5B: result = (a || b); break;
						}
						ctx.stack.push_back(result ? 1 : 0);
					}
					break;
				case 0x56:
				case 0x57:
					{
						int value;
						if (!Pop(value))
							return false;
						bool bOdd = (Math::RoundToInt(ctx.roundValue(float(value)) / TT_F26Dot6) & 1) != 0;
						ctx.stack.push_back(((op == 0x56) ? bOdd : !bOdd) ? 1 : 0);
					}
					break;
				case 0x58:
					{
						int cond;
						if (!Pop(cond))
							return false;
						if (!cond)
						{
							if (!SkipInstructionStream(ip, end, true))
								return false;
						}
					}
					break;
				case 0x5C:
					if (ctx.stack.empty())
						return false;
					ctx.stack.back() = ctx.stack.back() ? 0 : 1;
					break;
				case 0x5D:
					break;
				case 0x5E: { int value; if (!Pop(value)) return false; } break;
				case 0x5F: { int value; if (!Pop(value)) return false; } break;
				case 0x60: case 0x61: case 0x62: case 0x63:
					{
						int a, b;
						if (!Pop2(a, b))
							return false;
						switch (op)
						{
						case 0x60: ctx.stack.push_back(a + b); break;
						case 0x61: ctx.stack.push_back(a - b); break;
						case 0x62: ctx.stack.push_back((b != 0) ? int((int64(a) * 64) / b) : 0); break;
						case 0x63: ctx.stack.push_back(int((int64(a) * b) / 64)); break;
						}
					}
					break;
				case 0x64: if (ctx.stack.empty()) return false; ctx.stack.back() = Math::Abs(ctx.stack.back()); break;
				case 0x65: if (ctx.stack.empty()) return false; ctx.stack.back() = -ctx.stack.back(); break;
				case 0x66: if (ctx.stack.empty()) return false; ctx.stack.back() = Math::FloorToInt(float(ctx.stack.back()) / TT_F26Dot6) * 64; break;
				case 0x67: if (ctx.stack.empty()) return false; ctx.stack.back() = Math::CeilToInt(float(ctx.stack.back()) / TT_F26Dot6) * 64; break;
				case 0x68: case 0x69: case 0x6A: case 0x6B:
				case 0x6C: case 0x6D: case 0x6E: case 0x6F:
					if (ctx.stack.empty())
						return false;
					ctx.stack.back() = Math::RoundToInt(ctx.roundValue(float(ctx.stack.back())));
					break;
				case 0x70:
					{
						int cvtIndex, value;
						if (!Pop2(cvtIndex, value))
							return false;
						if (0 <= cvtIndex && cvtIndex < ctx.cvt.size())
							ctx.cvt[cvtIndex] = float(value) * scale * TT_F26Dot6;
					}
					break;
				case 0x76: case 0x77:
					{
						int value;
						if (!Pop(value))
							return false;
						ctx.state.roundMode = 0;
					}
					break;
				case 0x78:
				case 0x79:
					{
						int offset, cond;
						if (!Pop2(cond, offset))
							return false;
						bool bJump = (op == 0x78) ? (cond != 0) : (cond == 0);
						if (bJump)
							ip += offset - 1;
					}
					break;
				case 0x7A:
					ctx.state.roundMode = 2;
					break;
				case 0x7C:
					ctx.state.roundMode = 1;
					break;
				case 0x7D:
					ctx.state.roundMode = 4;
					break;
				case 0x85:
					{
						int value;
						if (!Pop(value))
							return false;
						ctx.state.scanControl = uint16(value & 0xffff);
					}
					break;
				case 0x8D:
					{
						int value;
						if (!Pop(value))
							return false;
						ctx.state.scanType = value & 0xffff;
					}
					break;
				case 0x8E:
					if (ctx.stack.empty())
						return false;
					ctx.stack.pop_back();
					break;
				case 0x88:
					ctx.stack.push_back(0);
					break;
				case 0x8A:
					if (ctx.stack.size() < 3)
						return false;
					{
						int a = ctx.stack[ctx.stack.size() - 3];
						ctx.stack[ctx.stack.size() - 3] = ctx.stack[ctx.stack.size() - 2];
						ctx.stack[ctx.stack.size() - 2] = ctx.stack[ctx.stack.size() - 1];
						ctx.stack[ctx.stack.size() - 1] = a;
					}
					break;
				case 0x8B:
				case 0x8C:
					{
						int a, b;
						if (!Pop2(a, b))
							return false;
						ctx.stack.push_back((op == 0x8B) ? Math::Max(a, b) : Math::Min(a, b));
					}
					break;
				default:
					if (0xC0 <= op && op <= 0xDF)
					{
						int flags = op - 0xC0;
						int point;
						if (!Pop(point))
							return false;
						TTZone& zone = GetZone(ctx.state.zp1);
						TTZone& refZone = GetZone(ctx.state.zp0);
						if (!zone.isValidIndex(point) || !refZone.isValidIndex(ctx.state.rp0))
							return false;
						float target = GetPointCoord(zone, point, false) - GetPointCoord(refZone, ctx.state.rp0, false);
						if (flags & 0x04)
							target = ctx.roundValue(target);
						if (flags & 0x08)
							target = ApplyMinimumDistance(target);
						target += GetPointCoord(refZone, ctx.state.rp0, true);
						if (!MovePointToCoord(zone, point, target))
							return false;
						ctx.state.rp1 = ctx.state.rp0;
						ctx.state.rp2 = point;
						if (flags & 0x10)
							ctx.state.rp0 = point;
						break;
					}
					if (0xE0 <= op && op <= 0xFF)
					{
						int flags = op - 0xE0;
						int point, cvtIndex;
						if (!Pop2(cvtIndex, point))
							return false;
						TTZone& zone = GetZone(ctx.state.zp1);
						TTZone& refZone = GetZone(ctx.state.zp0);
						if (!zone.isValidIndex(point) || !refZone.isValidIndex(ctx.state.rp0))
							return false;
						float target = (0 <= cvtIndex && cvtIndex < ctx.cvt.size()) ? ctx.cvt[cvtIndex] : 0.0f;
						float orgDist = GetPointCoord(zone, point, false) - GetPointCoord(refZone, ctx.state.rp0, false);
						if (ctx.state.bAutoFlip && ((orgDist < 0) != (target < 0)))
							target = -target;
						if (flags & 0x04)
							target = ctx.roundValue(target);
						if (flags & 0x08)
							target = ApplyMinimumDistance(target);
						target += GetPointCoord(refZone, ctx.state.rp0, true);
						if (!MovePointToCoord(zone, point, target))
							return false;
						ctx.state.rp1 = ctx.state.rp0;
						ctx.state.rp2 = point;
						if (flags & 0x10)
							ctx.state.rp0 = point;
						break;
					}
					return false;
				}
			}

			return true;
		};

		ParseFunctionDefs(mFpgmRange);
		if (mPrepRange.length)
		{
			int prepIp = int(mPrepRange.offset);
			if (!ExecProgram(prepIp, int(mPrepRange.offset + mPrepRange.length), 0))
				return false;
		}

		if (!ioGlyph.instructions.empty())
		{
			size_t tempStart = mBuffer.size();
			TArray< uint8 > tempBuffer = mBuffer;
			tempBuffer.append(ioGlyph.instructions.begin(), ioGlyph.instructions.end());
			auto oldBuffer = mBuffer;
			const_cast< TrueTypeFileLoader* >(this)->mBuffer.swap(tempBuffer);
			int glyphIp = int(tempStart);
			bool bExecOk = ExecProgram(glyphIp, int(tempStart + ioGlyph.instructions.size()), 0);
			const_cast< TrueTypeFileLoader* >(this)->mBuffer.swap(tempBuffer);
			if (!bExecOk)
				return false;
		}

		ioGlyph.segments.clear();
		ioGlyph.contours.clear();
		ioGlyph.bound.invalidate();
		uint16 contourStart = 0;
		for (int contourIndex = 0; contourIndex < ioGlyph.endPts.size(); ++contourIndex)
		{
			uint16 contourEnd = ioGlyph.endPts[contourIndex];
			TArray< GlyphPoint > points;
			points.resize(contourEnd - contourStart + 1);
			for (int i = contourStart; i <= contourEnd; ++i)
			{
				points[i - contourStart].pos = glyphPoints[i].curPos / (scale * TT_F26Dot6);
				points[i - contourStart].bOnCurve = glyphPoints[i].bOnCurve;
				ioGlyph.bound.addPoint(points[i - contourStart].pos);
			}
			if (!appendContour(points, 0, uint16(points.size() - 1), RenderTransform2D::Identity(), ioGlyph.segments, &ioGlyph.contours))
				return false;
			contourStart = contourEnd + 1;
		}
		ioGlyph.bHasOutline = !ioGlyph.segments.empty();
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

		TArray< GlyphPoint > points;
		TArray< uint16 > endPts;
		if (!loadSimpleGlyphPoints(glyphStart, glyphEnd, numContours, points, endPts))
			return false;

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

	bool TrueTypeFileLoader::loadSimpleGlyphPoints(uint32 glyphStart, uint32 glyphEnd, int numContours, TArray< GlyphPoint >& outPoints, TArray< uint16 >& outEndPts) const
	{
		if (numContours <= 0)
		{
			outPoints.clear();
			outEndPts.clear();
			return true;
		}

		size_t pos = glyphStart + sizeof(int16) + sizeof(int16) * GlyphBoundValueCount;
		if (pos + sizeof(uint16) * numContours + GlyphInstructionLengthSize > glyphEnd)
			return false;

		outEndPts.resize(numContours);
		for (int i = 0; i < numContours; ++i)
		{
			outEndPts[i] = readU16(pos);
		}

		uint16 numPoints = outEndPts.back() + 1;
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

		outPoints.resize(numPoints);
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
			outPoints[i].pos.x = float(curX);
			outPoints[i].bOnCurve = !!(flag & GlyphFlagOnCurve);
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
			outPoints[i].pos.y = float(curY);
		}

		return true;
	}

	bool TrueTypeFileLoader::appendGlyphOutline(uint16 glyphIndex, RenderTransform2D const& transform, TArray< GlyphPoint >& outPoints, TArray< uint16 >& outEndPts, int depth) const
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
		pos = glyphStart + GlyphHeaderSize;
		if (numContours >= 0)
		{
			TArray< GlyphPoint > points;
			TArray< uint16 > endPts;
			if (!loadSimpleGlyphPoints(glyphStart, glyphEnd, numContours, points, endPts))
				return false;

			uint16 pointBase = uint16(outPoints.size());
			outPoints.reserve(outPoints.size() + points.size());
			for (GlyphPoint const& point : points)
			{
				outPoints.push_back({ transform.transformPosition(point.pos), point.bOnCurve });
			}

			outEndPts.reserve(outEndPts.size() + endPts.size());
			for (uint16 contourEnd : endPts)
			{
				outEndPts.push_back(pointBase + contourEnd);
			}
			return true;
		}

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

			if (flags & ArgsAreXYValues)
			{
				Vector2 componentOffset{ float(arg1), float(arg2) };
				if ((flags & ScaledComponentOffset) && !(flags & UnscaledComponentOffset))
				{
					component.P = component.M.leftMul(componentOffset);
				}
				else
				{
					component.P = componentOffset;
				}
			}
			else
			{
				TArray< GlyphPoint > componentPoints;
				TArray< uint16 > componentEndPts;
				if (!appendGlyphOutline(componentGlyphIndex, RenderTransform2D(component.M, Vector2::Zero()), componentPoints, componentEndPts, depth + 1))
					return false;
				if (arg1 < 0 || arg2 < 0 || arg1 >= int(outPoints.size()) || arg2 >= int(componentPoints.size()))
					return false;

				component.P = outPoints[arg1].pos - componentPoints[arg2].pos;
			}

			if (flags & RoundXYToGrid)
			{
				component.P.x = Math::Round(component.P.x);
				component.P.y = Math::Round(component.P.y);
			}

			if (!appendGlyphOutline(componentGlyphIndex, component * transform, outPoints, outEndPts, depth + 1))
				return false;

			bMore = !!(flags & MoreComponents);
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
		TArray< GlyphPoint > compositePoints;
		TArray< uint16 > compositeEndPts;
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

			if (flags & ArgsAreXYValues)
			{
				Vector2 componentOffset{ float(arg1), float(arg2) };
				if ((flags & ScaledComponentOffset) && !(flags & UnscaledComponentOffset))
				{
					component.P = component.M.leftMul(componentOffset);
				}
				else
				{
					component.P = componentOffset;
				}
			}
			else
			{
				TArray< GlyphPoint > componentPoints;
				TArray< uint16 > componentEndPts;
				if (!appendGlyphOutline(componentGlyphIndex, RenderTransform2D(component.M, Vector2::Zero()), componentPoints, componentEndPts, depth + 1))
					return false;

				if (arg1 < 0 || arg2 < 0 || arg1 >= int(compositePoints.size()) || arg2 >= int(componentPoints.size()))
					return false;

				component.P = compositePoints[arg1].pos - componentPoints[arg2].pos;
			}

			if (flags & RoundXYToGrid)
			{
				component.P.x = Math::Round(component.P.x);
				component.P.y = Math::Round(component.P.y);
			}

			if (!appendGlyphOutline(componentGlyphIndex, component, compositePoints, compositeEndPts, depth + 1))
				return false;
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

	bool TrueTypeCharDataProvider::initialize(HashString name, ITrueTypeLoader& loader, int pixelSize, int sampleFactor /*= 4*/)
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

	bool TrueTypeCharDataProvider::initialize(HashString name, ITrueTypeLoader& loader, int pixelSize, int fontHeight, int baseline, int sampleFactor /*= 4*/)
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

		ITrueTypeLoader::GlyphData glyph;
		if (!mLoader->loadGlyph(charWord, glyph))
			return false;
		if (!mLoader->hintGlyph(glyph, float(mPixelSize)))
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


