#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "LifeCore.h"

#include "Math/GeometryPrimitive.h"
#include "DataStructure/Grid2D.h"
#include "RHI/RHIGraphics2D.h"
#include "ConsoleSystem.h"

namespace Life
{
	using namespace Render;

	class Viewport
	{
	public:

		Vec2i   screenSize;
		float   cellLength;
		Vector2 pos;
		float   zoomOut;

		RenderTransform2D xform;
		float cellRenderScale;

		void updateTransform()
		{
			cellRenderScale = cellLength / zoomOut;
			xform.setIdentity();
			xform.translateLocal(0.5 * Vector2(screenSize));
			xform.scaleLocal(Vector2(cellRenderScale, cellRenderScale));
			Vector2 offset = -pos;
			xform.translateLocal(offset);
		}


		BoundBox calcBound(Vector2 const& min, Vector2 const& max) const
		{
			BoundBox result;
			Vector2 cellMinVP = xform.transformInvPosition(min);
			result.min.x = Math::FloorToInt(cellMinVP.x);
			result.min.y = Math::FloorToInt(cellMinVP.y);
			Vector2 cellMaxVP = xform.transformInvPosition(max);
			result.max.x = Math::CeilToInt(cellMaxVP.x);
			result.max.y = Math::CeilToInt(cellMaxVP.y);
			return result;
		}

		BoundBox getViewBound() const
		{
			return calcBound(Vector2::Zero(), Vector2::Zero() + screenSize);
		}
	};

	class IRenderer
	{
	public:
		virtual void draw(RHIGraphics2D& g, Viewport const& viewport, BoundBox const& boundRender) = 0;
	
		static void DrawCell(RHIGraphics2D& g , int x , int y)
		{
			g.drawRect(Vector2(x, y), Vector2(1, 1));
		}
	};

	Vec2i constexpr NegihborOffsets[] =
	{
		Vec2i(1,0),Vec2i(1,1),Vec2i(0,1),Vec2i(-1,1),
		Vec2i(-1,0),Vec2i(-1,-1),Vec2i(0,-1),Vec2i(1,-1),
	};

	class Rule
	{
	public:
		Rule()
		{
			std::fill_n(mEvolvetinMap, ARRAY_SIZE(mEvolvetinMap), 0);
			mEvolvetinMap[(3 << 1) + 1] = 1;
			mEvolvetinMap[(2 << 1) + 1] = 1;
			mEvolvetinMap[(3 << 1) + 0] = 1;
		}

		uint8 getEvoluteValue(uint32 count , uint8 value)
		{
			uint32 index = (count << 1) | value;
			return mEvolvetinMap[index];
		}

		uint8 mEvolvetinMap[2 * 9];
	};


	class SimpleAlgo : public IAlgorithm
	{
	public:
		SimpleAlgo(int x, int y)
		{
			mGrid.resize(x, y);
			mGrid.fillValue({ 0, 0});
		}
		virtual bool setCell(int x, int y, uint8 value) final
		{
			if (mGrid.checkRange(x, y))
			{
				mGrid(x, y).data[mIndex] = value;
				return true;
			}
			return false;
		}
		virtual uint8 getCell(int x, int y) final
		{
			if (mGrid.checkRange(x, y))
			{
				return mGrid(x, y).data[mIndex];
			}
			return 0;
		}
		virtual void clearCell() final
		{
			mGrid.fillValue({ 0, 0 });
		}
		virtual BoundBox getBound() final
		{
			return getLimitBound();
		}
		virtual BoundBox getLimitBound() final
		{
			BoundBox result;
			result.min = Vec2i::Zero();
			result.max = Vec2i(mGrid.getSizeX() - 1, mGrid.getSizeY() - 1);
			return result;
		}
		virtual void  step() final
		{
			int indexPrev = mIndex;
			mIndex = 1 - mIndex;

			for (int j = 0; j < mGrid.getSizeY(); ++j)
			{
				for (int i = 0; i < mGrid.getSizeX(); ++i)
				{
					uint32 count = 0;
					for (auto const& offset : NegihborOffsets)
					{
						Vec2i pos = Vec2i(i, j) + offset;
						if (mGrid.checkRange(pos.x, pos.y))
						{
							if ( mGrid(pos.x, pos.y).data[indexPrev] )
							{
								++count;
							}
						}
					}

					auto data = mGrid(i, j).data;
					data[mIndex] = mRule.getEvoluteValue(count, data[indexPrev]);
				}
			}
		}

