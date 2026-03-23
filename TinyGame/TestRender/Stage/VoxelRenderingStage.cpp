#include "Stage/TestRenderStageBase.h"
#include "Renderer/MeshBuild.h"
#include "Math/PrimitiveTest.h"
#include "Math/GeometryPrimitive.h"
#include "RHI/SceneRenderer.h"
#include "Renderer/MeshImportor.h"
#include <algorithm>
#include "ProfileSystem.h"

namespace Render
{

	enum class ERenderMethod
	{
		Instanced,
		RawDDA,
		OctTree,
		S64Tree,
		
		COUNT,
	};


	struct VoxelPoint
	{
		IntVector3 pos;
		uint8 data;
	};

	struct VoxelRawData
	{
		TArray< uint8 > data;
		IntVector3 dims;

		void initData()
		{
			data.resize(size_t(dims.x) * size_t(dims.y) * size_t(dims.z), 0);
		}

		void initData(TArray< VoxelPoint > const& points)
		{
			data.resize(size_t(dims.x) * size_t(dims.y) * size_t(dims.z), 0);
			for (auto const& point : points)
			{
				auto const& pos = point.pos;
				size_t index = pos.x + dims.x * size_t(pos.y + dims.y * pos.z);
				data[index] = point.data;
			}
		}
	};



	using AABBox = Math::TAABBox< Vector3 >;

	struct OctTreeNode
	{
		int children[8];
	};
	
	struct SvtNode64
	{
		uint32_t childPtr;  // bit 31: isLeaf, bits 0-30: child index
		uint32_t maskLow;   // bits 0-31
		uint32_t maskHigh;  // bits 32-63
		uint32_t padding;
	};

	class FVoxelBuild
	{
	public:
		static bool MeshSurface(VoxelRawData& voxelRawData, AABBox const& bbox, MeshRawData const& meshData, InputLayoutDesc const& inputLayoutDesc, Matrix4 const& transform)
		{
			IntVector3 const& dims = voxelRawData.dims;
			Vector3 boundSize = bbox.getSize();
			Vector3 voxelSize = { boundSize.x / dims.x, boundSize.y / dims.y, boundSize.z / dims.z };

			auto posReader = meshData.makeAttributeReader(inputLayoutDesc, EVertex::ATTRIBUTE_POSITION);
			auto GetVertexPos = [&](uint32 index) -> Vector3
			{
				return TransformPosition(posReader[index], transform);
			};

			float thresholdLength = voxelSize.length() * 0.5f;

			for (int i = 0; i < (int)meshData.indices.size(); i += 3)
			{
				Vector3 v0 = GetVertexPos(meshData.indices[i]);
				Vector3 v1 = GetVertexPos(meshData.indices[i + 1]);
				Vector3 v2 = GetVertexPos(meshData.indices[i + 2]);

				Vector3 tMin = v0.min(v1).min(v2);
				Vector3 tMax = v0.max(v1).max(v2);

				int iMin[3], iMax[3];
				iMin[0] = Math::Max(0, Math::FloorToInt((tMin.x - bbox.min.x) / voxelSize.x));
				iMin[1] = Math::Max(0, Math::FloorToInt((tMin.y - bbox.min.y) / voxelSize.y));
				iMin[2] = Math::Max(0, Math::FloorToInt((tMin.z - bbox.min.z) / voxelSize.z));

				iMax[0] = Math::Min(dims.x - 1, Math::FloorToInt((tMax.x - bbox.min.x) / voxelSize.x));
				iMax[1] = Math::Min(dims.y - 1, Math::FloorToInt((tMax.y - bbox.min.y) / voxelSize.y));
				iMax[2] = Math::Min(dims.z - 1, Math::FloorToInt((tMax.z - bbox.min.z) / voxelSize.z));

				for (int z = iMin[2]; z <= iMax[2]; ++z)
				{
					for (int y = iMin[1]; y <= iMax[1]; ++y)
					{
						for (int x = iMin[0]; x <= iMax[0]; ++x)
						{
							Vector3 p = bbox.min + Vector3((x + 0.5f) * voxelSize.x, (y + 0.5f) * voxelSize.y, (z + 0.5f) * voxelSize.z);
							float outSide;
							Vector3 closest = Math::PointToTriangleClosestPoint(p, v0, v1, v2, outSide);
							if ((p - closest).length2() < thresholdLength * thresholdLength)
							{
								size_t index = x + dims.x * size_t(y + dims.y * z);
								voxelRawData.data[index] = 1;
							}
						}
					}
				}
			}
			return true;
		}


