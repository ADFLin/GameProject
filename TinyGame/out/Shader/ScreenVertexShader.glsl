#include "Common.glsl"

struct VSOutput
{
	float2 UVs;
};

#if VERTEX_SHADER

out VSOutput vsOutput;

void ScreenVS()
{
	gl_Position = gl_Vertex;
	vsOutput.UVs = gl_MultiTexCoord0.xy;
}

#endif