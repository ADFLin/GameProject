#pragma once
#ifndef SceneScript_H_3F3F71B4_4B75_4F3F_B556_4C10CF688F7C
#define SceneScript_H_3F3F71B4_4B75_4F3F_B556_4C10CF688F7C

#include "RHI/BaseType.h"
#include "RHI/LazyObject.h"

namespace RenderGL
{
	class Material;
	class StaticMesh;
	class Texture2D;
	class Mesh;
	class Scene;
	class CycleTrack;
	class VectorCurveTrack;
	class DebugGeometryObject;

	class IAssetProvider
	{
	public:
		virtual ~IAssetProvider(){}
		virtual TLazyObjectGuid< Material >   getMaterial(int idx) = 0;
		virtual TLazyObjectGuid< StaticMesh > getMesh(int idx) = 0;
		virtual Mesh&       getSimpleMesh(int idx) = 0;
		virtual Texture2D&  getTexture(int idx) = 0;
		virtual CycleTrack* addCycleTrack(Vector3& value) { return nullptr; }
		virtual VectorCurveTrack* addCurveTrack(Vector3& value) { return nullptr; }
		virtual DebugGeometryObject* getDebugGeometryObject() { return nullptr; }
	};

	class ISceneScript
	{
	public:
		virtual ~ISceneScript(){}
		virtual bool setup( Scene& scene, IAssetProvider& assetProvider , char const* fileName ) = 0;
		virtual void release() = 0;


		static std::string   GetFilePath(char const* fileName);
		static ISceneScript* Create();
	};


}//namespace RenderGL

#endif // SceneScript_H_3F3F71B4_4B75_4F3F_B556_4C10CF688F7C