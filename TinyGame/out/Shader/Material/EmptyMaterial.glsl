#define MATERIAL_TEXCOORD_NUM 1
#define MATERIAL_BLENDING_MASKED 0

#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

uniform sampler2D BaseTexture;

#ifdef VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = float3(0.0);
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	input.baseColor = texture2D(BaseTexture, parameters.texCoords[0].xy).rgb * parameters.vectexColor.rgb;
	input.metallic  = 0.1;
	input.roughness = 0.9;
	input.specular  = 0.1;
	float2 value = 2 * frac(5 * (parameters.texCoords[0].xy)) - 1;
	//input.mask = value.x * value.y > 0 ? 1 : 0;
	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif //PIXEL_SHADER