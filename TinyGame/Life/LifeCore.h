#pragma once
#ifndef LifeCore_H_42A517B8_B9C7_4EEA_9E5E_12CB48F12BF6
#define LifeCore_H_42A517B8_B9C7_4EEA_9E5E_12CB48F12BF6

#include "Math/GeometryPrimitive.h"

#include <vector>
#include "Math/Vector2.h"

namespace Life
{
	using Vec2i = TVector2<int>;
	using BoundBox = ::Math::TAABBox< Vec2i >;

	class IRenderer;

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
		virtual IRenderer* getRenderer() { return nullptr; }
	};



}//namespace Life

#endif // LifeCore_H_42A517B8_B9C7_4EEA_9E5E_12CB48F12BF6