#pragma once
#ifndef SpatialIndexTest_H_BA852919_CF9F_4974_8913_2084D785180B
#define SpatialIndexTest_H_BA852919_CF9F_4974_8913_2084D785180B

#include "TestStageHeader.h"

#include "WidgetUtility.h"
#include "Template/ArrayView.h"
#include "Math/Math2D.h"
#include "Math/PrimitiveTest2D.h"

#include <algorithm>

typedef Math::Vector2 Vector2;

struct BoundBox2D
{
	Vector2 min;
	Vector2 max;

	bool isIntersect(BoundBox2D const& rhs) const
	{
		return Math2D::BoxBoxTest(min, max, rhs.min, rhs.max);
	}

	bool isValid() const
	{
		return min < max;
	}

	BoundBox2D& operator += (BoundBox2D const& rhs)
	{
		min = min.min(rhs.min);
		max = max.max(rhs.max);
		return *this;
	}

	BoundBox2D& operator += (Vector2 const& pos)
	{
		min = min.min(pos);
		max = max.max(pos);
		return *this;
	}

	BoundBox2D() {}
	BoundBox2D(EForceInit)
	{
		min = Vector2(0, 0);
		max = Vector2(0, 0);
	}
};

struct PrimitiveData
{
	int id;
	BoundBox2D BBox;
};


struct Ray
{
	Vector2 pos;
	Vector2 dir;

	Vector2 getPosition(float dist) const { return pos + dist * dir; }
	int     testIntersect(BoundBox2D const& bound , float outDists[2] ) const
	{
		if( !Math2D::LineBoxTest(pos, dir, bound.min, bound.max, outDists) )
			return 0;

		if( outDists[0] < 0 )
		{
			if( outDists[1] < 0 )
				return 0;
			outDists[0] = outDists[1];
			return 1;
		}
		return 2;
	}
};

struct RayResult
{
	int   indexData;
	float dist;


};

class KDTree
{
public:
	static int const EMPTY_LEAF_ID = std::numeric_limits<int>::min();

	KDTree()
	{
		mRootId = EMPTY_LEAF_ID;
	}

	struct Node
	{
		int   axis;
		float value;
		int idxLeft;
		int idxRight;
	};

	struct Leaf
	{
		std::vector<int> dataIndices;
	};

	struct Edge
	{
		float value;
		bool  bStart;
		int   indexData;
	};

	BoundBox2D mBound;

	void build();

	bool raycast(Ray const& ray , RayResult& result );


	int  makeLeaf(std::vector< int >& dataIndices);
	int  buildNode_R(std::vector< int >& dataIndices, BoundBox2D bound);



	template< class Fun >
	void visitNode(Fun& visitFun)
	{
		visitNode_R(mRootId, visitFun, mBound);
	}

	template< class Fun >
	void visitNode_R(int nodeId, Fun& visitFun, BoundBox2D bound)
	{
		if( nodeId < 0 )
		{
			if( nodeId != EMPTY_LEAF_ID )
			{
				Leaf const& leaf = mLeaves[-(nodeId + 1)];
				visitFun(leaf, bound);
			}
			return;
		}

		Node const& node = mNodes[nodeId];

		visitFun(node, bound);

		{
			BoundBox2D childBound = bound;
			childBound.max[node.axis] = node.value;
			visitNode_R(node.idxLeft, visitFun, childBound);
		}

		{
			BoundBox2D childBound = bound;
			childBound.min[node.axis] = node.value;
			visitNode_R(node.idxRight, visitFun, childBound);
		}
	}

	int mRootId;


	std::vector< Node > mNodes;
	std::vector< Leaf > mLeaves;
	std::vector< PrimitiveData > mDataVec;
};

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
	RayResult mRayResult;
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

		std::vector< PrimitiveData > shapes;
		shapes.resize(numShape);
		for( int i = 0; i < numShape; ++i )
		{
			PrimitiveData& data = shapes[i];
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
