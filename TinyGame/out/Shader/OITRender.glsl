//#include "Common.glsl"
#include "OITCommon.glsl"

#include "ViewParam.glsl"

#ifndef OIT_USE_MATERIAL
#define OIT_USE_MATERIAL 0
#endif

#if OIT_USE_MATERIAL
#include "MaterialProcess.glsl"
#include "VertexProcess.glsl"
#else
struct OutputVSParam
{
	float4 pos;
	float4 color;
	float4 viewPos;
};
#endif


#if VERTEX_SHADER

#if OIT_USE_MATERIAL

out VertexFactoryOutputVS VFOutputVS;

#else //OIT_USE_MATERIAL

out OutputVSParam OutputVS;
uniform float4 BaseColor;
uniform mat4   WorldTransform;
layout(location = ATTRIBUTE_POSITION) in float3 InPosition;
layout(location = ATTRIBUTE_NORMAL) in float3 InNoraml;

#endif //OIT_USE_MATERIAL

void BassPassVS()
{
#if OIT_USE_MATERIAL
	VertexFactoryIntermediatesVS intermediates = GetVertexFactoryIntermediatesVS();

	MaterialInputVS materialInput = InitMaterialInputVS();
	MaterialParametersVS materialParameters = GetMaterialParameterVS(intermediates);
	CalcMaterialInputVS(materialInput, materialParameters);

	gl_Position = CalcVertexFactoryOutputVS(VFOutputVS, intermediates, materialInput, materialParameters);

#else //OIT_USE_MATERIAL
	float4 worldPos = WorldTransform * float4(InPosition, 1.0);
	OutputVS.pos = View.worldToClip * worldPos;
	OutputVS.color = float4(clamp(InPosition / 10 , 0, 1), 0.2);
	float3 normal = mat3(WorldTransform) * InNoraml;
	OutputVS.color *= float4( 0.5 * normal + 0.5 , 1.0);
	OutputVS.viewPos = View.worldToView * worldPos;

	gl_Position = View.worldToClip * worldPos;
#endif // OIT_USE_MATERIAL
}


#endif //VERTEX_SHADER

#if PIXEL_SHADER

#if OIT_USE_MATERIAL
#define VFInputPS VFOutputVS
in VertexFactoryIutputPS VFOutputVS;
#else //OIT_USE_MATERIAL
in OutputVSParam OutputVS;
#endif //OIT_USE_MATERIAL

layout(location = 0) out float4 OutColor;
layout(early_fragment_tests) in;

void BassPassPS()
{
#if OIT_USE_MATERIAL
	MaterialInputPS materialInput = InitMaterialInputPS();
	MaterialParametersPS materialParameters = GetMaterialParameterPS(VFInputPS);

	CalcMaterialParameters(materialInput, materialParameters);

	float4 pixelColor = float4( materialInput.baseColor , materialInput.opacity );
	//pixelColor.a = 0.4;
	//float  pixelDepth = materialParameters.screenPos.z;
	float  pixelDepth = -(View.worldToView * float4(materialParameters.worldPos, 1)).z;
#else //OIT_USE_MATERIAL

	float4 pixelColor = OutputVS.color;
	float pixelDepth = 0.5 * OutputVS.pos.z + 0.5;
	
#endif //OIT_USE_MATERIAL

	OITProcessPS(pixelColor, pixelDepth);

	OutColor = float4(1, 0, 0, 0);
	//gl_FragColor = float4(index, 0, 0, 0);
}


layout(location = 1) out int2 OutValue;
void ClearStoragePS()
{
	OutColor = float4(1, 0, 0, 0);
	OutValue = int2(-1, 0);
}

#endif //PIXEL_SHADER


