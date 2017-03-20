#define MATERIAL_TEXCOORD_NUM 2
#define MATERIAL_BLENDING_MASKED 1

#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

uniform sampler2D BaseTexture;
uniform float4    BaseColor = float4(1,1,1,1);

#ifdef VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = float3(0.0);
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	float s = 5 * (sin( 10 * View.gameTime) + 1);
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	//input.baseColor = ( 0.5 * sin(View.gameTime) + 0.5 )* parameters.vectexColor;
	//input.baseColor = BaseColor.rgb * parameters.vectexColor.rgb;
	input.baseColor = float3(1, 0, 0);
	//input.baseColor = float3( parameters.texCoords[0].xy , 0 );
	//input.baseColor = float3(parameters.texCoords[0].xy, 0);
	input.metallic = 0.9;
	input.roughness = 0;
	//input.roughness = texture2D(BaseTexture, parameters.texCoords[0].xy).r * 0.9;
	input.specular = 0.1;
	float2 value = 2 * frac( 8 * ( parameters.texCoords[0].xy ) ) - 1;
	//input.mask = value.x * value.y > 0 ? 1 : 0;
	//input.emissiveColor = float3(parameters.texCoords[0].xy, 0);
}

#endif //PIXEL_SHADER