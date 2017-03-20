#define MATERIAL_TEXCOORD_NUM 0
#define MATERIAL_BLENDING_MASKED 1
#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

#ifdef VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = float3(0, 0, 0);
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	float c = cos( 0.001 * parameters.worldPos.x );
	float3 n = normalize( float3(-c, 0, 1) );
	float3 t = normalize( float3(1, 0, c) );
	input.baseColor = parameters.vectexColor.rgb;
	//input.baseColor = float3(1,1,1) * float3( parameters.worldPos.xy , 0 );
	input.metallic = 0.9;
	input.roughness = 0.7;
	input.specular = 0.2;
	//float2 value = 2 * frac(10 * (parameters.worldPos.x + parameters.worldPos.y + parameters.worldPos.z )) - 1;
	float2 value = 2 * frac(10 * (parameters.worldPos.xy)) - 1;
	//input.mask = value.x * value.y > 0 ? 1 : 0;
	//input.normal = float3(0,0,1);
}

#endif //PIXEL_SHADER