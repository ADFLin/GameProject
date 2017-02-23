#pragma once

#define LIGHTTYPE_SPOT 0
#define LIGHTTYPE_POINT 1
#define LIGHTTYPE_DIRECTIONAL 2

#define float2 vec2
#define float3 vec3
#define float4 vec4

#define PI 3.1415926535897932

float Square(float x)
{
	return x*x;
}

float rcp(float x)
{
	return 1.0 / x;
}

#ifdef PIXEL_SHADER
void WritePxielDepth(float depth)
{
	gl_FragDepth = (gl_DepthRange.diff * depth  + gl_DepthRange.near + gl_DepthRange.far) * 0.5;
}
#endif