		virtual void  getPattern(std::vector<Vec2i>& outList) final
		{
			for (int j = 0; j < mGrid.getSizeY(); ++j)
			{
				for (int i = 0; i < mGrid.getSizeX(); ++i)
				{
					auto data = mGrid(i, j).data[mIndex];
					if (data)
					{
						outList.push_back(Vec2i(i, j));
					}
				}
			}
		}

		virtual void  getPattern(BoundBox const& bound , std::vector<Vec2i>& outList) final
		{
			BoundBox boundClip = bound.intersection(getBound());
			for (int j = boundClip.min.y; j <= boundClip.max.y; ++j)
			{
				for (int i = boundClip.min.x; i <= boundClip.min.x; ++i)
				{
					auto data = mGrid(i, j).data[mIndex];
					if (data)
					{
						outList.push_back(Vec2i(i, j));
					}
				}
			}
		}

		struct DataValue
		{
			uint8 data[2];
		};
		int mIndex = 0;
		TGrid2D< DataValue > mGrid;
		Rule mRule;
	};

	template< typename T >
	T  AlignUp(T  value , T  align)
	{
		return (value + align - 1) / align;
	}
#define USE_CHUNK_COUNT 1
	class ChunkAlgo : public IAlgorithm , public IRenderer
	{
	public:
		ChunkAlgo(int32 x, int32 y)
		{
			for (int i = 0; i < 2; ++i)
			{
				mChunkMap.resize(AlignUp(x, ChunkLength) , AlignUp(y, ChunkLength));
				mChunkMap.fillValue(nullptr);
			}

			mChunkBound.invalidate();
		}

