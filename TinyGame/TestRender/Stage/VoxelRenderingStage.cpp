#include "Stage/TestRenderStageBase.h"
#include "Renderer/MeshBuild.h"
#include "Math/PrimitiveTest.h"
#include "Math/GeometryPrimitive.h"
#include "RHI/SceneRenderer.h"
#include "Renderer/MeshImportor.h"
#include "ProfileSystem.h"
#include "Async/AsyncWork.h"

#include "DDGIProbeManager.h"

#include <algorithm>
#include <atomic>
#include "Memory/BuddyAllocator.h"

namespace Render
{
#if 0
	static constexpr int ChunkSize = 64;
	static constexpr int ChunkSizeExp = 4;
#else
	static constexpr int ChunkSize = 256;
	static constexpr int ChunkSizeExp = 6;
#endif

	enum class ERenderMethod
	{
		Instanced,
		RawDDA,
		OctTree,
		S64Tree,
		S64TreeChunk,
		
		COUNT,
	};

	struct IntVectorCompare
	{
		bool operator()(IntVector3 const& a, IntVector3 const& b) const
		{
			if (a.x != b.x) return a.x < b.x;
			if (a.y != b.y) return a.y < b.y;
			return a.z < b.z;
		}
	};


	struct VoxelPoint
	{
		IntVector3 pos;
		uint32 data;
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
				data[index] = (uint8)point.data;
			}
		}

		template< class Op >
		void serialize(Op& op)
		{
			op & data & dims;
		}
	};



	using AABBox = Math::TAABBox< Vector3 >;

	struct OctTreeNode
	{
		int children[8];
	};
	
	struct SvtNode64
	{
		uint32 childPtr;
		uint64 mask;      
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
			float thresholdLength = voxelSize.length() * 0.5f;

			int numTriangles = (int)meshData.indices.size() / 3;
			int numThreads = SystemPlatform::GetProcessorNumber();
			QueueThreadPool threadPool;
			threadPool.init(numThreads);

			TArray< TArray<VoxelPoint> > threadPoints;
			threadPoints.resize(numThreads);

			std::atomic<int> nextTriIdx(0);
			const int batchSize = 128;

			for (int iThread = 0; iThread < numThreads; ++iThread)
			{
				threadPool.addFunctionWork([=, &nextTriIdx, &threadPoints, &meshData, &bbox, &dims, &voxelSize]()
				{
					auto& localPoints = threadPoints[iThread];
					auto posReader = meshData.makeAttributeReader(inputLayoutDesc, EVertex::ATTRIBUTE_POSITION);
					auto GetVertexPos = [&](uint32 index) -> Vector3
					{
						return TransformPosition(posReader[index], transform);
					};

					while (true)
					{
						int startTriIdx = nextTriIdx.fetch_add(batchSize);
						if (startTriIdx >= numTriangles)
							break;

						int endTriIdx = Math::Min(startTriIdx + batchSize, numTriangles);

						for (int i = startTriIdx; i < endTriIdx; ++i)
						{
							Vector3 v0 = GetVertexPos(meshData.indices[3 * i]);
							Vector3 v1 = GetVertexPos(meshData.indices[3 * i + 1]);
							Vector3 v2 = GetVertexPos(meshData.indices[3 * i + 2]);

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
											localPoints.push_back({ IntVector3(x, y, z) , 1 });
										}
									}
								}
							}
						}
					}
				});
			}

			threadPool.waitAllWorkComplete();

			for (auto const& points : threadPoints)
			{
				inoutPoints.append(points);
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

		static int Build64Tree(TArray<VoxelPoint>& points, IntVector3 const& dims, TArray<SvtNode64>& outNodes, bool bAllowDataLeaves = false)
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

			SvtNode64 rootNode = Build64TreeRecursive(points, tempPoints, 0, (int)points.size(), outNodes, sizeExp, bAllowDataLeaves);
			int rootIdx = (int)outNodes.size();
			outNodes.push_back(rootNode);
			return rootIdx;
		}

	private:

		static SvtNode64 Build64TreeRecursive(TArray<VoxelPoint>& points, TArray<VoxelPoint>& tempPoints, int start, int end, TArray<SvtNode64>& outNodes, int sizeExp, bool bAllowDataLeaves)
		{
			SvtNode64 node = { 0, 0 };
			if (start == end) return node;

			if (sizeExp == 0)
			{
				node.childPtr = (1u << 31) | uint32(points[start].data);
				return node;
			}

			if (sizeExp == 2 && !bAllowDataLeaves)
			{
				node.childPtr = (1u << 31); // IsLeaf = 1
				uint64 mask = 0;
				for (int i = start; i < end; ++i)
				{
					IntVector3 p = points[i].pos;
					int bitIdx = (p.x & 3) + ((p.z & 3) << 2) + ((p.y & 3) << 4);
					mask |= (1ull << bitIdx);
				}
				node.mask = mask;
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
					SvtNode64 childNode = Build64TreeRecursive(points, tempPoints, childStart, childEnd, outNodes, nextSizeExp, bAllowDataLeaves);
					if (childNode.mask != 0 || (childNode.childPtr & 0x7FFFFFFF) != 0 || (childNode.childPtr & (1u << 31)) != 0)
					{
						mask |= (1ull << i);
						children.push_back(childNode);
					}
				}
				currentOffset += bucketSize;
			}

			node.mask = mask;

			if (!children.empty())
			{
				node.childPtr = (uint32)outNodes.size();
				outNodes.insert(outNodes.end(), children.begin(), children.end());
			}

			return node;
		}
	};
	
	struct VoxelHit
	{
		bool hit = false;
		float t = 1e10f;
		Vector3 pos;
		Vector3 normal;
		Vector3 voxelPos;
		uint32 data;
	};

	struct CPUNode64
	{
		uint64 mask = 0;
		uint32 data[64]; // Stores voxel data for leaves, or pointers for internal?
		CPUNode64* children = nullptr;

		CPUNode64() { std::fill_n(data, 64, 0); }
		~CPUNode64() { if (children) delete[] children; }

		bool hasChildren() const { return children != nullptr; }
		void ensureChildren() { if (!children) children = new CPUNode64[64]; }
		
		void clear()
		{
			mask = 0;
			if (children) { delete[] children; children = nullptr; }
			std::fill_n(data, 64, 0);
		}
	};

	struct VoxelChunk
	{
		IntVector3 pos;
		uint32 nodeOffset = 0;
		uint32 nodeCount = 0;
		BuddyAllocatorBase::Allocation mAllocation = { 0xFFFFFFFF, 0 };
		CPUNode64 root;
		bool bIsDirty = false;

		void clear()
		{
			root.clear();
			nodeOffset = 0;
			nodeCount = 0;
			mAllocation = { 0xFFFFFFFF, 0 };
			bIsDirty = false;
		}

		void addPoint(IntVector3 const& localPos, uint32 data, int sizeExp = ChunkSizeExp)
		{
			AddInternal(root, localPos, data, sizeExp);
			bIsDirty = true;
		}

		void removePoint(IntVector3 const& localPos, int sizeExp = ChunkSizeExp)
		{
			RemoveInternal(root, localPos, sizeExp);
			bIsDirty = true;
		}

		static void AddInternal(CPUNode64& node, IntVector3 const& p, uint32 val, int sizeExp)
		{
			int childIdx = ((p.x >> sizeExp) & 3) | (((p.z >> sizeExp) & 3) << 2) | (((p.y >> sizeExp) & 3) << 4);
			if (sizeExp == 0) // Voxel Level (Inside a 4x4x4 leaf)
			{
				node.mask |= (1ull << childIdx);
				node.data[childIdx] = val;
			}
			else
			{
				node.ensureChildren();
				node.mask |= (1ull << childIdx);
				AddInternal(node.children[childIdx], p, val, sizeExp - 2);
			}
		}

		static void RemoveInternal(CPUNode64& node, IntVector3 const& p, int sizeExp)
		{
			int childIdx = ((p.x >> sizeExp) & 3) | (((p.z >> sizeExp) & 3) << 2) | (((p.y >> sizeExp) & 3) << 4);
			if (!(node.mask & (1ull << childIdx))) return;

			if (sizeExp == 0)
			{
				node.mask &= ~(1ull << childIdx);
				node.data[childIdx] = 0;
			}
			else if (node.hasChildren())
			{
				RemoveInternal(node.children[childIdx], p, sizeExp - 2);
				if (node.children[childIdx].mask == 0)
				{
					node.mask &= ~(1ull << childIdx);
					// We could prune the child array here if all children are gone
				}
			}
		}

		void collectPoints(TArray<VoxelPoint>& outPoints) const
		{
			CollectInternal(root, IntVector3(0, 0, 0), pos * ChunkSize, ChunkSizeExp, outPoints);
		}

		SvtNode64 generateSvtNodes(TArray<SvtNode64>& outNodes) const
		{
			outNodes.clear();
			return GenerateSvtRecursive(root, ChunkSizeExp, outNodes);
		}

		static SvtNode64 GenerateSvtRecursive(CPUNode64 const& cpuNode, int sizeExp, TArray<SvtNode64>& outNodes)
		{
			SvtNode64 svtNode;
			svtNode.mask = cpuNode.mask;

			if (sizeExp == 0)
			{
				svtNode.childPtr = (1u << 31);
				return svtNode;
			}

			TArray<SvtNode64> children;
			for (int i = 0; i < 64; ++i)
			{
				if (cpuNode.mask & (1ull << i))
				{
					if (cpuNode.hasChildren())
						children.push_back(GenerateSvtRecursive(cpuNode.children[i], sizeExp - 2, outNodes));
					else
						children.push_back({ (1u << 31), 0 }); // Fallback
				}
			}

			if (!children.empty())
			{
				svtNode.childPtr = (uint32)outNodes.size();
				outNodes.append(children);
			}
			else
			{
				svtNode.childPtr = (1u << 31);
			}

			return svtNode;
		}

		static void CollectInternal(CPUNode64 const& node, IntVector3 const& localPos, IntVector3 const& basePos, int sizeExp, TArray<VoxelPoint>& outPoints)
		{
			if (node.mask == 0) return;
			for (int i = 0; i < 64; ++i)
			{
				if (node.mask & (1ull << i))
				{
					IntVector3 p;
					p.x = localPos.x | ((i & 3) << sizeExp);
					p.z = localPos.z | (((i >> 2) & 3) << sizeExp);
					p.y = localPos.y | (((i >> 4) & 3) << sizeExp);

					if (sizeExp == 0)
					{
						outPoints.push_back({ basePos + p, node.data[i] });
					}
					else if (node.hasChildren())
					{
						CollectInternal(node.children[i], p, basePos, sizeExp - 2, outPoints);
					}
				}
			}
		}
	};

	class VoxelChunkManager
	{
	public:
		struct DataRange 
		{ 
			uint32 start; 
			uint32 count; 
		};

		VoxelChunkManager()
		{
			mNodePoolAllocator.initialize(1 << 24, 64);
		}

		void addDirtyRange(uint32 start, uint32 count)
		{
			if (count == 0) return;
			// Merge into existing dirty ranges or append
			bool bMerged = false;
			for (auto& range : mDirtyRanges)
			{
				if (start <= range.start + range.count && range.start <= start + count)
				{
					uint32 end = Math::Max(start + count, range.start + range.count);
					range.start = Math::Min(start, range.start);
					range.count = end - range.start;
					bMerged = true;
					break;
				}
			}
			if (!bMerged)
				mDirtyRanges.push_back({ start, count });
		}

		~VoxelChunkManager() { clear(); }

		void addPoint(VoxelPoint const& point, IntVector3 const& totalDims)
		{
			IntVector3 chunkPos = { point.pos.x / ChunkSize, point.pos.y / ChunkSize, point.pos.z / ChunkSize };
			auto it = mChunks.find(chunkPos);
			VoxelChunk* chunk = nullptr;
			if (it == mChunks.end())
			{
				chunk = new VoxelChunk();
				chunk->pos = chunkPos;
				mChunks[chunkPos] = chunk;
			}
			else
			{
				chunk = it->second;
			}

			IntVector3 localPos = { point.pos.x % ChunkSize, point.pos.y % ChunkSize, point.pos.z % ChunkSize };
			chunk->addPoint(localPos, point.data, 4); 
			mbDirty = true;
		}

		void removePoint(IntVector3 const& pos, IntVector3 const& totalDims)
		{
			IntVector3 chunkPos = { pos.x / ChunkSize, pos.y / ChunkSize, pos.z / ChunkSize };
			auto itChunk = mChunks.find(chunkPos);
			if (itChunk != mChunks.end())
			{
				IntVector3 localPos = { pos.x % ChunkSize, pos.y % ChunkSize, pos.z % ChunkSize };
				itChunk->second->removePoint(localPos, 4);
				mbDirty = true;
			}
		}

		bool rayCast(Vector3 const& origin, Vector3 const& dir, float tMax, VoxelHit& outHit) const
		{
			outHit.hit = false;
			outHit.t = tMax;

			Vector3 invDir = { 1.0f / dir.x, 1.0f / dir.y, 1.0f / dir.z };
			for (auto const& pair : mChunks)
			{
				VoxelChunk const& chunk = *pair.second;
				IntVector3 minPos = chunk.pos * ChunkSize;
				IntVector3 maxPos = minPos + IntVector3(ChunkSize, ChunkSize, ChunkSize);
				AABBox chunkBox{ Vector3(minPos), Vector3(maxPos) };

				float t[2];
				if (Math::LineAABBTest(origin, dir, chunkBox.min, chunkBox.max, t))
				{
					if (t[0] < outHit.t && t[1] > 0)
					{
						if (RayCastInternal(chunk.root, minPos, ChunkSizeExp, origin, dir, Math::Max(0.0f, t[0]), Math::Min(outHit.t, t[1]), outHit))
						{
							// No extra work here, outHit.voxelPos is set inside
						}
					}
				}
			}
			return outHit.hit;
		}

		static bool RayCastInternal(CPUNode64 const& node, IntVector3 const& pMin, int sizeExp, Vector3 const& origin, Vector3 const& dir, float tMin, float tMax, VoxelHit& outHit)
		{
			int step = (1 << sizeExp);
			bool bHit = false;

			struct ChildHit { int idx; float t; };
			ChildHit hits[64];
			int hitCount = 0;

			if (node.mask)
			{
				uint64 mask = node.mask;
				int index;
				while (FBitUtility::IterateMask<64>(mask, index))
				{
					IntVector3 childMin = pMin;
					childMin.x += (index & 3) * step;
					childMin.z += ((index >> 2) & 3) * step;
					childMin.y += ((index >> 4) & 3) * step;

					Vector3 cMin = Vector3(childMin);
					Vector3 cMax = Vector3(childMin + IntVector3(step, step, step));

					float t[2];
					if (Math::LineAABBTest(origin, dir, cMin, cMax, t))
					{
						if (t[0] < tMax && t[1] > tMin)
						{
							hits[hitCount++] = { index, Math::Max(tMin, t[0]) };
						}
					}
				}

				std::sort(hits, hits + hitCount, [](ChildHit const& a, ChildHit const& b) { return a.t < b.t; });
			}

			for (int i = 0; i < hitCount; ++i)
			{
				int childIdx = hits[i].idx;
				if (hits[i].t >= outHit.t) continue;

				IntVector3 childMin = pMin;
				childMin.x += (childIdx & 3) * step;
				childMin.z += ((childIdx >> 2) & 3) * step;
				childMin.y += ((childIdx >> 4) & 3) * step;

				if (sizeExp == 0)
				{
					outHit.hit = true;
					outHit.t = hits[i].t;
					outHit.pos = origin + outHit.t * dir;
					outHit.voxelPos = outHit.pos;
					outHit.data = node.data[childIdx];
					
					Vector3 center = Vector3(childMin) + Vector3(0.5f, 0.5f, 0.5f);
					Vector3 diff = outHit.pos - center;
					Vector3 absDiff = { Math::Abs(diff.x), Math::Abs(diff.y), Math::Abs(diff.z) };
					if (absDiff.x > absDiff.y && absDiff.x > absDiff.z) outHit.normal = Vector3(diff.x > 0 ? 1.0f : -1.0f, 0, 0);
					else if (absDiff.y > absDiff.z) outHit.normal = Vector3(0, diff.y > 0 ? 1.0f : -1.0f, 0);
					else outHit.normal = Vector3(0, 0, diff.z > 0 ? 1.0f : -1.0f);

					return true;
				}
				else if (node.children)
				{
					if (RayCastInternal(node.children[childIdx], childMin, sizeExp - 2, origin, dir, hits[i].t, tMax, outHit))
					{
						bHit = true;
						tMax = outHit.t;
					}
				}
			}
			return bHit;
		}

		void queryAABB(AABBox const& box, TArray<VoxelPoint>& outPoints) const
		{
			for (auto const& pair : mChunks)
			{
				VoxelChunk const& chunk = *pair.second;
				IntVector3 minPos = chunk.pos * ChunkSize;
				IntVector3 maxPos = minPos + IntVector3(ChunkSize, ChunkSize, ChunkSize);
				AABBox chunkBox{ Vector3(minPos), Vector3(maxPos) };
				if (!chunkBox.isIntersect(box)) continue;

				QueryAABBInternal(chunk.root, minPos, ChunkSizeExp, box, outPoints);
			}
		}

		static void QueryAABBInternal(CPUNode64 const& node, IntVector3 const& pMin, int sizeExp, AABBox const& box, TArray<VoxelPoint>& outPoints)
		{
			int step = (1 << sizeExp);
			for (int i = 0; i < 64; ++i)
			{
				if (node.mask & (1ull << i))
				{
					IntVector3 childMin = pMin;
					childMin.x += (i & 3) * step;
					childMin.z += ((i >> 2) & 3) * step;
					childMin.y += ((i >> 4) & 3) * step;
					AABBox childBox{ Vector3(childMin), Vector3(childMin + IntVector3(step, step, step)) };
					
					if (!childBox.isIntersect(box)) continue;

					if (sizeExp == 0)
					{
						outPoints.push_back({ childMin, node.data[i] });
					}
					else if (node.children)
					{
						QueryAABBInternal(node.children[i], childMin, sizeExp - 2, box, outPoints);
					}
				}
			}
		}

		void querySphere(Vector3 const& center, float radius, TArray<VoxelPoint>& outPoints) const
		{
			float rSq = radius * radius;
			for (auto const& pair : mChunks)
			{
				VoxelChunk const& chunk = *pair.second;
				IntVector3 minPos = chunk.pos * ChunkSize;
				IntVector3 maxPos = minPos + IntVector3(ChunkSize, ChunkSize, ChunkSize);
				AABBox chunkBox{ Vector3(minPos), Vector3(maxPos) };
				
				if (Math::DistanceSqure(chunkBox, center) > rSq) continue;

				QuerySphereInternal(chunk.root, minPos, ChunkSizeExp, center, rSq, outPoints);
			}
		}

		static void QuerySphereInternal(CPUNode64 const& node, IntVector3 const& pMin, int sizeExp, Vector3 const& center, float rSq, TArray<VoxelPoint>& outPoints)
		{
			int step = (1 << sizeExp);
			for (int i = 0; i < 64; ++i)
			{
				if (node.mask & (1ull << i))
				{
					IntVector3 childMin = pMin;
					childMin.x += (i & 3) * step;
					childMin.z += ((i >> 2) & 3) * step;
					childMin.y += ((i >> 4) & 3) * step;
					AABBox childBox{ Vector3(childMin), Vector3(childMin + IntVector3(step, step, step)) };

					if (Math::DistanceSqure(childBox, center) > rSq) continue;

					if (sizeExp == 0)
					{
						outPoints.push_back({ childMin, node.data[i] });
					}
					else if (node.children)
					{
						QuerySphereInternal(node.children[i], childMin, sizeExp - 2, center, rSq, outPoints);
					}
				}
			}
		}

		void addVoxel(IntVector3 const& pos, uint32 data)
		{
			IntVector3 chunkPos = { pos.x / ChunkSize, pos.y / ChunkSize, pos.z / ChunkSize };
			IntVector3 localPos = { pos.x % ChunkSize, pos.y % ChunkSize, pos.z % ChunkSize };
			
			auto it = mChunks.find(chunkPos);
			VoxelChunk* chunk = nullptr;
			if (it == mChunks.end())
			{
				chunk = new VoxelChunk();
				chunk->pos = chunkPos;
				mChunks[chunkPos] = chunk;
				mbRootTreeDirty = true;
			}
			else
			{
				chunk = it->second;
			}

			chunk->addPoint(localPos, data);
			chunk->bIsDirty = true;
			mbDirty = true;
		}

		void removeVoxel(IntVector3 const& pos)
		{
			IntVector3 chunkPos = { pos.x / ChunkSize, pos.y / ChunkSize, pos.z / ChunkSize };
			IntVector3 localPos = { pos.x % ChunkSize, pos.y % ChunkSize, pos.z % ChunkSize };
			
			auto it = mChunks.find(chunkPos);
			if (it != mChunks.end())
			{
				it->second->removePoint(localPos);
				it->second->bIsDirty = true;
				mbDirty = true;
			}
		}

		void initializeChunk(IntVector3 const& chunkPos, TArray<VoxelPoint>& points, IntVector3 const& dims)
		{
			auto it = mChunks.find(chunkPos);
			VoxelChunk* chunk = nullptr;
			if (it == mChunks.end())
			{
				chunk = new VoxelChunk();
				chunk->pos = chunkPos;
				mChunks[chunkPos] = chunk;
			}
			chunk->root.clear();
			for (auto const& p : points)
			{
				IntVector3 localPos = { p.pos.x % ChunkSize, p.pos.y % ChunkSize, p.pos.z % ChunkSize };
				chunk->addPoint(localPos, p.data);
			}
			updateChunk(it->first, chunk, dims);
		}

		void updateChunk(IntVector3 const& chunkPos, IntVector3 const& dims)
		{
			auto it = mChunks.find(chunkPos);
			VoxelChunk* chunk = nullptr;
			if (it == mChunks.end())
			{
				chunk = new VoxelChunk();
				chunk->pos = chunkPos;
				mChunks[chunkPos] = chunk;
			}
			else
			{
				chunk = it->second;
			}
			updateChunk(chunkPos, chunk, dims);
		}

		void updateChunk(IntVector3 const& chunkPos, VoxelChunk* chunk, IntVector3 const& dims)
		{
			TArray<SvtNode64> chunkNodes;
			SvtNode64 rootNode = chunk->generateSvtNodes(chunkNodes);
			int totalCount = (int)chunkNodes.size() + 1;

			bool bAddressChanged = true;
			if (chunk->mAllocation.offset != 0xFFFFFFFF && totalCount <= (int)chunk->mAllocation.maxSize)
			{
				bAddressChanged = false;
			}
			else
			{
				if (chunk->mAllocation.offset != 0xFFFFFFFF)
				{
					mNodePoolAllocator.deallocate(chunk->mAllocation);
				}
				
				BuddyAllocatorBase::Allocation alloc;
				if (mNodePoolAllocator.alloc(totalCount, 1, alloc))
				{
					chunk->mAllocation = alloc;
				}
			}

			uint32 poolOffset = chunk->mAllocation.pos;
			uint32 childrenStartOffset = poolOffset + 1;

			for (auto& node : chunkNodes)
			{
				if ((node.childPtr & (1u << 31)) == 0)
				{
					node.childPtr += childrenStartOffset;
				}
			}

			if ((rootNode.childPtr & (1u << 31)) == 0)
			{
				rootNode.childPtr += childrenStartOffset;
			}

			if (poolOffset + totalCount > mNodePool.size())
			{
				mNodePool.resize(poolOffset + totalCount);
				mbBufferResized = true;
			}

			mNodePool[poolOffset] = rootNode;
			if (!chunkNodes.empty())
			{
				std::copy(chunkNodes.begin(), chunkNodes.end(), mNodePool.begin() + poolOffset + 1);
			}

			addDirtyRange(poolOffset, (uint32)totalCount);

			chunk->nodeOffset = poolOffset;
			chunk->nodeCount = (uint32)totalCount;
			if (bAddressChanged)
				mbRootTreeDirty = true;
			mbDirty = true;
		}

		void clear()
		{
			for (auto pair : mChunks)
			{
				delete pair.second;
			}
			mChunks.clear();
			mNodePool.clear();
			mNodePoolAllocator.deallcateAll();
			mDirtyRanges.clear();
			mbBufferResized = true;
			mbDirty = true;
		}

		TArray<SvtNode64> const& getNodePool() const { return mNodePool; }
		bool isDirty() const { return mbDirty; }
		void clearDirty() { mbDirty = false; }

		TStructuredBuffer<SvtNode64>& getGlobalBuffer() { return mGlobalBuffer; }
		TStructuredBuffer<SvtNode64>& getPoolBuffer() { return mPoolBuffer; }
		int getRootIndex() const { return mRootIndex; }

		int buildRootTree(IntVector3 const& totalDims)
		{
			TArray<VoxelPoint> chunkPoints;
			for (auto const& pair : mChunks)
			{
				chunkPoints.push_back({ pair.first , pair.second->nodeOffset });
			}

			IntVector3 globalDims;
			globalDims.x = Math::Max(1, (totalDims.x + ChunkSize - 1) / ChunkSize);
			globalDims.y = Math::Max(1, (totalDims.y + ChunkSize - 1) / ChunkSize);
			globalDims.z = Math::Max(1, (totalDims.z + ChunkSize - 1) / ChunkSize);

			mGlobalNodes.clear();
			mbDirty = true;
			mRootIndex = FVoxelBuild::Build64Tree(chunkPoints, globalDims, mGlobalNodes, true);
			return mRootIndex;
		}


		void updateGPUResource(RHICommandList& commandList, IntVector3 const& totalDims)
		{
			if (!mbDirty) return;

			for (auto it = mChunks.begin(); it != mChunks.end(); )
			{
				VoxelChunk* chunk = it->second;
				if (chunk->bIsDirty)
				{
					if (chunk->root.mask == 0)
					{
						if (chunk->mAllocation.offset != 0xFFFFFFFF)
						{
							mNodePoolAllocator.deallocate(chunk->mAllocation);
						}
						delete chunk;
						it = mChunks.erase(it);
						mbRootTreeDirty = true;
						continue;
					}

					updateChunk(it->first, chunk, totalDims);
					chunk->bIsDirty = false;
				}
				++it;
			}

			if (mbBufferResized)
			{
				mPoolBuffer.initializeResource(mNodePool, EStructuredBufferType::StaticBuffer);
				mbBufferResized = false;
			}
			else if (mDirtyRanges.size() > 0)
			{
				for (auto const& range : mDirtyRanges)
				{
					RHIUpdateBuffer(commandList, *mPoolBuffer.getRHI(), (int)range.start, (int)range.count, &mNodePool[range.start]);
				}
			}
			mDirtyRanges.clear();

			if (mbRootTreeDirty)
			{
				mRootIndex = buildRootTree(totalDims);
				if (mGlobalNodes.size() > 0)
				{
					mGlobalBuffer.initializeResource(mGlobalNodes, EStructuredBufferType::StaticBuffer);
				}
				mbRootTreeDirty = false;
			}

			mbDirty = false;
		}

		void initializeGPUResources(RHICommandList& commandList)
		{
			if (mGlobalNodes.size() > 0)
			{
				mGlobalBuffer.initializeResource(mGlobalNodes, EStructuredBufferType::StaticBuffer);
			}

			if (mNodePool.size() > 0)
			{
				mPoolBuffer.initializeResource(mNodePool, EStructuredBufferType::StaticBuffer);
				mbBufferResized = false;
			}

			mDirtyRanges.clear();
		}

		void releaseGPUResources()
		{
			mGlobalBuffer.releaseResource();
			mPoolBuffer.releaseResource();
		}

	private:
		TStructuredBuffer<SvtNode64> mGlobalBuffer;
		TStructuredBuffer<SvtNode64> mPoolBuffer;
		TArray<SvtNode64> mGlobalNodes;

		std::map<IntVector3, VoxelChunk*, IntVectorCompare> mChunks;
		TArray<SvtNode64> mNodePool;
		int  mRootIndex = -1;
		bool mbDirty = true;
		bool mbRootTreeDirty = true;
		bool mbBufferResized = true;
		BuddyAllocatorBase mNodePoolAllocator;
		TArray<DataRange> mDirtyRanges;
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
		SHADER_PERMUTATION_TYPE_BOOL(UseChunk, SHADER_PARAM(USE_CHUNK));

		using PermutationDomain = TShaderPermutationDomain<UseOctTree, Use64Tree, UseChunk>;

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
			option.addDefine("VOXEL_CHUNK_SIZE", domain.get< UseChunk >() ? ChunkSize : 0);
			option.addDefine("VOXEL_CHUNK_DEPTH", domain.get< UseChunk >() ? ChunkSizeExp / 2 + 1 : 0);
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
			BIND_SHADER_PARAM(parameterMap, VoxelTreeDepth);
			BIND_SHADER_PARAM(parameterMap, VoxelColor);
			BIND_SHADER_PARAM(parameterMap, VoxelData);
			BIND_SHADER_PARAM(parameterMap, OctTreeData);
			BIND_SHADER_PARAM(parameterMap, Svt64Data);
			BIND_SHADER_PARAM(parameterMap, Svt64ChunkData);
			BIND_SHADER_PARAM(parameterMap, RootNodeIndex);

			// DDGI
			BIND_SHADER_PARAM(parameterMap, DDGIVolumeMin);
			BIND_SHADER_PARAM(parameterMap, DDGIProbeSpacing);
			BIND_SHADER_PARAM(parameterMap, DDGIProbeCount);
			BIND_SHADER_PARAM(parameterMap, DDGIIrradianceTexture);
			BIND_SHADER_PARAM(parameterMap, DDGIDistanceTexture);
			BIND_SHADER_PARAM(parameterMap, DDGISampler);
			
			BIND_SHADER_PARAM(parameterMap, PointLightPositions);
			BIND_SHADER_PARAM(parameterMap, PointLightColors);
		}

		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(WorldToLocal);
		DEFINE_SHADER_PARAM(BBoxMin);
		DEFINE_SHADER_PARAM(BBoxMax);
		DEFINE_SHADER_PARAM(VoxelDims);
		DEFINE_SHADER_PARAM(VoxelTreeDepth);
		DEFINE_SHADER_PARAM(VoxelColor);
		DEFINE_SHADER_PARAM(VoxelData);
		DEFINE_SHADER_PARAM(OctTreeData);
		DEFINE_SHADER_PARAM(Svt64Data);
		DEFINE_SHADER_PARAM(Svt64ChunkData);
		DEFINE_SHADER_PARAM(RootNodeIndex);

		DEFINE_SHADER_PARAM(DDGIVolumeMin);
		DEFINE_SHADER_PARAM(DDGIProbeSpacing);
		DEFINE_SHADER_PARAM(DDGIProbeCount);
		DEFINE_SHADER_PARAM(DDGIIrradianceTexture);
		DEFINE_SHADER_PARAM(DDGIDistanceTexture);
		DEFINE_SHADER_PARAM(DDGISampler);
		
		DEFINE_SHADER_PARAM(PointLightPositions);
		DEFINE_SHADER_PARAM(PointLightColors);
	};

	IMPLEMENT_SHADER_PROGRAM(VoxelRayMarchProgram);

	class DDGIVoxelTraceShader : public GlobalShader
	{
		DECLARE_SHADER(DDGIVoxelTraceShader, Global);
	public:
		SHADER_PERMUTATION_TYPE_BOOL(UseOctTree, SHADER_PARAM(USE_OCTREE));
		SHADER_PERMUTATION_TYPE_BOOL(Use64Tree, SHADER_PARAM(USE_64TREE));
		SHADER_PERMUTATION_TYPE_BOOL(UseChunk, SHADER_PARAM(USE_CHUNK));

		using PermutationDomain = TShaderPermutationDomain<UseOctTree, Use64Tree, UseChunk >;

		static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
		{
			option.addDefine("VOXEL_CHUNK_SIZE", domain.get< UseChunk >() ? ChunkSize : 0);
			option.addDefine("VOXEL_CHUNK_DEPTH", domain.get< UseChunk >() ? ChunkSizeExp / 2 + 1 : 0);
		}

		static char const* GetShaderFileName() { return "Shader/Game/DDGIVoxelTrace"; }
		void bindParameters(ShaderParameterMap const& parameterMap)
		{
			BIND_SHADER_PARAM(parameterMap, DDGIVolumeMin);
			BIND_SHADER_PARAM(parameterMap, DDGIProbeSpacing);
			BIND_SHADER_PARAM(parameterMap, DDGIProbeCount);
			BIND_SHADER_PARAM(parameterMap, DDGIRayCountPerProbe);
			BIND_SHADER_PARAM(parameterMap, DDGIRandomRotation);
			BIND_SHADER_PARAM(parameterMap, RayHitBuffer);
			
			// From VoxelRayTraceCommon
			BIND_SHADER_PARAM(parameterMap, VoxelDims);
			BIND_SHADER_PARAM(parameterMap, VoxelTreeDepth);
			BIND_SHADER_PARAM(parameterMap, RootNodeIndex);
			BIND_SHADER_PARAM(parameterMap, VoxelData);
			BIND_SHADER_PARAM(parameterMap, OctTreeData);
			BIND_SHADER_PARAM(parameterMap, Svt64Data);
			BIND_SHADER_PARAM(parameterMap, Svt64ChunkData);
			BIND_SHADER_PARAM(parameterMap, WorldToLocal);
			BIND_SHADER_PARAM(parameterMap, LocalToWorld);
			BIND_SHADER_PARAM(parameterMap, DDGIIrradianceTexture);
			BIND_SHADER_PARAM(parameterMap, DDGIDistanceTexture);
			BIND_SHADER_PARAM(parameterMap, DDGISampler);
			
			BIND_SHADER_PARAM(parameterMap, PointLightPositions);
			BIND_SHADER_PARAM(parameterMap, PointLightColors);
		}

		DEFINE_SHADER_PARAM(DDGIVolumeMin);
		DEFINE_SHADER_PARAM(DDGIProbeSpacing);
		DEFINE_SHADER_PARAM(DDGIProbeCount);
		DEFINE_SHADER_PARAM(DDGIRayCountPerProbe);
		DEFINE_SHADER_PARAM(DDGIRandomRotation);
		DEFINE_SHADER_PARAM(RayHitBuffer);
		
		DEFINE_SHADER_PARAM(VoxelDims);
		DEFINE_SHADER_PARAM(VoxelTreeDepth);
		DEFINE_SHADER_PARAM(RootNodeIndex);
		DEFINE_SHADER_PARAM(VoxelData);
		DEFINE_SHADER_PARAM(OctTreeData);
		DEFINE_SHADER_PARAM(Svt64Data);
		DEFINE_SHADER_PARAM(Svt64ChunkData);
		DEFINE_SHADER_PARAM(LocalToWorld);
		DEFINE_SHADER_PARAM(WorldToLocal);
		
		DEFINE_SHADER_PARAM(DDGIIrradianceTexture);
		DEFINE_SHADER_PARAM(DDGIDistanceTexture);
		DEFINE_SHADER_PARAM(DDGISampler);
		
		DEFINE_SHADER_PARAM(PointLightPositions);
		DEFINE_SHADER_PARAM(PointLightColors);
	};
	IMPLEMENT_SHADER(DDGIVoxelTraceShader, EShader::Compute, SHADER_ENTRY(MainCS));
	

	static bool BuildMeshVoxelData(char const* meshPath, Matrix4 const& meshXForm, int maxVoxelLen,
		Mesh& outMesh, TArray<VoxelPoint>& outPoints,
		VoxelRawData& outRawData, Math::TAABBox<Vector3>& outBBox,
		TArray<OctTreeNode>& outOctTreeNodes, int& outOctRooIndex,
		TArray<SvtNode64>& outSvtNodes, int& outSvtRootIndex)
	{
		DataCacheKey key;
		key.typeName = "MESH_VOXEL_DATA";
		key.version = "B7E2A91C-4F3B-4A71-8D92-2C5E1A8F6D34";
		key.keySuffix.addFileAttribute(meshPath);
		key.keySuffix.add(meshXForm, maxVoxelLen);

		auto VoxelLoad = [&](IStreamSerializer& serializer) -> bool
		{
			serializer >> outBBox.min >> outBBox.max;
			serializer >> outPoints >> outRawData;
			serializer >> outOctTreeNodes >> outOctRooIndex;
			serializer >> outSvtNodes >> outSvtRootIndex;
			return true;
		};

		auto VoxelSave = [&](IStreamSerializer& serializer) -> bool
		{
			serializer << outBBox.min << outBBox.max;
			serializer << outPoints << outRawData;
			serializer << outOctTreeNodes << outOctRooIndex;
			serializer << outSvtNodes << outSvtRootIndex;
			return true;
		};

		MeshImportData importData;
		if (!LoadMeshDataFromFile(importData, meshPath))
		{
			return false;
		}

		outMesh.mInputLayoutDesc = importData.desc;
		outMesh.mType = EPrimitive::TriangleList;
		outMesh.createRHIResource(importData);

		if (::Global::DataCache().loadDelegate(key, VoxelLoad))
		{
			return true;
		}

		outBBox = importData.getBoundBox(meshXForm);
		outBBox.expand(Vector3(0.01, 0.01, 0.01));

		Vector3 boundSize = outBBox.getSize();
		float maxSideSize = Math::Max(boundSize.x, Math::Max(boundSize.y, boundSize.z));
		float voxelSizeVal = maxSideSize / float(maxVoxelLen);
		Vector3 size = boundSize / voxelSizeVal;
		outRawData.dims.x = Math::CeilToInt(size.x);
		outRawData.dims.y = Math::CeilToInt(size.y);
		outRawData.dims.z = Math::CeilToInt(size.z);

		outPoints.clear();
		FVoxelBuild::MeshSurface(outPoints, outRawData.dims, outBBox, importData, importData.desc, meshXForm);

		if (maxVoxelLen <= 1024)
		{
			outRawData.initData(outPoints);
		}

		outOctTreeNodes.clear();
		outOctRooIndex = FVoxelBuild::BuildOctTree(outPoints, outRawData.dims, outOctTreeNodes);

		outSvtNodes.clear();
		outSvtRootIndex = FVoxelBuild::Build64Tree(outPoints, outRawData.dims, outSvtNodes);

		LogMsg("Voxel Dims: %d %d %d", outRawData.dims.x, outRawData.dims.y, outRawData.dims.z);
		LogMsg("BBox Min: %f %f %f", outBBox.min.x, outBBox.min.y, outBBox.min.z);
		LogMsg("BBox Max: %f %f %f", outBBox.max.x, outBBox.max.y, outBBox.max.z);

		if (!::Global::DataCache().saveDelegate(key, VoxelSave))
		{

		}

		return true;
	}


	class VoxelRenderingStage : public TestRenderStageBase
	{
		using BaseClass = TestRenderStageBase;
	public:
		VoxelRenderingStage() {}

		enum
		{
			UI_RenderMethod = BaseClass::NEXT_UI_ID,
			UI_DDGI_Update,
			UI_DDGI_DebugDraw,
			UI_DDGI_RayCount,

			NEXT_UI_ID,
		};

		ERenderMethod mRenderMethod = ERenderMethod::Instanced;
		bool bShowModel;
		TArray<VoxelPoint> mPoints;

		Mesh mModelMesh;
		Matrix4 mModelXForm;

		// DDGI Parameters
		IntVector3 mProbeCount = IntVector3(32, 32, 16);
		float mProbeSpacing = 2.0f;
		int mRayCountPerProbe = 64;
		bool bUpdateDDGI = true;
		bool bDebugDrawProbes = false;
		float mTotalTime = 0.0f;

		TStructuredBuffer<DDGIProbeVisualizeParams> mProbeVisBuffer;
		RHITexture2DRef mIrradianceTexture;
		RHITexture2DRef mDistanceTexture;
		RHIBufferRef mRayHitBuffer;
		int          mRayHitBufferCount = 0; // Track allocated size for dynamic adjustment
		Vector3      mProbeVolumeMin;

		bool onInit() override
		{
			if (!BaseClass::onInit())
				return false;

#if 1
#if 0
			mModelXForm = Matrix4::Scale(0.1) * Matrix4::Rotate(Vector3(1, 0, 0), Math::DegToRad(90));
			BuildMeshVoxelData("Model/Sponza.obj", mModelXForm, 2048, mModelMesh, mPoints, mRawData, mBBox, mOctTreeNodes, mRootNodeIndex, mSvtNodes, mSvtRootIndex);
#else
			mModelXForm = Matrix4::Rotate(Vector3(1, 0, 0), Math::DegToRad(90));
			BuildMeshVoxelData("Model/BistroExterior.fbx", mModelXForm, 2048 * 2, mModelMesh, mPoints, mRawData, mBBox, mOctTreeNodes, mRootNodeIndex, mSvtNodes, mSvtRootIndex);
#endif

			Vector3 boundSize = mBBox.getSize();
			mProbeSpacing = Math::Max(boundSize.x / mProbeCount.x, Math::Max(boundSize.y / mProbeCount.y, boundSize.z / mProbeCount.z));
			mProbeVolumeMin = mBBox.min;
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

			if (mPoints.size() > 0)
			{
				std::map<IntVector3, TArray<VoxelPoint>, IntVectorCompare> chunkedPoints;
				for (auto const& p : mPoints)
				{
					IntVector3 chunkPos = p.pos / ChunkSize;
					VoxelPoint localP = p;
					localP.pos = p.pos - chunkPos * ChunkSize;
					chunkedPoints[chunkPos].push_back(localP);
				}

				mChunkManager.clear();
				for (auto& pair : chunkedPoints)
				{
					mChunkManager.initializeChunk(pair.first, pair.second, mRawData.dims);
				}

				mChunkManager.buildRootTree(mRawData.dims);
			}



			::Global::GUI().cleanupWidget();

			auto frame = WidgetUtility::CreateDevFrame();
			auto choice = frame->addChoice("DebugDisplay Mode", UI_RenderMethod);
			char const* MethodTextList[] =
			{
				"Instanced",
				"RawDDA",
				"OctTree",
				"64-Tree",
				"64-Tree Chunk",
			};
			static_assert(ARRAY_SIZE(MethodTextList) == (int)ERenderMethod::COUNT);
			for (int i = 0; i < (int)ERenderMethod::COUNT; ++i)
			{
				choice->addItem(MethodTextList[i]);
			}
			choice->setSelection((int)mRenderMethod);
			frame->addCheckBox("Show Model", bShowModel);
			
			frame->addCheckBox("Update DDGI", bUpdateDDGI);
			frame->addCheckBox("Debug Draw Probes", bDebugDrawProbes);
			FWidgetProperty::Bind(frame->addSlider("Ray Count"), mRayCountPerProbe, 32, 256);

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
			systemConfigs.bufferCount = 3;
			systemConfigs.bVSyncEnable = false;
		}


		DDGIProbeManager mProbeManager;

		virtual bool setupRenderResource(ERenderSystem systemName) override
		{
			VERIFY_RETURN_FALSE(BaseClass::setupRenderResource(systemName));
			VERIFY_RETURN_FALSE(createSimpleMesh());
			VERIFY_RETURN_FALSE(loadCommonShader());
			VERIFY_RETURN_FALSE(mProgVoxel = ShaderManager::Get().getGlobalShaderT<VoxelProgram>(true));


			FMeshBuild::CubeOffset(mVoxelBoundMesh, 0.5, Vector3(0.5,0.5,0.5));
			mVoxelMesh.setupMesh(getMesh(SimpleMeshId::Box));
			
			if (!mRawData.data.empty())
			{
				TArray<uint32> bufferData;
				bufferData.resize(mRawData.data.size());
				for (size_t i = 0; i < mRawData.data.size(); ++i)
					bufferData[i] = mRawData.data[i];
				mVoxelBuffer.initializeResource(bufferData, EStructuredBufferType::StaticBuffer);
			}

			mChunkManager.initializeGPUResources(RHICommandList::GetImmediateList());

			updateVoxelMesh();

			if (mOctTreeNodes.size() > 0 && mOctTreeNodes.size() < size_t(1024 * 1024 * 1024) / sizeof(OctTreeNode))
			{
				mOctTreeBuffer.initializeResource((uint32)mOctTreeNodes.size(), EStructuredBufferType::StaticBuffer, BCF_None, mOctTreeNodes.data());
			}
			if (mSvtNodes.size() > 0)
			{
				mSvtBuffer.initializeResource(mSvtNodes, EStructuredBufferType::StaticBuffer);
			}

			// DDGI Resource Allocation
			{

				DDGIConfig config;
				config.gridCount = mProbeCount;
				config.probeSpacing = mProbeSpacing;
				config.rayCountPerProbe = mRayCountPerProbe;

				mProbeManager.initializeRHI(config);
				mProbeManager.setVolumeMin(mProbeVolumeMin);
				
				int probeTotal = mProbeCount.x * mProbeCount.y * mProbeCount.z;
				// Irradiance: 8x8 texels per probe
				mIrradianceTexture = mProbeManager.getIrradianceAtlas();
				
				// Distance: 8x8 texels per probe (moments: dist & dist^2)
				mDistanceTexture = mProbeManager.getDistanceAtlas();
				
				GTextureShowManager.registerTexture("Irradiance", mIrradianceTexture);
				GTextureShowManager.registerTexture("Distance", mDistanceTexture);

				// RayHitBuffer: float4 per ray (RGB + Dist)
				mRayHitBufferCount = probeTotal * mRayCountPerProbe;
				mRayHitBuffer = mProbeManager.getRayHitBuffer();
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
			mChunkManager.releaseGPUResources();
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
			mTotalTime += deltaTime;
		}

		void onRender(float dFrame) override
		{
			RHICommandList& commandList = RHICommandList::GetImmediateList();
			mChunkManager.updateGPUResource(commandList, mRawData.dims);
			initializeRenderState();

			Vector4 lightPositions[4] = 
			{
				Vector4(0, 0, 20, 1),
				Vector4(50, 20, 30, 1),
				Vector4(20, 50, 15, 1),
				Vector4(40, 40,  60, 1)
			};
			Vector4 lightColors[4] =
			{
				Vector4(20.0, 0.0, 0.0, 1), // red
				Vector4(2.0, 20.0, 2.0, 1), // green
				Vector4(2.0, 2.0, 20.0, 1), // blue
				Vector4(10.0, 10.0, 10.0, 1)  // white
			};


			Matrix4 boxLocalToWorld = Matrix4::Scale(mBBox.getSize()) * Matrix4::Translate(mBBox.min);
			Matrix4 boxWorldToLocal;
			float det;
			boxLocalToWorld.inverse(boxWorldToLocal, det);


			bool bUse64Tree = mRenderMethod == ERenderMethod::S64Tree || mRenderMethod == ERenderMethod::S64TreeChunk;

			if (bUpdateDDGI && mRenderMethod != ERenderMethod::Instanced)
			{
				GPU_PROFILE("UpdateDDGI");

				// Better rotation: sweep all axes over time to ensure full spherical coverage
				Matrix4 ddgiRotation = Matrix4::Rotate(Vector3(0, 0, 1), mTotalTime * 0.131f) * 
				                       Matrix4::Rotate(Vector3(0, 1, 0), mTotalTime * 0.073f) *
				                       Matrix4::Rotate(Vector3(1, 0, 0), mTotalTime * 0.041f);
				mProbeManager.setRandomRotation(ddgiRotation);

				// Execute DDGIVoxelTraceShader
				{
					mProbeManager.prepareForGpuUpdate(commandList);

					GPU_PROFILE("Probe Trace");

					DDGIVoxelTraceShader::PermutationDomain permutationVector;
					permutationVector.set<DDGIVoxelTraceShader::UseOctTree>(mRenderMethod == ERenderMethod::OctTree);
					permutationVector.set<DDGIVoxelTraceShader::Use64Tree>(bUse64Tree);
					permutationVector.set<DDGIVoxelTraceShader::UseChunk>(mRenderMethod == ERenderMethod::S64TreeChunk);
					DDGIVoxelTraceShader* shaderTrace = ShaderManager::Get().getGlobalShaderT<DDGIVoxelTraceShader>(permutationVector);
					RHISetComputeShader(commandList, shaderTrace->getRHI());

					SET_SHADER_PARAM(commandList, *shaderTrace, DDGIVolumeMin, mProbeVolumeMin);
					SET_SHADER_PARAM(commandList, *shaderTrace, DDGIProbeSpacing, mProbeSpacing);
					SET_SHADER_PARAM(commandList, *shaderTrace, DDGIProbeCount, mProbeCount);
					SET_SHADER_PARAM(commandList, *shaderTrace, DDGIRayCountPerProbe, mRayCountPerProbe);
					SET_SHADER_PARAM(commandList, *shaderTrace, DDGIRandomRotation, ddgiRotation);

					// Dynamic Buffer Recreation for RayHitBuffer
					int probeTotal = mProbeCount.x * mProbeCount.y * mProbeCount.z;
					int requiredCount = probeTotal * mRayCountPerProbe;
					if (requiredCount > mRayHitBufferCount)
					{
						mRayHitBufferCount = requiredCount;
						mRayHitBuffer = RHICreateBuffer(sizeof(float) * 4, mRayHitBufferCount, BCF_Structured | BCF_CreateUAV);
					}

					SET_SHADER_PARAM(commandList, *shaderTrace, VoxelDims, mRawData.dims);
					SET_SHADER_PARAM(commandList, *shaderTrace, RootNodeIndex, (mRenderMethod == ERenderMethod::S64TreeChunk) ? mChunkManager.getRootIndex() : (bUse64Tree ? mSvtRootIndex : mRootNodeIndex));

					SET_SHADER_PARAM(commandList, *shaderTrace, WorldToLocal, boxWorldToLocal);
					SET_SHADER_PARAM(commandList, *shaderTrace, LocalToWorld, boxLocalToWorld);
					

					shaderTrace->setParam(commandList, shaderTrace->mParamPointLightPositions, lightPositions, ARRAY_SIZE(lightPositions));
					shaderTrace->setParam(commandList, shaderTrace->mParamPointLightColors, lightColors, ARRAY_SIZE(lightColors));
					
					shaderTrace->setTexture(commandList, shaderTrace->mParamDDGIIrradianceTexture, mIrradianceTexture);
					shaderTrace->setTexture(commandList, shaderTrace->mParamDDGIDistanceTexture, mDistanceTexture);
					shaderTrace->setSampler(commandList, shaderTrace->mParamDDGISampler, TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());

					shaderTrace->setStorageBuffer(commandList, shaderTrace->mParamRayHitBuffer, *mRayHitBuffer, EAccessOp::WriteOnly);
					{
						uint32 maxDim = std::max({ mRawData.dims.x, mRawData.dims.y, mRawData.dims.z, 1 });
						uint32 treeRes = 1;
						int treeDepth = 1;
						while (treeRes < maxDim) { treeRes *= 4; ++treeDepth; }
						SET_SHADER_PARAM(commandList, *shaderTrace, VoxelTreeDepth, treeDepth);
					}

					if (mRenderMethod == ERenderMethod::OctTree)
					{
						if (mOctTreeBuffer.isValid())
						{
							shaderTrace->setStorageBuffer(commandList, shaderTrace->SHADER_MEMBER_PARAM(OctTreeData), *mOctTreeBuffer.getRHI(), EAccessOp::ReadOnly);
						}
					}
					else if (mRenderMethod == ERenderMethod::S64TreeChunk)
					{
						if (mChunkManager.getGlobalBuffer().isValid() && mChunkManager.getPoolBuffer().isValid())
						{
							shaderTrace->setStorageBuffer(commandList, shaderTrace->SHADER_MEMBER_PARAM(Svt64ChunkData), *mChunkManager.getGlobalBuffer().getRHI(), EAccessOp::ReadOnly);
							shaderTrace->setStorageBuffer(commandList, shaderTrace->SHADER_MEMBER_PARAM(Svt64Data), *mChunkManager.getPoolBuffer().getRHI(), EAccessOp::ReadOnly);
						}
					}
					else if (mRenderMethod == ERenderMethod::S64Tree)
					{
						if (mSvtBuffer.isValid())
						{
							shaderTrace->setStorageBuffer(commandList, shaderTrace->SHADER_MEMBER_PARAM(Svt64Data), *mSvtBuffer.getRHI(), EAccessOp::ReadOnly);
						}
					}
					else
					{
						if (mVoxelBuffer.isValid())
						{
							shaderTrace->setStorageBuffer(commandList, shaderTrace->SHADER_MEMBER_PARAM(Svt64Data), *mSvtBuffer.getRHI(), EAccessOp::ReadOnly);
							shaderTrace->setStorageBuffer(commandList, shaderTrace->SHADER_MEMBER_PARAM(VoxelData), *mVoxelBuffer.getRHI(), EAccessOp::ReadOnly);
						}
					}

					RHIDispatchCompute(commandList, (mProbeCount.x * mProbeCount.y * mProbeCount.z * mRayCountPerProbe + 31) / 32, 1, 1);
				}

				mProbeManager.finalizeGpuUpdate(commandList);
			}

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

				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::Front>::GetRHI());

				VoxelRayMarchProgram::PermutationDomain permutationVector;
				permutationVector.set<VoxelRayMarchProgram::UseOctTree>(mRenderMethod == ERenderMethod::OctTree);
				permutationVector.set<VoxelRayMarchProgram::Use64Tree>(bUse64Tree);
				permutationVector.set<VoxelRayMarchProgram::UseChunk>(mRenderMethod == ERenderMethod::S64TreeChunk);
				VoxelRayMarchProgram* progRayMarch = ShaderManager::Get().getGlobalShaderT<VoxelRayMarchProgram>(permutationVector);

				RHISetShaderProgram(commandList, progRayMarch->getRHI());
				mView.setupShader(commandList, *progRayMarch);
				SET_SHADER_PARAM(commandList, *progRayMarch, LocalToWorld, boxLocalToWorld);
				SET_SHADER_PARAM(commandList, *progRayMarch, WorldToLocal, boxWorldToLocal);
				SET_SHADER_PARAM(commandList, *progRayMarch, BBoxMin, mBBox.min);
				SET_SHADER_PARAM(commandList, *progRayMarch, BBoxMax, mBBox.max);
				SET_SHADER_PARAM(commandList, *progRayMarch, VoxelDims, mRawData.dims);
				{
					uint32 maxDim = std::max({ mRawData.dims.x, mRawData.dims.y, mRawData.dims.z, 1 });
					uint32 treeRes = 1;
					int treeDepth = 1;
					while (treeRes < maxDim) { treeRes *= 4; ++treeDepth; }
					SET_SHADER_PARAM(commandList, *progRayMarch, VoxelTreeDepth, treeDepth);
				}
				SET_SHADER_PARAM(commandList, *progRayMarch, VoxelColor, LinearColor(0, 1, 0, 1));
				SET_SHADER_PARAM(commandList, *progRayMarch, RootNodeIndex, (mRenderMethod == ERenderMethod::S64TreeChunk) ? mChunkManager.getRootIndex() : (bUse64Tree ? mSvtRootIndex : mRootNodeIndex));
				
				// DDGI Parameters for Pixel Shader
				SET_SHADER_PARAM(commandList, *progRayMarch, DDGIVolumeMin, mProbeVolumeMin);
				SET_SHADER_PARAM(commandList, *progRayMarch, DDGIProbeSpacing, mProbeSpacing);
				SET_SHADER_PARAM(commandList, *progRayMarch, DDGIProbeCount, mProbeCount);
				SET_SHADER_TEXTURE(commandList, *progRayMarch, DDGIIrradianceTexture, *mIrradianceTexture);
				SET_SHADER_TEXTURE(commandList, *progRayMarch, DDGIDistanceTexture, *mDistanceTexture);
				progRayMarch->setSampler(commandList, progRayMarch->mParamDDGISampler, TStaticSamplerState<ESampler::Bilinear, ESampler::Clamp, ESampler::Clamp, ESampler::Clamp>::GetRHI());
				SET_SHADER_PARAM(commandList, *progRayMarch, DebugDrawProbes, bDebugDrawProbes ? 1 : 0);
				
				progRayMarch->setParam(commandList, progRayMarch->mParamPointLightPositions, lightPositions, ARRAY_SIZE(lightPositions));
				progRayMarch->setParam(commandList, progRayMarch->mParamPointLightColors, lightColors, ARRAY_SIZE(lightColors));
				if (mRenderMethod == ERenderMethod::OctTree)
				{
					if (mOctTreeBuffer.isValid())
					{
						progRayMarch->setStorageBuffer(commandList, progRayMarch->SHADER_MEMBER_PARAM(OctTreeData), *mOctTreeBuffer.getRHI(), EAccessOp::ReadOnly);
					}	
				}
				else if (mRenderMethod == ERenderMethod::S64TreeChunk)
				{
					if (mChunkManager.getGlobalBuffer().isValid() && mChunkManager.getPoolBuffer().isValid())
					{
						progRayMarch->setStorageBuffer(commandList, progRayMarch->SHADER_MEMBER_PARAM(Svt64ChunkData), *mChunkManager.getGlobalBuffer().getRHI(), EAccessOp::ReadOnly);
						progRayMarch->setStorageBuffer(commandList, progRayMarch->SHADER_MEMBER_PARAM(Svt64Data), *mChunkManager.getPoolBuffer().getRHI(), EAccessOp::ReadOnly);
					}
				}
				else if (mRenderMethod == ERenderMethod::S64Tree)
				{
					if (mSvtBuffer.isValid())
					{
						progRayMarch->setStorageBuffer(commandList, progRayMarch->SHADER_MEMBER_PARAM(Svt64Data), *mSvtBuffer.getRHI(), EAccessOp::ReadOnly);
					}
				}
				else
				{
					if (mVoxelBuffer.isValid())
					{
						progRayMarch->setStorageBuffer(commandList, progRayMarch->SHADER_MEMBER_PARAM(Svt64Data), *mSvtBuffer.getRHI(), EAccessOp::ReadOnly);
						progRayMarch->setStorageBuffer(commandList, progRayMarch->SHADER_MEMBER_PARAM(VoxelData), *mVoxelBuffer.getRHI(), EAccessOp::ReadOnly);
					}
				}

				mVoxelBoundMesh.draw(commandList);
			}

			if (bDebugDrawProbes)
			{
				GPU_PROFILE("DDGI Probe Visualize");
				mProbeManager.renderDebug(commandList, mView, 0.3f );
			}

			if (bShowModel)
			{
				RHISetDepthStencilState(commandList, StaticDepthDisableState::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None, EFillMode::Wireframe>::GetRHI());
				RHISetFixedShaderPipelineState(commandList, mModelXForm * mView.worldToClipRHI);
				mModelMesh.draw(commandList, LinearColor(1, 0, 0));
			}


			{

				RHISetBlendState(commandList, TStaticBlendState<>::GetRHI());
				RHISetRasterizerState(commandList, TStaticRasterizerState<ECullMode::None>::GetRHI());
				RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
				RHISetShaderProgram(commandList, mProgSphere->getRHI());
				mView.setupShader(commandList, *mProgSphere);


				float radius = 0.85f;
				for (int i = 0; i < ARRAY_SIZE(lightPositions); ++i)
				{
					mProgSphere->setParameters(commandList, lightPositions[i].xyz(), radius, lightColors[i].xyz());
					mSimpleMeshs[SimpleMeshId::SpherePlane].draw(commandList);
				}
			}

			RHISetDepthStencilState(commandList, TStaticDepthStencilState<>::GetRHI());
			RHISetRasterizerState(commandList, TStaticRasterizerState<>::GetRHI());
			RHISetFixedShaderPipelineState(commandList, mView.worldToClipRHI);
			DrawUtility::AixsLine(commandList, 100);

			if (mLastMouseHit.hit)
			{
				Vector3 v[] = { mLastMouseHit.pos, Vector3(1,1,0), mLastMouseHit.pos + mLastMouseHit.normal * 2.0f, Vector3(1,1,0) };
				TRenderRT< RTVF_XYZ_C >::Draw(commandList, EPrimitive::LineList, v, 2, 2 * sizeof(Vector3));
			}
		}

		MsgReply onMouse(MouseMsg const& msg) override
		{
			if (msg.onMoving())
			{
				Vector2 pos = msg.getPos();

				Vector3 lookPos = mCamera.getPos();
				Vector3 dir = mCamera.getViewDir();

				Matrix4 boxLocalToWorld = Matrix4::Scale(mBBox.getSize()) * Matrix4::Translate(mBBox.min);
				Matrix4 boxWorldToLocal;
				float det;
				boxLocalToWorld.inverse(boxWorldToLocal, det);

				// Transfrom world ray to [0, 1] local space
				Vector3 localOrigin = TransformPosition(lookPos, boxWorldToLocal);
				Vector3 localDir = TransformVector(dir, boxWorldToLocal);

				// Scale to Voxel space [0, Dims]
				localOrigin = localOrigin.mul(Vector3(mRawData.dims));
				localDir = localDir.mul(Vector3(mRawData.dims));

				if (mChunkManager.rayCast(localOrigin, localDir, 2000.0f, mLastMouseHit))
				{
					// Div back to [0, 1] and then to world space
					Vector3 hitPosLocal = mLastMouseHit.voxelPos.div(Vector3(mRawData.dims));
					mLastMouseHit.pos = TransformPosition(hitPosLocal, boxLocalToWorld);
					mLastMouseHit.normal = TransformVector(mLastMouseHit.normal, boxLocalToWorld);
					mLastMouseHit.normal.normalize();
				}
			}

			if (msg.onRightDown() && mLastMouseHit.hit)
			{
				IntVector3 center = IntVector3(Math::Floor(mLastMouseHit.voxelPos.x), Math::Floor(mLastMouseHit.voxelPos.y), Math::Floor(mLastMouseHit.voxelPos.z));
				for (int x = -2; x < 2; ++x)
				for (int y = -2; y < 2; ++y)
				for (int z = -2; z < 2; ++z)
				{
					IntVector3 p = center + IntVector3(x, y, z);
					if (p.x >= 0 && p.x < mRawData.dims.x && p.y >= 0 && p.y < mRawData.dims.y && p.z >= 0 && p.z < mRawData.dims.z)
					{
						mChunkManager.addVoxel(p, 1);
					}
				}
			}
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

		VoxelChunkManager mChunkManager;
		VoxelHit mLastMouseHit;

		Mesh mVoxelBoundMesh;

		bool bRayMarchMode = false;
	};

	REGISTER_STAGE_ENTRY("Voxel Rendering", VoxelRenderingStage, EExecGroup::FeatureDev);

}