#pragma once

#define float2 vec2
#define float3 vec3
#define float4 vec4
#define float4x4 mat4
#define float3x3 mat3

#define PI 3.1415926535897932

#define lerp mix
#define frac fract

#define ATTRIBUTE0 0
#define ATTRIBUTE1 1
#define ATTRIBUTE2 2
#define ATTRIBUTE3 3
#define ATTRIBUTE4 4
#define ATTRIBUTE5 5
#define ATTRIBUTE6 6
#define ATTRIBUTE7 7
#define ATTRIBUTE8 8 
#define ATTRIBUTE9 9
#define ATTRIBUTE10 10
#define ATTRIBUTE11 11
#define ATTRIBUTE12 12
#define ATTRIBUTE13 13
#define ATTRIBUTE14 14
#define ATTRIBUTE15 15

#define LIGHTTYPE_SPOT 0
#define LIGHTTYPE_POINT 1
#define LIGHTTYPE_DIRECTIONAL 2

float4x4 Transform(float4x4 m1, float4x4 m2)
{
	return m2 * m1;
}

float4 Transform(float4 v, float4x4 m)
{
	return m * v;
}

float3 TransformInv(float3 v, float3x3 m)
{
	return v * m;
}

float Square(float x)
{
	return x*x;
}

float rcp(float x)
{
	return 1.0 / x;
}

float saturate(float x)
{
	return clamp(x, 0.0, 1.0);
}

#ifdef PIXEL_SHADER
void WritePxielDepth(float depth)
{
	gl_FragDepth = (gl_DepthRange.diff * depth  + gl_DepthRange.near + gl_DepthRange.far) * 0.5;
}
#endif

