#include "TestRenderStageBase.h"
#include "GameRenderSetup.h"

#include "RHI/RHICommand.h"
#include "Renderer/MeshImportor.h"
#include "Math/GeometryPrimitive.h"
#include "PropertySet.h"
#include "ProfileSystem.h"
#include "ReflectionCollect.h"

using namespace Render;


class RayTracingPS : public GlobalShader
{
	using BaseClass = GlobalShader;
	DECLARE_SHADER(RayTracingPS, Global);
public:
	static char const* GetShaderFileName()
	{
		return "Shader/Game/RayTracing";
	}

	void bindParameters(ShaderParameterMap const& parameterMap)
	{
		BIND_SHADER_PARAM(parameterMap, HistoryTexture);
	}

	DEFINE_SHADER_PARAM(HistoryTexture);

};



struct GPU_ALIGN MaterialData
{
	DECLARE_BUFFER_STRUCT(MaterialDataBlock, Materials);

	Color3f baseColor;
	float   roughness;
	Color3f emissiveColor;
	float   refractiveIndex;
	float   refractive;
	Vector3 dummy;

	REFLECT_OBJECT_BEGIN(MaterialData)
		REF_PROPERTY(baseColor)
		REF_PROPERTY(roughness)
		REF_PROPERTY(emissiveColor)
		REF_PROPERTY(refractiveIndex)
		REF_PROPERTY(refractive)
	REFLECT_OBJECT_END()
};

#define OBJ_SPHERE        0
#define OBJ_CUBE          1
#define OBJ_TRIANGLE_MESH 2
#define OBJ_QUAD          3


template<typename T, typename Q>
FORCEINLINE T AsValue(Q value)
{
	T* ptr = (T*)&value;
	return *ptr;
}


struct GPU_ALIGN ObjectData
{
	DECLARE_BUFFER_STRUCT(ObjectDataBlock, Objects);

	Vector3 pos;
	int32   type;
	Quaternion rotation;
	Vector3 meta;
	int32   materialId;
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
	DECLARE_BUFFER_STRUCT(MeshVertexDataBlock, MeshVertices);

	Vector3 pos;
	//float   dummy;
	Vector3 normal;
	//float   dummy2;
};

struct GPU_ALIGN MeshData
{
	DECLARE_BUFFER_STRUCT(MeshDataBlock, Meshes);

	Vector3 boundMin;
	int32 startIndex;
	Vector3 boundMax;
	int32 numTriangles;
	int32 nodeIndex;
	int32 dummy[3];
};



struct SceneBVHNodeData
{
	DECLARE_BUFFER_STRUCT(SceneBVHNodeDataBlock, SceneBVHNodes);
};

struct PrimitiveIdData
{
	DECLARE_BUFFER_STRUCT(TriangleIdlock, TriangleIds);
};

struct ObjectIdData
{
	DECLARE_BUFFER_STRUCT(ObjectIdBlock, ObjectIds);
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