		static bool MeshSurface(TArray< VoxelPoint >& inoutPoints, IntVector3 const& dims, AABBox const& bbox, MeshRawData const& meshData, InputLayoutDesc const& inputLayoutDesc, Matrix4 const& transform)
		{
			TIME_SCOPE("MeshSurface");
			Vector3 boundSize = bbox.getSize();
			Vector3 voxelSize = { boundSize.x / dims.x, boundSize.y / dims.y, boundSize.z / dims.z };

			auto posReader = meshData.makeAttributeReader(inputLayoutDesc, EVertex::ATTRIBUTE_POSITION);
			auto GetVertexPos = [&](uint32 index) -> Vector3
			{
				return TransformPosition(posReader[index], transform);
			};

			float thresholdLength = voxelSize.length() * 0.5f;

			for (int i = 0; i < (int)meshData.indices.size(); i += 3)
			{
				Vector3 v0 = GetVertexPos(meshData.indices[i]);
				Vector3 v1 = GetVertexPos(meshData.indices[i + 1]);
				Vector3 v2 = GetVertexPos(meshData.indices[i + 2]);

				Vector3 tMin = v0.min(v1).min(v2);
				Vector3 tMax = v0.max(v1).max(v2);

				int iMin[3], iMax[3];
				iMin[0] = Math::Max(0, Math::FloorToInt((tMin.x - bbox.min.x) / voxelSize.x));
				iMin[1] = Math::Max(0, Math::FloorToInt((tMin.y - bbox.min.y) / voxelSize.y));
				iMin[2] = Math::Max(0, Math::FloorToInt((tMin.z - bbox.min.z) / voxelSize.z));

				iMax[0] = Math::Min(dims.x - 1, Math::FloorToInt((tMax.x - bbox.min.x) / voxelSize.x));
				iMax[1] = Math::Min(dims.y - 1, Math::FloorToInt((tMax.y - bbox.min.y) / voxelSize.y));
				iMax[2] = Math::Min(dims.z - 1, Math::FloorToInt((tMax.z - bbox.min.z) / voxelSize.z));

				for (int z = iMin[2]; z <= iMax[2]; ++z)
				{
					for (int y = iMin[1]; y <= iMax[1]; ++y)
					{
						for (int x = iMin[0]; x <= iMax[0]; ++x)
						{
							Vector3 p = bbox.min + Vector3((x + 0.5f) * voxelSize.x, (y + 0.5f) * voxelSize.y, (z + 0.5f) * voxelSize.z);
							float outSide;
							Vector3 closest = Math::PointToTriangleClosestPoint(p, v0, v1, v2, outSide);
							if ((p - closest).length2() < thresholdLength * thresholdLength)
							{
								inoutPoints.push_back({ IntVector3(x, y, z) , 1 });
							}
						}
					}
				}
			}
			return true;
		}

		static bool MeshSolid(VoxelRawData& voxelRawData, AABBox const& bbox, MeshRawData const& meshData, InputLayoutDesc const& inputLayoutDesc, Matrix4 const& transform)
		{
			IntVector3 const& dims = voxelRawData.dims;
			Vector3 boundSize = bbox.getSize();
			Vector3 voxelSize = { boundSize.x / dims.x, boundSize.y / dims.y, boundSize.z / dims.z };

			auto posReader = meshData.makeAttributeReader(inputLayoutDesc, EVertex::ATTRIBUTE_POSITION);
			auto GetVertexPos = [&](uint32 index) -> Vector3
			{
				return TransformPosition(posReader[index], transform);
			};

			TArray< TArray<float> > zIntercepts;
			zIntercepts.resize(dims.x * dims.y);

			for (int i = 0; i < (int)meshData.indices.size(); i += 3)
			{
				Vector3 v0 = GetVertexPos(meshData.indices[i]);
				Vector3 v1 = GetVertexPos(meshData.indices[i + 1]);
				Vector3 v2 = GetVertexPos(meshData.indices[i + 2]);

				Vector3 tMin = v0.min(v1).min(v2);
				Vector3 tMax = v0.max(v1).max(v2);

				int iMinX = Math::Max(0, Math::FloorToInt((tMin.x - bbox.min.x) / voxelSize.x));
				int iMinY = Math::Max(0, Math::FloorToInt((tMin.y - bbox.min.y) / voxelSize.y));
				int iMaxX = Math::Min(dims.x - 1, Math::FloorToInt((tMax.x - bbox.min.x) / voxelSize.x));
				int iMaxY = Math::Min(dims.y - 1, Math::FloorToInt((tMax.y - bbox.min.y) / voxelSize.y));

				Vector3 e1 = v1 - v0;
				Vector3 e2 = v2 - v0;
				Vector3 n = e1.cross(e2);
				if (Math::Abs(n.z) < 1e-6f)
					continue;

				for (int y = iMinY; y <= iMaxY; ++y)
				{
					for (int x = iMinX; x <= iMaxX; ++x)
					{
						Vector3 p = bbox.min + Vector3((x + 0.5f) * voxelSize.x, (y + 0.5f) * voxelSize.y, 0.0f);

						// Check if (x,y) is inside 2D projection of triangle
						Vector2 p2(p.x, p.y);
						Vector2 a2(v0.x, v0.y);
						Vector2 b2(v1.x, v1.y);
						Vector2 c2(v2.x, v2.y);

						float w0 = ((b2.y - c2.y) * (p2.x - c2.x) + (c2.x - b2.x) * (p2.y - c2.y)) / ((b2.y - c2.y) * (a2.x - c2.x) + (c2.x - b2.x) * (a2.y - c2.y));
						float w1 = ((c2.y - a2.y) * (p2.x - c2.x) + (a2.x - c2.x) * (p2.y - c2.y)) / ((b2.y - c2.y) * (a2.x - c2.x) + (c2.x - b2.x) * (a2.y - c2.y));
						float w2 = 1.0f - w0 - w1;

						if (w0 >= 0 && w1 >= 0 && w2 >= 0)
						{
							// Calculate Z using plane equation: n.x*(x-v0.x) + n.y*(y-v0.y) + n.z*(z-v0.z) = 0
							float z = v0.z - (n.x * (p.x - v0.x) + n.y * (p.y - v0.y)) / n.z;
							zIntercepts[x + dims.x * y].push_back((z - bbox.min.z) / voxelSize.z);
						}
					}
				}
			}

			for (int y = 0; y < dims.y; ++y)
			{
				for (int x = 0; x < dims.x; ++x)
				{
					auto& list = zIntercepts[x + dims.x * y];
					if (list.empty()) continue;
					std::sort(list.begin(), list.end());

					for (int i = 0; i + 1 < (int)list.size(); i += 2)
					{
						int zStart = Math::Max(0, (int)Math::Round(list[i]));
						int zEnd = Math::Min(dims.z - 1, (int)Math::Round(list[i + 1]));
						for (int z = zStart; z <= zEnd; ++z)
						{
							size_t index = x + dims.x * size_t(y + dims.y * z);
							voxelRawData.data[index] = 1;
						}
					}
				}
			}

			return true;
		}

