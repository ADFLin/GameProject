#define MATERIAL_TEXCOORD_NUM 1
#define MATERIAL_BLENDING_MASKED 1
#define MATERIAL_USE_DEPTH_OFFSET 1

#include "MaterialCommon.glsl"
#include "ViewParam.glsl"
#include "ParallaxOcclusionCommon.glsl"

uniform sampler2D BaseTexture;
uniform sampler2D NoramlTexture;
uniform float3    DispFactor;
#ifdef VERTEX_SHADER

void  CalcMaterialInputVS(inout MaterialInputVS input, inout MaterialParametersVS parameters)
{
	input.worldOffset = 0;//3 * sin( 10 * View.gameTime ) * parameters.noraml;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

void CalcMaterialInputPS(inout MaterialInputPS input, inout MaterialParametersPS parameters)
{
	float3 viewVector = View.worldPos - parameters.worldPos;
	float3 viewVectorTangent = viewVector * parameters.tangentToWorld;

	POMParameters pomParams;
	pomParams.dispTexture = NoramlTexture;
	pomParams.dispMask = float4(0, 0, 0, DispFactor.x);
	pomParams.dispBias = DispFactor.y;
	pomParams.parallaxScale = 0.2;
	pomParams.iteratorParams = float2(15,80);
	pomParams.shadowIteratorParams = float2(15, 128);

	POMOutput pomOutput = POMapping(pomParams, normalize(viewVectorTangent), parameters.texCoords[0].xy);

	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	input.baseColor = tex2D(BaseTexture, pomOutput.UVs);
	//input.emissiveColor = tex2D(BaseTexture, pomOutput.UVs) * parameters.vectexColor;
	//input.emissiveColor = normalize(parameters.tangentToWorld[0]);
	input.metallic = 0.9;
	input.roughness = 0.5;
	input.specular = 0.1;
	input.normal = tex2D(NoramlTexture, pomOutput.UVs).xyz;
	if( pomOutput.depth > 0 )
		input.depthOffset = CalcPOMCorrectDepth(pomOutput, parameters.tangentToWorld, viewVectorTangent) - parameters.svPosition.z;
	else
		input.depthOffset = 0;
	float2 value = 2 * frac(5 * (parameters.texCoords[0].xy)) - 1;
	//input.mask = value.x * value.y > 0 ? 1 : 0;
	//input.emissiveColor = abs( float3(0.3, 0.3, 0.3) * parameters.worldNormal );
}

#endif //PIXEL_SHADER