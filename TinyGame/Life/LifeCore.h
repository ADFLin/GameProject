#pragma once
#ifndef LifeCore_H_42A517B8_B9C7_4EEA_9E5E_12CB48F12BF6
#define LifeCore_H_42A517B8_B9C7_4EEA_9E5E_12CB48F12BF6

#include "Math/GeometryPrimitive.h"

#include <vector>
#include "Math/Vector2.h"
#include "Renderer/RenderTransform2D.h"

namespace Life
{
	using Vec2i = TVector2<int>;
	using BoundBox = ::Math::TAABBox< Vec2i >;
	using Math::Vector2;
	using Render::RenderTransform2D;

	class IRenderProxy;

	class IAlgorithm
	{
	public:
		virtual ~IAlgorithm() {}
		virtual bool  setCell(int x, int y, uint8 value) = 0;
		virtual uint8 getCell(int x, int y) = 0;
		virtual void  clearCell() = 0;
		virtual BoundBox getBound() = 0;
		virtual BoundBox getLimitBound() = 0;
		virtual void  getPattern(std::vector<Vec2i>& outList) = 0;
		virtual void  getPattern(BoundBox const& bound, std::vector<Vec2i>& outList) = 0;
		virtual void  step() = 0;
		virtual IRenderProxy* getRenderProxy() { return nullptr; }
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

			// B3/S23
			mEvolvetinMap[ToIndex(3, 1)] = 1;
			mEvolvetinMap[ToIndex(2, 1)] = 1;
			mEvolvetinMap[ToIndex(3, 0)] = 1;
		}

		uint8 getEvoluteValue(uint32 count, uint8 value)
		{
			return mEvolvetinMap[ToIndex(count, value)];
		}

		static uint32 ToIndex(uint32 count, uint8 value)
		{
			return (count << 1) | value;
		}
		uint8 mEvolvetinMap[9 * 2];

		bool bWarpX = true;
		bool bWarpY = true;
	};


	class Viewport
	{
	public:

		Vec2i   screenSize;
		float   cellLength;
		Vector2 pos;
		float   zoomOut;

		RenderTransform2D xform;
		float cellPixelSize;

		void updateTransform()
		{
			cellPixelSize = cellLength / zoomOut;
			xform.setIdentity();
			xform.translateLocal(0.5 * Vector2(screenSize));
			xform.scaleLocal(Vector2(cellPixelSize, cellPixelSize));
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
		virtual void drawCell(int x, int y) = 0;
	};

	class IRenderProxy
	{
	public:
		virtual void draw(IRenderer& renderer, Viewport const& viewport, BoundBox const& boundRender) = 0;
	};

}//namespace Life

#endif // LifeCore_H_42A517B8_B9C7_4EEA_9E5E_12CB48F12BF6