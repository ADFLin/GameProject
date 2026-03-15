#pragma once

#ifndef PathTracingBVH_H_40B79932_12D2_4929_9870_1B75F12C94D7
#define PathTracingBVH_H_40B79932_12D2_4929_9870_1B75F12C94D7

#include "DataStructure/BVHTree.h"
#include "RHI/ShaderCore.h"
#include "Math/Vector3.h"
#include "Math/Vector4.h"

namespace PathTracing
{
	using Math::Vector3;
	using Math::Vector4;

	struct GPU_ALIGN BVHNodeData
	{
		DECLARE_BUFFER_STRUCT(BVHNodes);

		Vector3 boundMin;
		int32 left;
		Vector3 boundMax;
		int32 right;

		bool isLeaf() const
		{
			return right < 0;
		}
	};

	struct GPU_ALIGN BVH4NodeData
	{
		DECLARE_BUFFER_STRUCT(BVH4Nodes);

		Vector4 boundMinX;
		Vector4 boundMinY;
		Vector4 boundMinZ;
		Vector4 boundMaxX;
		Vector4 boundMaxY;
		Vector4 boundMaxZ;
		int32 children[4];
		int32 primitiveCounts[4];

	};

	struct FBVH
	{
		static void Generate(BVHTree const& BVH, TArray< BVHNodeData >& nodes, TArray< int >& primitiveIds);
		static void Generate(BVHTree const& BVH, TArray< BVHNodeData >& nodes, int indexTriangleStart);
		static int Generate(BVHTree const& BVH, TArray< BVH4NodeData >& nodes, TArray< int >& primitiveIds);
		static int Generate(BVHTree const& BVH, TArray< BVH4NodeData >& nodes, int indexTriangleStart);

	private:
		static void GetQuadChildren(BVHTree const& BVH, int bNodeId, int outNodes[4], int& outCount);
		static int Collapse_R(BVHTree const& BVH, int binaryNodeIndex, TArray< BVH4NodeData >& outNodes, TArray< int >& primitiveIds);
		static int Collapse_R(BVHTree const& BVH, int binaryNodeIndex, TArray< BVH4NodeData >& outNodes, int& indexTriangleCur);
	};
}//namespace PathTracing

#endif // PathTracingBVH_H_40B79932_12D2_4929_9870_1B75F12C94D7