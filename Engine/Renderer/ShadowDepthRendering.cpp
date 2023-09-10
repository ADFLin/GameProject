#include "ShadowDepthRendering.h"

namespace Render
{

#if CORE_SHARE_CODE
	IMPLEMENT_SHADER_PROGRAM(ShadowDepthProgram);
	IMPLEMENT_SHADER_PROGRAM(ShadowDepthPositionOnlyProgram);
#endif

	void ShadowProjectParam::setupLight(LightInfo const& inLight)
	{
		light = &inLight;
		switch (light->type)
		{
		case ELightType::Spot:
		case ELightType::Point:
			shadowParam.y = 1.0 / inLight.radius;
			break;
		case ELightType::Directional:
			//#TODO
			shadowParam.y = 1.0;
			break;
		}
	}

	void ShadowProjectParam::setupShader(RHICommandList& commandList, ShaderProgram& program) const
	{
		program.setParam(commandList, SHADER_PARAM(ShadowParam), Vector2(shadowParam.x, shadowParam.y));

		auto& sampler = TStaticSamplerState< ESampler::Bilinear >::GetRHI();
		RHITextureBase* texture;
		switch (light->type)
		{
		case ELightType::Spot:
			 program.setParam(commandList, SHADER_PARAM(ProjectShadowOffset), shadowOffset);
			 program.setParam(commandList, SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 1);
			 texture = light->bCastShadow ? shadowTexture : GWhiteTexture2D.getResource();
			 program.setTexture(commandList, SHADER_PARAM(ShadowTexture2D), *texture, SHADER_PARAM(ShadowTexture2DSampler), sampler);
			break;
		case ELightType::Point:
			program.setParam(commandList, SHADER_PARAM(ProjectShadowOffset), shadowOffset);
			program.setParam(commandList, SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, 6);
			texture = light->bCastShadow ? shadowTexture : GWhiteTextureCube.getResource();
			program.setTexture(commandList, SHADER_PARAM(ShadowTextureCube), *texture, SHADER_PARAM(ShadowTexture2DSampler), sampler);
			break;
		case ELightType::Directional:
			return;
			program.setParam(commandList, SHADER_PARAM(ProjectShadowMatrix), shadowMatrix, numCascade);
			texture = light->bCastShadow ? shadowTexture : GWhiteTextureCube.getResource();
			program.setParam(commandList, SHADER_PARAM(NumCascade), numCascade);
			program.setParam(commandList, SHADER_PARAM(CacadeDepth), cacadeDepth, numCascade);
			program.setTexture(commandList, SHADER_PARAM(ShadowTexture2D), *texture, SHADER_PARAM(ShadowTexture2DSampler), sampler);
			break;
		}
	}

	void ShadowProjectParam::clearShaderResource(RHICommandList& commandList, ShaderProgram& program) const
	{
		if (light->bCastShadow)
		{
			switch (light->type)
			{
			case ELightType::Spot:
				program.clearTexture(commandList, SHADER_PARAM(ShadowTexture2D));
				break;
			case ELightType::Point:
				program.clearTexture(commandList, SHADER_PARAM(ShadowTextureCube));
				break;
			case ELightType::Directional:
				program.clearTexture(commandList, SHADER_PARAM(ShadowTexture2D));
				break;
			}
		}
	}

}