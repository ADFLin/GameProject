#include "PathTracingBVH.h"

namespace PathTracing
{

	void FBVH::Generate(BVHTree const& BVH, TArray< BVHNodeData >& nodes, TArray< int >& primitiveIds)
	{
		nodes.resize(BVH.nodes.size());

		for (int i = 0; i < nodes.size(); ++i)
		{
			auto& node = nodes[i];
			auto const& nodeCopy = BVH.nodes[i];
			node.boundMin = nodeCopy.bound.min;
			node.boundMax = nodeCopy.bound.max;

			if (nodeCopy.isLeaf())
			{
				auto const& leafCopy = BVH.leaves[nodeCopy.indexLeft];
				node.left = primitiveIds.size();
				node.right = -leafCopy.ids.size();
				primitiveIds.append(leafCopy.ids);
			}
			else
			{
				node.left = nodeCopy.indexLeft;
				node.right = nodeCopy.indexRight;
			}
		}
	}

	void FBVH::Generate(BVHTree const& BVH, TArray< BVHNodeData >& nodes, int indexTriangleStart)
	{
		int indexNodeStart = nodes.size();
		int indexTriangleCur = indexTriangleStart;
		nodes.resize(nodes.size() + BVH.nodes.size());

		for (int i = 0; i < BVH.nodes.size(); ++i)
		{
			auto& node = nodes[indexNodeStart + i];
			auto const& nodeCopy = BVH.nodes[i];
			node.boundMin = nodeCopy.bound.min;
			node.boundMax = nodeCopy.bound.max;

			if (nodeCopy.isLeaf())
			{
				auto const& leafCopy = BVH.leaves[nodeCopy.indexLeft];
				node.left = indexTriangleCur;
				node.right = -leafCopy.ids.size();
				indexTriangleCur += 3 * leafCopy.ids.size();
			}
			else
			{
				node.left = indexNodeStart + nodeCopy.indexLeft;
				node.right = indexNodeStart + nodeCopy.indexRight;
			}
		}
	}

	int FBVH::Generate(BVHTree const& BVH, TArray< BVH4NodeData >& nodes, TArray< int >& primitiveIds)
	{
		nodes.clear();
		if (BVH.nodes.empty())
			return 0;

		Collapse_R(BVH, 0, nodes, primitiveIds);
		return nodes.size();
	}

	int FBVH::Generate(BVHTree const& BVH, TArray< BVH4NodeData >& nodes, int indexTriangleStart)
	{
		int indexCur = indexTriangleStart;
		if (BVH.nodes.empty())
			return 0;

		int index = nodes.size();
		Collapse_R(BVH, 0, nodes, indexCur);
		return nodes.size() - index;
	}

	void FBVH::GetQuadChildren(BVHTree const& BVH, int bNodeId, int outNodes[4], int& outCount)
	{
		outCount = 0;
		int pass1[2];
		int pass1Count = 0;
		auto const& root = BVH.nodes[bNodeId];
		if (root.isLeaf())
		{
			pass1[pass1Count++] = bNodeId;
		}
		else
		{
			pass1[pass1Count++] = root.indexLeft;
			pass1[pass1Count++] = root.indexRight;
		}

		for (int i = 0; i < pass1Count; ++i)
		{
			auto const& n = BVH.nodes[pass1[i]];
			if (n.isLeaf())
			{
				outNodes[outCount++] = pass1[i];
			}
			else
			{
				outNodes[outCount++] = n.indexLeft;
				outNodes[outCount++] = n.indexRight;
			}
		}
	}

