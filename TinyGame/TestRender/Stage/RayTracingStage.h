#include "Stage/TestRenderStageBase.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "Renderer/MeshImportor.h"
#include "Renderer/IBLResource.h"
#include "Math/GeometryPrimitive.h"
#include "PropertySet.h"
#include "ProfileSystem.h"
#include "ReflectionCollect.h"

#include "Editor.h"
#include <memory>

using namespace Render;

enum class EDebugDsiplayMode
{
	None,
	BoundingBox,
	Triangle,
	Mix,
	HitNoraml,
	HitPos,
	BaseColor,
	EmissiveColor,
	Roughness,
	Specular,
	COUNT,
};


class RayTracingPS : public GlobalShader
{
	using BaseClass = GlobalShader;
	DECLARE_SHADER(RayTracingPS, Global);
public:
	SHADER_PERMUTATION_TYPE_BOOL(UseDebugDisplay, SHADER_PARAM(USE_DEBUG_DISPLAY));
	SHADER_PERMUTATION_TYPE_BOOL(UseSplitAccumulate, SHADER_PARAM(USE_SPLIT_ACCUMULATE));
	SHADER_PERMUTATION_TYPE_BOOL(UseMIS, SHADER_PARAM(USE_MIS));
	SHADER_PERMUTATION_TYPE_BOOL(UseBVH4, SHADER_PARAM(USE_BVH4));

	using PermutationDomain = TShaderPermutationDomain<UseDebugDisplay, UseSplitAccumulate, UseMIS, UseBVH4>;

	static char const* GetShaderFileName()
	{
		return "Shader/Game/RayTracing";
	}

	static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
	{

	}

	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, DisplayMode);
		BIND_SHADER_PARAM(parameterMap, BoundBoxWarningCount);
		BIND_SHADER_PARAM(parameterMap, TriangleWarningCount);
		BIND_SHADER_PARAM(parameterMap, NumObjects);
		BIND_SHADER_PARAM(parameterMap, NumEmittingObjects);
		BIND_SHADER_PARAM(parameterMap, BlendFactor);
		BIND_TEXTURE_PARAM(parameterMap, HistoryTexture);
		BIND_TEXTURE_PARAM(parameterMap, SkyTexture);
		mIBLParams.bindParameters(parameterMap);
	}

	DEFINE_SHADER_PARAM(DisplayMode);
	DEFINE_SHADER_PARAM(BoundBoxWarningCount);
	DEFINE_SHADER_PARAM(TriangleWarningCount);
	DEFINE_SHADER_PARAM(NumObjects);
	DEFINE_SHADER_PARAM(NumEmittingObjects);
	DEFINE_SHADER_PARAM(BlendFactor);
	DEFINE_TEXTURE_PARAM(HistoryTexture);
	DEFINE_TEXTURE_PARAM(SkyTexture);
	IBLShaderParameters mIBLParams;
};


class DenoisePS : public GlobalShader
{
	using BaseClass = GlobalShader;
	DECLARE_SHADER(DenoisePS, Global);
public:
	static char const* GetShaderFileName()
	{
		return "Shader/Game/RayTracing";
	}

	static void SetupShaderCompileOption(ShaderCompileOption& option)
	{

	}

	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_TEXTURE_PARAM(parameterMap, FrameTexture);
		BIND_TEXTURE_PARAM(parameterMap, GBufferTextureA);
		BIND_SHADER_PARAM(parameterMap, ScreenSizeInv);
	}

	DEFINE_TEXTURE_PARAM(FrameTexture);
	DEFINE_TEXTURE_PARAM(GBufferTextureA);
	DEFINE_SHADER_PARAM(ScreenSizeInv);
};


class AccumulatePS : public GlobalShader
{
	using BaseClass = GlobalShader;
	DECLARE_SHADER(AccumulatePS, Global);
public:
	SHADER_PERMUTATION_TYPE_BOOL(UseDebugDisplay, SHADER_PARAM(USE_DEBUG_DISPLAY));
	using PermutationDomain = TShaderPermutationDomain<UseDebugDisplay>;

	static char const* GetShaderFileName()
	{
		return "Shader/Game/RayTracing";
	}

	static void SetupShaderCompileOption(PermutationDomain const& domain, ShaderCompileOption& option)
	{

	}

	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_TEXTURE_PARAM(parameterMap, HistoryTexture);
		BIND_TEXTURE_PARAM(parameterMap, FrameTexture);
	}

	DEFINE_TEXTURE_PARAM(HistoryTexture);
	DEFINE_TEXTURE_PARAM(FrameTexture);
};


