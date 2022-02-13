#pragma once
#ifndef RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE
#define RenderContext_H_6EE6BADB_9AEF_4C48_867D_600B4D1CF4DE

#include "RHICommon.h"

#include "Material.h"
#include "Renderer/SceneView.h"

namespace Render
{
	class RHICommandList;
	class RenderContext;
	class VertexFactory;

	class SceneLight;


	class SceneInterface
	{
	public:
		virtual void render( RenderContext& param ) = 0;
		//virtual void renderShadow(LightInfo const& info , RenderContext& param) = 0;
		virtual void renderTranslucent( RenderContext& param) = 0;

		virtual void addLight(){}
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
