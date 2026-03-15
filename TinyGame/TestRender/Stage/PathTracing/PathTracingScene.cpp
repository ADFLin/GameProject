#include "PathTracingScene.h"

#include "PathTracingBVH.h"
//#TODO : Remove
#include "PathTracingRenderer.h"

#include "Renderer/MeshImportor.h"

#define CHAISCRIPT_NO_THREADS
#include "chaiscript/chaiscript.hpp"

#include "ProfileSystem.h"
#include "DataCacheInterface.h"
#include "FileSystem.h"

namespace Chai = chaiscript;

namespace PathTracing
{
	using namespace Render;

	class SceneBuilderImpl : public ISceneBuilder
		                   , public SceneBuildContext
	{
	public:

		SceneBuilderImpl(SceneBuildContext const& context)
			:SceneBuildContext(context)
		{
			meshImporter = MeshImporterRegistry::Get().getMeshImproter("FBX");
		}


		IMeshImporterPtr meshImporter;
		BVHTree meshBVH;


		static int GenerateTriangleVertices(BVHTree& meshBVH, MeshImportData& meshData, MeshVertexData* pOutData, Math::Transform const& transform)
		{
			auto posReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_POSITION);
			auto normalReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_NORMAL);

			Vector3 invScale = transform.getSafeInvScale();

			MeshVertexData* pData = pOutData;
			for (auto const& leaf : meshBVH.leaves)
			{
				for (int id : leaf.ids)
				{
					for (int n = 0; n < 3; ++n)
					{
						MeshVertexData& vertex = *(pData++);
						int vertexIndex = meshData.indices[id + n];
						vertex.pos = transform.transformPosition(posReader[vertexIndex]);
						vertex.normal = transform.rotation.rotate(normalReader[vertexIndex] * invScale);
						vertex.normal.normalize();
					}
				}
			}

