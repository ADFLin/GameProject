#include "Stage/TestStageHeader.h"
#include "GameRenderSetup.h"

#include "LifeCore.h"


#include "Math/GeometryPrimitive.h"
#include "DataStructure/Grid2D.h"
#include "RHI/RHIGraphics2D.h"
#include "ConsoleSystem.h"
#include "ProfileSystem.h"

namespace Life
{
	using namespace Render;


	float Remap(float v, float is, float ie, float os, float oe)
	{
		return Math::Lerp(os, oe, (v - is) / (ie - is));
	}

	float RemapClamp(float v, float is, float ie, float os, float oe)
	{
		return Math::Lerp(os, oe, Math::Clamp<float>( (v - is) / (ie - is) , 0.0f , 1.0f ));
	}

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

			RenderUtility::SetPen(g, EColor::Black);
			RenderUtility::SetBrush(g, EColor::White);
			for (int i = 0; i < 8; ++i)
			{
				Vector2 boxPos = pos + 0.5 * (size - Vector2(BoxSize)) * (GetBoxOffset()[i] + Vector2(1, 1));
				g.drawRect(boxPos, BoxSize);
			}

			g.beginBlend(pos + Vector2(BoxSize), size - 2 * Vector2(BoxSize), 0.1);
			RenderUtility::SetPen(g, EColor::Null);
			RenderUtility::SetBrush(g, EColor::Red);
			g.drawRect(pos + Vector2(BoxSize), size - 2 * Vector2(BoxSize));
			g.endBlend();

			RenderUtility::SetPen(g, EColor::Red);
			RenderUtility::SetBrush(g, EColor::Null);
			g.drawRect(pos + Vector2(BoxSize), size - 2 * Vector2(BoxSize));
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

			for (int i = 7; i >= 0; --i)
			{
				Vector2 boxPos = pos + 0.5 * ( size - Vector2(BoxSize) ) * ( GetBoxOffset()[i] + Vector2(1,1) );
				TRect< float > rect;
				rect.min = boxPos;
				rect.max = boxPos + BoxSize;
				if (rect.hitTest(hitPos))
				{
					return i;
				}
			}

			return INDEX_NONE;
		}

		int32 mIndexBoxDragging = INDEX_NONE;
		Vec2i mLastTrackingPos;
		Vector2 mLocalPinBoxPos;
		BoundBox mDraggingBound;
		bool  bStartDragging = false;
		void startResize(Vec2i const& pos)
		{
			bStartDragging = true;
			mLastTrackingPos = pos + 0.5 * Vector2(BoxLength,BoxLength);
			mIndexBoxDragging = 7;
			mDraggingBound.min = getPos();
			mDraggingBound.max = mDraggingBound.min + getSize();

			mManager->focusWidget(this);
			mManager->captureMouse(this);
		}

		MsgReply onMouseMsg(MouseMsg const& msg)
		{
			if (msg.onLeftDown())
			{
				int indexBox = hitBoxTest(msg.getPos());

				bStartDragging = true;
				mIndexBoxDragging = indexBox;
				mLastTrackingPos = msg.getPos();
				mDraggingBound.min = getPos();
				mDraggingBound.max = mDraggingBound.min + getSize();
				getManager()->captureMouse(this);
				return MsgReply::Handled();
			}
			else if (msg.onLeftUp())
			{
				if (bStartDragging)
				{
					bStartDragging = false;
					mIndexBoxDragging = INDEX_NONE;
					getManager()->releaseMouse();
					return MsgReply::Handled();
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
						setPos(mDraggingBound.min + offset);
					}
					else
					{
						Vector2 boxOffset = GetBoxOffset()[mIndexBoxDragging];
						Vector2 effectOffset = boxOffset.abs().mul(offset);
						Vector2 center = 0.5 * Vector2(mDraggingBound.min + mDraggingBound.max);
						Vector2 oldHalfSize = 0.5 * Vector2( mDraggingBound.getSize() );

						Vector2 rectOffset = boxOffset;
						if (rectOffset.x == 0)
							rectOffset.x = 1;
						if (rectOffset.y == 0)
							rectOffset.y = 1;
						Vector2 p1 = center + oldHalfSize * rectOffset + effectOffset;
						Vector2 p2 = center - oldHalfSize * rectOffset;

						Vector2 pos = p1.min(p2);
						Vector2 size = p1.max(p2) - pos;
						setPos(pos);
						setSize(size);
					}
	
					return MsgReply::Handled();
				}
			}

			return MsgReply(bStartDragging);
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
		struct Vec2iHash
		{
			std::size_t operator()(Vec2i const& v) const
			{
				uint32 result = HashValue(v.x);
				HashCombine(result, v.y);
				return result;
			}
		};
		std::unordered_set< Vec2i , Vec2iHash > cellList;
		BoundBox bound;

		void mirrorH()
		{
			CHECK(bound.isValid());

			std::unordered_set< Vec2i, Vec2iHash > temp = std::move(cellList);

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

			std::unordered_set< Vec2i, Vec2iHash > temp = std::move(cellList);

			for (auto& pos : temp)
			{
				Vec2i posNew;
				posNew.x = pos.x;
				posNew.y = bound.max.y - pos.y;
				cellList.insert(posNew);
			}
		}

		void rotate()
		{
			CHECK(bound.isValid());

			std::unordered_set< Vec2i, Vec2iHash > temp = std::move(cellList);
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

				std::unordered_set< Vec2i, Vec2iHash > temp = std::move(cellList);
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


		void restart();

		void onUpdate(long time) override
		{
			BaseClass::onUpdate(time);

			if (bRunEvolate)
			{
				accEvolateCount += evolateTimeRate * time / 1000.0;
				while (accEvolateCount > 0)
				{
					PROFILE_ENTRY("Life.Setp");
					mAlgorithm->step();
					accEvolateCount -= 1;
				}
			}
			else
			{
				accEvolateCount = 0;
			}
		}

		void onRender(float dFrame) override;

		bool savePattern(char const* name);
		bool loadPattern(char const* name);
		bool loadGolly(char const* name);


		bool bStartDragging = false;
		Vector2 draggingPos;
		SelectionRect* mSelectionRect = nullptr;

		Vec2i mLastMousePos;

		EditMode mEditMode;

		EditPattern mCopyPattern;

		RHITexture2DRef mTexture;
		std::vector< uint32 > mBuffer;

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

		MsgReply onMouse(MouseMsg const& msg) override;
		MsgReply onKey(KeyMsg const& msg) override;

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


		bool setupRenderSystem(ERenderSystem systemName) override;


		void preShutdownRenderSystem(bool bReInit = false) override;

	protected:
	};




}//namespace Life