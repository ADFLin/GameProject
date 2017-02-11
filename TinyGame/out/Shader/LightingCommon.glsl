#pragma once

#include "Common.glsl"

float3 phongShading(float3 diffuseColor , float3 SpecularColor , float3 N, float3 L, float3 V , float shininess )
{
	float3 H = normalize(L + V);

	float NoL = saturate(dot(N, L));
	float NoV = saturate(dot(N, V));
	float NoH = saturate(dot(N, H));

	float3 diff = diffuseColor * NoL;
	float3 spec = SpecularColor * pow( max( NoH , 0.001 ) , shininess );
	return diff + spec;
}