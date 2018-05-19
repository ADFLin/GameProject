#include "RenderContext.h"

#include "Material.h"

namespace RenderGL
{
	void RenderTechnique::setupWorld(RenderContext& context, Matrix4 const& mat)
	{
		if( context.mUsageProgram )
		{
			Matrix4 matInv;
			float det;
			mat.inverseAffine(matInv, det);
			context.mUsageProgram->setParam(SHADER_PARAM(Primitive.localToWorld), mat);
			context.mUsageProgram->setParam(SHADER_PARAM(Primitive.worldToLocal), matInv);
		}
		else
		{
			//Fixed pipeline
			glLoadMatrixf( mat * context.getView().worldToView );
		}
	}

	void RenderContext::setShader(ShaderProgram& program)
	{
		if( mUsageProgram != &program )
		{
			if( mUsageProgram )
			{
				mUsageProgram->unbind();
			}
			mUsageProgram = &program;
			mUsageProgram->bind();
		}
	}

	MaterialShaderProgram* RenderContext::setMaterial( Material* material , VertexFactory* vertexFactory )
	{
		MaterialShaderProgram* program;
		if( material )
		{
			program = mTechique->getMaterialShader(*this,*material->getMaster() , vertexFactory );
			if( program == nullptr || !program->isValid() )
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
			if( mUsageProgram )
			{
				mUsageProgram->unbind();
			}
			mUsageProgram = program;
			mbUseMaterialShader = true;
			mUsageProgram->bind();
			mCurView->setupShader(*mUsageProgram);
			mTechique->setupMaterialShader(*this, *mUsageProgram);
		}
		material->setupShader(*program);

		return program;
	}


}//namespace RenderGL

