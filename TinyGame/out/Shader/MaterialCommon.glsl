#pragma once
#include "Common.glsl"

#ifndef MATERIAL_TEXCOORD_NUM
#define MATERIAL_TEXCOORD_NUM 0
#endif
#ifndef MATERIAL_BLENDING_MASKED
#define MATERIAL_BLENDING_MASKED 0
#endif

#define SHADINGMODEL_UNLIT        0
#define SHADINGMODEL_DEFAULT_LIT  1

#ifdef VERTEX_SHADER

struct MaterialInputVS
{
	float3 worldOffset;
};

struct MaterialParametersVS
{
	float3 worldPos;
	float3 vectexColor;
	float3 noraml;
#if MATERIAL_TEXCOORD_NUM
	float2 texCoords[MATERIAL_TEXCOORD_NUM];
#endif
};

MaterialInputVS InitMaterialInputVS()
{
	MaterialInputVS input;
	input.worldOffset = 0;
	return input;
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

struct MaterialInputPS
{
	float3 baseColor;
	float3 normal;
	float3 emissiveColor;
	float  metallic;
	float  specular;
	float  roughness;
	float  mask;
	float  depthOffset;
	uint   shadingModel;
};

struct MaterialParametersPS
{
	float3 vectexColor;
	float3 worldPos;
	float4 screenPos;
	float3 svPosition;
	float3 worldNormal;
	mat3   tangentToWorld;
#if MATERIAL_TEXCOORD_NUM
	float2 texCoords[MATERIAL_TEXCOORD_NUM];
#endif
};

MaterialInputPS InitMaterialInputPS()
{
	MaterialInputPS input;
	input.baseColor = 0;
	input.normal = float3(0.0, 0.0, 1);
	input.emissiveColor = 0.0;
	input.metallic = 0.0;
	input.specular = 0.0;
	input.roughness = 0.0;
	input.mask = 1.0;
	input.shadingModel = SHADINGMODEL_DEFAULT_LIT;
	input.depthOffset = 0;
	return input;
}

#endif //PIXEL_SHADER