			return pData - pOutData;
		}

		struct BuildMeshResult
		{
			TArrayView< MeshVertexData const > vertices;
			TArrayView< BVHNodeData const >  nodes;
			TArrayView< BVH4NodeData const > nodesV4;
			int nodeIndex;
			int nodeIndexV4;

			int triangleIndex;
			int numTriangles;
		};

		void buildMeshData(MeshImportData& meshData, BuildMeshResult& buildResult, Math::Transform const& transform)
		{
			TIME_SCOPE("BuildMeshData");

			auto& meshVertices = mRenderResource.mMeshVertices;
			auto& meshBVHNodes = mRenderResource.mMeshBVHNodes;
			auto& meshBVH4Nodes = mRenderResource.mMeshBVH4Nodes;

			auto posReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_POSITION);
			auto normalReader = meshData.makeAttributeReader(EVertex::ATTRIBUTE_NORMAL);

			int numTriangles = meshData.indices.size() / 3;

			TArray< BVHTree::Primitive > primitives;
			primitives.resize(numTriangles);
			for (int indexTriangle = 0; indexTriangle < numTriangles; ++indexTriangle)
			{
				auto& primitive = primitives[indexTriangle];
				primitive.bound.invalidate();

				int index = 3 * indexTriangle;
				primitive.center = Vector3::Zero();
				for (int n = 0; n < 3; ++n)
				{
					Vector3 pos = transform.transformPosition(posReader[meshData.indices[index + n]]);
					primitive.bound.addPoint(pos);
					primitive.center += pos;
				}
				primitive.id = index;
				primitive.center /= 3.0f;
			}

			meshBVH.clear();
			int indexLeafStart = meshBVH.leaves.size();
			TIME_SCOPE("BuildBVH");
			BVHTreeBuilder builder(meshBVH);
			int indexRoot = builder.build(MakeConstView(primitives));

			auto stats = meshBVH.calcStats();
			LogMsg("Node Count : %u ,Leaf Count : %u", meshBVH.nodes.size(), meshBVH.leaves.size());
			LogMsg("Leaf Depth : %d - %d (%.2f)", stats.minDepth, stats.maxDepth, stats.meanDepth);
			LogMsg("Leaf Tris : %d - %d (%.2f)", stats.minCount, stats.maxCount, stats.meanCount);

			if (mScene.meshes.size() == 0)
			{
				mRenderResource.mDebugBVH = meshBVH;
			}

			int triangleIndex = meshVertices.size();
			meshVertices.resize(triangleIndex + 3 * numTriangles);

			int numV = GenerateTriangleVertices(meshBVH, meshData, meshVertices.data() + triangleIndex, transform);
			CHECK(numV == numTriangles * 3);
			int nodeIndex = meshBVHNodes.size();
			FBVH::Generate(meshBVH, meshBVHNodes, triangleIndex);
			int nodeIndexV4 = meshBVH4Nodes.size();
			int numNodeV4 = FBVH::Generate(meshBVH, meshBVH4Nodes, triangleIndex);

			buildResult.vertices = TArrayView<MeshVertexData const>(meshVertices.data() + triangleIndex, numV);
			buildResult.nodes = TArrayView<BVHNodeData const>(meshBVHNodes.data() + nodeIndex, meshBVH.nodes.size());
			buildResult.nodeIndex = nodeIndex;
			buildResult.nodesV4 = TArrayView<BVH4NodeData const>(meshBVH4Nodes.data() + nodeIndexV4, numNodeV4);
			buildResult.nodeIndexV4 = nodeIndexV4;
			buildResult.numTriangles = numTriangles;
			buildResult.triangleIndex = triangleIndex;
		}


		static void FixOffset(TArray< BVHNodeData >& nodes, int nodeOffset, int triangleOffset)
		{
			for (BVHNodeData& node : nodes)
			{
				if (node.isLeaf())
				{
					node.left += triangleOffset;
				}
				else
				{
					node.left += nodeOffset;
					node.right += nodeOffset;
				}
			}
		}

		static void FixOffset(TArray< BVH4NodeData >& nodes, int nodeOffset, int triangleOffset)
		{
			for (BVH4NodeData& node : nodes)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (node.children[i] == -1)
						continue;

					if (node.primitiveCounts[i] > 0)
					{
						node.children[i] += triangleOffset;
					}
					else
					{
						node.children[i] += nodeOffset;
					}
				}
			}
		}

		static MeshImportData const* GetRawMeshData(char const* path)
		{
			static std::unordered_map< std::string, MeshImportData > gRawDataCache;
			auto iter = gRawDataCache.find(path);
			if (iter != gRawDataCache.end())
				return &iter->second;

			MeshImportData& meshData = gRawDataCache[path];

			auto& dataCache = ::Global::DataCache();
			DataCacheKey cacheKey;
			cacheKey.typeName = "RAW_MESH";
			cacheKey.version = "f2a36b1d-9c8e-4a7b-a1b2-c3d4e5f6g7h8";
			cacheKey.keySuffix.add(path);
			cacheKey.keySuffix.addFileAttribute(path);

			if (dataCache.loadT(cacheKey, meshData))
			{
				return &meshData;
			}

			IMeshImporterPtr useImporter = MeshImporterRegistry::Get().getMeshImproter("FBX");
			if (FCString::Compare(FFileUtility::GetExtension(path), "glb") == 0)
			{
				useImporter = MeshImporterRegistry::Get().getMeshImproter("GLB");
			}

			if (!useImporter || !useImporter->importFromFile(path, meshData))
			{
				gRawDataCache.erase(path);
				return nullptr;
			}

			if (meshData.sections.empty())
				meshData.initSections();

			dataCache.saveT(cacheKey, meshData);
			return &meshData;
		}

		int loadMesh(char const* path, Math::Transform const& transform = Math::Transform::Identity())
		{
			auto& meshVertices = mRenderResource.mMeshVertices;
			auto& meshBVHNodes = mRenderResource.mMeshBVHNodes;
			auto& meshBVH4Nodes = mRenderResource.mMeshBVH4Nodes;

			TIME_SCOPE("LoadMesh");
			MeshData mesh;

			auto& dataCache = ::Global::DataCache();
			DataCacheKey cacheKey;
			cacheKey.typeName = "MESH_BVH";
			cacheKey.version = "1e987635-09dc-521a-af73-116bc953dfe6";
			cacheKey.keySuffix.addFileAttribute(path);
			cacheKey.keySuffix.addFormat("_%g_%g_%g_%g_%g_%g_%g_%g_%g_%g",
				transform.location.x, transform.location.y, transform.location.z,
				transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w,
				transform.scale.x, transform.scale.y, transform.scale.z);

			TArray< MeshVertexData > vertices;
			TArray< BVHNodeData > nodes;
			TArray< BVH4NodeData > nodesV4;
			int nodeIndex;
			int nodeIndexV4;
			int triangleIndex;
			auto LoadCache = [&](IStreamSerializer& serializer)-> bool
			{
				serializer >> vertices;
				serializer >> nodes;
				serializer >> nodeIndex;
				serializer >> triangleIndex;
				serializer >> nodesV4;
				serializer >> nodeIndexV4;
				return true;
			};
			if (dataCache.loadDelegate(cacheKey, LoadCache))
			{
				FixOffset(nodes, (int)meshBVHNodes.size() - nodeIndex, (int)meshVertices.size() - triangleIndex);
				FixOffset(nodesV4, (int)meshBVH4Nodes.size() - nodeIndexV4, (int)meshVertices.size() - triangleIndex);

				triangleIndex = (int)meshVertices.size();
				nodeIndex = (int)meshBVHNodes.size();
				nodeIndexV4 = (int)meshBVH4Nodes.size();
				meshBVHNodes.append(nodes);
				meshVertices.append(vertices);
				meshBVH4Nodes.append(nodesV4);

				mesh.startIndex = triangleIndex;
				mesh.numTriangles = (int)vertices.size() / 3;
				mesh.nodeIndex = nodeIndex;
				mesh.nodeIndexV4 = nodeIndexV4;

			}
			else
			{
				MeshImportData const* pMeshData = GetRawMeshData(path);
				if (!pMeshData)
				{
					return INDEX_NONE;
				}
				BuildMeshResult buildResult;
				buildMeshData(*const_cast<MeshImportData*>(pMeshData), buildResult, transform);
				dataCache.saveDelegate(cacheKey, [&buildResult](IStreamSerializer& serializer)-> bool
				{
					serializer << buildResult.vertices;
					serializer << buildResult.nodes;
					serializer << buildResult.nodeIndex;
					serializer << buildResult.triangleIndex;
					serializer << buildResult.nodesV4;
					serializer << buildResult.nodeIndexV4;
					return true;
				});

				mesh.startIndex = buildResult.triangleIndex;
				mesh.numTriangles = buildResult.numTriangles;
				mesh.nodeIndex = buildResult.nodeIndex;
				mesh.nodeIndexV4 = buildResult.nodeIndexV4;
			}

			mScene.meshes.push_back(mesh);

			MeshImportInfo info;
			info.path = path;
			info.transform = transform;
			mScene.meshInfos.push_back(info);

			CHECK(mScene.meshes.size() == mScene.meshInfos.size());
			return (int)mScene.meshes.size() - 1;
		}
	};

