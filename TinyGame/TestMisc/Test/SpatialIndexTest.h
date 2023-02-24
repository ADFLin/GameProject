#pragma once
#ifndef SpatialIndexTest_H_BA852919_CF9F_4974_8913_2084D785180B
#define SpatialIndexTest_H_BA852919_CF9F_4974_8913_2084D785180B

#include "Stage/TestStageHeader.h"

#include "Template/ArrayView.h"
#include "Math/Vector2.h"
#include "Math/Math2D.h"
#include "Math/GeometryPrimitive.h"
#include "Math/PrimitiveTest.h"
#include "DataStructure/KDTree.h"
#include <algorithm>


using Vector2 = Math::Vector2;
using BoundBox2D = Math::TAABBox< Vector2 >;
using Ray2D = Math::TRay< Vector2 >;

using KDTree = TKDTree< 2 >;

class BVH
{





};



class SpatialIndexTestStage : public StageBase
{
	using BaseClass = StageBase;
public:
	enum
	{
		UI_PAUSE_LEARNING = BaseClass::NEXT_UI_ID,
		NEXT_UI_ID,
	};
	KDTree    mTree;

	Ray2D       mTestRay;
	KDTree::RayResult mRayResult;
	SpatialIndexTestStage()
	{
		mTestRay.pos = Vector2(0, 0);
		mTestRay.dir = Vector2(1, 0);


		mRayResult.indexData = -1;
	}

	bool onInit() override
	{
		if( !BaseClass::onInit() )
			return false;
		::Global::RandSeed(generateRandSeed());
		::Global::GUI().cleanupWidget();

		WidgetUtility::CreateDevFrame();
		restart();
		return true;
	}

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for( int i = 0; i < frame; ++i )
			tick();

		updateFrame(frame);
	}

	float randFloat()
	{
		return (float)::Global::Random() / RAND_MAX;
	}
	void restart()
	{

		int numShape = 50;

		TArray< KDTree::PrimitiveData > shapes;
		shapes.resize(numShape);
		for( int i = 0; i < numShape; ++i )
		{
			KDTree::PrimitiveData& data = shapes[i];
			data.id = i;
			data.BBox.min = 100 * Vector2(randFloat(), randFloat());
			data.BBox.max = data.BBox.min + 10 * Vector2( 0.1 + randFloat(), 0.1 + randFloat() );
		}

		mTree.mDataVec = std::move(shapes);
		mTree.build();

	}
	void tick() {}
	void updateFrame(int frame) {}

	static Vector2 convertToScreen(Vector2 pos)
	{
		return Vector2(50, 50) + Vector2(5.0f * pos);
	}
	static Vector2 convertToWorld(Vector2 pos)
	{
		return (pos - Vector2(50, 50)) / 5.0f;
	}

	void onRender(float dFrame) override;

	int mIndexNearset = -1;

	MsgReply onMouse(MouseMsg const& msg) override
	{
		if( msg.onLeftDown() )
		{
			Vector2 worldPos = convertToWorld(msg.getPos());
			mTestRay.pos = worldPos;

			mRayResult.indexData = INDEX_NONE;
			mTree.raycast(mTestRay, mRayResult);
			struct DistFun
			{
				float operator()(KDTree::PrimitiveData const& data, Vector2 const& p , float) const
				{
					return Math::DistanceSqure( data.BBox , p );
				}
			};
			float distSqr;
			mIndexNearset = mTree.findNearst(worldPos, DistFun(), distSqr);
		}
		else if( msg.onRightDown() )
		{
			Vector2 worldPos = convertToWorld(msg.getPos());

			mTestRay.dir = worldPos - mTestRay.pos;
			if( mTestRay.dir.normalize() == 0 )
			{
				mTestRay.dir = Vector2(1, 0);
			}
			mRayResult.indexData = INDEX_NONE;
			mTree.raycast(mTestRay, mRayResult);
		}

		return BaseClass::onMouse(msg);
	}
	MsgReply onKey(KeyMsg const& msg) override
	{
		if(msg.isDown())
		{
			switch (msg.getCode())
			{
			case EKeyCode::R: restart(); break;
			}
		}
		return BaseClass::onKey(msg);
	}
};

#endif // SpatialIndexTest_H_BA852919_CF9F_4974_8913_2084D785180B
