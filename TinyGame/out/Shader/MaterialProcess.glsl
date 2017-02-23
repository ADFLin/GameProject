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

float3 GetMaterialNormal(in MaterialInputPS input, inout MaterialParametersPS parameters)
{
	return input.normal;
}

float3 GetMaterialWorldPositionAndCheckDepthOffset(in MaterialInputPS input, inout MaterialParametersPS parameters)
{
	if( input.depthOffset != 0 )
	{
		float3 svPosition = parameters.svPosition;
		svPosition.z += input.depthOffset;

		WritePxielDepth(svPosition.z);

		float4 worldPos = View.clipToWorld * float4( svPosition , 1 );
		return worldPos.xyz / worldPos.w;
	}
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