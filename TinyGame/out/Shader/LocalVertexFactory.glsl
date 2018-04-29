#pragma once
#include "Common.glsl"
#include "MaterialCommon.glsl"
#include "ViewParam.glsl"
#include "PrimitiveParam.glsl"

#ifndef VERTEX_FACTORY_USE_GEOMETRY_SHADER
#define VERTEX_FACTORY_USE_GEOMETRY_SHADER 0
#endif

#define VETEX_FACTORY_USE_VERTEX_ATTRIBUTE 0

#if VERTEX_FACTORY_USE_GEOMETRY_SHADER

struct VertexFactoryOutputVSToGS
{
	float3 worldPos;
	float3 worldNormal;
	float4 tangent;
	float4 vertexColor;

	float4 debug;
#if MATERIAL_TEXCOORD_NUM
	float2 texCoords[MATERIAL_TEXCOORD_NUM];
#endif
};


struct VertexFactoryOutputGSToPS
{
	float3 worldPos;
	float3 worldNormal;
	float4 tangent;
	float4 vertexColor;

	float4 debug;
#if MATERIAL_TEXCOORD_NUM
	float2 texCoords[MATERIAL_TEXCOORD_NUM];
#endif
};

#define VertexFactoryOutputVS VertexFactoryOutputVSToGS 
#define VertexFactoryIutputPS VertexFactoryOutputGSToPS 


#else //VERTEX_FACTORY_USE_GEOMETRY_SHADER
struct VertexFactoryOutputVSToPS
{
	float3 worldPos;
	float3 worldNormal;
	float4 tangent;
	float4 vertexColor;
	float4 clipPos;

	float4 debug;
#if MATERIAL_TEXCOORD_NUM
	float2 texCoords[MATERIAL_TEXCOORD_NUM];
#endif
};

#define VertexFactoryOutputVS VertexFactoryOutputVSToPS 
#define VertexFactoryIutputPS VertexFactoryOutputVSToPS 

#endif //VERTEX_FACTORY_USE_GEOMETRY_SHADER

#if VERTEX_SHADER

#if VETEX_FACTORY_USE_VERTEX_ATTRIBUTE

in VertexFactoryInputParameters
{
	layout(location = ATTRIBUTE_POSITION) float4 position;
	layout(location = ATTRIBUTE_NORMAL) float3 normal;
	layout(location = ATTRIBUTE_COLOR) float4 color;
	layout(location = ATTRIBUTE_TANGENT) float4 tangent;

#if MATERIAL_TEXCOORD_NUM
	#if MATERIAL_TEXCOORD_NUM > 0
	layout(location = ATTRIBUTE4) float2 texCoord0;
	#endif
	#if MATERIAL_TEXCOORD_NUM > 1
	layout(location = ATTRIBUTE5) float2 texCoord1;
	#endif
	#if MATERIAL_TEXCOORD_NUM > 2
	layout(location = ATTRIBUTE6) float2 texCoord2;
	#endif
	#if MATERIAL_TEXCOORD_NUM > 3
	layout(location = ATTRIBUTE7) float2 texCoord3;
	#endif
#endif//MATERIAL_TEXCOORD_NUM

} VertexInput;

#endif //VETEX_FACTORY_USE_VERTEX_ATTRIBUTE

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
	float noramlFactor = 1;// gl_FrontFacing ? 1 : -1;
#if VETEX_FACTORY_USE_VERTEX_ATTRIBUTE
	intermediates.normal = noramlFactor * float4(VertexInput.normal, 0) * Primitive.worldToLocal;
	intermediates.tangent = noramlFactor * float4(VertexInput.tangent.xyz, 0) * Primitive.worldToLocal;
	intermediates.tangent.w = VertexInput.tangent.w;
	intermediates.worldPos = Primitive.localToWorld * VertexInput.position;
	intermediates.vertexColor = VertexInput.color;
#else //USE_VERTEX_ATTRIBUTE_INDEX
	intermediates.normal = noramlFactor * float4(gl_Normal, 0) * Primitive.worldToLocal;
	intermediates.tangent = noramlFactor * float4(gl_Tangent.xyz, 0) * Primitive.worldToLocal;
	intermediates.tangent.w = gl_Tangent.w;
	intermediates.worldPos = Primitive.localToWorld * gl_Vertex;
	intermediates.vertexColor = gl_Color;
#endif //USE_VERTEX_ATTRIBUTE_INDEX

	return intermediates;
}

