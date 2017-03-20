#pragma once
#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

//Material Template
#if 0
#ifdef VERTEX_SHADER
void  CalcMaterialInputVS(inout MaterialInputVS input, in MaterialParametersVS parameters)
{
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER
void CalcMaterialInputPS(inout MaterialInputPS input, in MaterialParametersPS parameters)
{
}
#endif //PIXEL_SHADER
#endif

#ifdef PIXEL_SHADER

float3 GetMaterialWorldNormal(in MaterialInputPS input, inout MaterialParametersPS parameters)
{
#if MATERIAL_USE_WORLD_NORMAL
	return input.normal;
#else

	return normalize(parameters.tangentToWorld * input.normal);
#endif
}

float3 GetMaterialWorldPositionAndCheckDepthOffset(in MaterialInputPS input, inout MaterialParametersPS parameters)
{
#if MATERIAL_USE_DEPTH_OFFSET
	if( input.depthOffset != 0 )
	{
		float3 svPosition = parameters.svPosition;
		svPosition.z += input.depthOffset;

		WritePxielDepth(svPosition.z);

		float4 worldPos = View.clipToWorld * float4( svPosition , 1 );
		return worldPos.xyz / worldPos.w;
	}
	else
	{
		WritePxielDepth(parameters.svPosition.z);
	}
#endif
	return parameters.worldPos.xyz;
}

void CalcMaterialParameters(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	CalcMaterialInputPS(input, parameters);

#if MATERIAL_BLENDING_MASKED
	if( input.mask < 0.01 )
		discard;
#endif
}

#endif //PIXEL_SHADER