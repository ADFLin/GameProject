#include "MaterialCommon.glsl"

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
	//materialInput.baseColor = ( 0.5 * sin(View.gameTime) + 0.5 )* parameters.vectexColor;
	input.baseColor = parameters.vectexColor.rgb /** frac( dot(parameters.worldPos, parameters.worldPos) / 100 )*/;
	input.metallic = 0.7;
	input.roughness = 0.5;
	input.specular = 0.8;
	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif //PIXEL_SHADER