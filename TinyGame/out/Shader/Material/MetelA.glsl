#define MATERIAL_TEXCOORD_NUM 2
#define MATERIAL_BLENDING_MASKED 1

#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

uniform sampler2D BaseTexture;

#ifdef VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = float3(0.0);//3 * sin( 10 * View.gameTime ) * parameters.noraml;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	float s = 1;// 0.5 * (sin(10 * View.gameTime) + 1);
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	//input.baseColor = ( 0.5 * sin(View.gameTime) + 0.5 )* parameters.vectexColor;
	input.baseColor = s *texture2D( BaseTexture , parameters.texCoords[0].xy ).rgb * parameters.vectexColor.rgb /** frac( dot(parameters.worldPos, parameters.worldPos) / 100 )*/;
	//input.baseColor = float3(parameters.texCoords[0].xy, 0);
	input.metallic = 0.9;
	input.roughness = texture2D(BaseTexture, parameters.texCoords[0].xy).r * 0.9;
	input.specular = 0.1;
	float2 value = 2 * frac( 5 * ( parameters.texCoords[0].xy ) ) - 1;
	//input.mask = value.x * value.y > 0 ? 1 : 0;
	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif //PIXEL_SHADER