		virtual bool setCell(int x, int y, uint8 value) final
		{
			Vec2i cPos = ToChunkPos(x, y);

			if (mChunkMap.checkRange(cPos))
			{
				if (value)
				{
					auto chunk = fetchChunk(cPos);
					int ix = x - cPos.x * ChunkLength;
					int iy = y - cPos.y * ChunkLength;

					uint8& data = chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)];
					if (data == 0)
					{
						data = value;
#if USE_CHUNK_COUNT
						++chunk->count;
						chunk->bHadSleeped = false;
#endif
					}
				}
				else
				{
					auto chunk = mChunkMap(cPos);
					if (chunk)
					{
						int ix = x - cPos.x * ChunkLength;
						int iy = y - cPos.y * ChunkLength;

						uint8& data = chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)];
						if (data )
						{
							data = 0;
#if USE_CHUNK_COUNT
							--chunk->count;
#endif
						}
					}
				}

				return true;
			}
			return false;
		}
		virtual uint8 getCell(int x, int y) final
		{
			Vec2i cPos = ToChunkPos(x, y);

			if (mChunkMap.checkRange(cPos))
			{
				auto chunk = mChunkMap(cPos);
				if (chunk && chunk->count)
				{
					int ix = x - cPos.x * ChunkLength;
					int iy = y - cPos.y * ChunkLength;
					return chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)];
				}
			}
			return 0;
		}
		virtual void clearCell() final
		{
			for (auto chunk : mChunks)
			{
#if USE_CHUNK_COUNT
				if (chunk->bHadSleeped)
					continue;

				chunk->bHadSleeped = true;
				chunk->count = 0;
#endif
				uint8* pChunkData = chunk->data[0];
				memset(pChunkData, 0, 2 * ChunkDataSize);
			}
		}
		virtual BoundBox getBound() final
		{
			BoundBox result;
			if (mChunkBound.isValid())
			{
				result.min = ChunkLength * mChunkBound.min;
				result.max = ChunkLength * mChunkBound.max;
			}
			else
			{
				result.invalidate();
			}
			return result;
		}

		virtual BoundBox getLimitBound() final
		{
			BoundBox result;
			result.min = Vec2i::Zero();
			result.max = ChunkLength * mChunkMap.getSize() - Vec2i(1,1);
			return result;
		}

		virtual void  step() final
		{
			int indexPrev = mIndex;
			mIndex = 1 - mIndex;

			for (int indexChunk = 0; indexChunk < mChunks.size(); ++indexChunk)
			{
				auto chunk = mChunks[indexChunk];

				auto CheckEdge = [this, chunk, indexPrev](Vec2i const& cPosSrc, uint32 srcOffset, uint32 destOffset, uint32 checkOffst, uint32 offset) -> uint32
				{			
					if (!mChunkMap.checkRange(cPosSrc))
						return 0;
		
					auto& chunkSrc = mChunkMap(cPosSrc);
					if (chunkSrc == nullptr)
					{
#if USE_CHUNK_COUNT
						if (chunk->count == 0)
							return 0;
#endif
						uint8 const* check = chunk->data[indexPrev] + checkOffst;
						int index = 0;
						for (; index < ChunkLength; ++index)
						{
							if (*check)
								break;
							check += offset;
						}
						if (index == ChunkLength)
							return 0;

						chunkSrc = createChunk(cPosSrc);
					}

					uint8 const* src = chunkSrc->data[indexPrev] + srcOffset;
					uint8* dest = chunk->data[indexPrev] + destOffset;
					uint32 count = 0;
					for (int i = 0; i < ChunkLength; ++i)
					{
						*dest = *src;
						if (*dest)
						{
							++count;
						}

						dest += offset;
						src += offset;
					}
					return count;
				};

				uint32 neighborCount = 0;
				neighborCount += CheckEdge(chunk->pos + Vec2i(0,-1) , Chunk::ToDataIndex(0, ChunkLength - 1),
					Chunk::ToDataIndex(0, -1), Chunk::ToDataIndex(0, 0), 1);
				neighborCount += CheckEdge(chunk->pos + Vec2i(0, 1), Chunk::ToDataIndex(0, 0),
					Chunk::ToDataIndex(0, ChunkLength), Chunk::ToDataIndex(0, ChunkLength - 1), 1);
				neighborCount += CheckEdge(chunk->pos + Vec2i(-1, 0), Chunk::ToDataIndex(ChunkLength - 1, 0),
					Chunk::ToDataIndex(-1, 0), Chunk::ToDataIndex(0, 0), ChunkLength + 2);
				neighborCount += CheckEdge(chunk->pos + Vec2i(1, 0), Chunk::ToDataIndex(0, 0),
					Chunk::ToDataIndex(ChunkLength, 0), Chunk::ToDataIndex(ChunkLength - 1 , 0), ChunkLength + 2);

				auto CheckCorner = [this, chunk, indexPrev](Vec2i const& cPosSrc, uint32 srcOffset, uint32 destOffset, uint32 checkOffst) -> uint32
				{
					if (!mChunkMap.checkRange(cPosSrc))
						return 0;

					auto& chunkSrc = mChunkMap(cPosSrc);
					if (chunkSrc == nullptr)
					{
#if USE_CHUNK_COUNT
						if (chunk->count == 0)
							return 0;
#endif
						uint8* check = chunk->data[indexPrev] + checkOffst;
						if (*check == 0)
							return 0;

						chunkSrc = createChunk(cPosSrc);
					}

					uint8* dest = chunk->data[indexPrev] + destOffset;
					uint8 const* src = chunkSrc->data[indexPrev] + srcOffset;
					*dest = *src;

					return (*dest) ? 1 : 0;
				};
				neighborCount += CheckCorner(chunk->pos + Vec2i(-1, -1), Chunk::ToDataIndex(ChunkLength - 1, ChunkLength - 1),
					Chunk::ToDataIndex(-1,-1), Chunk::ToDataIndex(0, 0));
				neighborCount += CheckCorner(chunk->pos + Vec2i(1, -1), Chunk::ToDataIndex(0, ChunkLength - 1),
					Chunk::ToDataIndex(ChunkLength, -1), Chunk::ToDataIndex(ChunkLength - 1, 0));
				neighborCount += CheckCorner(chunk->pos + Vec2i(-1, 1), Chunk::ToDataIndex(ChunkLength - 1, 0),
					Chunk::ToDataIndex(-1, ChunkLength), Chunk::ToDataIndex(0, ChunkLength - 1));
				neighborCount += CheckCorner(chunk->pos + Vec2i(1, 1), Chunk::ToDataIndex(0, 0),
					Chunk::ToDataIndex(ChunkLength, ChunkLength), Chunk::ToDataIndex(ChunkLength - 1, ChunkLength - 1));
#if USE_CHUNK_COUNT
				if (chunk->count == 0 && neighborCount == 0)
				{
					if (!chunk->bHadSleeped)
					{
						uint8* pChunkData = chunk->data[0];
						memset(pChunkData, 0, 2 * ChunkDataSize);
						chunk->bHadSleeped = true;
					}
					continue;
				}
				chunk->bHadSleeped = false;
				chunk->count = 0;
#endif

				constexpr int NegihborIndexOffsets[] =
				{
					1, ChunkDataStrideLength + 1 ,ChunkDataStrideLength ,ChunkDataStrideLength -1,
					-1,-ChunkDataStrideLength-1,-ChunkDataStrideLength,1-ChunkDataStrideLength,
				};

				uint8* pChunkData = chunk->data[mIndex];
				uint8 const* pChunkDataPrev = chunk->data[indexPrev];
				for (int j = 0; j < ChunkLength; ++j)
				{
					for (int i = 0; i < ChunkLength ; ++i)
					{
						int dataIndex = Chunk::ToDataIndex(i, j);
						uint32 count = 0;
						for (int offset : NegihborIndexOffsets)
						{
							if (pChunkDataPrev[dataIndex + offset])
							{
								++count;
							}
						}

						pChunkData[dataIndex] = mRule.getEvoluteValue(count, pChunkDataPrev[dataIndex]);
#if USE_CHUNK_COUNT
						if (pChunkData[dataIndex])
						{
							++chunk->count;
						}
#endif
					}
				}
			}
		}

		virtual void draw(RHIGraphics2D& g, Viewport const& viewport, BoundBox const& boundRender)
		{
			BoundBox chunkBound;
			chunkBound.min = ToChunkPos(boundRender.min);
			chunkBound.max = ToChunkPos(boundRender.max);
			for (int cy = chunkBound.min.y; cy <= chunkBound.max.y; ++cy)
			{
				for (int cx = chunkBound.min.x; cx <= chunkBound.max.x; ++cx)
				{
					if ( !mChunkMap.checkRange(cx,cy) )
						continue;
					
					auto chunk = mChunkMap(cx, cy);
#if USE_CHUNK_COUNT
					if ( chunk == nullptr || chunk->count == 0)
						continue;
#else
					if (chunk == nullptr )
						continue;
#endif
					int ox = cx * ChunkLength;
					int oy = cy * ChunkLength;

					uint8 const* pChunkData = chunk->data[mIndex];

					for (int j = 0; j < ChunkLength; ++j)
					{
						for (int i = 0; i < ChunkLength; ++i)
						{
							if (pChunkData[Chunk::ToDataIndex(i, j)])
							{
								DrawCell(g, ox + i, oy + j);
							}
						}
					}
				}
			}
		}

		IRenderer* getRenderer() { return this; }


		virtual void  getPattern(std::vector<Vec2i>& outList) final
		{
			for (int indexChunk = 0; indexChunk < mChunks.size(); ++indexChunk)
			{
				auto chunk = mChunks[indexChunk];

#if USE_CHUNK_COUNT
				if (chunk->count == 0)
					continue;
#endif
				Vec2i chunkPos = ChunkLength * chunk->pos;

				uint8 const* pChunkData = chunk->data[mIndex];

				for (int j = 0; j < ChunkLength; ++j)
				{
					for (int i = 0; i < ChunkLength; ++i)
					{
						if (pChunkData[Chunk::ToDataIndex(i, j)])
						{
							outList.push_back(chunkPos + Vec2i(i, j));
						}
					}
				}
			}
		}


		virtual void  getPattern(BoundBox const& bound, std::vector<Vec2i>& outList) final
		{
			BoundBox boundClip = bound.intersection(getBound());

			BoundBox chunkBound;
			chunkBound.min = ToChunkPos(boundClip.min);
			chunkBound.max = ToChunkPos(boundClip.max);

			for (int indexChunk = 0; indexChunk < mChunks.size(); ++indexChunk)
			{
				auto chunk = mChunks[indexChunk];
#if USE_CHUNK_COUNT
				if (chunk->count == 0)
					continue;
#endif
				if (!chunkBound.isInside(chunk->pos))
					continue;

				Vec2i chunkPos = ChunkLength * chunk->pos;
				uint8 const* pChunkData = chunk->data[mIndex];

				for (int j = 0; j < ChunkLength; ++j)
				{
					for (int i = 0; i < ChunkLength; ++i)
					{
						Vec2i pos = chunkPos + Vec2i(i, j);
						if (!bound.isInside(pos) )
							continue;

						if (pChunkData[Chunk::ToDataIndex(i, j)])
						{
							outList.push_back(pos);
						}
					}
				}
			}
		}


		static constexpr int32 ChunkLength = 64; //1 << 8;
		static constexpr int32 ChunkDataStrideLength = ChunkLength + 2;
		static constexpr int32 ChunkDataSize = ChunkDataStrideLength * ChunkDataStrideLength;

		static constexpr int ToChunkValue(int value)
		{
			return Math::ToTileValue(value, ChunkLength);
		}
		static constexpr Vec2i ToChunkPos(Vec2i const& pos)
		{
			return Vec2i(ToChunkValue(pos.x), ToChunkValue(pos.y));
		}
		static constexpr Vec2i ToChunkPos(int x, int y)
		{
			return Vec2i(ToChunkValue(x), ToChunkValue(y));
		}

		bool CheckChunkPos()
		{
			static_assert(ToChunkPos( 0, ChunkLength - 1) == Vec2i(0, 0));
			static_assert(ToChunkPos(ChunkLength, 2 *ChunkLength - 1) == Vec2i(1, 1));
			static_assert(ToChunkPos(-1, -ChunkLength) == Vec2i(-1, -1));
			static_assert(ToChunkPos(-ChunkLength - 1, -2  *ChunkLength) == Vec2i(-2,-2));
			static_assert(ToChunkPos(-2 *ChunkLength - 1, -3 * ChunkLength) == Vec2i(-3, -3));
		}

		struct Chunk
		{
			uint8 data[2][ChunkDataSize];

#if USE_CHUNK_COUNT
			uint32 count;
			bool   bHadSleeped;
#endif
			int x, y;
			Vec2i pos;

			static int ToDataIndex(int i, int j)
			{
				return (i + 1) + ( j + 1 ) * ChunkDataStrideLength;
			}
		};

		Chunk* createChunk(Vec2i const& cPos)
		{

			Chunk* chunk = new Chunk;
			memset(chunk, 0, sizeof(Chunk));
			//std::fill_n(chunk->data[0], ARRAY_SIZE(chunk->data[0]), 0);
			//std::fill_n(chunk->data[1], ARRAY_SIZE(chunk->data[1]), 0);
			chunk->pos = cPos;
#if USE_CHUNK_COUNT
			chunk->bHadSleeped = true;
#endif
			mChunks.push_back(chunk);
			mChunkBound.addPoint(cPos);
			return chunk;
		}

		Chunk* fetchChunk(Vec2i const& cPos)
		{
			auto& chunk = mChunkMap(cPos);
			if (chunk == nullptr)
			{
				chunk = createChunk(cPos);
			}
			return chunk;
		}

		Rule  mRule;
		int   mIndex = 0;
		BoundBox mChunkBound;
		TGrid2D< Chunk* > mChunkMap;
		std::vector< Chunk* > mChunks;
	};


	class SelectionRect : public GWidget
	{
	public:
		SelectionRect(Vec2i const& pos, Vec2i const& size, GWidget* parent)
			:GWidget( pos - BoxSize , size + 2 * BoxSize , parent )
		{

		}

		constexpr static int BoxLength = 10;
		constexpr static Vec2i BoxSize = Vec2i(BoxLength, BoxLength);
		void onRender()
		{
			IGraphics2D& g = Global::GetIGraphics2D();
			Vector2 size = getSize();
			Vector2 pos = getWorldPos();

			Vector2 center = pos + 0.5 * size;

			Vector2 boxSize = Vector2(BoxLength, BoxLength);
			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::White);
			for (int i = 0; i < 8; ++i)
			{
				Vector2 offset = ( 0.5 * size ) * GetBoxOffset()[i] - 0.5 * ( GetBoxOffset()[i] + Vector2(1,1) ) * boxSize;
				g.drawRect(center + offset, BoxSize);
			}

			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(pos + boxSize, size - 2 * boxSize);
		}

		BoundBox getSelection()
		{
			BoundBox result;
			result.min = getWorldPos() + Vec2i(BoxLength, BoxLength);
			result.max = result.min + getSize() - 2 * Vec2i(BoxLength, BoxLength);
			return result;
		}

		Vector2 const* GetBoxOffset()
		{
			static Vector2 result[] =
			{
				Vector2(-1,-1) , Vector2(-1,0) , Vector2(-1,1) ,
				Vector2(0,-1) , Vector2(0,1) ,
				Vector2(1,-1) , Vector2(1,0) , Vector2(1,1) ,
			};
			return result;
		}

		int hitBoxTest( Vector2 const& hitPos)
		{
			Vector2 size = getSize();
			Vector2 pos = getWorldPos();

			Vector2 center = pos + 0.5 * size;
	
			for (int i = 0; i < 8; ++i)
			{
				Vector2 offset = (0.5 * size) * GetBoxOffset()[i] - 0.5 * (GetBoxOffset()[i] + Vector2(1, 1)) * BoxSize;
				TRect< float > rect;
				rect.min = center + offset;
				rect.max = rect.min + BoxSize;
				if (rect.hitTest(hitPos))
					return i;
			}

			return INDEX_NONE;
		}

		int32 mIndexBoxDragging = INDEX_NONE;
		Vec2i mLastTrackingPos;
		bool  bStartDragging = false;
		void startResize(Vec2i const& pos)
		{
			bStartDragging = true;
			mLastTrackingPos = pos + Vec2i(BoxLength,BoxLength);
			mIndexBoxDragging = 7;
			mManager->focusWidget(this);
			mManager->captureMouse(this);
		}

		bool onMouseMsg(MouseMsg const& msg)
		{
			if (msg.onLeftDown())
			{
				int indexBox = hitBoxTest(msg.getPos());

				bStartDragging = true;
				mIndexBoxDragging = indexBox;
				mLastTrackingPos = msg.getPos();
				getManager()->captureMouse(this);
				return false;
			}
			else if (msg.onLeftUp())
			{
				if (bStartDragging)
				{
					bStartDragging = false;
					mIndexBoxDragging = INDEX_NONE;
					getManager()->releaseMouse();
					return false;
				}
			}
			else if (msg.onRightDown())
			{
				if (onLock)
					onLock();
			}
			else if (msg.onMoving())
			{
				if (bStartDragging)
				{
					Vector2 offset = msg.getPos() - mLastTrackingPos;
					if (mIndexBoxDragging == INDEX_NONE)
					{
						setPos(getPos() + offset);
					}
					else
					{
						Vector2 boxOffset = GetBoxOffset()[mIndexBoxDragging];
						Vector2 delatSize = boxOffset.mul(offset);
						Vector2 size = getSize() + delatSize;
						Vector2 posOffset = Vector2::Zero();

						Vector2 newSize = getSize() + delatSize;
						if (newSize.x < 2 * BoxLength)
						{
							newSize.x = 2 * BoxLength;
						}
						if (newSize.y < 2 * BoxLength)
						{
							newSize.y = 2 * BoxLength;
						}
						setSize(newSize);
						if (boxOffset.x < 0)
						{
							posOffset.x = delatSize.x;
						}
						if (boxOffset.y < 0)
						{
							posOffset.y = delatSize.y;
						}
	
						setPos(getPos() - posOffset);
					}
	
					mLastTrackingPos = msg.getPos();
					return false;
				}
			}

			return !bStartDragging;
		}

		std::function< void() > onLock;
	};

	enum class EditMode : uint8
	{
		AddCell,
		SelectCell ,
		PastePattern ,
	
	};

	struct EditPattern
	{


		struct MyHash
		{
			std::size_t operator()(Vec2i const& s) const noexcept
			{
				uint32 result = HashValue(s.x);
				HashCombine(result, s.y);
				return result;
			}
		};
		std::unordered_set< Vec2i , MyHash > cellList;
		BoundBox bound;

		void mirrorH()
		{
			CHECK(bound.isValid());

			std::unordered_set< Vec2i, MyHash > temp = std::move(cellList);

			for (auto& pos : temp)
			{
				Vec2i posNew;
				posNew.x = bound.max.x - pos.x;
				posNew.y = pos.y;
				cellList.insert(posNew);
			}
		}

		void mirrorV()
		{
			CHECK(bound.isValid());

			std::unordered_set< Vec2i, MyHash > temp = std::move(cellList);

			for (auto& pos : temp)
			{
				Vec2i posNew;
				posNew.y = bound.max.y - pos.y;
				posNew.x = pos.x;
				cellList.insert(posNew);
			}
		}

		void rotate()
		{
			CHECK(bound.isValid());

			std::unordered_set< Vec2i, MyHash > temp = std::move(cellList);
			for (auto& pos : temp)
			{
				Vec2i posNew;
				posNew.x = bound.max.y - pos.y;
				posNew.y = pos.x;
				cellList.insert(posNew);
			}

			bound.max = Vec2i(bound.max.y, bound.max.x);
		}

		void validate()
		{
			if (bound.isValid() == false)
			{
				for (auto const& pos : cellList)
				{
					bound.addPoint(pos);
				}

				std::unordered_set< Vec2i, MyHash > temp = std::move(cellList);
				for (auto const& pos : temp)
				{
					cellList.insert(pos - bound.min);
				}

				bound.translate(-bound.min);
			}
		};
	};


	class TestStage : public StageBase
		            , public IGameRenderSetup
	{
		using BaseClass = StageBase;
	public:
		TestStage() {}

		float evolateTimeRate = 30;
		float accEvolateCount = 0.0;
		IAlgorithm* mAlgorithm = nullptr;
		bool bRunEvolate = false;
		Viewport mViewport;


		bool onInit() override;

		void onEnd() override
		{
			auto& console = ConsoleSystem::Get();
			console.unregisterAllCommandsByObject(this);

			BaseClass::onEnd();
		}

		bool savePattern(char const* name);
		bool loadPattern(char const* name);
		bool loadGolly(char const* name);

		void restart() 
		{
			//mAlgorithm = new SimpleAlgo(1024, 1024);
			mAlgorithm = new ChunkAlgo(1024, 1024);
			evolateTimeRate = 15;
			accEvolateCount = 0.0;
			bRunEvolate = false;
			BoundBox bound = mAlgorithm->getLimitBound();
			if (bound.isValid())
			{
				mViewport.pos = 0.5 * Vector2(bound.min + bound.max) + Vector2(0.5, 0.5);
			}
			else
			{
				mViewport.pos = Vector2::Zero();
			}

			mViewport.zoomOut = 1;
			mViewport.cellLength = 10.0;
			mViewport.screenSize = ::Global::GetScreenSize();
			mViewport.updateTransform();
		}

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			if (bRunEvolate)
			{
				accEvolateCount += evolateTimeRate * time / 1000.0;
				while (accEvolateCount > 0)
				{
					mAlgorithm->step();
					accEvolateCount -= 1;
				}
			}
			else
			{
				accEvolateCount = 0;
			}
		}

		float Remap(float v, float is, float ie, float os, float oe)
		{
			return Math::Lerp(os, oe, (v - is) / (ie - is));
		}

		void onRender(float dFrame) override;

		bool bStartDragging = false;
		Vector2 draggingPos;
		SelectionRect* mSelectionRect = nullptr;

		Vec2i mLastMousePos;

		EditMode mEditMode;

		EditPattern mCopyPattern;

		void selectPattern(bool bAppend = false)
		{
			CHECK(mSelectionRect);

			BoundBox bound = mSelectionRect->getSelection();

			BoundBox boundSelect = mViewport.calcBound(bound.min, bound.max);
			std::vector<Vec2i> posList;
			mAlgorithm->getPattern(boundSelect, posList);

			mCopyPattern.bound.invalidate();

			if (!bAppend)
			{
				mCopyPattern.cellList.clear();
			}
			for (auto const& pos : posList)
			{
				mCopyPattern.cellList.insert(pos);
			}

			mSelectionRect->destroy();
			mSelectionRect = nullptr;
		}

		bool onMouse(MouseMsg const& msg) override;
		bool onKey(KeyMsg const& msg) override;

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

		void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;

	protected:
	};




}//namespace Life