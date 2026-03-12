#pragma once
#ifndef BVHTree_H_A27C23A1_6D32_40F3_8DCF_E3D9B30FCC2B
#define BVHTree_H_A27C23A1_6D32_40F3_8DCF_E3D9B30FCC2B

#include "Math/Vector3.h"
#include "Math/GeometryPrimitive.h"
#include "DataStructure/Array.h"
#include "Core/IntegerType.h"

class BVHTree
{
public:

	typedef Math::TAABBox< Math::Vector3 > BoundType;
	struct Node
	{
		BoundType bound;
		int indexLeft;
		int indexRight;
		int depth;

		bool isLeaf() const { return indexRight < 0; }
	};


	struct Stats
	{
		int minDepth;
		int maxDepth;
		float meanDepth;
		int minCount;
		int maxCount;
		float meanCount;
	};

	struct Leaf
	{
		int depth;
		TArray<int> ids;
	};

	Stats calcStats() const;
	bool checkLeafIndexOrder();
	void clear();

	TArray<Node> nodes;
	TArray<Leaf> leaves;

	struct Primitive
	{
		int id;
		Math::Vector3 center;
		BoundType bound;
	};
};

class BVHTreeBuilder
{
public:
	BVHTreeBuilder(BVHTree& BVH);

	enum class SplitMethod
	{
		NodeBlance,
		SAH,
	};

	SplitMethod splitMethod = SplitMethod::SAH;
	int maxDepth = 32;
	int minSplitPrimitiveCount = 2;

	struct BuildData
	{
		float value;
		int   index;
	};

	int build(TArrayView< BVHTree::Primitive const > primitives);

private:
	int choiceAxis(Math::Vector3 const& size);
	int makeLeaf(TArrayView< BuildData >& dataList, int depth);
	int buildNode_R(TArrayView< BuildData >& dataList, int depth);
	int buildNode_R(TArrayView< BuildData >& dataList, BVHTree::BoundType const& bound, int depth);

	BVHTree& mBVH;
	TArrayView<BVHTree::Primitive const> mPrimitives;
};


#endif // BVHTree_H_A27C23A1_6D32_40F3_8DCF_E3D9B30FCC2B
