#pragma once
#ifndef PathTracingScene_H_719F81D5_CF5C_45B4_89D3_ED2A9622EAA1
#define PathTracingScene_H_719F81D5_CF5C_45B4_89D3_ED2A9622EAA1

#include "PathTracingCommon.h"

namespace PathTracing
{
	struct MeshImportInfo
	{
		std::string path;
		Transform transform = Transform::Identity();

		REFLECT_STRUCT_BEGIN(MeshImportInfo)
			REF_PROPERTY(path)
			REF_PROPERTY(transform)
		REFLECT_STRUCT_END()
	};

	class ISceneScript
	{
	public:
		virtual ~ISceneScript() {}
		virtual void release() = 0;

		static std::string   GetFilePath(char const* fileName);
		static ISceneScript* Create();

	};


	struct SceneData
	{
		TArray< MaterialData > materials;
		TArray< ObjectData > objects;
		TArray< MeshData > meshes;
		TArray< MeshImportInfo > meshInfos;


		void removeMaterial(int index)
		{


		}

		void clearScene()
		{
			meshInfos.clear();
			meshes.clear();
			materials.clear();
			objects.clear();
		}
	};


	class RenderResourceData;

	struct SceneBuildContext
	{
		SceneBuildContext(SceneData& scene, RenderResourceData& renderReource)
			:mScene(scene)
			,mRenderResource(renderReource)
		{


		}
		SceneData& mScene;
		RenderResourceData& mRenderResource;
	};

	class FSceneBuilder
	{
	public:
		static bool RunScript(SceneBuildContext& context, ISceneScript& script, char const* fileName);
		static int  LoadMesh(SceneBuildContext& context, char const* filePath, Transform const& transform);
		static int  LoadMeshAsync(SceneBuildContext& context, char const* filePath, Transform const& transform, std::function<void()> callback);
		static bool UpdateMeshImportTransform(SceneBuildContext& context, int meshId, Transform const& transform);
	};

}//namespace PathTracing

#endif // PathTracingScene_H_719F81D5_CF5C_45B4_89D3_ED2A9622EAA1
