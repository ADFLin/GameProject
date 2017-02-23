#define NUM_MATERIAL_TEXCOORD 2
#define MATERIAL_BLENDING_MASKED	0

#include "MaterialCommon.glsl"

uniform sampler2D BaseTexture;

#ifdef VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = 0;//3 * sin( 10 * View.gameTime ) * parameters.noraml;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	//input.baseColor = ( 0.5 * sin(View.gameTime) + 0.5 )* parameters.vectexColor;
	input.baseColor = tex2D( BaseTexture , parameters.texCoords[0].xy ) * parameters.vectexColor /** frac( dot(parameters.worldPos, parameters.worldPos) / 100 )*/;
	//input.baseColor = float3(parameters.texCoords[0].xy, 0);
	input.metallic = 0.2;
	input.roughness = 1;
	input.specular = 0.2;
	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif //PIXEL_SHADER