class EditorPreviewVS : public GlobalShader
{
	DECLARE_SHADER(EditorPreviewVS, Global);
public:
	static char const* GetShaderFileName() { return "Shader/Game/EditorPreview"; }
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, World);
	}
	DEFINE_SHADER_PARAM(World);
};

class EditorPreviewPS : public GlobalShader
{
	DECLARE_SHADER(EditorPreviewPS, Global);
public:
	static char const* GetShaderFileName() { return "Shader/Game/EditorPreview"; }
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, BaseColor);
		BIND_SHADER_PARAM(parameterMap, EmissiveColor);
		BIND_SHADER_PARAM(parameterMap, Roughness);
		BIND_SHADER_PARAM(parameterMap, Specular);
		BIND_SHADER_PARAM(parameterMap, SelectionAlpha);
	}
	DEFINE_SHADER_PARAM(BaseColor);
	DEFINE_SHADER_PARAM(EmissiveColor);
	DEFINE_SHADER_PARAM(Roughness);
	DEFINE_SHADER_PARAM(Specular);
	DEFINE_SHADER_PARAM(SelectionAlpha);
};

class ScreenOutlinePS : public GlobalShader
{
	DECLARE_SHADER(ScreenOutlinePS, Global);
public:
	static char const* GetShaderFileName() { return "Shader/Game/ScreenOutline"; }
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_TEXTURE_PARAM(parameterMap, SceneDepthTexture);
		BIND_TEXTURE_PARAM(parameterMap, SceneColorTexture);
		BIND_SHADER_PARAM(parameterMap, OutlineColor);
		BIND_SHADER_PARAM(parameterMap, ScreenSize);
	}
	DEFINE_TEXTURE_PARAM(SceneDepthTexture);
	DEFINE_TEXTURE_PARAM(SceneColorTexture);
	DEFINE_SHADER_PARAM(OutlineColor);
	DEFINE_SHADER_PARAM(ScreenSize);
};

class EditorOutlineVS : public GlobalShader
{
	DECLARE_SHADER(EditorOutlineVS, Global);
public:
	static char const* GetShaderFileName() { return "Shader/Game/EditorOutline"; }
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, World);
		BIND_SHADER_PARAM(parameterMap, OutlineThick);
	}
	DEFINE_SHADER_PARAM(World);
	DEFINE_SHADER_PARAM(OutlineThick);
};

class EditorOutlinePS : public GlobalShader
{
	DECLARE_SHADER(EditorOutlinePS, Global);
public:
	static char const* GetShaderFileName() { return "Shader/Game/EditorOutline"; }
	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, OutlineColor);
	}
	DEFINE_SHADER_PARAM(OutlineColor);
};




struct GPU_ALIGN MaterialData
{
	DECLARE_BUFFER_STRUCT(Materials);

	Color3f baseColor;
	float   roughness;
	Color3f emissiveColor;
	float   specular;
	float   refractive;
	float   refractiveIndex;

	Vector2 dummy;

	MaterialData()
	{
		baseColor = { 1, 1, 1 };
		roughness = 0.5f;
		emissiveColor = { 0, 0, 0 };
		specular = 0.0f;
		refractive = 0.0f;
		refractiveIndex = 1.0f;
	}

	MaterialData(Color3f const& inBaseColor, float inRoughness, float inSpecular = 0, Color3f const& inEmissiveColor = Color3f::Black(), float inRefractive = 0, float inRefractiveIndex = 0)
		:baseColor(inBaseColor)
		,roughness(inRoughness)
		,emissiveColor(inEmissiveColor)
		,specular(inSpecular)
		,refractive(inRefractive)
		,refractiveIndex(inRefractiveIndex)
	{


	}

	REFLECT_STRUCT_BEGIN(MaterialData)
		REF_PROPERTY(baseColor)
		REF_PROPERTY(emissiveColor)
		REF_PROPERTY(roughness)
		REF_PROPERTY(specular)
		REF_PROPERTY(refractiveIndex)
		REF_PROPERTY(refractive)
	REFLECT_STRUCT_END()
};

#define OBJ_SPHERE        0
#define OBJ_CUBE          1
#define OBJ_TRIANGLE_MESH 2
#define OBJ_QUAD          3


template<typename T, typename Q>
FORCEINLINE T AsValue(Q value)
{
	static_assert(sizeof(T) == sizeof(Q));
	T* ptr = (T*)&value;
	return *ptr;
}