		static int BuildOctTree(VoxelRawData const& voxelRawData, TArray<OctTreeNode>& outNodes)
		{
			TIME_SCOPE("BuildOctTree");
			outNodes.clear();
			IntVector3 const& dims = voxelRawData.dims;
			int maxSize = Math::Max(dims.x, Math::Max(dims.y, dims.z));
			int size = 1;
			while (size < maxSize) size *= 2;

			return BuildOctTreeRecursive(voxelRawData, outNodes, IntVector3(0, 0, 0), size);
		}

		static int BuildOctTree(TArray<VoxelPoint>& points, IntVector3 const& dims, TArray<OctTreeNode>& outNodes)
		{
			TIME_SCOPE("BuildOctTree");
			outNodes.clear();
			if (points.size() == 0)
				return -1;

			int maxSize = Math::Max(dims.x, Math::Max(dims.y, dims.z));
			int size = 1;
			while (size < maxSize) size *= 2;

			return BuildOctTreeRecursive(points, dims, 0, (int)points.size(), outNodes, IntVector3(0, 0, 0), size);
		}

		static int BuildOctTreeRecursive(TArray<VoxelPoint>& points, IntVector3 const& dims, int start, int end, TArray<OctTreeNode>& outNodes, IntVector3 const& pos, int size)
		{
			if (start == end)
				return -1;

			if (size == 1)
			{
				return -2;
			}

			int halfSize = size / 2;
			int childIndices[8];
			bool bAllSolid = true;
			bool bAllEmpty = true;

			int childStarts[9];
			childStarts[0] = start;
			childStarts[8] = end;

			// Partition points into 8 octants
			// Using multiple passes of std::partition is an easy way to do this in-place
			auto getX = [&](VoxelPoint const& p) { return p.pos.x; };
			auto getY = [&](VoxelPoint const& p) { return p.pos.y; };
			auto getZ = [&](VoxelPoint const& p) { return p.pos.z; };

			int midZ = pos.z + halfSize;
			int midY = pos.y + halfSize;
			int midX = pos.x + halfSize;

			auto itStart = points.begin() + start;
			auto itEnd = points.begin() + end;

			auto itMidZ = std::partition(itStart, itEnd, [&](VoxelPoint const& p) { return getZ(p) < midZ; });
			int splitZ = (int)(itMidZ - points.begin());

			auto itMidY0 = std::partition(itStart, itMidZ, [&](VoxelPoint const& p) { return getY(p) < midY; });
			int splitY0 = (int)(itMidY0 - points.begin());
			auto itMidY1 = std::partition(itMidZ, itEnd, [&](VoxelPoint const& p) { return getY(p) < midY; });
			int splitY1 = (int)(itMidY1 - points.begin());

			auto itMidX0 = std::partition(itStart, itMidY0, [&](VoxelPoint const& p) { return getX(p) < midX; });
			auto itMidX1 = std::partition(itMidY0, itMidZ, [&](VoxelPoint const& p) { return getX(p) < midX; });
			auto itMidX2 = std::partition(itMidZ, itMidY1, [&](VoxelPoint const& p) { return getX(p) < midX; });
			auto itMidX3 = std::partition(itMidY1, itEnd, [&](VoxelPoint const& p) { return getX(p) < midX; });

			childStarts[0] = start;
			childStarts[1] = (int)(itMidX0 - points.begin());
			childStarts[2] = (int)(itMidY0 - points.begin());
			childStarts[3] = (int)(itMidX1 - points.begin());
			childStarts[4] = (int)(itMidZ - points.begin());
			childStarts[5] = (int)(itMidX2 - points.begin());
			childStarts[6] = (int)(itMidY1 - points.begin());
			childStarts[7] = (int)(itMidX3 - points.begin());
			childStarts[8] = end;

			// Re-map the partition indices to Octant indices (bit 0:x, bit 1:y, bit 2:z)
			// Octant indices:
			// 0: 000 (x:0, y:0, z:0) -> [childStarts[0], childStarts[1])
			// 1: 001 (x:1, y:0, z:0) -> [childStarts[1], childStarts[2])
			// 2: 010 (x:0, y:1, z:0) -> [childStarts[2], childStarts[3])
			// 3: 011 (x:1, y:1, z:0) -> [childStarts[3], childStarts[4])
			// 4: 100 (x:0, y:0, z:1) -> [childStarts[4], childStarts[5])
			// 5: 101 (x:1, y:0, z:1) -> [childStarts[5], childStarts[6])
			// 6: 110 (x:0, y:1, z:1) -> [childStarts[6], childStarts[7])
			// 7: 111 (x:1, y:1, z:1) -> [childStarts[7], childStarts[8])

			for (int i = 0; i < 8; ++i)
			{
				IntVector3 childPos = pos + IntVector3((i & 1) ? halfSize : 0, (i & 2) ? halfSize : 0, (i & 4) ? halfSize : 0);
				childIndices[i] = BuildOctTreeRecursive(points, dims, childStarts[i], childStarts[i + 1], outNodes, childPos, halfSize);

				if (childIndices[i] != -2) bAllSolid = false;
				if (childIndices[i] != -1) bAllEmpty = false;
			}

			if (bAllSolid) return -2;
			if (bAllEmpty) return -1;

			int nodeIdx = (int)outNodes.size();
			outNodes.push_back(OctTreeNode());
			for (int i = 0; i < 8; ++i)
			{
				outNodes[nodeIdx].children[i] = childIndices[i];
			}
			return nodeIdx;
		}

