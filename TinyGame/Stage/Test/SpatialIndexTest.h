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


typedef Math::Vector2 Vector2;
typedef Math::TBoundBox< Vector2 > BoundBox2D;
typedef Math::TRay< Vector2 > Ray;

typedef TKDTree< 2 > KDTree;

class BVH
{





};



class SpatialIndexTestStage : public StageBase
{
	typedef StageBase BaseClass;
public:
	enum
	{
		UI_PAUSE_LEARNING = BaseClass::NEXT_UI_ID,
		NEXT_UI_ID,
	};
	KDTree    mTree;

	Ray       mTestRay;
	KDTree::RayResult mRayResult;
	SpatialIndexTestStage()
	{
		mTestRay.pos = Vector2(0, 0);
		mTestRay.dir = Vector2(1, 0);


		mRayResult.indexData = -1;
	}

	virtual bool onInit()
	{
		if( !BaseClass::onInit() )
			return false;
		::Global::RandSeed(generateRandSeed());
		::Global::GUI().cleanupWidget();

		WidgetUtility::CreateDevFrame();
		restart();
		return true;
	}

	virtual void onEnd()
	{
		BaseClass::onEnd();
	}

	virtual void onUpdate(long time)
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

		std::vector< KDTree::PrimitiveData > shapes;
		shapes.resize(numShape);
		for( int i = 0; i < numShape; ++i )
		{
			KDTree::PrimitiveData& data = shapes[i];
			data.id = i;
			data.BBox.min = 100 * Vector2(randFloat(), randFloat());
			data.BBox.max = data.BBox.min + 10 * Vector2( 0.1 + randFloat(), 0.1 + randFloat() );
		}

		mTree.mDataVec.swap(shapes);
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

	void onRender(float dFrame);

	int mIndexNearset = -1;

	bool onMouse(MouseMsg const& msg)
	{
		if( !BaseClass::onMouse(msg) )
			return false;

		if( msg.onLeftDown() )
		{
			Vector2 worldPos = convertToWorld(msg.getPos());
			mTestRay.pos = worldPos;

			mRayResult.indexData = -1;
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
			mRayResult.indexData = -1;
			mTree.raycast(mTestRay, mRayResult);
		}
		return true;
	}
	bool onKey(unsigned key, bool isDown)
	{
		if( !isDown )
			return false;
		switch( key )
		{
		case Keyboard::eR: restart(); break;
		}
		return false;
	}
};

#endif // SpatialIndexTest_H_BA852919_CF9F_4974_8913_2084D785180B
