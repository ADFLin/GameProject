#include "BVHTree.h"

#include <limits>
#include <algorithm>

using namespace Math;

BVHTree::Stats BVHTree::calcStats() const
{
	Stats result{ (int)MaxInt32,0,0,(int)MaxInt32,0,0 };

	int depthAcc = 0;
	int countAcc = 0;
	for (auto const& leaf : leaves)
	{
		if (leaf.depth > result.maxDepth)
			result.maxDepth = leaf.depth;
		if (leaf.depth < result.minDepth)
			result.minDepth = leaf.depth;

		depthAcc += leaf.depth;

		int count = (int)leaf.ids.size();
		if (count > result.maxCount)
			result.maxCount = count;
		if (count < result.minCount)
			result.minCount = count;

		countAcc += count;
	}

	if (leaves.size() > 0)
	{
		result.meanDepth = float(depthAcc) / (float)leaves.size();
		result.meanCount = float(countAcc) / (float)leaves.size();
	}
	return result;
}

bool BVHTree::checkLeafIndexOrder()
{
	int curIndex = 0;
	TArray< int > nodeStack;
	if (nodes.size() == 0)
		return true;

	nodeStack.push_back(0);
	while (nodeStack.size() > 0)
	{
		BVHTree::Node& node = nodes[nodeStack.back()];
		nodeStack.pop_back();

		if (node.isLeaf())
		{
			if (node.indexLeft != curIndex)
				return false;

			++curIndex;
		}
		else
		{
			nodeStack.push_back(node.indexRight);
			nodeStack.push_back(node.indexLeft);
		}
	}

	return true;
}

void BVHTree::clear()
{
	nodes.clear();
	leaves.clear();
}

BVHTreeBuilder::BVHTreeBuilder(BVHTree& BVH)
	:mBVH(BVH)
{
}

int BVHTreeBuilder::choiceAxis(Math::Vector3 const& size)
{
	if (size.x > size.y)
		return (size.x >= size.z) ? 0 : 2;
	return (size.y >= size.z) ? 1 : 2;
}

int BVHTreeBuilder::makeLeaf(TArrayView<BuildData>& dataList, int depth)
{
	BVHTree::Leaf leaf;
	leaf.ids.resize((int)dataList.size());
	leaf.depth = depth;
	for (int i = 0; i < (int)dataList.size(); ++i)
	{
		leaf.ids[i] = mPrimitives[dataList[i].index].id;
	}
	int indexLeaf = (int)mBVH.leaves.size();
	mBVH.leaves.push_back(std::move(leaf));
	return indexLeaf;
}

int BVHTreeBuilder::build(TArrayView<BVHTree::Primitive const> primitives)
{
	mPrimitives = primitives;
	if (mPrimitives.size() == 0)
		return -1;

	TArray< BuildData > dataList;
	dataList.resize((int)mPrimitives.size());
	for (int i = 0; i < (int)dataList.size(); ++i)
	{
		dataList[i].index = i;
	}
	int rootId = buildNode_R(MakeView(dataList), 0);
	return rootId;
}

int BVHTreeBuilder::buildNode_R(TArrayView<BuildData>& dataList, int depth)
{
	BVHTree::BoundType bound;
	bound = mPrimitives[dataList[0].index].bound;
	for (int i = 1; i < (int)dataList.size(); ++i)
	{
		int index = dataList[i].index;
		bound += mPrimitives[index].bound;
	}
	return buildNode_R(dataList, bound, depth);
}

