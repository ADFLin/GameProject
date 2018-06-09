#define MATERIAL_TEXCOORD_NUM 0
#define MATERIAL_BLENDING_MASKED 0
#define MATERIAL_USE_DEPTH_OFFSET 0

#include "MaterialCommon.glsl"

#if VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = float3(0.0);
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	input.baseColor = 0;
	input.metallic = 0.2;
	input.roughness = 1;
	input.specular = 0.2;
	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif //PIXEL_SHADER