	private:
		static int BuildOctTreeRecursive(VoxelRawData const& voxelRawData, TArray<OctTreeNode>& outNodes, IntVector3 const& pos, int size)
		{
			if (size == 1)
			{
				if (pos.x >= voxelRawData.dims.x || pos.y >= voxelRawData.dims.y || pos.z >= voxelRawData.dims.z)
					return -1;

				size_t index = pos.x + voxelRawData.dims.x * size_t(pos.y + voxelRawData.dims.y * pos.z);
				return voxelRawData.data[index] ? -2 : -1;
			}

			int halfSize = size / 2;
			int children[8];
			bool allSame = true;
			for (int i = 0; i < 8; ++i)
			{
				IntVector3 childPos = pos + IntVector3((i & 1) ? halfSize : 0, (i & 2) ? halfSize : 0, (i & 4) ? halfSize : 0);
				children[i] = BuildOctTreeRecursive(voxelRawData, outNodes, childPos, halfSize);
				if (i > 0 && children[i] != children[0])
					allSame = false;
			}

			if (allSame && children[0] < 0)
				return children[0];

			int nodeIndex = (int)outNodes.size();
			outNodes.push_back({});
			for (int i = 0; i < 8; ++i)
				outNodes[nodeIndex].children[i] = children[i];
			return nodeIndex;
		}

	public:

		static int Build64Tree(TArray<VoxelPoint>& points, IntVector3 const& dims, TArray<SvtNode64>& outNodes)
		{
			TIME_SCOPE("Build64Tree");
			outNodes.clear();
			if (points.empty()) return -1;

			int maxSize = Math::Max(dims.x, Math::Max(dims.y, dims.z));
			int sizeExp = 0;
			int size = 1;
			while (size < maxSize) 
			{ 
				size *= 4; 
				sizeExp += 2; 
			}

			TArray<VoxelPoint> tempPoints;
			tempPoints.resize(points.size());

			SvtNode64 rootNode = Build64TreeRecursive(points, tempPoints, 0, (int)points.size(), outNodes, sizeExp);
			int rootIdx = (int)outNodes.size();
			outNodes.push_back(rootNode);
			return rootIdx;
		}

	private:

		static SvtNode64 Build64TreeRecursive(TArray<VoxelPoint>& points, TArray<VoxelPoint>& tempPoints, int start, int end, TArray<SvtNode64>& outNodes, int sizeExp)
		{
			SvtNode64 node = { 0, 0, 0, 0 };
			if (start == end) return node;

			if (sizeExp == 2)
			{
				node.childPtr = (1u << 31); // IsLeaf = 1
				uint64 mask = 0;
				for (int i = start; i < end; ++i)
				{
					IntVector3 p = points[i].pos;
					int bitIdx = (p.x & 3) + ((p.z & 3) << 2) + ((p.y & 3) << 4);
					mask |= (1ull << bitIdx);
				}
				node.maskLow = (uint32)(mask & 0xFFFFFFFF);
				node.maskHigh = (uint32)(mask >> 32);
				return node;
			}

			int nextSizeExp = sizeExp - 2;

			int bucketSizes[64] = { 0 };
			for (int i = start; i < end; ++i)
			{
				IntVector3 p = points[i].pos;
				int ia = ((p.x >> nextSizeExp) & 3) + (((p.z >> nextSizeExp) & 3) << 2) + (((p.y >> nextSizeExp) & 3) << 4);
				bucketSizes[ia]++;
			}

			int offsets[64] = { 0 };
			int currentOffset = start;
			for (int i = 0; i < 64; ++i)
			{
				offsets[i] = currentOffset;
				currentOffset += bucketSizes[i];
			}

			for (int i = start; i < end; ++i)
			{
				IntVector3 p = points[i].pos;
				int ia = ((p.x >> nextSizeExp) & 3) + (((p.z >> nextSizeExp) & 3) << 2) + (((p.y >> nextSizeExp) & 3) << 4);
				int destIdx = offsets[ia]++;
				tempPoints[destIdx] = points[i];
			}

			for (int i = start; i < end; ++i)
				points[i] = tempPoints[i];

			uint64 mask = 0;
			TArray<SvtNode64> children;

			currentOffset = start;
			for (int i = 0; i < 64; ++i)
			{
				int bucketSize = bucketSizes[i];
				if (bucketSize > 0)
				{
					int childStart = currentOffset;
					int childEnd = currentOffset + bucketSize;
					SvtNode64 childNode = Build64TreeRecursive(points, tempPoints, childStart, childEnd, outNodes, nextSizeExp);
					if (childNode.maskLow != 0 || childNode.maskHigh != 0 || (childNode.childPtr & 0x7FFFFFFF) != 0 || (childNode.childPtr & (1u << 31)) != 0)
					{
						mask |= (1ull << i);
						children.push_back(childNode);
					}
				}
				currentOffset += bucketSize;
			}

			node.maskLow = (uint32)(mask & 0xFFFFFFFF);
			node.maskHigh = (uint32)(mask >> 32);

			if (!children.empty())
			{
				node.childPtr = (uint32)outNodes.size();
				outNodes.insert(outNodes.end(), children.begin(), children.end());
			}

			return node;
		}
	};

	class VoxelProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(VoxelProgram, Global);
	public:
		static void SetupShaderCompileOption(ShaderCompileOption& option)
		{
			option.addDefine(SHADER_PARAM(USE_INSTANCED), 1);
		}

		static char const* GetShaderFileName() { return "Shader/Game/Voxel"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries()
		{
			static ShaderEntryInfo const entries[] =
			{
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LocalToWorld);
			BIND_SHADER_PARAM(parameterMap, VoxelColor);
		}

		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(VoxelColor);
	};

	IMPLEMENT_SHADER_PROGRAM(VoxelProgram);

	class VoxelRayMarchProgram : public GlobalShaderProgram
	{
		DECLARE_SHADER_PROGRAM(VoxelRayMarchProgram, Global);
	public:
		SHADER_PERMUTATION_TYPE_BOOL(UseOctTree, SHADER_PARAM(USE_OCTREE));
		SHADER_PERMUTATION_TYPE_BOOL(Use64Tree, SHADER_PARAM(USE_64TREE));

		using PermutationDomain = TShaderPermutationDomain<UseOctTree, Use64Tree>;

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
		}