#define SCRIPT_NAME( NAME ) #NAME

	struct FTransform_Script {};
	struct FQuaternion_Script {};

	class FScriptCommon
	{
	public:
		template< typename TModuleType >
		static void RegisterMath(TModuleType& module)
		{
			module.add(chaiscript::type_conversion<double, float>([](double d) { return static_cast<float>(d); }));

			//Vector3
			module.add(Chai::constructor< Vector3(float) >(), SCRIPT_NAME(Vector3));
			module.add(Chai::constructor< Vector3(float, float, float) >(), SCRIPT_NAME(Vector3));
			module.add(Chai::fun(static_cast<Vector3& (Vector3::*)(Vector3 const&)>(&Vector3::operator=)), SCRIPT_NAME(= ));
			module.add(Chai::fun(static_cast<Vector3(*)(Vector3 const&, Vector3 const&)>(&Math::operator+)), SCRIPT_NAME(+));
			module.add(Chai::fun(static_cast<Vector3(*)(Vector3 const&, Vector3 const&)>(&Math::operator-)), SCRIPT_NAME(-));
			module.add(Chai::fun(static_cast<Vector3(*)(float, Vector3 const&)>(&Math::operator*)), SCRIPT_NAME(*));
			//Color3f
			module.add(Chai::constructor< Color3f(float, float, float) >(), SCRIPT_NAME(Color3f));
			module.add(Chai::fun(static_cast<Color3f& (Color3f::*)(Color3f const&)>(&Color3f::operator=)), SCRIPT_NAME(= ));

			//Quaternion
			module.add(Chai::constructor< Quaternion() >(), SCRIPT_NAME(Quaternion));
			module.add(Chai::fun([](FQuaternion_Script, Vector3 const& axis, float angle) { return Quaternion::Rotate(axis, Math::DegToRad(angle)); }), "Rotate");
			module.add(Chai::fun([](FQuaternion_Script, Vector3 const& v) { return Quaternion::EulerZYX(Vector3(Math::DegToRad(v.x), Math::DegToRad(v.y), Math::DegToRad(v.z))); }), "EulerZYX");
			module.add(Chai::fun(static_cast<Quaternion(*)(Quaternion const&, Quaternion const&)>(&Math::operator*)), SCRIPT_NAME(*));

			using Math::Transform;

			//Transform
			module.add(Chai::constructor< Transform() >(), SCRIPT_NAME(Transform));
			module.add(Chai::constructor< Transform(Vector3 const&, Quaternion const&, Vector3 const&) >(), SCRIPT_NAME(Transform));
			module.add(Chai::fun([](FTransform_Script) { return Transform::Identity(); }), "Identity");
			module.add(Chai::fun([](FTransform_Script, Vector3 const& axis, float angle) { return Transform::Rotate(Quaternion::Rotate(axis, Math::DegToRad(angle))); }), "Rotate");
			module.add(Chai::fun([](FTransform_Script, float scale) { return Transform::Scale(scale); }), "Scale");
			module.add(Chai::fun([](FTransform_Script, Vector3 const& v) { return Transform::EulerZYX(Vector3(Math::DegToRad(v.x), Math::DegToRad(v.y), Math::DegToRad(v.z))); }), "EulerZYX");
			module.add(Chai::fun(static_cast<Transform(Transform::*)(Transform const&) const>(&Transform::operator*)), SCRIPT_NAME(*));
			module.add(Chai::fun(&Transform::location), "location");
			module.add(Chai::fun(&Transform::rotation), "rotation");
			module.add(Chai::fun(&Transform::scale), "scale");
		}
	};


	class SceneScriptImpl : public ISceneScript
	{
	public:

		SceneBuilderImpl* mBuilder;
		Chai::ChaiScript script;
		Chai::ModulePtr CommonModule;
		SceneScriptImpl()
		{
			TIME_SCOPE("CommonModule Init");
			CommonModule.reset(new Chai::Module);
			FScriptCommon::RegisterMath(*CommonModule);
			registerAssetId(*CommonModule);
			registerSceneObject(*CommonModule);
			script.add(CommonModule);
			script.add_global_const(Chai::const_var(FTransform_Script()), "FTransform");
			script.add_global_const(Chai::const_var(FQuaternion_Script()), "FQuaternion");
			registerSceneFunc(script);
		}

		bool setup(SceneBuilderImpl& builder, char const* fileName)
		{
			mBuilder = &builder;
#if 1
			try
			{
				std::string path = ISceneScript::GetFilePath(fileName);
				TIME_SCOPE("Script Eval");
				script.eval_file(path.c_str());
			}
			catch (const std::exception& e)
			{
				LogMsg("Load Scene Fail!! : %s", e.what());
			}
#else
			mBuilder->addDefaultObjects();
#endif
			return true;
		}


		virtual void release() override
		{
			delete this;
		}

		static void registerSceneObject(Chai::Module& module)
		{

		}

		static void registerAssetId(Chai::Module& module)
		{

		}
		void registerSceneFunc(Chai::ChaiScript& script)
		{
			script.add(Chai::fun([this](int matId, Vector3 const& pos, float radius) { Sphere(matId, pos, radius); }), SCRIPT_NAME(Sphere));
			script.add(Chai::fun([this](int meshId, int matId, Vector3 const& pos, float scale) { Mesh(meshId, matId, pos, scale); }), SCRIPT_NAME(Mesh));
			script.add(Chai::fun([this](int meshId, int matId, Vector3 const& pos, float scale, Quaternion const& rotation) { Mesh(meshId, matId, pos, scale, rotation); }), SCRIPT_NAME(Mesh));
			script.add(Chai::fun([this](int matId, Vector3 pos, Vector3 size) { Box(matId, pos, size); }), SCRIPT_NAME(Box));
			script.add(Chai::fun([this](Color3f baseColor, float roughness, float specular, Color3f emissiveColor, float refractiveIndex, float refractive)
			{ return Material(baseColor, roughness, specular, emissiveColor, refractiveIndex, refractive); }), SCRIPT_NAME(Material));
			script.add(Chai::fun([this](std::string const& path) { return LoadMesh(path); }), SCRIPT_NAME(LoadMesh));
			script.add(Chai::fun([this](std::string const& path, Math::Transform const& trans) { return LoadMesh(path, trans); }), SCRIPT_NAME(LoadMesh));
		}

		void Sphere(int matId, Vector3 const& pos, float radius)
		{
			mBuilder->mScene.objects.push_back(FObject::Sphere(radius, matId, pos));
		}
		void Mesh(int meshId, int matId, Vector3 const& pos, float scale)
		{
			mBuilder->mScene.objects.push_back(FObject::Mesh(meshId, scale, matId, pos));
		}
		void Mesh(int meshId, int matId, Vector3 const& pos, float scale, Quaternion const& rotation)
		{
			mBuilder->mScene.objects.push_back(FObject::Mesh(meshId, scale, matId, pos, rotation));
		}

		void Box(int matId, Vector3 pos, Vector3 size)
		{
			mBuilder->mScene.objects.push_back(FObject::Box(size, matId, pos));
		}

		int LoadMesh(std::string const& path)
		{
			return mBuilder->loadMesh(path.c_str());
		}

		int LoadMesh(std::string const& path, Math::Transform const& trans)
		{
			return mBuilder->loadMesh(path.c_str(), trans);
		}

		int Material(
			Color3f baseColor,
			float   roughness,
			float   specular,
			Color3f emissiveColor,
			float   refractiveIndex,
			float   refractive)
		{
			int result = mBuilder->mScene.materials.size();
			mBuilder->mScene.materials.push_back(MaterialData(baseColor, roughness, specular, emissiveColor, refractive, refractiveIndex));
			return result;
		}
	};

	std::string ISceneScript::GetFilePath(char const* fileName)
	{
		InlineString< 512 > path;
		path.format("Script/RTScene/%s.chai", fileName);
		return path;
	}

	ISceneScript* ISceneScript::Create()
	{
		return new SceneScriptImpl;
	}

	bool ISceneBuilder::RunScript(SceneBuildContext& context, ISceneScript& script, char const* fileName)
	{
		PathTracing::SceneBuilderImpl builder(context);

		VERIFY_RETURN_FALSE(static_cast<SceneScriptImpl&>(script).setup(builder, "DefaultScene"));
		VERIFY_RETURN_FALSE(context.mRenderResource.buildMeshResource(MakeConstView(context.mScene.meshes)));
		return true;
	}

	int ISceneBuilder::LoadMesh(SceneBuildContext& context, char const* filePath, Transform const& transform)
	{
		PathTracing::SceneBuilderImpl builder(context);
		int result = builder.loadMesh(filePath, transform);

		if (result != INDEX_NONE)
		{
			context.mRenderResource.updateMeshResource(MakeConstView(context.mScene.meshes), result);
		}

		return result;
	}
}

