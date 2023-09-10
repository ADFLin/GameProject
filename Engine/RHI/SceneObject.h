#pragma once
#ifndef SceneObject_H_39A04D57_E8EC_4831_855C_91B74D94377E
#define SceneObject_H_39A04D57_E8EC_4831_855C_91B74D94377E

#include "Renderer/Mesh.h"

#include "LazyObject.h"
#include "Scene.h"

namespace Render
{
	class RenderContext;
	class Material;
	class PrimitivesCollection;


	typedef std::shared_ptr< Material > MaterialPtr;
	class StaticMesh : public Mesh
	{
	public:
		void postLoad()
		{



		}

		void render(Matrix4 const& worldTrans, RenderContext& context, Material* material);
		void render(Matrix4 const& worldTrans, RenderContext& context);

		void setMaterial(int idx, MaterialPtr material)
		{
			if( idx >= mMaterials.size() )
				mMaterials.resize(idx + 1);
			mMaterials[idx] = material;
		}
		Material* getMaterial(int idx)
		{
			if( idx < mMaterials.size() )
				return mMaterials[idx].get();
			return nullptr;
		}
		TArray< MaterialPtr > mMaterials;
		std::string name;
	};

	class StaticMeshObject : public SceneObject
	{
	public:
		TLazyObjectPtr<StaticMesh> meshPtr;
		Matrix4  worldTransform;

		virtual void getPrimitives(PrimitivesCollection& primitiveCollection);
	};

	class SimpleMeshObject : public SceneObject
	{
	public:
		Mesh*     mesh;
		Matrix4   worldTransform;
		Vector4   color;
		std::shared_ptr< Material > material;

		virtual void getPrimitives(PrimitivesCollection& primitiveCollection);

		REFLECT_STRUCT_BEGIN(SimpleMeshObject)
			REF_PROPERTY(mesh)
			REF_PROPERTY(worldTransform)
			REF_PROPERTY(color)
			REF_PROPERTY(material)
		REFLECT_STRUCT_END()
	};

	class DebugGeometryObject : public SceneObject
	{
	public:
		virtual void getDynamicPrimitives(PrimitivesCollection& primitiveCollection, ViewInfo& view);

		void addLine(Vector3 const& posStart, Vector3 const& posEnd, Vector3 const& color, float thickness = 1, float time = -1);

		virtual void tick( float deltaTime ) override;

		struct LineElement : LineBatch
		{
			float time;
		};

		struct PointElement
		{
			float time;
		};
		TArray< LineElement > mLines;
		TArray< PointElement > mPoints;

	};


	class SkeltalMesh : public SceneObject
	{



	};
}

#endif // SceneObject_H_39A04D57_E8EC_4831_855C_91B74D94377E

