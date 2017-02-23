#include "Common.glsl"

#define USE_POINT_LIGHT 1
#define USE_SHADOW_MAP 1

uniform float2 DepthParam;

struct VSOutput
{
	float3 viewOffsetV;
};

#ifdef VERTEX_SHADER

out VSOutput vsOutput;
void MainVS()
{
	vsOutput.viewOffsetV = float3(gl_ModelViewMatrix * gl_Vertex);
	gl_Position = ftransform();
}

#endif //VERTEX_SHADER

#ifdef PIXEL_SHADER

in VSOutput vsOutput;
void MainPS() 
{
#if USE_POINT_LIGHT
	float depth = ( length(vsOutput.viewOffsetV) - DepthParam.x ) / ( DepthParam.y - DepthParam.x );
#else
	float depth = ( -vsOutput.viewOffsetV.z  - DepthParam.x ) / ( DepthParam.y - DepthParam.x );
#endif
	//depth = 1.0;
	gl_FragColor = vec4(depth,depth,depth,1);
}

#endif //PIXEL_SHADER