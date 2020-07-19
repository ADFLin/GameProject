#pragma once
#ifndef RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE
#define RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE

#include "RHICommon.h"

#include "Material.h"
#include "Math/GeometryPrimitive.h"

namespace Render
{
	class RHICommandList;
	class RenderContext;
	class VertexFactory;

	class SceneLight;
	using Math::Plane;

	enum class LightType
	{
		Spot,
		Point,
		Directional,
	};


	class SceneInterface
	{
	public:
		virtual void render( RenderContext& param ) = 0;
		//virtual void renderShadow(LightInfo const& info , RenderContext& param) = 0;
		virtual void renderTranslucent( RenderContext& param) = 0;

		virtual void addLight(){}
	};


	struct ViewInfo
	{
		Matrix4 worldToClip;
		Matrix4 worldToView;
		Matrix4 viewToWorld;
		Matrix4 viewToClip;
		Matrix4 clipToWorld;
		Matrix4 clipToView;

		Matrix4  worldToClipPrev;

		Vector3 worldPos;
		Vector3 direction;
		int     frameCount;
		float   gameTime;
		float   realTime;

		IntVector2 rectSize;
		IntVector2 rectOffset;


		Plane frustumPlanes[6];

		RHIVertexBufferRef mUniformBuffer;
		bool   mbDataDirty = true;

		bool frustumTest(Vector3 const& pos, float radius) const
		{
			for( int i = 0; i < 6; ++i )
			{
				if( frustumPlanes[i].calcDistance(pos) > radius )
					return false;
			}
			return true;
		}

		Vector3 getViewForwardDir() const
		{
			return TransformVector(Vector3(0, 0,-1), viewToWorld);
		}
		Vector3 getViewRightDir() const
		{
			return TransformVector(Vector3(1, 0, 0), viewToWorld);
		}
		Vector3 getViewUpDir() const
		{
			return TransformVector(Vector3(0, 1, 0), viewToWorld);
		}

		void updateBuffer();
		void setupTransform(Matrix4 const& inViewMatrix, Matrix4 const& inProjectMatrix);

		IntVector2 getViewportSize() const;
		void  setupShader(RHICommandList& commandList, ShaderProgram& program);
		void  setupShader(RHICommandList& commandList, Shader& shader);


		void updateFrustumPlanes();
	};

	

	class RenderTechnique
	{
	public:
		virtual MaterialShaderProgram* getMaterialShader(RenderContext& context, MaterialMaster& material , VertexFactory* vertexFactory ) { return nullptr; }
		virtual void setupMaterialShader(RenderContext& context, MaterialShaderProgram& program) {}
		virtual void setupWorld(RenderContext& context, Matrix4 const& mat );
	};

	class RenderContext
	{
	public:
		RenderContext(RHICommandList& commandList, ViewInfo& view , RenderTechnique& techique)
			:mCommandList(&commandList)
			,mTechique(&techique)
			,mCurView( &view )
		{
			mUsageProgram = nullptr;
		}

		RHICommandList& getCommnadList() { return *mCommandList; }
		ViewInfo& getView() { return *mCurView; }

		void setupTechique(RenderTechnique& techique)
		{
			mTechique = &techique;
		}

		void setWorld(Matrix4 const& mat)
		{
			if( mTechique )
				mTechique->setupWorld(*this, mat);

		}
		void beginRender()
		{
			mUsageProgram = nullptr;
			mbUseMaterialShader = false;
		}
		void endRender();

		void setShader(ShaderProgram& program);
		MaterialShaderProgram* setMaterial(Material* material, VertexFactory* vertexFactory = nullptr);


		RHICommandList*  mCommandList;
		RenderTechnique* mTechique;
		ViewInfo*        mCurView;
		VertexFactory*   mUsageVertexFactory;
		ShaderProgram*   mUsageProgram;
		bool             mbUseMaterialShader;
	};

}//namespace Render

#endif // RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE
