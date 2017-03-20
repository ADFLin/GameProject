#pragma once



struct LightParam
{
	float4   worldPosAndRadius;
	float3   color;
	float3   dir;
	float3   spotParam;
	int      type;
	int      bCastShadow;
};

uniform LightParam GLight;