#pragma once

#define gl_Tangent gl_MultiTexCoord7

#define float2 vec2
#define float3 vec3
#define float4 vec4
#define uint2 uvec2
#define uint3 uvec3
#define uint4 uvec4
#define int2 ivec2
#define int3 ivec3
#define int4 ivec4
#define bool2 bvec2
#define bool3 bvec3
#define bool4 bvec4

#define float2x2 mat2
#define float3x3 mat3
#define float4x4 mat4

#define gl_Tangent gl_MultiTexCoord7

#define lerp mix
#define frac fract

float rcp(float x)
{
	return 1.0 / x;
}
DEFINE_VECTOR_FUNCTION(rcp, { return 1.0 / x; })
float saturate(float x)
{
	return clamp(x, 0.0, 1.0);
}
DEFINE_VECTOR_FUNCTION(saturate, { return clamp(x, 0.0, 1.0); })

#define TSincos( TYPE )\
	void sincos(TYPE x, out TYPE s, out TYPE c)\
	{\
		s = sin(x);\
		c = cos(x);\
	}
VECTOR_FUNCTION_LIST(TSincos)
#undef TSincos

#if PIXEL_SHADER
void WritePixelDepth(float depth)
{
	gl_FragDepth = (gl_DepthRange.diff * depth + gl_DepthRange.near + gl_DepthRange.far) * 0.5;
}
#endif

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