struct GPU_ALIGN ObjectData
{
	DECLARE_BUFFER_STRUCT(Objects);

	Vector3 pos;
	int32   type;
	Quaternion rotation;
	Vector3 meta;
	int32   materialId;

	ObjectData()
	{
		pos = { 0, 0, 0 };
		type = OBJ_SPHERE;
		rotation = Quaternion::Identity();
		meta = { 1, 0, 0 };
		materialId = 0;
	}

	REFLECT_STRUCT_BEGIN(ObjectData)
		REF_PROPERTY(type)
		REF_PROPERTY(pos)
		REF_PROPERTY(rotation)
		REF_PROPERTY(meta)
		REF_PROPERTY(materialId)
	REFLECT_STRUCT_END()
};

struct FObject
{
	static ObjectData Sphere(float radius, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
	{
		ObjectData result;
		result.type = OBJ_SPHERE;
		result.pos = pos;
		result.rotation = rotation;
		result.materialId = materialId;
		result.meta = Vector3(radius, 0, 0);
		return result;
	}
	static ObjectData Box(Vector3 size, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
	{
		ObjectData result;
		result.type = OBJ_CUBE;
		result.pos = pos;
		result.rotation = rotation;
		result.materialId = materialId;
		result.meta = 0.5 * size;
		return result;
	}

	static ObjectData Quad(Vector2 size, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
	{
		ObjectData result;
		result.type = OBJ_QUAD;
		result.pos = pos;
		result.rotation = rotation;
		result.materialId = materialId;
		result.meta = 0.5 * Vector3(size.x, size.y, 0);
		return result;
	}

	static ObjectData Mesh(int32 meshId, float scale, int materialId, Vector3 pos, Quaternion rotation = Quaternion::Identity())
	{
		ObjectData result;
		result.type = OBJ_TRIANGLE_MESH;
		result.pos = pos;
		result.rotation = rotation;
		result.materialId = materialId;
		result.meta = Vector3(AsValue<float>(meshId), scale, 0);
		return result;
	}
};


struct MeshVertexData
{
	DECLARE_BUFFER_STRUCT(MeshVertices);

	Vector3 pos;
	//float   dummy;
	Vector3 normal;
	//float   dummy2;
};

struct GPU_ALIGN MeshData
{
	DECLARE_BUFFER_STRUCT(Meshes);

	int32 startIndex;
	int32 numTriangles;
	int32 nodeIndex;
	int32 nodeIndexV4;
};



struct SceneBVHNodeData
{
	DECLARE_BUFFER_STRUCT(SceneBVHNodes);
};

struct SceneBVH4NodeData
{
	DECLARE_BUFFER_STRUCT(SceneBVH4Nodes);
};

struct PrimitiveIdData
{
	DECLARE_BUFFER_STRUCT(TriangleIds);
};

struct ObjectIdData
{
	DECLARE_BUFFER_STRUCT(ObjectIds);
};

struct EmittingObjectIdData
{
	DECLARE_BUFFER_STRUCT(EmittingObjectIds);
};


class BVHTree
{
public:

	typedef Math::TAABBox< Vector3 > BoundType;
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

	Stats calcStats() const
	{
		Stats result{ MaxInt32,0,0,MaxInt32,0,0 };

		int depthAcc = 0;
		int countAcc = 0;
		for (auto const& leaf : leaves)
		{
			if (leaf.depth > result.maxDepth)
			  result.maxDepth = leaf.depth;
			else if ( leaf.depth < result.minDepth )
			  result.minDepth = leaf.depth;

			depthAcc += leaf.depth;

			int count = leaf.ids.size();
			if (count > result.maxCount)
				result.maxCount = count;
			else if (count < result.minCount)
				result.minCount = count;

			countAcc += count;
		}

		result.meanDepth = float(depthAcc) / leaves.size();
		result.meanCount = float(countAcc) / leaves.size();
		return result;
	}

	bool checkLeafIndexOrder()
	{
		int curIndex = 0;
		TArray< int > nodeStack;
		nodeStack.push_back(0);
		while (!nodeStack.empty())
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

	void clear()
	{
		nodes.clear();
		leaves.clear();
	}

	TArray<Node> nodes;
	TArray<Leaf> leaves;

	struct Primitive
	{
		int id;
		Vector3   center;
		BoundType bound;
	};

	struct Builder
	{
		Builder(BVHTree& BVH)
			:mBVH(BVH)
		{
		}

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

		int choiceAxis(Math::Vector3 const& size)
		{
			if (size.x > size.y)
				return (size.x >= size.z) ? 0 : 2;
			return (size.y >= size.z) ? 1 : 2;
		}

		int  makeLeaf(TArrayView< BuildData >& dataList, int depth)
		{
			Leaf leaf;
			leaf.ids.resize(dataList.size());
			leaf.depth = depth;
			for (int i = 0; i < dataList.size(); ++i)
			{
				leaf.ids[i] = mPrimitives[dataList[i].index].id;
			}
			int indexLeaf = mBVH.leaves.size();
			mBVH.leaves.push_back(std::move(leaf));
			return indexLeaf;
		}

		int build(TArrayView< Primitive const > primitives)
		{
			mPrimitives = primitives;

			TArray< BuildData > dataList;
			dataList.resize(mPrimitives.size());
			for (int i = 0; i < dataList.size(); ++i)
			{
				dataList[i].index = i;
			}
			int rootId = buildNode_R(MakeView(dataList), 0);
			return rootId;
		}

		int  buildNode_R(TArrayView< BuildData >& dataList, int depth)
		{
			BoundType bound;
			bound = mPrimitives[dataList[0].index].bound;
			for (int i = 1; i < dataList.size(); ++i)
			{
				int index = dataList[i].index;
				bound += mPrimitives[index].bound;
			}
			return buildNode_R(dataList, bound, depth);
		}

		int  buildNode_R(TArrayView< BuildData >& dataList, BoundType const& bound, int depth)
		{
			int   minCount = -1;
			int   minAxis;
			BoundType minBounds[2];

			if (dataList.size() > minSplitPrimitiveCount)
			{
				Vector3 size = bound.getSize();

				if (splitMethod == SplitMethod::NodeBlance)
				{
					minAxis = choiceAxis(size);
					minCount = dataList.size() / 2;
				}
				else
				{
					constexpr int BucketCount = 16;
					struct Bucket
					{
						BoundType bound;
						int count;

						Bucket()
						{
							count = 0;
							bound.invalidate();
						}
					};

					auto GetSurfaceArea = [](BoundType const& bound)
					{
						Vector3 size = bound.getSize();
						return size.x * size.y + size.y * size.z + size.z * size.x;
					};

					float nodeScore = GetSurfaceArea(bound) * dataList.size();
					float minScore = std::numeric_limits<float>::max();
					for (int axis = 0; axis < 3; ++axis)
					{
						float axisSize = size[axis];
						if (axisSize < 1e-6)
							continue;

						Bucket buckets[BucketCount];
						for (int i = 0; i < dataList.size(); ++i)
						{
							auto& data = dataList[i];
							int index = data.index;
							Primitive const& primitive = mPrimitives[index];

							int indecBucket = Math::FloorToInt(float(BucketCount) * (primitive.center[axis] - bound.min[axis]) / axisSize);
							if (indecBucket >= BucketCount)
								indecBucket = BucketCount - 1;

							buckets[indecBucket].count += 1;
							buckets[indecBucket].bound += primitive.bound;
						}

						int leftCount = 0;
						BoundType leftBound;
						leftBound.invalidate();

						for (int i = 0; i < BucketCount - 1; ++i)
						{
							if (buckets[i].count == 0)
								continue;

							leftCount += buckets[i].count;
							leftBound += buckets[i].bound;

							int rightCount = 0;
							BoundType rightBound;
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

							float score = GetSurfaceArea(leftBound) * leftCount + GetSurfaceArea(rightBound) * rightCount;
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

			int indexNode = mBVH.nodes.size();
			mBVH.nodes.push_back(Node());

			if (minCount == -1)
			{
				Node& node = mBVH.nodes[indexNode];
				node.indexLeft = makeLeaf(dataList, depth);
				node.indexRight = -1;
				node.bound = bound;
				node.depth = depth;
			}
			else
			{
				for (int i = 0; i < dataList.size(); ++i)
				{
					auto& data = dataList[i];
					int index = data.index;
					Primitive const& primitive = mPrimitives[index];
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
					indexRight = buildNode_R(TArrayView<BuildData>(dataList.data() + minCount, dataList.size() - minCount), depth + 1);
				}
				else
				{
					indexLeft = buildNode_R(TArrayView<BuildData>(dataList.data(), minCount), minBounds[0], depth + 1);
					indexRight = buildNode_R(TArrayView<BuildData>(dataList.data() + minCount, dataList.size() - minCount), minBounds[1], depth + 1);
				}

				CHECK(indexLeft == indexNode + 1);
				Node& node = mBVH.nodes[indexNode];
				node.indexLeft = indexLeft;
				node.indexRight = indexRight;
				node.bound = bound;
				node.depth = depth;
			}
			return indexNode;
		}

		BVHTree& mBVH;
		TArrayView<Primitive const> mPrimitives;
	};


};

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

	static void Generate(BVHTree const& BVH, TArray< BVHNodeData >& nodes, TArray< int >& primitiveIds)
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

	static void Generate(BVHTree const& BVH, TArray< BVHNodeData >& nodes, int indexTriangleStart)
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
				node.left  = indexTriangleCur;
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

};

struct GPU_ALIGN BVH4NodeData
{
	DECLARE_BUFFER_STRUCT(BVH4Nodes);

	Math::Vector4 boundMinX;
	Math::Vector4 boundMinY;
	Math::Vector4 boundMinZ;
	Math::Vector4 boundMaxX;
	Math::Vector4 boundMaxY;
	Math::Vector4 boundMaxZ;
	int32 children[4];
	int32 primitiveCounts[4];

	static int Generate(BVHTree const& BVH, TArray< BVH4NodeData >& nodes, TArray< int >& primitiveIds)
	{
		nodes.clear();
		if (BVH.nodes.empty()) 
			return 0;
		
		Collapse_R(BVH, 0, nodes, primitiveIds);
		return nodes.size();
	}

	static int Generate(BVHTree const& BVH, TArray< BVH4NodeData >& nodes, int indexTriangleStart)
	{
		int indexCur = indexTriangleStart;
		if (BVH.nodes.empty()) 
			return 0;
		
		int index = nodes.size();
		Collapse_R(BVH, 0, nodes, indexCur);
		return nodes.size() - index;
	}

private:
	static void GetQuadChildren(BVHTree const& BVH, int bNodeId, int outNodes[4], int& outCount)
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

		for(int i = 0; i < pass1Count; ++i) 
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

	static int Collapse_R(BVHTree const& BVH, int binaryNodeIndex, TArray< BVH4NodeData >& outNodes, TArray< int >& primitiveIds)
	{
		int outNodeIndex = outNodes.size();
		outNodes.push_back(BVH4NodeData());

		int childrenNodes[4];
		int childCount = 0;
		GetQuadChildren(BVH, binaryNodeIndex, childrenNodes, childCount);

		Math::Vector4 bMinX(1e30f, 1e30f, 1e30f, 1e30f), bMinY(1e30f, 1e30f, 1e30f, 1e30f), bMinZ(1e30f, 1e30f, 1e30f, 1e30f);
		Math::Vector4 bMaxX(-1e30f, -1e30f, -1e30f, -1e30f), bMaxY(-1e30f, -1e30f, -1e30f, -1e30f), bMaxZ(-1e30f, -1e30f, -1e30f, -1e30f);
		int32 childIndices[4] = {-1, -1, -1, -1};
		int32 childCounts[4] = {0, 0, 0, 0};

		for(int i = 0; i < childCount; ++i)
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
		for(int i=0; i<4; ++i) 
		{
			outNode.children[i] = childIndices[i];
			outNode.primitiveCounts[i] = childCounts[i];
		}

		return outNodeIndex;
	}

	static int Collapse_R(BVHTree const& BVH, int binaryNodeIndex, TArray< BVH4NodeData >& outNodes, int& indexTriangleCur)
	{
		int outNodeIndex = outNodes.size();
		outNodes.push_back(BVH4NodeData());

		int childrenNodes[4];
		int childCount = 0;
		GetQuadChildren(BVH, binaryNodeIndex, childrenNodes, childCount);

		Math::Vector4 bMinX(1e30f, 1e30f, 1e30f, 1e30f), bMinY(1e30f, 1e30f, 1e30f, 1e30f), bMinZ(1e30f, 1e30f, 1e30f, 1e30f);
		Math::Vector4 bMaxX(-1e30f, -1e30f, -1e30f, -1e30f), bMaxY(-1e30f, -1e30f, -1e30f, -1e30f), bMaxZ(-1e30f, -1e30f, -1e30f, -1e30f);
		int32 childIndices[4] = {-1, -1, -1, -1};
		int32 childCounts[4] = {0, 0, 0, 0};

		for(int i = 0; i < childCount; ++i)
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
		for(int i=0; i<4; ++i) {
			outNode.children[i] = childIndices[i];
			outNode.primitiveCounts[i] = childCounts[i];
		}
		return outNodeIndex;
	}
};

namespace RT
{
	class ISceneBuilder
	{
	public:
		//virtual void addSphere(Vector3 const& pos, float radius) = 0;
	};

	class SceneData
	{
	public:

		TArray< MaterialData > materials;
		TArray< ObjectData > objects;
		TArray< MeshData > meshes;
		TArray< std::shared_ptr<Render::Mesh> > mSceneMeshes;
		
		TArray< MeshVertexData > mMeshVertices;
		TArray< BVHNodeData > mMeshBVHNodes;
		TArray< BVHNodeData > mSceneBVHNodes;
		TArray< int32 > mSceneObjectIds;

		TStructuredBuffer< MaterialData > mMaterialBuffer;
		TStructuredBuffer< ObjectData > mObjectBuffer;
		TStructuredBuffer< MeshVertexData > mVertexBuffer;
		TStructuredBuffer< MeshData > mMeshBuffer;
		TStructuredBuffer< BVHNodeData > mBVHNodeBuffer;
		TStructuredBuffer< BVH4NodeData > mBVH4NodeBuffer;

		TStructuredBuffer< BVHNodeData > mSceneBVHNodeBuffer;
		TStructuredBuffer< BVH4NodeData > mSceneBVH4NodeBuffer;
		TStructuredBuffer< int32 > mObjectIdBuffer;
		TStructuredBuffer< int32 > mObjectIdBufferV4;
		TStructuredBuffer< int32 > mEmittingObjectIdBuffer;
		int mNumObjects;
		int mNumEmittingObjects;
		PrimitivesCollection mDebugPrimitives;

		void rebuildSceneBVH();

		static Math::TAABBox<Vector3> GetAABB(Quaternion const& rotation, Vector3 const halfSize)
		{
			Math::TAABBox<Vector3> result;
			result.invalidate();
			for (uint32 i = 0; i < 8; ++i)
			{
				Vector3 offset;
				offset.x = (i & 0x1) ? halfSize.x : -halfSize.x;
				offset.y = (i & 0x2) ? halfSize.y : -halfSize.y;
				offset.z = (i & 0x4) ? halfSize.z : -halfSize.z;
				result.addPoint(rotation.rotate(offset));
			}
			return result;
		}

		static Math::TAABBox<Vector3> GetAABB2(Vector3 const& center, Quaternion const& rotation, Vector3 const halfSize)
		{
			Math::TAABBox<Vector3> result;
			result.invalidate();
			for (uint32 i = 0; i < 8; ++i)
			{
				Vector3 offset;
				offset.x = (i & 0x1) ? halfSize.x : -halfSize.x;
				offset.y = (i & 0x2) ? halfSize.y : -halfSize.y;
				offset.z = (i & 0x4) ? halfSize.z : -halfSize.z;
				result.addPoint(rotation.rotate(offset + center));
			}
			return result;
		}


		BVHTree mDebugBVH;


		void addDebugAABB(Math::TAABBox<Vector3> const& bound)
		{
			Vector3 posList[8];
			for (uint32 i = 0; i < 8; ++i)
			{
				posList[i].x = (i & 0x1) ? bound.max.x : bound.min.x;
				posList[i].y = (i & 0x2) ? bound.max.y : bound.min.y;
				posList[i].z = (i & 0x4) ? bound.max.z : bound.min.z;
			}

			constexpr int LineIndices[] =
			{
				0,1,1,3,3,2,2,0,
				4,5,5,7,7,6,6,4,
				0,4,1,5,3,7,2,6,
			};
			for (int i = 0; i < ARRAY_SIZE(LineIndices); i += 2)
			{
				mDebugPrimitives.addLine(posList[LineIndices[i]], posList[LineIndices[i + 1]], Vector3(1, 0, 0));
			}

		}


		void showNodeBound(int depth)
		{
			mDebugPrimitives.clear();
			for (int i = 0; i < mDebugBVH.nodes.size(); ++i)
			{
				auto const& node = mDebugBVH.nodes[i];
				if (node.depth == depth)
				{
					addDebugAABB(node.bound);
				}
			}
		}
	};

	class ISceneScript
	{
	public:
		virtual ~ISceneScript() {}
		virtual bool setup(ISceneBuilder& builder, char const* fileName) = 0;
		virtual void release() = 0;

		static std::string   GetFilePath(char const* fileName);
		static ISceneScript* Create();
	};
}



class RayTracingTestStage : public TestRenderStageBase
	                      , public RT::SceneData
#if TINY_WITH_EDITOR
						  , public IEditorGameViewport
#endif
{
	using BaseClass = TestRenderStageBase;
public:

	enum
	{
		UI_StatsMode = BaseClass::NEXT_UI_ID,


		NEXT_UI_ID,
	};

	RT::ISceneScript* mScript = nullptr;
	RayTracingTestStage();

#if TINY_WITH_EDITOR
	TVector2<int> getInitialSize() override;
	void resizeViewport(int w, int h) override;
	void renderViewport(IEditorViewportRenderContext& context) override;
	void onViewportMouseEvent(MouseMsg const& msg) override;
	void onViewportKeyEvent(unsigned key, bool isDown) override;
	void onViewportCharEvent(unsigned code) override;

	IEditorDetailView* mDetailView = nullptr;
	IEditorDetailView* mObjectsDetailView = nullptr;
	IEditorDetailView* mMaterialsDetailView = nullptr;
	IEditorToolBar* mToolBar = nullptr;
	SimpleCamera mEditorCamera;
	int mSelectedObjectId = INDEX_NONE;
	bool bEditorCameraValid = false;
	Vec2i mEditorViewportSize = Vec2i(0, 0);
	bool bDataChanged = false;
	bool mbControlDown = false;
	void refreshDetailView();
#endif



	bool onInit() override;

	void loadCameaTransform()
	{
		char const* text;
		if (::Global::GameConfig().tryGetStringValue("LastPos", "RayTracing", text))
		{
			sscanf(text, "%f %f %f", &mLastPos.x, &mLastPos.y, &mLastPos.z);
			mCamera.setPos(mLastPos);
		}
		if (::Global::GameConfig().tryGetStringValue("LastRoation", "RayTracing", text))
		{
			sscanf(text, "%f %f %f", &mLastRoation.x, &mLastRoation.y, &mLastRoation.z);
			mCamera.setRotation(mLastRoation.z, mLastRoation.x, mLastRoation.y);
		}
	}

	void saveCameraTransform()
	{
		InlineString<128> text;
		text.format("%g %g %g", mLastPos.x, mLastPos.y, mLastPos.z);
		::Global::GameConfig().setKeyValue("LastPos", "RayTracing", text.c_str());
		text.format("%g %g %g", mLastRoation.x, mLastRoation.y, mLastRoation.z);
		::Global::GameConfig().setKeyValue("LastRoation", "RayTracing", text.c_str());
	}

	void loadScene(char const* path);
	void saveScene(char const* path);

	void onEnd() override
	{
		if (mScript)
		{
			mScript->release();
		}
#if TINY_WITH_EDITOR
		if (mDetailView) mDetailView->release();
		if (mObjectsDetailView) mObjectsDetailView->release();
		if (mMaterialsDetailView) mMaterialsDetailView->release();
		if (mToolBar) mToolBar->release();

		if (::Global::Editor())
		{
			::Global::Editor()->removeGameViewport(this);
		}
#endif
		BaseClass::onEnd();
	}

	void restart() {}

	void onUpdate(GameTimeSpan deltaTime) override;


	Vector3 mLastPos;
	Vector3 mLastRoation;

	bool mbDrawDebug = false;
	int  mDebugShowDepth = 1;

	void onRender(float dFrame) override;


	MsgReply onMouse(MouseMsg const& msg) override
	{
		static Vec2i oldPos = msg.getPos();

		if (msg.onLeftDown())
		{
			oldPos = msg.getPos();
		}
		if (msg.onMoving() && msg.isLeftDown())
		{
			float rotateSpeed = 0.01;
			Vector2 off = rotateSpeed * Vector2(msg.getPos() - oldPos);
			mCamera.rotateByMouse(off.x, off.y);
			oldPos = msg.getPos();
		}

		return BaseClass::onMouse(msg);
	}

	MsgReply onKey(KeyMsg const& msg) override;



	bool onWidgetEvent(int event, int id, GWidget* ui) override
	{
		switch (id)
		{
		case UI_StatsMode:
			{
				mDebugDisplayMode = EDebugDsiplayMode(ui->cast<GChoice>()->getSelection());

			}
			return true;
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::D3D11;
	}

	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override;

	bool setupRenderResource(ERenderSystem systemName) override;

	bool loadSceneRHIResource();

	void preShutdownRenderSystem(bool bReInit = false) override;



	void addLine(Vector3 p1, Vector3 p2, Color3f color)
	{
		Line line;
		line.start.x = p1.x;
		line.start.y = p1.y;
		line.end.x = p2.x;
		line.end.y = p2.y;
		line.color = color;
		mPaths.push_back(line);
	}

	enum class EGizmoType
	{
		None,
		Translate,
		Rotate,
		Scale,
	};
	enum class EGizmoMode
	{
		Local,
		World,
	};
	EGizmoType mGizmoType = EGizmoType::Translate;
	EGizmoMode mGizmoMode = EGizmoMode::World;
	int mGizmoAxis = -1;
	Vector3 mGizmoLastRayPos;
	Vector3 mGizmoLastRayDir;
	bool mIsGizmoDragging = false;

	void drawGizmo(RHICommandList& commandList, ViewInfo& view);

	struct Line
	{

		Vector2 start;
		Vector2 end;
		Color3f color;
	};
	TArray< Line > mPaths;
	
	Vector3 spherePos = Vector3::Zero();
	float radius = 100;
	void generatePath()
	{
		float refractiveIndex = 1.5;
		Vector3 viewPos = Vector3(400, 0, 0);


		int num = 250;
		auto TestRay = [&](Vector3 rayDir)
		{
			float dists[2];
			if (!LineSphereTest(viewPos, rayDir, spherePos, radius, dists))
				return;


			Color3f CRed(1, 0, 0);
			Color3f CYellow(1, 1, 0);

			Vector3 hitPos = viewPos + dists[1] * rayDir;
			addLine(viewPos, hitPos, CRed);
			
			{
				Vector3 N = GetNormal(hitPos - spherePos);
				Vector3 V = GetNormal(viewPos - hitPos);

				float cosI = N.dot(V);
				float sinI = Math::Sqrt(1 - Math::Square(cosI));
				float sinO = sinI / refractiveIndex;
				float cosO = Math::Sqrt(1 - Math::Square(sinO));
				rayDir = -GetNormal(N * cosO + GetNormal(V - cosI * N) * sinO);
			}


			LineSphereTest(hitPos, rayDir, spherePos, radius, dists);

			Vector3 hitPos2 = hitPos + dists[1] * rayDir;

	
			{
				Vector3 N = GetNormal(spherePos - hitPos2);
				Vector3 V = -rayDir;

				float cosI = N.dot(V);
				float sinI = Math::Sqrt(1 - Math::Square(cosI));
				float sinO = refractiveIndex * sinI;
				if (sinO <= 1)
				{
					float cosO = Math::Sqrt(1 - Math::Square(sinO));
					rayDir = -GetNormal(N * cosO + GetNormal(V - cosI * N) * sinO);
					addLine(hitPos, hitPos2, CRed);
					addLine(hitPos2, hitPos2 + 100 * rayDir, CRed);
				}
				else
				{
					addLine(hitPos, hitPos2, CYellow);
				}
			}
		};
		

		for (int i = 0; i < num; ++i)
		{
			float theta = 0.5 * Math::PI * i / float(num);

			float c , s;
			Math::SinCos(theta, s, c);

			Vector3 rayDir = Vector3(c, s, 0);
			TestRay(rayDir);
		}
	}



	RenderTransform2D mWorldToScreen;
	RenderTransform2D mScreenToWorld;
	


	FrameRenderTargets mSceneRenderTargets;

	ScreenVS* mScreenVS = nullptr;
	RHIFrameBufferRef mFrameBuffer;
	RHITexture2DRef mDenoiseTexture;
	RHIFrameBufferRef mDenoiseFrameBuffer;

	EDebugDsiplayMode mDebugDisplayMode = EDebugDsiplayMode::None;
	EDebugDsiplayMode mLastDebugDisplayModeRender = EDebugDsiplayMode::None;

	int mBoundBoxWarningCount = 500;
	int mTriangleWarningCount = 50;

	bool bSplitAccumulate = false;
	bool bUseMIS = false;
	bool bUseBVH4 = false;
	bool bUseDenoise = true;

	AccumulatePS* mAccumulatePS;
	DenoisePS* mDenoisePS;

	EditorPreviewVS* mPreviewVS;
	EditorPreviewPS* mPreviewPS;
	ScreenOutlinePS* mScreenOutlinePS;

	RHITexture2DRef mHDRImage;
	IBLResource mIBLResource;
	IBLResourceBuilder mIBLResourceBuilder;
};