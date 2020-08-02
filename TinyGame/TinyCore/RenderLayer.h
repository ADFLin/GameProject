#pragma once

#ifndef RenderLayer_H_F762602C_23EB_4595_956C_B911E3E4920A
#define RenderLayer_H_F762602C_23EB_4595_956C_B911E3E4920A


#include "Math/TVector2.h"

typedef TVector2<int> Vec2i;

class RenderLayer
{
public:
	RenderLayer()
		:mScenePos(0, 0)
		, mScale(1.0)
	{

	}

	void         setSurfaceScale(float scale) { mScale = scale; }
	void         setSurfacePos(Vec2i const& pos) { mScenePos = pos; }
	Vec2i const& getSurfacePos() const { return mScenePos; }
	float        getSurfaceScale() { return mScale; }
protected:
	Vec2i        mScenePos;
	float        mScale;
};

#endif // RenderLayer_H_F762602C_23EB_4595_956C_B911E3E4920A