		bool isLeaf() const { return indexRight < 0; }
	};

	struct Leaf
	{
		TArray<int> ids;
	};

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
		int maxLeafPrimitiveCount = 5;

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


		int  makeLeaf(TArrayView< BuildData >& dataList)
		{
			Leaf leaf;
			leaf.ids.resize(dataList.size());
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
			int rootId = buildNode_R(MakeView(dataList));
			return rootId;
		}

		int  buildNode_R(TArrayView< BuildData >& dataList)
		{
			BoundType bound;
			bound = mPrimitives[dataList[0].index].bound;
			for (int i = 1; i < dataList.size(); ++i)
			{
				int index = dataList[i].index;
				bound += mPrimitives[index].bound;
			}
			return buildNode_R(dataList, bound);
		}

		int  buildNode_R(TArrayView< BuildData >& dataList, BoundType const& bound)
		{
			int   minCount = -1;
			int   minAxis;
			BoundType minBounds[2];

			if (dataList.size() > maxLeafPrimitiveCount)
			{
				Vector3 size = bound.getSize();

				if (splitMethod == SplitMethod::NodeBlance)
				{
					minAxis = choiceAxis(size);
					minCount = dataList.size() / 2;
				}
				else
				{
					float minScore = std::numeric_limits<float>::max();
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

						auto GetSurfaceArea = [](BoundType const& bound)
						{
							Vector3 size = bound.getSize();
							return size.x * size.y + size.y * size.z + size.z * size.x;
						};

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
				}
			}

			int indexNode = mBVH.nodes.size();
			mBVH.nodes.push_back(Node());

			if (minCount == -1)
			{
				Node& node = mBVH.nodes[indexNode];
				node.indexLeft = makeLeaf(dataList);
				node.indexRight = -1;
				node.bound = bound;
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
					indexLeft = buildNode_R(TArrayView<BuildData>(dataList.data(), minCount));
					indexRight = buildNode_R(TArrayView<BuildData>(dataList.data() + minCount, dataList.size() - minCount));
				}
				else
				{
					indexLeft = buildNode_R(TArrayView<BuildData>(dataList.data(), minCount), minBounds[0]);
					indexRight = buildNode_R(TArrayView<BuildData>(dataList.data() + minCount, dataList.size() - minCount), minBounds[1]);
				}

				CHECK(indexLeft == indexNode + 1);
				Node& node = mBVH.nodes[indexNode];
				node.indexLeft = indexLeft;
				node.indexRight = indexRight;
				node.bound = bound;
			}
			return indexNode;
		}

		BVHTree& mBVH;
		TArrayView<Primitive const> mPrimitives;
	};


};

struct GPU_ALIGN BVHNodeData
{
	DECLARE_BUFFER_STRUCT(BVHNodeDataBlock, BVHNodes);

	Vector3 boundMin;
	int32 left;
	Vector3 boundMax;
	int32 right;

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

};

template< typename TStruct, typename TShaderObject >
static void SetStructuredStorageBuffer(RHICommandList& commandList, TShaderObject& shaderObject, TStructuredBuffer<TStruct>& buffer)
{
	shaderObject.setStructuredStorageBufferT< TStruct >(commandList, *buffer.getRHI());
}

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

		TStructuredBuffer< MaterialData > mMaterialBuffer;
		TStructuredBuffer< ObjectData > mObjectBuffer;
		TStructuredBuffer< MeshVertexData > mVertexBuffer;
		TStructuredBuffer< MeshData > mMeshBuffer;
		TStructuredBuffer< BVHNodeData > mBVHNodeBuffer;
		TStructuredBuffer< BVHNodeData > mSceneBVHNodeBuffer;
		TStructuredBuffer< int32 > mTriangleIdBuffer;
		TStructuredBuffer< int32 > mObjectIdBuffer;
		int mNumObjects;
		PrimitivesCollection mDebugPrimitives;
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




class RayTracingTestStage : public StageBase
	                      , public IGameRenderSetup
	                      , public RT::SceneData
{
	using BaseClass = StageBase;
public:

	ViewInfo      mView;
	ViewFrustum   mViewFrustum;
	SimpleCamera  mCamera;
	bool          mbGamePased;

	RayTracingTestStage()
	{

	}

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

	void onEnd() override
	{
		BaseClass::onEnd();
	}

	void restart() {}
	void tick() {}
	void updateFrame(int frame) {}

	void onUpdate(long time) override
	{
		BaseClass::onUpdate(time);

		int frame = time / gDefaultTickTime;
		for (int i = 0; i < frame; ++i)
		{
			float dt = gDefaultTickTime / 1000.0;
			mView.gameTime += dt;

			if (!mbGamePased)
			{
				mView.gameTime += dt;
			}
			tick();
		}

		float dt = float(time) / 1000;
		mCamera.updatePosition(dt);
		updateFrame(frame);
	}


	Vector3 mLastPos;
	Vector3 mLastRoation;

	bool mbDrawDebug = false;


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
		default:
			break;
		}

		return BaseClass::onWidgetEvent(event, id, ui);
	}

	ERenderSystem getDefaultRenderSystem() override
	{
		return ERenderSystem::D3D11;
	}

	void configRenderSystem(ERenderSystem systenName, RenderSystemConfigs& systemConfigs) override
	{
		systemConfigs.screenWidth = 1024;
		systemConfigs.screenHeight = 768;
	}

	bool setupRenderSystem(ERenderSystem systemName) override;

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
	RayTracingPS* mRayTracingPS = nullptr;
};

