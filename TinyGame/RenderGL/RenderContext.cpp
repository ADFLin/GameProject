#include "RenderContext.h"

#include "Material.h"

namespace RenderGL
{
	void RenderTechique::setupWorld(RenderContext& context, Matrix4 const& mat)
	{
		//#TODO : Remove GL Matrix fun
		glMultMatrixf(mat);
		if( context.mUsageShader )
		{
			Matrix4 matInv;
			float det;
			mat.inverseAffine(matInv, det);
			context.mUsageShader->setParam(SHADER_PARAM(Primitive.localToWorld), mat);
			context.mUsageShader->setParam(SHADER_PARAM(Primitive.worldToLocal), matInv);
		}
	}

	void RenderContext::setShader(ShaderProgram& shader)
	{
		if( mUsageShader != &shader )
		{
			if( mUsageShader )
			{
				mUsageShader->unbind();
			}
			mUsageShader = &shader;
			mUsageShader->bind();
		}
	}

	void RenderContext::setupShader( Material* material , VertexFactory* vertexFactory )
	{
		MaterialShaderProgram* shader;
		if( material )
		{
			shader = mTechique->getMaterialShader(*this,*material->getMaster() , vertexFactory );
			if( shader == nullptr || !shader->isate() )
			{
				material = nullptr;
			}
		}

		if( material == nullptr )
		{
			material = GDefalutMaterial;
			shader = mTechique->getMaterialShader(*this, *GDefalutMaterial , vertexFactory );
		}

		if( shader == nullptr )
			return;

		if( mUsageShader != shader )
		{
			if( mUsageShader )
			{
				mUsageShader->unbind();
			}
			mUsageShader = shader;
			mbUseMaterialShader = true;
			mUsageShader->bind();
			mCurView->setupShader(*mUsageShader);
			mTechique->setupMaterialShader(*this, *mUsageShader);
		}
		material->setupShader(*shader);
	}


}//namespace RenderGL