int BVHTreeBuilder::buildNode_R(TArrayView<BuildData>& dataList, BVHTree::BoundType const& bound, int depth)
{
	int   minCount = -1;
	int   minAxis;
	BVHTree::BoundType minBounds[2];

	if ((int)dataList.size() > minSplitPrimitiveCount)
	{
		Vector3 vSize = bound.getSize();

		if (splitMethod == SplitMethod::NodeBlance)
		{
			minAxis = choiceAxis(vSize);
			minCount = (int)dataList.size() / 2;
		}
		else
		{
			constexpr int BucketCount = 16;
			struct Bucket
			{
				BVHTree::BoundType bound;
				int count;

				Bucket()
				{
					count = 0;
					bound.invalidate();
				}
			};

			auto GetSurfaceArea = [](BVHTree::BoundType const& b)
			{
				Vector3 size = b.getSize();
				return size.x * size.y + size.y * size.z + size.z * size.x;
			};

			float nodeScore = GetSurfaceArea(bound) * (float)dataList.size();
			float minScore = std::numeric_limits<float>::max();
			for (int axis = 0; axis < 3; ++axis)
			{
				float axisSize = vSize[axis];
				if (axisSize < 1e-6)
					continue;

				Bucket buckets[BucketCount];
				for (int i = 0; i < (int)dataList.size(); ++i)
				{
					auto& data = dataList[i];
					int index = data.index;
					BVHTree::Primitive const& primitive = mPrimitives[index];

					int indexBucket = Math::FloorToInt((float)BucketCount * (primitive.center[axis] - bound.min[axis]) / axisSize);
					if (indexBucket >= BucketCount)
						indexBucket = BucketCount - 1;
					else if (indexBucket < 0)
						indexBucket = 0;

					buckets[indexBucket].count += 1;
					buckets[indexBucket].bound += primitive.bound;
				}

				int leftCount = 0;
				BVHTree::BoundType leftBound;
				leftBound.invalidate();

				for (int i = 0; i < BucketCount - 1; ++i)
				{
					if (buckets[i].count == 0)
						continue;

					leftCount += buckets[i].count;
					leftBound += buckets[i].bound;

					int rightCount = 0;
					BVHTree::BoundType rightBound;
					rightBound.invalidate();
					for (int j = i + 1; j < BucketCount; ++j)
					{
						if (buckets[j].count == 0)
							continue;

						rightCount += buckets[j].count;
						rightBound += buckets[j].bound;
					}

					if (rightCount == 0)
						continue;

					float score = GetSurfaceArea(leftBound) * (float)leftCount + GetSurfaceArea(rightBound) * (float)rightCount;
					if (score < minScore)
					{
						minScore = score;
						minCount = leftCount;
						minAxis = axis;
						minBounds[0] = leftBound;
						minBounds[1] = rightBound;
					}
				}
			}

			if (minScore >= nodeScore)
			{
				minCount = -1;
			}
		}
	}

	int indexNode = (int)mBVH.nodes.size();
	mBVH.nodes.push_back(BVHTree::Node());

	if (minCount == -1)
	{
		BVHTree::Node& node = mBVH.nodes[indexNode];
		node.indexLeft = makeLeaf(dataList, depth);
		node.indexRight = -1;
		node.bound = bound;
		node.depth = depth;
	}
	else
	{
		for (int i = 0; i < (int)dataList.size(); ++i)
		{
			auto& data = dataList[i];
			int index = data.index;
			BVHTree::Primitive const& primitive = mPrimitives[index];
			data.value = primitive.center[minAxis];
		}

		std::partial_sort(dataList.begin(), dataList.begin() + minCount, dataList.end(),
			[](BuildData const& lhs, BuildData const& rhs)->bool
			{
				return lhs.value < rhs.value;
			}
		);

		int indexLeft;
		int indexRight;
		if (splitMethod == SplitMethod::NodeBlance)
		{
			indexLeft = buildNode_R(TArrayView<BuildData>(dataList.data(), minCount), depth + 1);
			indexRight = buildNode_R(TArrayView<BuildData>(dataList.data() + minCount, (int)dataList.size() - minCount), depth + 1);
		}
		else
		{
			indexLeft = buildNode_R(TArrayView<BuildData>(dataList.data(), minCount), minBounds[0], depth + 1);
			indexRight = buildNode_R(TArrayView<BuildData>(dataList.data() + minCount, (int)dataList.size() - minCount), minBounds[1], depth + 1);
		}

		BVHTree::Node& node = mBVH.nodes[indexNode];
		node.indexLeft = indexLeft;
		node.indexRight = indexRight;
		node.bound = bound;
		node.depth = depth;
	}
	return indexNode;
}