MaterialParametersVS GetMaterialParameterVS(in VertexFactoryIntermediatesVS intermediates)
{
	MaterialParametersVS parameters;

	parameters.noraml = intermediates.normal.xyz;
	parameters.worldPos = intermediates.worldPos.xyz;
	parameters.vectexColor = intermediates.vertexColor;

#if VETEX_FACTORY_USE_VERTEX_ATTRIBUTE
	parameters.vertexPos = VertexInput.position;

#if MATERIAL_TEXCOORD_NUM > 0
	parameters.texCoords[0] = VertexInput.texCoord0.xy;
	#endif
#if MATERIAL_TEXCOORD_NUM > 1
	parameters.texCoords[1] = VertexInput.texCoord1.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 2
	parameters.texCoords[2] = VertexInput.texCoord2.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 3
	parameters.texCoords[3] = VertexInput.texCoord3.xy;
#endif

#else //USE_VERTEX_ATTRIBUTE_INDEX
	parameters.vertexPos = gl_Vertex;

#if MATERIAL_TEXCOORD_NUM > 0
	parameters.texCoords[0] = gl_MultiTexCoord0.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 1
	parameters.texCoords[1] = gl_MultiTexCoord1.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 2
	parameters.texCoords[2] = gl_MultiTexCoord2.xy;
#endif
#if MATERIAL_TEXCOORD_NUM > 3
	parameters.texCoords[3] = gl_MultiTexCoord3.xy;
#endif

#endif //USE_VERTEX_ATTRIBUTE_INDEX

	return parameters;
}

float4 CalcVertexFactoryOutputVS(out VertexFactoryOutputVS vertexFactoryOutput,
								 in VertexFactoryIntermediatesVS intermediates,
								 in MaterialInputVS materialInput,
								 in MaterialParametersVS materialParameters)
{
	vertexFactoryOutput.worldNormal = intermediates.normal.xyz;
	vertexFactoryOutput.worldPos = intermediates.worldPos.xyz + materialInput.worldOffset;
	vertexFactoryOutput.vertexColor = intermediates.vertexColor;
	vertexFactoryOutput.tangent = intermediates.tangent;
#if VERTEX_FACTORY_USE_GEOMETRY_SHADER

#else
	vertexFactoryOutput.clipPos = View.worldToClip * float4(vertexFactoryOutput.worldPos, 1);
#endif //VERTEX_FACTORY_USE_GEOMETRY_SHADER

#if MATERIAL_TEXCOORD_NUM
	for( int i = 0; i < MATERIAL_TEXCOORD_NUM; ++i )
		vertexFactoryOutput.texCoords[i] = materialParameters.texCoords[i];
#endif
	return vertexFactoryOutput.clipPos;
}

#endif //VERTEX_SHADER

#if PIXEL_SHADER

MaterialParametersPS GetMaterialParameterPS(in VertexFactoryIutputPS vertexFactoryInput)
{
	MaterialParametersPS parameters;

	mat3   tangentToWorld;
	float3 worldNormal = normalize(vertexFactoryInput.worldNormal.xyz);
	float normalYSign = sign(vertexFactoryInput.tangent.w);

	if( true )
	{
		float normalFactor = 1.0; // gl_FrontFacing ? 1.0 : -1.0;
		float3 tangentX = normalize(vertexFactoryInput.tangent.xyz);
		tangentX = normalize(tangentX - dot(tangentX, worldNormal));
		float3 tangentZ = normalFactor * worldNormal;
		float3 tangentY = cross(tangentZ, tangentX);
		tangentToWorld = mat3(tangentX, tangentY, tangentZ);
	}
	else
	{
		tangentToWorld = mat3(1, 0, 0, 0, 1, 0, 0, 0, 1);
		normalYSign = 0;
	}

	parameters.vectexColor = vertexFactoryInput.vertexColor;
	parameters.worldPos = vertexFactoryInput.worldPos;
	parameters.worldNormal = worldNormal;
	parameters.screenPos = vertexFactoryInput.clipPos;
	parameters.clipPos = parameters.screenPos.xyz / parameters.screenPos.w;
	parameters.tangentToWorld = tangentToWorld;
#if MATERIAL_TEXCOORD_NUM
	for( int i = 0; i < MATERIAL_TEXCOORD_NUM; ++i )
		parameters.texCoords[i] = vertexFactoryInput.texCoords[i];
#endif
	return parameters;
}

#endif //PIXEL_SHADER