	int FBVH::Collapse_R(BVHTree const& BVH, int binaryNodeIndex, TArray< BVH4NodeData >& outNodes, TArray< int >& primitiveIds)
	{
		int outNodeIndex = outNodes.size();
		outNodes.push_back(BVH4NodeData());

		int childrenNodes[4];
		int childCount = 0;
		GetQuadChildren(BVH, binaryNodeIndex, childrenNodes, childCount);

		Vector4 bMinX(1e30f, 1e30f, 1e30f, 1e30f), bMinY(1e30f, 1e30f, 1e30f, 1e30f), bMinZ(1e30f, 1e30f, 1e30f, 1e30f);
		Vector4 bMaxX(-1e30f, -1e30f, -1e30f, -1e30f), bMaxY(-1e30f, -1e30f, -1e30f, -1e30f), bMaxZ(-1e30f, -1e30f, -1e30f, -1e30f);
		int32 childIndices[4] = { -1, -1, -1, -1 };
		int32 childCounts[4] = { 0, 0, 0, 0 };

		for (int i = 0; i < childCount; ++i)
		{
			auto const& bNode = BVH.nodes[childrenNodes[i]];
			bMinX[i] = bNode.bound.min.x;
			bMinY[i] = bNode.bound.min.y;
			bMinZ[i] = bNode.bound.min.z;
			bMaxX[i] = bNode.bound.max.x;
			bMaxY[i] = bNode.bound.max.y;
			bMaxZ[i] = bNode.bound.max.z;

			if (bNode.isLeaf())
			{
				auto const& leafCopy = BVH.leaves[bNode.indexLeft];
				childIndices[i] = primitiveIds.size();
				childCounts[i] = leafCopy.ids.size();
				primitiveIds.append(leafCopy.ids);
			}
			else
			{
				childIndices[i] = Collapse_R(BVH, childrenNodes[i], outNodes, primitiveIds);
				childCounts[i] = 0;
			}
		}

		auto& outNode = outNodes[outNodeIndex];
		outNode.boundMinX = bMinX; outNode.boundMinY = bMinY; outNode.boundMinZ = bMinZ;
		outNode.boundMaxX = bMaxX; outNode.boundMaxY = bMaxY; outNode.boundMaxZ = bMaxZ;
		for (int i = 0; i < 4; ++i)
		{
			outNode.children[i] = childIndices[i];
			outNode.primitiveCounts[i] = childCounts[i];
		}

		return outNodeIndex;
	}

	int FBVH::Collapse_R(BVHTree const& BVH, int binaryNodeIndex, TArray< BVH4NodeData >& outNodes, int& indexTriangleCur)
	{
		int outNodeIndex = outNodes.size();
		outNodes.push_back(BVH4NodeData());

		int childrenNodes[4];
		int childCount = 0;
		GetQuadChildren(BVH, binaryNodeIndex, childrenNodes, childCount);

		Vector4 bMinX(1e30f, 1e30f, 1e30f, 1e30f), bMinY(1e30f, 1e30f, 1e30f, 1e30f), bMinZ(1e30f, 1e30f, 1e30f, 1e30f);
		Vector4 bMaxX(-1e30f, -1e30f, -1e30f, -1e30f), bMaxY(-1e30f, -1e30f, -1e30f, -1e30f), bMaxZ(-1e30f, -1e30f, -1e30f, -1e30f);
		int32 childIndices[4] = { -1, -1, -1, -1 };
		int32 childCounts[4] = { 0, 0, 0, 0 };

		for (int i = 0; i < childCount; ++i)
		{
			auto const& bNode = BVH.nodes[childrenNodes[i]];
			bMinX[i] = bNode.bound.min.x;
			bMinY[i] = bNode.bound.min.y;
			bMinZ[i] = bNode.bound.min.z;
			bMaxX[i] = bNode.bound.max.x;
			bMaxY[i] = bNode.bound.max.y;
			bMaxZ[i] = bNode.bound.max.z;

			if (bNode.isLeaf())
			{
				auto const& leafCopy = BVH.leaves[bNode.indexLeft];
				childIndices[i] = indexTriangleCur;
				childCounts[i] = leafCopy.ids.size();
				indexTriangleCur += 3 * leafCopy.ids.size();
			}
			else
			{
				childIndices[i] = Collapse_R(BVH, childrenNodes[i], outNodes, indexTriangleCur);
				childCounts[i] = 0;
			}
		}

		auto& outNode = outNodes[outNodeIndex];
		outNode.boundMinX = bMinX; outNode.boundMinY = bMinY; outNode.boundMinZ = bMinZ;
		outNode.boundMaxX = bMaxX; outNode.boundMaxY = bMaxY; outNode.boundMaxZ = bMaxZ;
		for (int i = 0; i < 4; ++i)
		{
			outNode.children[i] = childIndices[i];
			outNode.primitiveCounts[i] = childCounts[i];
		}
		return outNodeIndex;
	}

}//namespace PathTracing