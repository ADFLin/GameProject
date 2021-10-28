#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"


#include "Math/GeometryPrimitive.h"
#include "DataStructure/Grid2D.h"
#include "RHI/RHIGraphics2D.h"
#include "ConsoleSystem.h"
#include "Serialize/SerializeFwd.h"


namespace Life
{
	using namespace Render;
	using BoundBox = ::Math::TAABBox< Vec2i >;


	class Viewport
	{
	public:
		Vector2 pos;
		Vector2 size;
		float   zoomOut;

		RenderTransform2D xform;

		void updateTransform(Vec2i const& screenSize)
		{
			Vector2 cellSize = Vector2(screenSize).div(zoomOut * size);
			float cellScale = cellSize.x;

			xform.setIdentity();
			xform.translateLocal(Vector2(screenSize) / 2);
			xform.scaleLocal(Vector2(cellScale, cellScale));
			Vector2 offset = -pos;
			xform.translateLocal(offset);
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

	class IAlgorithm
	{
	public:
		virtual ~IAlgorithm(){}
		virtual bool  setCell(int x, int y, uint8 value) = 0;
		virtual uint8 getCell(int x, int y) = 0;
		virtual void  clearCell() = 0;
		virtual BoundBox getBound() = 0;
		virtual BoundBox getLimitBound() = 0;
		virtual void  getPattern(std::vector<Vec2i>& outList) = 0;
		virtual void  step() = 0;
		virtual IRenderer* getRenderer() { return nullptr; }
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
			BoundBox result;
			result.min = Vec2i::Zero();
			result.max = Vec2i(mGrid.getSizeX() , mGrid.getSizeY());
			return result;
		}
		virtual BoundBox getLimitBound() final
		{
			BoundBox result;
			result.min = Vec2i::Zero();
			result.max = Vec2i(mGrid.getSizeX(), mGrid.getSizeY());
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
		ChunkAlgo(uint32 x, uint32 y)
		{
			for (int i = 0; i < 2; ++i)
			{
				mChunkMap.resize(AlignUp(x, ChunkLength) , AlignUp(y, ChunkLength));
				mChunkMap.fillValue(nullptr);
			}
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
					chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)] = value;
#if USE_CHUNK_COUNT
					++chunk->count;
					chunk->bHadSleeped = false;
#endif
				}
				else
				{
					auto chunk = mChunkMap(cPos);
					if (chunk)
					{
						int ix = x - cPos.x * ChunkLength;
						int iy = y - cPos.y * ChunkLength;
						chunk->data[mIndex][Chunk::ToDataIndex(ix, iy)] = 0;
#if USE_CHUNK_COUNT
						--chunk->count;
#endif
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
					int ix = x % ChunkLength;
					int iy = y % ChunkLength;
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
				uint8* pChunkData = chunk->data[mIndex];
				memset(pChunkData, 0, ChunkDataSize);
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
			result.max = ChunkLength * mChunkMap.getSize();
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
						uint8* pChunkData = chunk->data[mIndex];
						memset(pChunkData, 0, ChunkDataSize);
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
				if (chunk == nullptr || chunk->count == 0)
					continue;
#else
				if (chunk == nullptr)
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


		static constexpr uint32 ChunkLength = 64; //1 << 8;
		static constexpr uint32 ChunkDataStrideLength = ChunkLength + 2;
		static constexpr uint32 ChunkDataSize = ChunkDataStrideLength * ChunkDataStrideLength;

		static Vec2i ToChunkPos(Vec2i const& pos)
		{
			return ToChunkPos(pos.x, pos.y);
		}
		static Vec2i ToChunkPos(int x, int y)
		{
			return Vec2i(AlignUp<int>(x, ChunkLength) - 1, AlignUp<int>(y, ChunkLength) - 1);
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


	struct PatternData
	{
		std::vector< Vec2i > posList;

		template< class OP >
		void serialize(OP&& op)
		{
			op & posList;
		}
	};

	TYPE_SUPPORT_SERIALIZE_FUNC(PatternData);


	class SelectionRect : public GWidget
	{
	public:




	};

	enum EditMode
	{
		AddCell,
		Bound ,
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

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;
			::Global::GUI().cleanupWidget();

			auto& console = ConsoleSystem::Get();

#define REGISTER_COM( NAME , FUNC , ... )\
			console.registerCommand("Life."NAME, &TestStage::FUNC, this , ##__VA_ARGS__ )
			REGISTER_COM("Save", savePattern);
			REGISTER_COM("Load", loadPattern);
#undef REGISTER_COM

			restart();
			
			auto devFrame = WidgetUtility::CreateDevFrame();
			FWidgetProperty::Bind(devFrame->addCheckBox(UI_ANY, "Run"), bRunEvolate);
			devFrame->addButton("Step", [this](int event, GWidget*)
			{
				if (bRunEvolate == false)
				{
					mAlgorithm->step();
				}
				return false;
			});
			devFrame->addText("Evolate Time Rate");
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), evolateTimeRate, 0.5, 100, 2, [this](float v)
			{
				mViewport.updateTransform(::Global::GetScreenSize());
			});

			devFrame->addText("Zoom Out");
			FWidgetProperty::Bind(devFrame->addSlider(UI_ANY), mViewport.zoomOut, 0.25, 10.0, 2 , [this](float v)
			{
				mViewport.updateTransform(::Global::GetScreenSize());
			});
			devFrame->addButton("Clear", [this](int event, GWidget*)
			{
				if (bRunEvolate == false)
				{
					mAlgorithm->clearCell();
				}
				return false;
			});
			return true;
		}

		void onEnd() override
		{
			auto& console = ConsoleSystem::Get();
			console.unregisterAllCommandsByObject(this);

			BaseClass::onEnd();
		}

		bool savePattern(char const* name);

		bool loadPattern(char const* name);

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
			mViewport.size = Vec2i(80,80);

			mViewport.updateTransform(::Global::GetScreenSize());
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

		bool onMouse(MouseMsg const& msg) override
		{
			if (msg.onLeftDown())
			{
				if (bRunEvolate == false)
				{
					Vector2 localPos = mViewport.xform.transformInvPosition(msg.getPos());
					Vec2i cellPos = { Math::FloorToInt(localPos.x), Math::FloorToInt(localPos.y) };

					uint8 value = mAlgorithm->getCell(cellPos.x, cellPos.y);
					mAlgorithm->setCell(cellPos.x, cellPos.y, !value);
				}
			}
			else if (msg.onRightDown())
			{
				bStartDragging = true;
				draggingPos = mViewport.xform.transformInvPosition(msg.getPos());
			}
			else if (msg.onRightUp())
			{
				bStartDragging = false;
			}
			else if (msg.onMoving())
			{
				if (bStartDragging)
				{
					Vector2 localPos = mViewport.xform.transformInvPosition(msg.getPos());
					mViewport.pos -= (localPos - draggingPos);
					mViewport.updateTransform(::Global::GetScreenSize());
				}
			}

			return BaseClass::onMouse(msg);
		}

		bool onKey(KeyMsg const& msg) override
		{
			if (!msg.isDown())
				return false;

			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
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




}//namespace Life