		static char const* GetShaderFileName() { return "Shader/Game/VoxelRayMarch"; }
		static TArrayView< ShaderEntryInfo const > GetShaderEntries(PermutationDomain const& domain)
		{
			static ShaderEntryInfo const entries[] = {
				{ EShader::Vertex , SHADER_ENTRY(MainVS) },
				{ EShader::Pixel  , SHADER_ENTRY(MainPS) },
			};
			return entries;
		}

		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, LocalToWorld);
			BIND_SHADER_PARAM(parameterMap, WorldToLocal);
			BIND_SHADER_PARAM(parameterMap, BBoxMin);
			BIND_SHADER_PARAM(parameterMap, BBoxMax);
			BIND_SHADER_PARAM(parameterMap, VoxelDims);
			BIND_SHADER_PARAM(parameterMap, VoxelColor);
			BIND_SHADER_PARAM(parameterMap, VoxelData);
			BIND_SHADER_PARAM(parameterMap, OctTreeData);
			BIND_SHADER_PARAM(parameterMap, Svt64Data);
			BIND_SHADER_PARAM(parameterMap, RootNodeIndex);
		}

		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(WorldToLocal);
		DEFINE_SHADER_PARAM(BBoxMin);
		DEFINE_SHADER_PARAM(BBoxMax);
		DEFINE_SHADER_PARAM(VoxelDims);
		DEFINE_SHADER_PARAM(VoxelColor);
		DEFINE_SHADER_PARAM(VoxelData);
		DEFINE_SHADER_PARAM(OctTreeData);
		DEFINE_SHADER_PARAM(Svt64Data);
		DEFINE_SHADER_PARAM(RootNodeIndex);
	};

	IMPLEMENT_SHADER_PROGRAM(VoxelRayMarchProgram);

	class VoxelRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		VoxelRenderingStage() {}

		enum
		{
			UI_RenderMethod = BaseClass::NEXT_UI_ID,


			NEXT_UI_ID,
		};

		ERenderMethod mRenderMethod = ERenderMethod::Instanced;
		bool bShowModel;
		TArray<VoxelPoint> mPoints;

		Mesh mModelMesh;
		Matrix4 mModelXForm;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

#if 1
			MeshImportData importData;
			LoadMeshDataFromFile(importData, "Model/Sponza.obj");

			mModelMesh.mInputLayoutDesc = importData.desc;
			mModelMesh.mType = EPrimitive::TriangleList;
			mModelMesh.createRHIResource(importData);

			mModelXForm = Matrix4::Scale(0.1) * Matrix4::Rotate(Vector3(1, 0, 0), Math::DegToRad(90));
			AABBox localBBox = importData.getBoundBox();
			mBBox.invalidate();
			for (int i = 1; i < 8; ++i)
			{
				Vector3 p;
				p.x = (i & 1) ? localBBox.max.x : localBBox.min.x;
				p.y = (i & 2) ? localBBox.max.y : localBBox.min.y;
				p.z = (i & 4) ? localBBox.max.z : localBBox.min.z;
				mBBox.addPoint(TransformPosition(p, mModelXForm));
			}
			mBBox.expand(Vector3(0.01, 0.01, 0.01));

			Vector3 boundSize = mBBox.getSize();
			float maxSideSize = Math::Max(boundSize.x, Math::Max(boundSize.y, boundSize.z));
			float voxelSizeVal = maxSideSize / 1024.0f;
			Vector3 size = boundSize / voxelSizeVal;
			mRawData.dims.x = Math::CeilToInt(size.x);
			mRawData.dims.y = Math::CeilToInt(size.y);
			mRawData.dims.z = Math::CeilToInt(size.z);

			mPoints.clear();
			FVoxelBuild::MeshSurface(mPoints, mRawData.dims, mBBox, importData, importData.desc, mModelXForm);
			
			mRawData.initData(mPoints);

			mOctTreeNodes.clear();
			mRootNodeIndex = FVoxelBuild::BuildOctTree(mPoints, mRawData.dims, mOctTreeNodes);
			
			mSvtNodes.clear();
			mSvtRootIndex = FVoxelBuild::Build64Tree(mPoints, mRawData.dims, mSvtNodes);
			
			LogMsg("Voxel Dims: %d %d %d", mRawData.dims.x, mRawData.dims.y, mRawData.dims.z);
			LogMsg("BBox Min: %f %f %f", mBBox.min.x, mBBox.min.y, mBBox.min.z);
			LogMsg("BBox Max: %f %f %f", mBBox.max.x, mBBox.max.y, mBBox.max.z);
#else
			MeshRawData doughnutData;
			InputLayoutDesc inputLayout;
			FMeshBuild::Doughnut(doughnutData, inputLayout, 2.0f, 1.0f, 60, 60);

			mBBox.min = Vector3(-3.5, -3.5, -1.5);
			mBBox.max = Vector3(3.5, 3.5, 1.5);
			mRawData.dims = 2 * IntVector3(64, 64, 32);
			
			//mRawData.initData();
			//FVoxelBuild::MeshSurface(mRawData, mBBox, doughnutData, inputLayout, Matrix4::Identity());

			FVoxelBuild::MeshSurface(mPoints, mRawData.dims, mBBox, doughnutData, inputLayout, Matrix4::Identity());
			mRootNodeIndex = FVoxelBuild::BuildOctTree(mPoints, mRawData.dims, mOctTreeNodes);
