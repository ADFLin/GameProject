#include "RenderContext.h"

#include "Material.h"
#include "RHICommand.h"
#include "PrimitiveBatch.h"

//#TODO: remove me
#include "OpenGLCommon.h"


namespace Render
{
	void RenderTechnique::setupWorld(RenderContext& context, Matrix4 const& mat)
	{
		if( context.mUsageProgram )
		{
			Matrix4 matInv;
			float det;
			mat.inverseAffine(matInv, det);
			context.mUsageProgram->setParam(context.getCommnadList(), SHADER_PARAM(Primitive.localToWorld), mat);
			context.mUsageProgram->setParam(context.getCommnadList(), SHADER_PARAM(Primitive.worldToLocal), matInv);
		}
		else
		{
			//Fixed pipeline
			glLoadMatrixf( mat * context.getView().worldToView );
		}
	}

	void RenderContext::draw(MeshBatch const& meshBatch)
	{
		RHICommandList& commandList = getCommnadList();

		setMaterial(meshBatch.material);

		for (int i = 0; i < meshBatch.elements.size(); ++i)
		{
			MeshBatchElement const& meshElement = meshBatch.elements[i];

			InputStreamInfo inputSteam;
			inputSteam.buffer = meshElement.vertexBuffer;
			inputSteam.offset = 0;
			RHISetInputStream(commandList, meshBatch.inputLayout, &inputSteam, 1);

			setWorld(meshElement.world);
			if (meshElement.indexBuffer)
			{
				RHISetIndexBuffer(commandList, meshElement.indexBuffer);
				RHIDrawIndexedPrimitive(commandList, meshBatch.primitiveType, meshElement.idxStart, meshElement.numElement);
			}
			else
			{
				RHIDrawPrimitive(commandList, meshBatch.primitiveType, meshElement.idxStart, meshElement.numElement);
			}
		}
	}

	void RenderContext::draw(PrimitivesCollection& collection)
	{
		collection.drawDynamic(getCommnadList(), getView());
	}

	void RenderContext::endRender()
	{
		if( mUsageProgram )
		{
			RHISetShaderProgram(getCommnadList(), nullptr);
			mUsageProgram = nullptr;
		}
	}

	void RenderContext::setShader(ShaderProgram& program)
	{
		if( mUsageProgram != &program )
		{
			mUsageProgram = &program;
			RHISetShaderProgram( getCommnadList() , program.getRHI());
		}
	}

	MaterialShaderProgram* RenderContext::setMaterial( Material* material , VertexFactory* vertexFactory )
	{
		MaterialShaderProgram* program;
		RHICommandList& commandList = getCommnadList();

		if( material )
		{
			program = mTechique->getMaterialShader(*this,*material->getMaster() , vertexFactory );
			if( program == nullptr || !program->getRHI() )
			{
				material = nullptr;
			}
		}

		if( material == nullptr )
		{
			material = GDefalutMaterial;
			program = mTechique->getMaterialShader(*this, *GDefalutMaterial , vertexFactory );
		}

		if( program == nullptr )
		{
			return nullptr;
		}

		if( mUsageProgram != program )
		{
			mUsageProgram = program;
			mbUseMaterialShader = true;
			RHISetShaderProgram(commandList, program->getRHI());
			mCurView->setupShader(commandList, *program);
			mTechique->setupMaterialShader(*this, *program);
		}
		material->setupShader(commandList, *program);

		return program;
	}


}//namespace Render

