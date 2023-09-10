#include "SceneLighting.h"

namespace Render
{

	IMPLEMENT_SHADER_PROGRAM(DeferredLightingProgram);
	IMPLEMENT_SHADER_PROGRAM(LightingShowBoundProgram);

	void LightInfo::setupShaderGlobalParam(RHICommandList& commandList, ShaderProgram& shaderProgram) const
	{
		shaderProgram.setParam(commandList, SHADER_PARAM(GLight.worldPosAndRadius), Vector4(pos, radius));
		shaderProgram.setParam(commandList, SHADER_PARAM(GLight.color), intensity * color);
		shaderProgram.setParam(commandList, SHADER_PARAM(GLight.type), int(type));
		shaderProgram.setParam(commandList, SHADER_PARAM(GLight.bCastShadow), int(bCastShadow));
		shaderProgram.setParam(commandList, SHADER_PARAM(GLight.dir), Math::GetNormal(dir));

		Vector3 spotParam;
		float angleInner = Math::Min(spotAngle.x, spotAngle.y);
		spotParam.x = Math::Cos(Math::DegToRad(Math::Min<float>(89.9, angleInner)));
		spotParam.y = Math::Cos(Math::DegToRad(Math::Min<float>(89.9, spotAngle.y)));
		shaderProgram.setParam(commandList, SHADER_PARAM(GLight.spotParam), spotParam);
	}

}