#endif



			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();
			auto choice = frame->addChoice("DebugDisplay Mode", UI_RenderMethod);
			char const* MethodTextList[] =
			{
				"Instanced",
				"RawDDA",
				"OctTree",
				"64-Tree",
			};
			static_assert(ARRAY_SIZE(MethodTextList) == (int)ERenderMethod::COUNT);
			for (int i = 0; i < (int)ERenderMethod::COUNT; ++i)
			{
				choice->addItem(MethodTextList[i]);
			}
			choice->setSelection((int)mRenderMethod);
			frame->addCheckBox("Show Model", bShowModel);
			return true;
		}


		virtual ERenderSystem getDefaultRenderSystem() override
		{
			return ERenderSystem::None;
		}

		virtual void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
		{
			systemConfigs.screenWidth = 1024;
			systemConfigs.screenHeight = 768;
		}

		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(createSimpleMesh());
			VERIFY_RETURN_FALSE(loadCommonShader());
			VERIFY_RETURN_FALSE(mProgVoxel = ShaderManager::Get().getGlobalShaderT<VoxelProgram>(true));

			mVoxelMesh.setupMesh(getMesh(SimpleMeshId::Box));
			
			if (!mRawData.data.empty())
			{
				TArray<uint32> bufferData;
				bufferData.resize(mRawData.data.size());
				for (size_t i = 0; i < mRawData.data.size(); ++i)
					bufferData[i] = mRawData.data[i];
				mVoxelBuffer.initializeResource((uint32)bufferData.size(), EStructuredBufferType::StaticBuffer, BCF_None, bufferData.data());
			}

			updateVoxelMesh();

			if (mOctTreeNodes.size() > 0)
			{
				mOctTreeBuffer.initializeResource((uint32)mOctTreeNodes.size(), EStructuredBufferType::StaticBuffer, BCF_None, mOctTreeNodes.data());
			}
			if (mSvtNodes.size() > 0)
			{
				mSvtBuffer.initializeResource((uint32_t)mSvtNodes.size(), EStructuredBufferType::StaticBuffer, BCF_None, mSvtNodes.data());
			}
			return true;
		}

		void updateVoxelMesh()
		{
			mVoxelMesh.clearInstance();

			IntVector3 const& dims = mRawData.dims;
			Vector3 boundSize = mBBox.getSize();
			Vector3 voxelSize = { boundSize.x / dims.x, boundSize.y / dims.y, boundSize.z / dims.z };

			if (!mPoints.empty())
			{
				LogMsg("Voxel Count = %d", mPoints.size());

				int maxShowSize = Math::Min<int>(mPoints.size(), 100000);

				for(int i = 0; i < maxShowSize; ++i)
				{
					auto const& pos = mPoints[i].pos;
					Vector3 p = mBBox.min + Vector3((pos.x + 0.5f) * voxelSize.x, (pos.y + 0.5f) * voxelSize.y, (pos.z + 0.5f) * voxelSize.z);
					mVoxelMesh.addInstance(p, voxelSize * 0.5f, Quaternion::Identity(), Vector4(0, 1, 0, 1));
				}			
			}
			else
			{
				for (int z = 0; z < dims.z; ++z)
				{
					for (int y = 0; y < dims.y; ++y)
					{
						for (int x = 0; x < dims.x; ++x)
						{
							size_t index = x + dims.x * size_t(y + dims.y * z);
							if (mRawData.data[index] == 1)
							{
								Vector3 p = mBBox.min + Vector3((x + 0.5f) * voxelSize.x, (y + 0.5f) * voxelSize.y, (z + 0.5f) * voxelSize.z);
								mVoxelMesh.addInstance(p, voxelSize * 0.5f, Quaternion::Identity(), Vector4(0, 1, 0, 1));
							}
						}
					}
				}
			}

		}

		virtual void preShutdownRenderSystem(bool bReInit = false) override
		{
			mVoxelBuffer.releaseResource();
			mOctTreeBuffer.releaseResource();
			mSvtBuffer.releaseResource();
			mVoxelMesh.releaseRHI();
			BaseClass::preShutdownRenderSystem(bReInit);
		}


		void onEnd() override
		{


		}

		void restart() override
		{

		}

		void onUpdate(GameTimeSpan deltaTime) override
		{
			BaseClass::onUpdate(deltaTime);
		}

		void onRender(float dFrame) override
		{
			initializeRenderState();

			RHICommandList& commandList = RHICommandList::GetImmediateList();

			if (mRenderMethod == ERenderMethod::Instanced)
			{
				GPU_PROFILE("Instanced");
#if 1
				RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());

				RHISetShaderProgram(commandList, mProgVoxel->getRHI());
				mView.setupShader(commandList, *mProgVoxel);
				SET_SHADER_PARAM(commandList, *mProgVoxel, LocalToWorld, Matrix4::Identity());
				SET_SHADER_PARAM(commandList, *mProgVoxel, VoxelColor, LinearColor(0, 1, 0, 1));
				mVoxelMesh.draw(commandList);
#endif
			}
			else
			{
				GPU_PROFILE("RayMarch");

				Matrix4 boxLocalToWorld = Matrix4::Scale(0.5 * mBBox.getSize()) * Matrix4::Translate(mBBox.getCenter());
#if 0

				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None, EFillMode::Wireframe>::GetRHI());
				RHISetFixedShaderPipelineState(commandList, boxLocalToWorld * mView.worldToClipRHI);
				mSimpleMeshs[SimpleMeshId::Box].draw(commandList, LinearColor(1, 0, 0));
