#pragma once
#include "MaterialCommon.glsl"
#include "ViewParam.glsl"

struct VertexFactoryParameters
{
	mat4 localToWorld;
	mat4 worldToLocal;
	uint haveTangent;
};

uniform VertexFactoryParameters VertexFactoryParams;

struct VertexFactoryOutputVSToPS
{
	float3 worldPos;
	float3 worldNormal;
	float4 tangent;
	float4 vertexColor;
	float4 screenPos;

	float4 debug;
#if MATERIAL_TEXCOORD_NUM
	float2 texCoords[MATERIAL_TEXCOORD_NUM];
#endif
};

#define VertexFactoryOutputVS VertexFactoryOutputVSToPS 
#define VertexFactoryIutputPS VertexFactoryOutputVSToPS 

#ifdef VERTEX_SHADER

struct VertexFactoryIntermediatesVS
{
	float4 worldPos;
	float4 normal;
	float4 tangent;
	float4 vertexColor;
	uint   haveTangent;
};

VertexFactoryIntermediatesVS GetVertexFactoryIntermediatesVS()
{
	VertexFactoryIntermediatesVS intermediates;
	float noramlFactor = 1;// (gl_FrontFacing) ? 1 : -1;
	intermediates.normal = noramlFactor * float4(gl_Normal, 0) * VertexFactoryParams.worldToLocal;
	intermediates.tangent = noramlFactor * float4(gl_MultiTexCoord1.xyz, 0) * VertexFactoryParams.worldToLocal;
	intermediates.tangent.w = gl_MultiTexCoord1.w;
	intermediates.worldPos = VertexFactoryParams.localToWorld * gl_Vertex;
	intermediates.vertexColor = gl_Color;

	return intermediates;
}

MaterialParametersVS GetMaterialParameterVS(in VertexFactoryIntermediatesVS intermediates)
{
	MaterialParametersVS parameters;
	parameters.noraml = intermediates.normal.xyz;
	parameters.worldPos = intermediates.worldPos.xyz;
	parameters.vectexColor = intermediates.vertexColor;
#if MATERIAL_TEXCOORD_NUM > 0
	parameters.texCoords[0] = gl_MultiTexCoord0.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 1
	parameters.texCoords[1] = gl_MultiTexCoord2.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 2
	parameters.texCoords[2] = gl_MultiTexCoord3.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 3
	parameters.texCoords[3] = gl_MultiTexCoord4.xy;
#endif
	return parameters;
}

float4 CalcVertexFactoryOutputVS( out VertexFactoryOutputVS output , 
								 in VertexFactoryIntermediatesVS intermediates ,  
								 in MaterialInputVS materialInput ,
								 in MaterialParametersVS materialParameters )
{
	output.worldNormal = intermediates.normal.xyz;
	output.worldPos = intermediates.worldPos.xyz + materialInput.worldOffset;
	output.vertexColor = intermediates.vertexColor;
	output.tangent = intermediates.tangent;
	output.screenPos = View.worldToClip * float4(output.worldPos, 1);
#if MATERIAL_TEXCOORD_NUM
	for( int i = 0; i < MATERIAL_TEXCOORD_NUM; ++i )
		output.texCoords[i] = materialParameters.texCoords[i];
#endif
	return output.screenPos;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

struct VertexFactoryIntermediatesPS
{

	mat3   tangentToWorld;
	float3 worldNormal;
	float  normalYSign;
};

VertexFactoryIntermediatesPS GetVertexFactoryIntermediatesPS(in VertexFactoryIutputPS vertexFactoryInput)
{
	VertexFactoryIntermediatesPS intermediates;

	float3 worldNormal = normalize(vertexFactoryInput.worldNormal.xyz);
	float noramlYSign = sign(vertexFactoryInput.tangent.w);
	if ( 1 )
	{
		float3 tangentX = normalize(vertexFactoryInput.tangent.xyz );
		tangentX = normalize(tangentX - dot(tangentX, worldNormal));
		float3 tangentZ = worldNormal;
		float3 tangentY = cross(tangentZ, tangentX);
		intermediates.tangentToWorld = mat3(tangentX, tangentY, tangentZ);
		intermediates.normalYSign = noramlYSign;
	}
	else
	{
		intermediates.tangentToWorld = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);
		intermediates.normalYSign = 0;
	}
	intermediates.worldNormal = worldNormal;
	return intermediates;
}

MaterialParametersPS GetMaterialParameterPS(in VertexFactoryIutputPS vertexFactoryInput , 
											in VertexFactoryIntermediatesPS intermediates)
{
	MaterialParametersPS parameters;

	parameters.vectexColor = vertexFactoryInput.vertexColor;
	parameters.worldPos = vertexFactoryInput.worldPos;
	parameters.worldNormal = intermediates.worldNormal;
	parameters.screenPos = vertexFactoryInput.screenPos;
	parameters.svPosition = parameters.screenPos.xyz / parameters.screenPos.w;
	parameters.tangentToWorld = intermediates.tangentToWorld;
#if MATERIAL_TEXCOORD_NUM
	for( int i = 0; i < MATERIAL_TEXCOORD_NUM; ++i )
		parameters.texCoords[i] = vertexFactoryInput.texCoords[i];
#endif
	return parameters;
}

#endif //PIXEL_SHADER