#endif

				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Front>::GetRHI());


				VoxelRayMarchProgram::PermutationDomain permutationVector;
				permutationVector.set<VoxelRayMarchProgram::UseOctTree>(mRenderMethod == ERenderMethod::OctTree);
				permutationVector.set<VoxelRayMarchProgram::Use64Tree>(mRenderMethod == ERenderMethod::S64Tree);
				VoxelRayMarchProgram* progRayMarch = ShaderManager::Get().getGlobalShaderT<VoxelRayMarchProgram>(permutationVector);


				RHISetShaderProgram(commandList, progRayMarch->getRHI());
				mView.setupShader(commandList, *progRayMarch);
				SET_SHADER_PARAM(commandList, *progRayMarch, LocalToWorld, boxLocalToWorld);

				float det;
				Matrix4 boxWorldToLocal;
				boxLocalToWorld.inverse(boxWorldToLocal, det);

				SET_SHADER_PARAM(commandList, *progRayMarch, WorldToLocal, boxWorldToLocal);
				SET_SHADER_PARAM(commandList, *progRayMarch, BBoxMin, mBBox.min);
				SET_SHADER_PARAM(commandList, *progRayMarch, BBoxMax, mBBox.max);
				SET_SHADER_PARAM(commandList, *progRayMarch, VoxelDims, mRawData.dims);
				SET_SHADER_PARAM(commandList, *progRayMarch, VoxelColor, LinearColor(0, 1, 0, 1));
				SET_SHADER_PARAM(commandList, *progRayMarch, RootNodeIndex, (mRenderMethod == ERenderMethod::S64Tree) ? mSvtRootIndex : mRootNodeIndex);
				if (mRenderMethod == ERenderMethod::OctTree)
				{
					if (mOctTreeBuffer.isValid())
					{
						progRayMarch->setStorageBuffer(commandList, progRayMarch->mParamOctTreeData, *mOctTreeBuffer.getRHI(), EAccessOp::ReadOnly);
					}	
				}
				else if (mRenderMethod == ERenderMethod::S64Tree)
				{
					if (mSvtBuffer.isValid())
					{
						progRayMarch->setStorageBuffer(commandList, progRayMarch->mParamSvt64Data, *mSvtBuffer.getRHI(), EAccessOp::ReadOnly);
					}
				}
				else
				{
					if (mVoxelBuffer.isValid())
					{
						progRayMarch->setStorageBuffer(commandList, progRayMarch->mParamVoxelData, *mVoxelBuffer.getRHI(), EAccessOp::ReadOnly);
					}
				}
				getMesh(SimpleMeshId::Box).draw(commandList);
			}

			if (bShowModel)
			{
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None, EFillMode::Wireframe>::GetRHI());
				RHISetFixedShaderPipelineState(commandList, mModelXForm * mView.worldToClipRHI);
				mModelMesh.draw(commandList, LinearColor(1, 0, 0));
			}
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			return BaseClass::onMouse(msg);
		}

		MsgReply onKey(KeyMsg const& msg) override
		{
			if (msg.isDown())
			{
				switch (msg.getCode())
				{
				case EKeyCode::M:
					bRayMarchMode = !bRayMarchMode;
					break;
				default:
					break;
				}
			}
			return BaseClass::onKey(msg);
		}

		bool onWidgetEvent(int event, int id, GWidget* ui) override
		{
			switch (id)
			{
			case UI_RenderMethod:
				{
					mRenderMethod = ERenderMethod(ui->cast<GChoice>()->getSelection());
				}
			return true;
			default:
				break;
			}

			return BaseClass::onWidgetEvent(event, id, ui);
		}

	protected:
		VoxelRawData mRawData;
		AABBox mBBox;
		InstancedMesh mVoxelMesh;
		VoxelProgram* mProgVoxel;

		TStructuredBuffer<uint32> mVoxelBuffer;
		TArray<OctTreeNode> mOctTreeNodes;
		int mRootNodeIndex;
		TStructuredBuffer<OctTreeNode> mOctTreeBuffer;

		TArray<SvtNode64> mSvtNodes;
		int mSvtRootIndex;
		TStructuredBuffer<SvtNode64> mSvtBuffer;

		bool bRayMarchMode = false;
	};

	REGISTER_STAGE_ENTRY("Voxel Rendering", VoxelRenderingStage, EExecGroup::FeatureDev);

}